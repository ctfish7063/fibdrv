#include "bn.h"

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
        node->val += bn_node_val(longer_cur) + carry;
        carry = node->val / BOUND;
        node->val %= BOUND;
        longer_cur = longer_cur->next;
        if (longer_cur == longer) {
            break;
        }
    }
    while (longer_cur != longer) {
        bn_newnode(shorter, bn_node_val(longer_cur) + carry);
        node = list_last_entry(shorter, bn_node, list);
        carry = node->val / BOUND;
        node->val %= BOUND;
        longer_cur = longer_cur->next;
    }
    if (carry) {
        if (bn_size(shorter) > bn_size(longer)) {
            bn_node_val(node->list.next) += carry;
        } else
            bn_newnode(shorter, carry);
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
        if (node->val >= tmp) {
            node->val -= tmp;
            carry = 0;
        } else {
            node->val += BOUND - tmp;
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
    struct list_head *tmp_a = bn_new(0);
    struct list_head *tmp_b = bn_new(0);
    bn_copy(tmp_a, a);
    bn_copy(tmp_b, b);
    bn_node *node;
    // zeroing c
    list_for_each_entry (node, c, list) {
        node->val = 0;
    }
    while (!list_empty(tmp_b)) {
        uint64_t bit = bn_first_val(tmp_b);
        if (bit & 1) {
            bn_add(c, tmp_a);
        }
        bn_rshift(tmp_b, 1);
        bn_lshift(tmp_a, 1);
    }
    bn_free(tmp_a);
    bn_free(tmp_b);
}

void bn_lshift(struct list_head *head, int bit)
{
    if (bit <= 4) {
        __bn_lshift(head, bit);
    } else {
        while (bit > 4) {
            __bn_lshift(head, 4);
            bit -= 4;
        }
        __bn_lshift(head, bit);
    }
}

void __bn_lshift(struct list_head *head, int bit)
{
    int carry = 0;
    bn_node *node;
    list_for_each_entry (node, head, list) {
        node->val <<= bit;
        node->val += carry;
        carry = node->val / BOUND;
        node->val %= BOUND;
    }
    if (carry) {
        bn_newnode(head, carry);
    }
}

void bn_rshift(struct list_head *head, int bit)
{
    for (int i = 0; i < bit; i++) {
        __bn_rshift(head);
    }
}

void __bn_rshift(struct list_head *head)
{
    uint64_t carry = 0;
    bn_node *node;
    list_for_each_entry_reverse(node, head, list)
    {
        uint64_t tmp = node->val & 1;
        node->val >>= 1;
        node->val += carry;
        carry = tmp * BOUND / 2;
    }
    bn_node *last = list_last_entry(head, bn_node, list);
    if (last->val == 0) {
        bn_size(head)--;
        list_del(&last->list);
        kfree(last);
    }
}

char *bn_to_string(struct list_head *head)
{
    if (!head || list_empty(head)) {
        return NULL;
    }
    bn_clean(head);
    uint64_t first_num = bn_last_val(head);
    size_t ceil = MAX_DIGITS;
    for (size_t floor = 0; floor + 1 != ceil;) {
        size_t *new = first_num >= pow10[(floor + ceil) / 2] ? &floor : &ceil;
        *new = (floor + ceil) / 2;
    }
    size_t size = (bn_size(head) - 1) * MAX_DIGITS + ceil + 1;
    char *str = kmalloc(size * sizeof(char), GFP_KERNEL);
    str[--size] = '\0';
    // if the bn represent a zero
    if (unlikely(bn_last_val(head) == 0 && list_is_singular(head))) {
        str[--size] = '0';
    } else {
        bn_node *node;
        list_for_each_entry (node, head, list) {
            uint64_t num = node->val;
            size_t tmp_size = size;
            for (; num > 0; num /= 10) {
                str[--size] = num % 10 + '0';
            }
            while (tmp_size - size < MAX_DIGITS && size > 0) {
                str[--size] = '0';
            }
        }
    }
    return str;
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