#ifndef __BIGNUM_H_
#define __BIGNUM_H_

#include <linux/slab.h>

// Each node of bn list can store a value within 10^18
// BOUND = 10^18
#define BOUND 1000000000000000000UL
#define MAX_DIGITS 18

// Use division to replace floating number calculation
// DIVISOR = 10^6 to prevent multiplication overflow
#define DIVISOR 1000000
#define LOG10PHI 208987
#define LOG10SQRT5 349485
#define LOG2_10 3321928

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// static array to find power of 10 in O(1)
static const uint64_t pow10[MAX_DIGITS] = {1UL,
                                           10UL,
                                           100UL,
                                           1000UL,
                                           10000UL,
                                           100000UL,
                                           1000000UL,
                                           10000000UL,
                                           100000000UL,
                                           1000000000UL,
                                           10000000000UL,
                                           100000000000UL,
                                           1000000000000UL,
                                           10000000000000UL,
                                           100000000000000UL,
                                           1000000000000000UL,
                                           10000000000000000UL,
                                           100000000000000000UL};

/**
 * bn_head - store the head of bn list
 * @size: size of the list
 * @sign: sign of the bn
 * @list: list_head of the list
 */
typedef struct {
    size_t size;
    struct list_head list;
} bn_head;

/**
 * bn_node - store a node of bn list
 * The value should be within 10^19
 * @val: value of the node
 * @list: list_head of the node
 */
typedef struct {
    uint64_t val;
    struct list_head list;
} bn_node;

/**
 * bn_alloc: allocate memory for a bn list
 * Exit if failed to allocate memory
 * @return: the list_head of the list
 */
static inline struct list_head *bn_alloc(void)
{
    bn_head *bn_list = kmalloc(sizeof(bn_head), GFP_KERNEL);
    bn_list->size = 0;
    INIT_LIST_HEAD(&bn_list->list);
    return &bn_list->list;
}

/**
 * bn_newnode: create a new node and add it to the tail of the list
 * @val: value of the node
 * @head: head of the list
 */
static inline void bn_newnode(struct list_head *head, uint64_t val)
{
    bn_node *node = kmalloc(sizeof(bn_node), GFP_KERNEL);
    node->val = val;
    INIT_LIST_HEAD(&node->list);
    list_add_tail(&node->list, head);
    list_entry(head, bn_head, list)->size++;
}

/**
 * bn_free: free a bn list
 * @head: head of the bn list to be freed
 */
static inline void bn_free(struct list_head *head)
{
    if (!head)
        return;
    bn_node *node, *tmp;
    list_for_each_entry_safe (node, tmp, head, list) {
        list_del(&node->list);
        kfree(node);
    }
    kfree(list_entry(head, bn_head, list));
}

/**
 * bn_new: create a new bn to store fib(n)
 * Uses logrithmic of the Binets formula to calculate number of digits
 * Allocate the nodes of the list according to the number of digits
 * fib(n) = (phi^n - (1 - phi)^n) / sqrt(5)
 * digits = log10(fib(n)) = n * log10(phi) - log10(sqrt(5))
 * The return bn have zeros in each node value
 * @n: offset of fib
 * @return: head of bn list
 */
static inline struct list_head *bn_new(size_t n)
{
    unsigned int list_len =
        n > 1 ? (n * LOG10PHI - LOG10SQRT5) / DIVISOR / MAX_DIGITS + 1 : 1;
    struct list_head *head = bn_alloc();
    for (; list_len; list_len--) {
        bn_newnode(head, 0);
    }
    return head;
}

/**
 * bn_set: set the value of a bn list
 * The set value should be within UINT64_MAX
 * If the set value has more digits than the list, expand the list
 * @head: head of the bn list
 * @val: value to be set
 */
static inline void bn_set(struct list_head *head, uint64_t val)
{
    bn_node *node;
    uint64_t value = val;
    list_for_each_entry (node, head, list) {
        node->val = value % BOUND;
        value /= BOUND;
    }
    if (value) {
        bn_newnode(head, value);
    }
}

