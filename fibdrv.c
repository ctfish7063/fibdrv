#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include "bn.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 10000
#define CLZ(x) __builtin_clzll(x)

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static ktime_t kt;

// naive fibonacci calculation
static inline size_t fib_sequence_naive(long long k, uint64_t **fib)
{
    BN_INIT_VAL(a, 1, 0);
    BN_INIT_VAL(b, 1, 1);
    for (int i = 2; i <= k; i++) {
        bn_add_to_smaller(a, b);
    }
    size_t ret = bn_size((k & 1) ? b : a);
    *fib = bn_to_array((k & 1) ? b : a);
    bn_free(a);
    bn_free(b);
    return ret;
}

// fast doubling
void fast_doubling(struct list_head *fib_n0,
                   struct list_head *fib_n1,
                   struct list_head *fib_2n0,
                   struct list_head *fib_2n1)
{
    // fib(2n+1) = fib(n)^2 + fib(n+1)^2
    // use fib_2n0 to store the result temporarily
    bn_mul(fib_n0, fib_n0, fib_2n1);
    bn_mul(fib_n1, fib_n1, fib_2n0);
    bn_add(fib_2n1, fib_2n0);
    // fib(2n) = fib(n) * (2 * fib(n+1) - fib(n))
    bn_lshift(fib_n1, 1);
    bn_sub(fib_n1, fib_n0);
    bn_mul(fib_n1, fib_n0, fib_2n0);
}

/**
 * fib_sequence: calculate the fibonacci number with fast doubling algorithm.
 * It's a bottom up approach to avoid recursion.
 * @param k: the index of the fibonacci number
 * @return: the fibonacci number in char*
 */
static inline size_t fib_sequence(long long k, uint64_t **fib)
{
    if (unlikely(k < 0)) {
        return 0;
    }
    // return fib[n] without calculation for n <= 2
    if (unlikely(k <= 2)) {
        *fib = kmalloc(sizeof(uint64_t) * 2, GFP_KERNEL);
        (*fib)[0] = !!k;
        return 1;
    }
    // starting from n = 1, fib[n] = 1, fib [n+1] = 1
    uint8_t count = 63 - CLZ(k);
    BN_INIT_VAL(a, 0, 1);
    BN_INIT_VAL(b, 1, 1);
    BN_INIT(c, 0);
    BN_INIT(d, 0);
    int n = 1;
    for (uint8_t i = count; i-- > 0;) {
        fast_doubling(a, b, c, d);
        if (k & (1LL << i)) {
            bn_copy(a, d);
            bn_add(c, d);
            bn_copy(b, c);
            n = 2 * n + 1;
        } else {
            bn_copy(a, c);
            bn_copy(b, d);
            n = 2 * n;
        }
    }
    size_t res = bn_size(a);
    *fib = bn_to_array(a);
    // for (int i = 0; i < res; i++) {
    //     printk(KERN_INFO "fibdrv[%llu][%i]: %llu",k, i, (*fib)[i]);
    // }
    bn_free(a);
    bn_free(b);
    bn_free(c);
    bn_free(d);
    return res;
}

static size_t fib_time_proxy(long long k, uint64_t **fib)
{
    kt = ktime_get();
    size_t ret = fib_sequence_naive(k, fib);
    kt = ktime_sub(ktime_get(), kt);
    return ret;
}

static size_t my_copy_to_user(char *buf, uint64_t *src, size_t size)
{
    size_t lbytes = src[size - 1] ? CLZ(src[size - 1]) >> 3 : 7;
    size_t i = size * sizeof(uint64_t) - lbytes;
    printk(KERN_INFO "fibdrv: total %zu bytes, copy_to_user %zu bytes",
           size * sizeof(uint64_t), i);
    for (int j = 0; j < size; j++) {
        printk(KERN_INFO "fibdrv[%i]: %llu", j, src[j]);
    }
    return copy_to_user(buf, src, i);
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    printk(KERN_INFO "fibdrv: reading on offset %lld \n", *offset);
    uint64_t *fib = NULL;
    size_t fib_size = fib_time_proxy(*offset, &fib);
    if (!fib) {
        printk(KERN_INFO "fibdrv: calculation failed\n");
        return -EFAULT;
    }
    printk(KERN_INFO "fibdrv: read\n");
    if (my_copy_to_user(buf, fib, fib_size)) {
        printk(KERN_INFO "fibdrv: copy to user failed\n");
        return -EFAULT;
    };
    kfree(fib);
    return ktime_to_ns(kt);
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
