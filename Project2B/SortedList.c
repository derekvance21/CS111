#include "SortedList.h"
#include <sched.h>
#include <stdlib.h>


void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedList_t* iter = list->next;
    while (iter->key && (*(element->key) > *(iter->key))) {
        iter = iter->next;
    }
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    iter->prev->next = element;
    element->prev = iter->prev;
    iter->prev = element;
    element->next = iter;
}

int SortedList_delete( SortedListElement_t *element) {
    if (!element)
        return 0;
    if (element->next->prev != element || element->prev->next != element)
        return 0;
    element->prev->next = element->next;
    if (opt_yield & DELETE_YIELD)
        sched_yield();
    element->next->prev = element->prev;
    return 1;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedList_t* iter = list->next;
    while (iter->key && (*(iter->key) != *key)) {
        iter = iter->next;
    }
    if (opt_yield & LOOKUP_YIELD)
        sched_yield();
    return iter->key ? iter : NULL;
}

int SortedList_length(SortedList_t *list) {
    int len = 0;
    SortedList_t* iter = list->next;

    while (iter != list) {
        iter = iter->next;  
        len++;
    }
    if (opt_yield & LOOKUP_YIELD)
        sched_yield();
    return len;
}