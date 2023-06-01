#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
// #include <sys/time.h>

#define limit 10000
#define DIVISOR 100000
#define LOG2PHI 69424
#define LOG2SQRT5 116096
#define uint128_t __uint128_t


#define FIB_DEV "/dev/fibonacci"

char *bn_2_string(uint64_t *head, int head_size, uint64_t n)
{
    // log10(fib(n)) = nlog10(phi) - log10(5)/2
    double logfib = n * 0.20898764025 - 0.34948500216;
    size_t size = n > 1 ? (size_t) logfib + 2 : 2;
    char *res = malloc(sizeof(char) * size);
    res[--size] = '\0';
    if (n < 3) {
        res[0] = !!head[0] + '0';
        return res;
    }
    for (int i = size; --i >= 0;) {
        uint128_t tmp = 0;
        for (int j = head_size; --j >= 0;) {
            tmp <<= 64;
            tmp |= head[j];
            head[j] = tmp / 10;
            tmp %= 10;
        }
        res[i] = tmp + '0';
    }
    return res;
}

int main(int argc, char *argv[])
{
    long long sz;
    // struct timespec start, end;

    char write_buf[] = "testing writing";
    int offset = limit; /* TODO: try test something bigger than the limit */
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }
    if (argc > 1)
        sz = write(fd, argv[1], strlen(argv[1]));
    // for (uint64_t i = 0; i <= 100; i++) {
    //     uint64_t n = i;
    //     size_t list_size =
    //         n > 1 ? (n * LOG2PHI - LOG2SQRT5) / DIVISOR / 64 + 1 : 1;
    //     uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
    //     memset(buf, 0, sizeof(uint64_t) * list_size);
    //     lseek(fd, n, SEEK_SET);
    //     sz = read(fd, buf, sizeof(uint64_t) * list_size);
    //     char *res = bn_2_string(buf, list_size, n);
    //     // printf("\n%d,%lld,%s\n", n, sz, res);
    //     printf("%d %lld\n", n, sz);
    //     // free(res);
    //     free(buf);
    // }

    for (int i = 0; i <= offset; i++) {
        uint64_t n = ((uint64_t) i * LOG2PHI - LOG2SQRT5) / DIVISOR / 64;
        size_t list_size = i > 1 ? n + 1 : 1;
        uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
        memset(buf, 0, sizeof(uint64_t) * list_size);
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(uint64_t) * list_size);
        char *res = bn_2_string(buf, list_size, i);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence %s.\n",
               i, res);
        free(res);
        free(buf);
    }

    for (int i = offset; i >= 0; i--) {
        uint64_t n = ((uint64_t) i * LOG2PHI - LOG2SQRT5) / DIVISOR / 64;
        size_t list_size = i > 1 ? n + 1 : 1;
        uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
        memset(buf, 0, sizeof(uint64_t) * list_size);
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(uint64_t) * list_size);
        char *res = bn_2_string(buf, list_size, i);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence %s.\n",
               i, res);
        free(res);
        free(buf);
    }

    close(fd);
    return 0;
}
