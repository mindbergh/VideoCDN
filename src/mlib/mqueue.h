/* This is a queue
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Date: 11/26/2014
 */

#ifndef MQUEUE_H
#define MQUEUE_H
#include <mlib/mtypes.h>
#include <mlib/mlist.h>

typedef struct _MQueue MQueue;

struct _MQueue {
  MList* head;
  MList* tail;
  unsigned int length;
};



MQueue* mqueue_new(void);
bool mqueue_is_empty(MQueue* queue);
unsigned int mqueue_get_length(MQueue* queue);
void mqueue_foreach(MQueue* queue, MFunc func, void* func_data);
MList* mqueue_find(MQueue* queue, const void* data);
void mqueue_push_head(MQueue* queue, void* data);
void mqueue_push_tail(MQueue* queue, void* data);
void* mqueue_pop_head(MQueue *queue);
void* mqueue_pop_tail(MQueue *queue);

#endif
