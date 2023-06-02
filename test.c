#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define limit 10000
#define DIVISOR 100000
#define LOG2PHI 69424
#define LOG2SQRT5 116096
#define uint128_t __uint128_t


#define FIB_DEV "/dev/fibonacci"

long long getnanosec()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

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
    int offset = limit; /* TODO: try test something bigger than the limit */
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    if (argc > 1) {
        long long sz = write(fd, argv[1], strlen(argv[1]));
        assert(sz == !!(int) (argv[1][0] - 'n'));
    }
    if (argc > 2) {
        offset = atoi(argv[2]);
    }
    for (uint64_t i = 0; i <= offset; i++) {
        long long kt, ut, st;
        uint64_t n = i;
        size_t list_size =
            n > 1 ? (n * LOG2PHI - LOG2SQRT5) / DIVISOR / 64 + 1 : 1;
        uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
        memset(buf, 0, sizeof(uint64_t) * list_size);
        st = getnanosec();
        lseek(fd, n, SEEK_SET);
        kt = read(fd, buf, sizeof(uint64_t) * list_size);
        ut = getnanosec() - st;
        char *res = bn_2_string(buf, list_size, n);
        printf("%lu %lld %lld %lld\n", n, kt, ut, ut - kt);
        free(res);
        free(buf);
    }

    close(fd);
    return 0;
}
