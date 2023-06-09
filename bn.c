#include "bn.h"
#include "ntt.h"

#define chunck_size 8
#define chunk_mask 0xff
#define mask 0xffffffffffffffff
#define val_size 64
#define per_size (val_size / chunck_size)

void bn_add(struct list_head *a, struct list_head *b)
{
    __bn_add(a, b);
}

void bn_add_to_smaller(struct list_head *a, struct list_head *b)
{
    int cmp = bn_cmp(a, b);
    if (cmp >= 0) {
        __bn_add(b, a);
    } else {
        __bn_add(a, b);
    }
}

void __bn_add(struct list_head *shorter, struct list_head *longer)
{
    int carry = 0;
    bn_node *node;
    struct list_head *longer_cur = longer->next;
    list_for_each_entry (node, shorter, list) {
        uint64_t tmp = node->val;
        node->val += bn_node_val(longer_cur) + carry;
        carry = U64_MAX - tmp >= bn_node_val(longer_cur) + carry ? 0 : 1;
        longer_cur = longer_cur->next;
        if (longer_cur == longer) {
            break;
        }
    }
    while (longer_cur != longer) {
        uint64_t tmp = bn_node_val(longer_cur);
        bn_newnode(shorter, bn_node_val(longer_cur) + carry);
        carry = U64_MAX - tmp >= carry ? 0 : 1;
        longer_cur = longer_cur->next;
    }
    while (carry) {
        if (bn_size(shorter) > bn_size(longer)) {
            uint64_t tmp = bn_node_val(node->list.next);
            bn_node_val(node->list.next) += carry;
            carry = U64_MAX - tmp >= carry ? 0 : 1;
        } else {
            bn_newnode(shorter, carry);
            break;
        }
    }
}

void bn_sub(struct list_head *a, struct list_head *b)
{
    int cmp = bn_cmp(a, b);
    if (cmp >= 0) {
        __bn_sub(a, b);
    } else {
        __bn_sub(b, a);
    }
}

void __bn_sub(struct list_head *more, struct list_head *less)
{
    int carry = 0;
    bn_node *node;
    struct list_head *less_cur = less->next;
    list_for_each_entry (node, more, list) {
        uint64_t tmp =
            (less_cur == less) ? carry : bn_node_val(less_cur) + carry;
        if (node->val >= tmp && likely(bn_node_val(less_cur) != U64_MAX - 1)) {
            node->val -= tmp;
            carry = 0;
        } else {
            node->val += (U64_MAX - tmp) + 1;
            carry = 1;
        }
        if (less_cur != less) {
            less_cur = less_cur->next;
        }
    }
    bn_node *last = list_last_entry(more, bn_node, list);
    if (last->val == 0 && likely(!list_is_singular(more))) {
        bn_size(more)--;
        list_del(&last->list);
        kfree(last);
    }
}

void bn_mul(struct list_head *a, struct list_head *b, struct list_head *c)
{
    bn_node *node;
    // zeroing c
    list_for_each_entry (node, c, list) {
        node->val = 0;
    }
    // append c to match size of b
    for (int diff = bn_size(b) - bn_size(c); diff > 0; diff--) {
        bn_newnode(c, 0);
    }
    bn_node *node_a, *node_b;
    struct list_head *base = c->next;
    list_for_each_entry (node_a, a, list) {
        uint64_t carry = 0;
        struct list_head *cur = base;
        list_for_each_entry (node_b, b, list) {
            uint128_t tmp = (uint128_t) node_b->val * (uint128_t) node_a->val;
            uint64_t n_carry = tmp >> 64;
            if (U64_MAX - bn_node_val(cur) < tmp << 64 >> 64)
                n_carry++;
            bn_node_val(cur) += tmp;
            if (U64_MAX - bn_node_val(cur) < carry)
                n_carry++;
            bn_node_val(cur) += carry;
            carry = n_carry;
            cur = cur->next;
            if (cur == c) {
                break;
            }
        }
        while (carry) {
            if (cur == c) {
                bn_newnode(c, carry);
                break;
            }
            uint64_t tmp = bn_node_val(cur);
            bn_node_val(cur) += carry;
            carry = U64_MAX - tmp >= carry ? 0 : 1;
            cur = cur->next;
        }
        base = base->next;
    }
}

