#ifndef __NTT_H__
#define __NTT_H__
#include <linux/slab.h>

#define CLZ(x) __builtin_clzll(x)

// the modulo could be altered
#define mod 1107296257
#define rou 10

static inline int nextpow2(uint64_t x)
{
    return 1 << (64 - CLZ(x - 1));
}

static inline int reverse_bits(int x, int n)
{
    int result = 0;
    for (int i = 0; i < n; i++) {
        result <<= 1;
        result |= (x & 1);
        x >>= 1;
    }
    return result;
}

static inline uint64_t fast_pow(uint64_t x, uint64_t n, uint64_t p)
{
    uint64_t result = 1;
    while (n) {
        if (n & 1) {
            result = result * x % p;
        }
        x = x * x % p;
        n >>= 1;
    }
    return result;
}

/**
 * ntt - number theoretic transform
 * @a: array of coefficients
 * @n: length of a
 * @p: prime number
 * @g: primitive root of p
 */
static inline void ntt(uint64_t *a, int n, uint64_t p, uint64_t g)
{
    uint64_t len = 64 - CLZ(n - 1);
    for (int i = 0; i < n; i++) {
        if (i < reverse_bits(i, len)) {
            a[reverse_bits(i, len)] ^= a[i];
            a[i] ^= a[reverse_bits(i, len)];
            a[reverse_bits(i, len)] ^= a[i];
        }
    }
    for (int m = 2; m <= n; m <<= 1) {
        uint64_t wm = fast_pow(g, (p - 1) / m, p);
        for (int k = 0; k < n; k += m) {
            uint64_t w = 1;
            for (uint64_t j = 0; j < m / 2; j++) {
                uint64_t t = w * a[k + j + m / 2] % p;
                uint64_t u = a[k + j];
                a[k + j] = (u + t) % p;
                a[k + j + m / 2] = (u - t + p) % p;
                w = w * wm % p;
            }
        }
    }
}

static inline void intt(uint64_t *a, int n, uint64_t p, uint64_t g)
{
    uint64_t len = 64 - CLZ(n - 1);
    for (int i = 0; i < n; i++) {
        if (i < reverse_bits(i, len)) {
            a[reverse_bits(i, len)] ^= a[i];
            a[i] ^= a[reverse_bits(i, len)];
            a[reverse_bits(i, len)] ^= a[i];
        }
    }
    for (int m = 2; m <= n; m <<= 1) {
        uint64_t wm = fast_pow(g, (p - 1) / m, p);
        // modular inverse
        wm = fast_pow(wm, p - 2, p);
        for (int k = 0; k < n; k += m) {
            uint64_t w = 1;
            for (int j = 0; j < m / 2; j++) {
                uint64_t t = w * a[k + j + m / 2] % p;
                uint64_t u = a[k + j];
                a[k + j] = (u + t) % p;
                a[k + j + m / 2] = (u - t + p) % p;
                w = w * wm % p;
            }
        }
    }
    // inv by Fermat's little theorem
    uint64_t inv = fast_pow(n, p - 2, p);
    printk(KERN_INFO "inv: %llu\n", inv);
    for (int i = 0; i < n; i++) {
        a[i] = a[i] * inv % p;
    }
}
#endif