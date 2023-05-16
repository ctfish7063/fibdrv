#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DIVISOR 1000000
#define LOG10PHI 208987
#define LOG10SQRT5 349485

#define FIB_DEV "/dev/fibonacci"

int main()
{
    long long sz;

    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    for (int i = 0; i <= offset; i++) {
        size_t list_size = (i * LOG10PHI - LOG10SQRT5) / DIVISOR + 2;
        char *buf = malloc(sizeof(char) * list_size);
        memset(buf, '\0', sizeof(char) * list_size);
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(char) * list_size);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence %s.\n",
               i, buf);
        free(buf);
        buf = NULL;
    }

    for (int i = offset; i >= 0; i--) {
        size_t list_size = (i * LOG10PHI - LOG10SQRT5) / DIVISOR + 2;
        char *buf = malloc(sizeof(char) * list_size);
        memset(buf, '\0', sizeof(char) * list_size);
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(char) * list_size);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence %s.\n",
               i, buf);
        free(buf);
        buf = NULL;
    }

    close(fd);
    return 0;
}
