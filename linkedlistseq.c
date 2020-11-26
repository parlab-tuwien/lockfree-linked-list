/* (C) Jesper Larsson Traff, May 2020 */
/* Improved lock-free linked list implementations */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <assert.h>

#include "linkedlist.h"

//#define DOUBLY
//#define CURSOR / only with DOUBLY

#ifdef COUNTERS
#define INC(_c) ((_c)++)
#else
#define INC(_c)
#endif

void init(node_t *head, node_t *tail, list_t *list)
{
  list->head = head;
  list->tail = tail;
  
  // the sentinels
  list->head->key = LONG_MIN;
  list->head->next = tail;
  list->head->prev = NULL;

  list->tail->key = LONG_MAX;
  list->tail->next = NULL;
  list->tail->prev = head;

  list->pred = head;
  list->curr = NULL;
  
  list->free = NULL;

#ifdef COUNTERS
  list->adds = 0;
  list->rems = 0;
  list->cons = 0;
  list->trav = 0;
  list->fail = 0;
  list->rtry = 0;
#endif
}

void clean(list_t *list)
{
  node_t *next, *node;

  next = list->free;
  while (next!=NULL) {
    node = next; next = next->free;
    free(node);
  }
  list->free = NULL;
}

void pos(long key, list_t *list)
{
  node_t *pred, *curr;

#ifdef DOUBLY
#ifdef CURSOR
  curr = list->pred;
#else
  curr = list->head;
#endif
  if (curr->key>=key) {
    pred = curr->prev;
    while (key<pred->key) {
      curr = pred; pred = pred->prev;
      INC(list->trav);
    }
  } else
#else
  curr = list->head;
#endif
    {
      do {
	pred = curr; curr = curr->next;
	INC(list->trav);
      } while (key>curr->key);
    }
  
  list->pred = pred;
  list->curr = curr;
  assert(pred->key<curr->key);
}

int add(long key, list_t *list)
{
  node_t *pred, *curr, *node;

  pos(key,list);
  pred = list->pred; curr = list->curr;
  if (curr->key==key) return 0; // already there
  
  INC(list->adds);
  
  node = (node_t*)malloc(sizeof(node_t));
  assert(node!=NULL);
  
  node->key = key;
  node->next = curr;
  pred->next = node;
  
#ifdef DOUBLY
  node->prev = pred;
  curr->prev = node;
#endif
  
  return 1;
}

int rem(long key, list_t *list)
{
  node_t *pred, *node;

  pos(key,list);
  pred = list->pred;
  node = list->curr;
  if (node->key!=key) return 0; // not there
  
  INC(list->rems);
  
  pred->next = node->next;
#ifdef DOUBLY
  node->next->prev = pred;
#endif
  
  node->free = list->free;
  list->free = node;
  
  return 1;
}
 
int con(long key, list_t *list)
{
  node_t *curr;

#ifdef DOUBLY
#ifdef CURSOR
  curr = list->pred;
#else
  curr = list->head;
#endif
  INC(list->cons);
  while (key<curr->key) {
    curr = curr->prev;
    INC(list->cons);
  }
#else
  curr = list->head;
#endif
  while (key>curr->key) {
    curr = curr->next;
    INC(list->cons);
  }

#ifdef DOUBLY
#ifdef CURSOR
  list->pred = curr;
#endif
#endif
  
  return (curr->key==key);
}
