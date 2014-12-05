/* This is a double linked list
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Date: 11/26/2014
 */

#ifndef MLIST_H
#define MLIST_H
#include <mlib/mtypes.h>

typedef struct _MList MList;

struct _MList {
  void* data;
  MList* prev;
  MList* next;
};


MList* mlist_new(void);
MList* mlist_append(MList* list, void* data);
MList* mlist_prepend(MList* list, void* data);
MList* mlist_insert (MList* list, void* data, int position);
MList* mlist_get(MList *list, unsigned int n);
void* mlist_getdata(MList* list, unsigned int n);
MList* mlist_find(MList* list, const void* data);
MList* mlist_find_custom(MList* list, const void* data, MCompareFunc func);
int mlist_index(MList* list, void* data);
MList* mlist_last(MList *list);
unsigned int mlist_length(MList *list);
void mlist_foreach(MList* list, MFunc func, void* func_data);
MList* mlist_remove(MList *list, void* data);
MList* mlist_remove_all(MList *list, void* data);
MList* mlist_remove_link(MList *list, MList *link);
MList* mlist_delete_link(MList *list, MList *link);
void mlist_free(MList *list);
void mlist_free_full(MList *list);

#endif
