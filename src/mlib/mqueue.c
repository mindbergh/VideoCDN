/* This is a queue
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Date: 11/26/2014
 */

#include "mqueue.h"
#include "mutil.h"

MQueue* mqueue_new(void) {
    return mnew0(MQueue);
}


bool mqueue_is_empty(MQueue* queue) {
    return queue->head == NULL;
}

unsigned int mqueue_get_length(MQueue* queue) {
    return queue->length;
}

void mqueue_foreach(MQueue* queue, MFunc func, void* func_data) {
    MList *list;

    if (queue == NULL) return;
    if (func == NULL) return;

    list = queue->head;
    while (list) {
        MList *next = list->next;
        (*func)(list->data, func_data);
        list = next;
    }
}


MList* mqueue_find(MQueue* queue, const void* data) {
    if (queue == NULL) return NULL;
    return mlist_find(queue->head, data);
}

MList* mqueue_find_custom(MQueue* queue, const void* data, MCompareFunc func) {
    if (queue == NULL) return NULL;
    if (func == NULL) return NULL;

    return mlist_find_custom(queue->head, data, func);
}


void mqueue_push_head(MQueue* queue, void* data) {
    if (queue == NULL) return;

    queue->head = mlist_prepend(queue->head, data);
    if (!queue->tail)
        queue->tail = queue->head;
    queue->length++;
}

void mqueue_push_tail(MQueue* queue, void* data) {
    if (queue == NULL) return;

    queue->tail = mlist_append(queue->tail, data);
    if (queue->tail->next)
        queue->tail = queue->tail->next;
    else
        queue->head = queue->tail;
    queue->length++;
}

void* mqueue_pop_head(MQueue *queue) {
    if (queue == NULL) return NULL;

    if (queue->head) {
        MList* node = queue->head;
        void* data = node->data;

        queue->head = node->next;
        if (queue->head)
            queue->head->prev = NULL;
        else
            queue->tail = NULL;
        free(node);
        queue->length--;

        return data;
    }

    return NULL;
}

void* mqueue_pop_tail(MQueue *queue) {
    if (queue == NULL) return NULL;

    if (queue->tail) {
        MList *node = queue->tail;
        void* data = node->data;

        queue->tail = node->prev;
        if (queue->tail)
            queue->tail->next = NULL;
        else
            queue->head = NULL;
        queue->length--;
        free(node);

        return data;
    }
    return NULL;
}