/**
 * bn_copy: copy a bn list to another
 * @dest: destination bn list to be copied to
 * @target: target bn list to be copied from
 */
static inline void bn_copy(struct list_head *dest, struct list_head *target)
{
    if (list_is_singular(target)) {
        list_first_entry(dest, bn_node, list)->val =
            list_first_entry(target, bn_node, list)->val;
        return;
    }
    bn_node *node;
    struct list_head *cur = dest->next;
    uint8_t size = 0;
    list_for_each_entry (node, target, list) {
        if (!node->val && ++size < list_entry(target, bn_head, list)->size)
            continue;
        if (cur != dest) {
            list_entry(cur, bn_node, list)->val = node->val;
            cur = cur->next;
        } else {
            bn_newnode(dest, node->val);
        }
    }
    while (cur != dest) {
        struct list_head *tmp = cur->next;
        list_del(cur);
        kfree(list_entry(cur, bn_node, list));
        list_entry(dest, bn_head, list)->size--;
        cur = tmp;
    }
}

/**
 * bn_clean: remove the leading zeros of a bn list
 * @head: head of the bn list
 */
static inline void bn_clean(struct list_head *head)
{
    if (list_empty(head) || list_is_singular(head))
        return;
    bn_node *node, *tmp;
    list_for_each_entry_safe_reverse(node, tmp, head, list)
    {
        if (node->val || list_is_singular(head))
            break;
        list_del(&node->list);
        kfree(node);
        list_entry(head, bn_head, list)->size--;
    }
}

/**
 * bn_add: add two bn a, b to the shorter one
 *
 * @a: first bn
 * @b: second bn
 */
void bn_add(struct list_head *a, struct list_head *b);

/**
 * __bn_add: add two bns to the first one
 * the result is expected to be positive
 * @shorter: first bn
 * @longer: second bn
 */
void __bn_add(struct list_head *shorter, struct list_head *longer);

/**
 * bn_sub: subtract two bns and store result to c
 * the result is expected to be positive
 * c = a - b , a > b
 * c = b - a , a < b
 * @a: first bn
 * @b: second bn

 */
void bn_sub(struct list_head *a, struct list_head *b, struct list_head *c);

void __bn_sub(struct list_head *a, struct list_head *b);

/**
 * bn_mul: multiply two bns and store result to c
 * c = a * b
 * @a: first bn
 * @b: second bn
 * @c: result bn
 */
void bn_mul(struct list_head *a, struct list_head *b, struct list_head *c);

/**
 * bn_lshift: left shift a bn by bit
 * @head: bn to be shifted
 * @bit: number of bits to be shifted
 */
void bn_lshift(struct list_head *head, int bit);

/**
 * __bn_lshift: left shift a bn by bit
 * Number of bits to be shifted is expected to be smaller than 4,
 * since (10^18-1) << 4 < UINT64_MAX
 * @head: bn to be shifted
 * @bit: number of bits to be shifted
 */
void __bn_lshift(struct list_head *head, int bit);

/**
 * bn_rshift: right shift a bn by bit
 * @head: bn to be shifted
 * @bit: number of bits to be shifted
 */
void bn_rshift(struct list_head *head, int bit);

/**
 * __bn_rshift: right shift a bn by a bit
 * @head: bn to be shifted
 * @bit: number of bits to be shifted
 */
void __bn_rshift(struct list_head *head);

/**
 * bn_to_string: convert a bn list to string
 * @list: head of the bn list
 * @return: string representation of the bn list
 */
char *bn_to_string(struct list_head *list);

/**
 * bn_compare: compare two bn lists
 * @a: first bn
 * @b: second bn
 * @return: 1 if a > b, 0 if a == b, -1 if a < b
 */
int bn_cmp(struct list_head *a, struct list_head *b);

#endif