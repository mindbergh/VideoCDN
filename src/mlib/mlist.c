/* This is a doubly linked list for mlib
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Date: 11/26/2014
 */

#include "mlist.h"
#include "mutil.h"

MList* mlist_new(void) {
    return mnew0(MList);
}

MList* mlist_append(MList* list, void* data) {
    MList *new_list;
    MList *last;

    new_list = mlist_new();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = mlist_last(list);
        last->next = new_list;
        new_list->prev = last;

        return list;
    } else {
        new_list->prev = NULL;
        return new_list;
    }
}

MList* mlist_prepend(MList* list, void* data) {
    MList *new_list;

    new_list = mlist_new();
    new_list->data = data;
    new_list->next = list;

    if (list) {
        new_list->prev = list->prev;
        if (list->prev)
            list->prev->next = new_list;
        list->prev = new_list;
    } else {
        new_list->prev = NULL;
    }
    return new_list;
}

/* insert data into the given list at given position
 * @param list the list into which the data is inserted
 * @param data the given data to be instered
 * @param position if position <  0: append new data
 *                 if position == 0: prepend new data
 *                 otherwise       : insert new data
 * @return the pointer to the head
 */

MList* mlist_insert(MList* list, void* data, int position) {
    MList *new_list;
    MList *tmp_list;

    if (position < 0)
        return mlist_append(list, data);
    else if (position == 0)
        return mlist_prepend(list, data);

    tmp_list = mlist_get(list, position);
    if (!tmp_list)
        return mlist_append (list, data);

    new_list = mlist_new();
    new_list->data = data;
    new_list->prev = tmp_list->prev;
    tmp_list->prev->next = new_list;
    new_list->next = tmp_list;
    tmp_list->prev = new_list;

    return list;
}


MList* mlist_get(MList *list, unsigned int n) {
    while((n-- > 0) && list)
        list = list->next;

    return list;
}

void* mlist_getdata(MList* list, unsigned int n) {
    while (n-- > 0 && list)
        list = list->next;

    return list ? list->data : NULL;
}

MList* mlist_find(MList* list, const void* data) {
    while (list) {
        if (list->data == data)
            break;
        list = list->next;
    }

    return list;
}

MList* mlist_find_custom(MList* list, const void* data, MCompareFunc func) {
    if (func == NULL) return NULL;

    while (list) {
        if (!(*func)(list->data, data))
            return list;
        list = list->next;
    }
    return NULL;
}


int mlist_index(MList* list, void* data) {
    int i = 0;
    while (list) {
        if (list->data == data)
            return i;
        i++;
        list = list->next;
    }

    return -1;
}

MList* mlist_last(MList *list) {
    if (list) {
        while (list->next)
            list = list->next;
    }

    return list;
}

unsigned int mlist_length(MList *list) {
    unsigned int length = 0;
    while (list) {
        length++;
        list = list->next;
    }

    return length;
}

void mlist_foreach(MList* list, MFunc func, void* func_data) {
    while (list) {
        MList *next = list->next;
        (*func)(list->data, func_data);
        list = next;
    }
}


/* remove a node with the given data
 * if multiple nodes contain the given data, only remove the 1st one
 */
MList* mlist_remove(MList *list, void* data) {
    MList *tmp;

    tmp = list;
    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        } else {
            list = mlist_remove_link(list, tmp);
            free(tmp);

            break;
        }
    }
    return list;
}


MList* mlist_remove_all(MList *list, void* data) {
    MList *tmp = list;

    while (tmp) {
        if (tmp->data != data)
            tmp = tmp->next;
        else {
            MList* next = tmp->next;

            if (tmp->prev)
                tmp->prev->next = next;
            else
                list = next;
            if (next)
                next->prev = tmp->prev;

            free(tmp);
            tmp = next;
        }
    }
    return list;
}


/* remove given node from the list
*/
MList* mlist_remove_link(MList *list, MList *link) {
    if (link == NULL)
        return list;

    if (link->prev) {
        if (link->prev->next == link)
            link->prev->next = link->next;
        else
            fprintf(stderr, "corrupted double-linked list detected");
    }
    if (link->next) {
        if (link->next->prev == link)
            link->next->prev = link->prev;
        else
            fprintf(stderr, "corrupted double-linked list detected");
    }

    if (link == list)
        list = list->next;

    link->next = NULL;
    link->prev = NULL;

    return list;
}


/* remove given node from the list and free it
*/
MList* mlist_delete_link(MList* list, MList* link) {
    list = mlist_remove_link(list, link);
    free(link);

    return list;
}


/* Free the entire list but does not free data
*/
void mlist_free(MList *list) {
    if (list == NULL) {
        return;
    }
    mlist_free(list->next);
    free(list);
    return;
}

/* Free the entire list and associated data
 */
void mlist_free_full(MList *list) {
    if (list == NULL) {
        return;
    }
    mlist_free(list->next);
    free(list->data);
    free(list);
    return;
}


