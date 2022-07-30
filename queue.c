#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

typedef int (*cmp_func_t)(const struct list_head *, const struct list_head *);

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (head)
        INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    if (!list_empty(head)) {
        element_t *curr, *next;
        list_for_each_entry_safe (curr, next, head, list) {
            q_release_element(curr);
        }
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *elem = malloc(sizeof(element_t));
    if (!elem)
        return false;

    elem->value = strdup(s);
    if (!elem->value) {
        free(elem);
        return false;
    }

    list_add(&elem->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *elem = malloc(sizeof(element_t));
    if (!elem)
        return false;

    elem->value = strdup(s);
    if (!elem->value) {
        free(elem);
        return false;
    }

    list_add_tail(&elem->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *first = list_first_entry(head, element_t, list);
    if (sp) {
        strncpy(sp, first->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del(&first->list);
    return first;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *last = list_last_entry(head, element_t, list);
    if (sp) {
        strncpy(sp, last->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del(&last->list);
    return last;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;

    int cnt = 0;
    struct list_head *iter;
    list_for_each (iter, head) {
        ++cnt;
    }
    return cnt;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;

    struct list_head *slow, *fast;
    for (slow = head->next, fast = slow->next;
         fast != head && fast->next != head;
         slow = slow->next, fast = fast->next->next) {
    }
    element_t *mid = list_entry(slow, element_t, list);
    list_del(slow);
    q_release_element(mid);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;

    if (list_empty(head))
        return true;

    bool del = false;
    element_t *curr, *next, *tmp;
    list_for_each_entry_safe (curr, next, head, list) {
        while (&next->list != head && !strcmp(curr->value, next->value)) {
            tmp = list_first_entry(&next->list, element_t, list);
            list_del(&next->list);
            q_release_element(next);
            del = true;
            next = tmp;
        }
        if (del) {
            list_del(&curr->list);
            q_release_element(curr);
            del = false;
        }
    }
    return true;
}

static inline void swap(struct list_head **a, struct list_head **b)
{
    struct list_head *tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head))
        return;

    struct list_head *odd, *even;
    for (odd = head->next, even = odd->next; odd != head && even != head;
         odd = odd->next, even = odd->next) {
        list_move(odd, even);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *left, *right;
    for (left = head->next, right = head->prev; left != right;
         left = left->next, right = right->prev) {
        struct list_head *right_prev = right->prev;
        if (right_prev == left) {
            list_move(left, right);
            return;
        }
        list_move(right, left);
        list_move(left, right_prev);
        swap(&left, &right);
    }
}

int list_cmp(const struct list_head *a, const struct list_head *b)
{
    return strcmp(list_entry(a, element_t, list)->value,
                  list_entry(b, element_t, list)->value);
}

static struct list_head *merge(cmp_func_t cmp,
                               struct list_head *former,
                               struct list_head *latter)
{
    struct list_head *head = NULL, **indirect = &head;
    while (former && latter) {
        struct list_head **smaller =
            (cmp(former, latter) <= 0) ? &former : &latter;
        *indirect = *smaller;
        indirect = &(*indirect)->next;
        *smaller = (*smaller)->next;
    }
    *indirect = (struct list_head *) ((uintptr_t) former | (uintptr_t) latter);

    return head;
}

static struct list_head *impl_sort(struct list_head *head)
{
    struct list_head *slow = head, *fast = head->next;
    if (!fast) {
        return slow;
    }

    for (; fast && fast->next; slow = slow->next, fast = fast->next->next) {
    }
    fast = slow->next;
    slow->next = NULL;
    slow = head;
    slow = impl_sort(slow);
    fast = impl_sort(fast);
    return merge(list_cmp, slow, fast);
}

void my_mergesort(struct list_head *head)
{
    head->prev->next = NULL;
    head->next = impl_sort(head->next);

    struct list_head *curr, *prev = head;
    for (curr = head->next; curr; curr = curr->next) {
        curr->prev = prev;
        prev = curr;
    }
    head->prev = prev;
    prev->next = head;
}

/* Sort elements of queue in ascending order */
void q_sort(struct list_head *head)
{
    if (!head || list_empty(head) || head->prev == head->next)
        return;

    my_mergesort(head);
}