void bn_strassen(struct list_head *a, struct list_head *b, struct list_head *c)
{
    int a_size = bn_size(a) * per_size - CLZ(bn_last_val(a)) / chunck_size;
    int b_size = bn_size(b) * per_size - CLZ(bn_last_val(b)) / chunck_size;
    // could not do ntt if size is too small
    if (a_size < 2 || b_size < 2) {
        bn_mul(a, b, c);
        return;
    }
    // zero padding
    int size = nextpow2((uint64_t)(a_size + b_size - 1));
    uint64_t *a_array = bn_split(a, size);
    uint64_t *b_array = bn_split(b, size);
    // number theoretic transform
    ntt(a_array, size, mod, rou);
    ntt(b_array, size, mod, rou);
    // pointwise multiplication
    for (int i = 0; i < size; i++) {
        a_array[i] *= b_array[i] % mod;
    }
    // inverse ntt
    intt(a_array, size, mod, rou);
    // carrying
    uint64_t carry = 0;
    for (int i = 0; i < size; i++) {
        a_array[i] += carry;
        carry = a_array[i] >> chunck_size;
        a_array[i] &= chunk_mask;
    }
    // convert to bn
    for (; bn_size(c) < size / per_size;) {
        bn_newnode(c, 0);
    }
    bn_node *node;
    int i = 0;
    list_for_each_entry (node, c, list) {
        // four at a time : 8 bits to uint64_t
        uint64_t val = 0;
        for (int j = 0; j < val_size; j += chunck_size) {
            if (i < size) {
                val |= a_array[i++] << j;
            } else if (carry) {
                val |= (carry & chunk_mask) << j;
                carry >>= chunck_size;
            }
        }
        node->val = val;
    }
    if (carry) {
        bn_newnode(c, carry);
    }
    bn_clean(c);
    kfree(a_array);
    kfree(b_array);
}

void bn_lshift(struct list_head *head, int bit)
{
    int tmp = bit;
    for (; tmp > 64; tmp -= 63) {
        __bn_lshift(head, 63);
    }
    __bn_lshift(head, tmp);
}

void __bn_lshift(struct list_head *head, int bit)
{
    uint64_t carry = 0;
    bn_node *node;
    list_for_each_entry (node, head, list) {
        uint64_t tmp = node->val;
        node->val <<= bit;
        node->val |= carry;
        carry = tmp >> (64 - bit);
    }
    if (carry) {
        bn_newnode(head, carry);
    }
}

void bn_rshift(struct list_head *head, int bit)
{
    int tmp = bit;
    for (; tmp > 64; tmp -= 63) {
        __bn_rshift(head, 63);
    }
    __bn_rshift(head, tmp);
}

void __bn_rshift(struct list_head *head, int bit)
{
    uint64_t carry = 0;
    bn_node *node;
    list_for_each_entry_reverse(node, head, list)
    {
        uint64_t tmp = node->val;
        node->val >>= bit;
        node->val |= carry;
        carry = tmp << (64 - bit);
    }
    if (bn_last_val(head) == 0) {
        bn_pop(head);
    }
}

uint64_t *bn_to_array(struct list_head *head)
{
    bn_clean(head);
    uint64_t *res = kmalloc(sizeof(uint64_t) * bn_size(head), GFP_KERNEL);
    int i = 0;
    bn_node *node;
    list_for_each_entry (node, head, list) {
        res[i++] = node->val;
    }
    return res;
}

uint64_t *bn_split(struct list_head *head, size_t size)
{
    bn_clean(head);
    uint64_t *res = kmalloc(sizeof(uint64_t) * size, GFP_KERNEL);
    memset(res, 0, sizeof(uint64_t) * size);
    int i = 0;
    bn_node *node;
    list_for_each_entry (node, head, list) {
        for (int j = 0; j < val_size; j += chunck_size) {
            res[i++] |= (node->val >> j) & chunk_mask;
        }
    }
    return res;
}

int bn_cmp(struct list_head *a, struct list_head *b)
{
    if (bn_size(a) < bn_size(b)) {
        return -1;
    } else if (bn_size(a) > bn_size(b)) {
        return 1;
    }
    // equal in size, compare each node from the end
    else {
        bn_node *node_a;
        struct list_head *b_cur = b->prev;
        list_for_each_entry_reverse(node_a, a, list)
        {
            bn_node *node_b = list_entry(b_cur, bn_node, list);
            if (node_a->val < node_b->val) {
                return -1;
            } else if (node_a->val > node_b->val) {
                return 1;
            } else {
                b_cur = b_cur->prev;
            }
        }
    }
    return 0;
}