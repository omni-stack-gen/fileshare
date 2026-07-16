#include "list.h"

void base_list_clear (BaseList * list)
{
   list -> sentinel.next = & list -> sentinel;
   list -> sentinel.previous = & list -> sentinel;
}

BaseListIterator base_list_insert (BaseListIterator position, void * data)
{
   BaseListIterator result = (BaseListIterator) data;

   result -> previous = position -> previous;
   result -> next = position;

   result -> previous -> next = result;
   position -> previous = result;

   return result;
}

BaseListIterator base_list_insertafter (BaseListIterator position, void * data)
{
	BaseListIterator result = (BaseListIterator) data;

	result->next = position->next;
	position->next->previous = result;

	position->next = result;
	result->previous = position;

	return result;
}

void * base_list_remove (BaseListIterator position)
{
   position -> previous -> next = position -> next;
   position -> next -> previous = position -> previous;

   return position;
}

BaseListIterator base_list_move (BaseListIterator position, void * dataFirst, void * dataLast)
{
   BaseListIterator first = (BaseListIterator) dataFirst,
                    last = (BaseListIterator) dataLast;

   first -> previous -> next = last -> next;
   last -> next -> previous = first -> previous;

   first -> previous = position -> previous;
   last -> next = position;

   first -> previous -> next = first;
   position -> previous = last;
    
   return first;
}

size_t base_list_size (BaseList * list)
{
   size_t size = 0;
   BaseListIterator position;

   for (position = base_list_begin (list);
        position != base_list_end (list);
        position = base_list_next (position))
     ++ size;
   
   return size;
}

/** @} */
