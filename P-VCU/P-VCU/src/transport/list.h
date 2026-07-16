/** 
 @file  list.h
*/
#ifndef __BASE_LIST_H__
#define __BASE_LIST_H__

#include <stdlib.h>

typedef struct _BaseListNode
{
   struct _BaseListNode * next;
   struct _BaseListNode * previous;
} BaseListNode;

typedef BaseListNode * BaseListIterator;

typedef struct _BaseList
{
   BaseListNode sentinel;
} BaseList;

extern void base_list_clear (BaseList *);

extern BaseListIterator base_list_insert (BaseListIterator, void *);
BaseListIterator base_list_insertafter (BaseListIterator position, void * data);
extern void * base_list_remove (BaseListIterator);
extern BaseListIterator base_list_move (BaseListIterator, void *, void *);

extern size_t base_list_size (BaseList *);

#define base_list_begin(list) ((list) -> sentinel.next)
#define base_list_end(list) (& (list) -> sentinel)

#define base_list_empty(list) (base_list_begin (list) == base_list_end (list))

#define base_list_next(iterator) ((iterator) -> next)
#define base_list_previous(iterator) ((iterator) -> previous)

#define base_list_front(list) ((void *) (list) -> sentinel.next)
#define base_list_back(list) ((void *) (list) -> sentinel.previous)

#endif /* __BASE_LIST_H__ */

