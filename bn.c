#include "bn.h"

void bn_add(struct list_head *a, struct list_head *b)
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
        node->val += list_entry(longer_cur, bn_node, list)->val + carry;
        carry = node->val / BOUND;
        node->val %= BOUND;
        longer_cur = longer_cur->next;
    }
    while (longer_cur != longer) {
        bn_newnode(shorter, list_entry(longer_cur, bn_node, list)->val + carry);
        node = list_last_entry(shorter, bn_node, list);
        carry = node->val / BOUND;
        node->val %= BOUND;
        longer_cur = longer_cur->next;
    }
    if (carry) {
        bn_newnode(shorter, carry);
    }
}

void bn_sub(struct list_head *a, struct list_head *b, struct list_head *c)
{
    ;
}

void bn_mul(struct list_head *a, struct list_head *b, struct list_head *c)
{
    ;
}

void bn_lshift(struct list_head *head, int bit)
{
    ;
}

void bn_rshift(struct list_head *head, int bit)
{
    ;
}

char *bn_to_string(struct list_head *head)
{
    // printf("\nbn_to_string");
    if (!head || list_empty(head)) {
        return NULL;
    }
    bn_head *bn_list = list_entry(head, bn_head, list);
    uint64_t first_num = list_last_entry(head, bn_node, list)->val;

    size_t ceil = MAX_DIGITS;
    for (size_t floor = 0; floor + 1 != ceil;) {
        size_t *new = first_num > pow10[(floor + ceil) / 2] ? &floor : &ceil;
        *new = (floor + ceil) / 2;
    }
    size_t size = (bn_list->size - 1) * MAX_DIGITS + ceil + 1;
    char *str = kmalloc(size * sizeof(char), GFP_KERNEL);
    str[--size] = '\0';
    bn_node *node;
    list_for_each_entry (node, head, list) {
        uint64_t num = node->val;
        if (!num)
            str[--size] = '0';
        for (; num > 0; num /= 10) {
            str[--size] = num % 10 + '0';
        }
    }
    return str;
}

int bn_cmp(struct list_head *a, struct list_head *b)
{
    if (list_entry(a, bn_head, list)->size <
        list_entry(b, bn_head, list)->size) {
        return -1;
    } else if (list_entry(a, bn_head, list)->size >
               list_entry(b, bn_head, list)->size) {
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