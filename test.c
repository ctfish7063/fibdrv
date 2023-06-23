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

#define limit 1000000
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

int main(int argc, char *argv[])
{
    int offset = 10000; /* TODO: try test something bigger than the limit */
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
        if (offset < 0) {
            offset = limit;
        }
    }
    for (int i = 0; i < 100; i++) {
        long long sz;
        size_t list_size =
            i > 1 ? ((double) i * 0.69424191363 - 1.16096404744) / 64 + 1 : 1;
        uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
        memset(buf, 0, sizeof(uint64_t) * list_size);
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(uint64_t) * list_size);
        free(buf);
        buf = NULL;
    }
    // for (uint64_t i = 0; i <= offset; i++) {
    long long kt, ut, st;
    // uint64_t n = i;
    uint64_t n = offset;
    size_t list_size =
        n > 1 ? ((double) n * 0.69424191363 - 1.16096404744) / 64 + 1 : 1;
    uint64_t *buf = malloc(sizeof(uint64_t) * list_size);
    memset(buf, 0, sizeof(uint64_t) * list_size);
    st = getnanosec();
    lseek(fd, n, SEEK_SET);
    kt = read(fd, buf, sizeof(uint64_t) * list_size);
    ut = getnanosec() - st;
    printf("%lu %lld %lld %lld\n", n, kt, ut, ut - kt);
    free(buf);
    buf = NULL;
    // }

    close(fd);
    return 0;
}
