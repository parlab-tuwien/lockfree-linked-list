/* (C) Jesper Larsson Traff, May 2020, October 2020 */
/* Improved lock-free linked list implementations */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <stdatomic.h> // gcc -latomic 

#include <assert.h>

#include "linkedlist.h"

//#define DOUBLY
//#define CURSOR / only with DOUBLY
//#define TEXTBOOK
//#define FETCH

// Memory model
//#define SC

#define UNMARK_MASK ~1
#define MARK_BIT 0x0000000000001

#define getpointer(_markedpointer)  ((node_t*)(((long)_markedpointer) & UNMARK_MASK))
#define ismarked(_markedpointer)    ((((long)_markedpointer) & MARK_BIT) != 0x0)
#define setmark(_markedpointer)     ((node_t*)(((long)_markedpointer) | MARK_BIT))

#ifdef SC
#define CAS(_a,_e,_d) atomic_compare_exchange_weak(_a,_e,_d)
#define LOAD(_a)      atomic_load(_a)
#define STORE(_a,_e)  atomic_store(_a,_e)
#define FAO(_a,_e)    atomic_fetch_or(_a,_e)
#else
#define CAS(_a,_e,_d) atomic_compare_exchange_weak_explicit(_a,_e,_d,memory_order_acq_rel,memory_order_acquire)
#define LOAD(_a)      atomic_load_explicit(_a,memory_order_acquire)
#define STORE(_a,_e)  atomic_store_explicit(_a,_e,memory_order_release)
#define FAO(_a,_e)    atomic_fetch_or_explicit(_a,_e,memory_order_acq_rel)
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
  while (next != NULL) {
    node = next;
    next = next->free;
    free(node);
  }
  list->free = NULL;
}

void pos(long key, list_t *list)
{
  node_t *pred, *succ, *curr, *next;

#ifdef DOUBLY
#ifdef CURSOR
  pred = list->pred;
#else
  pred = list->pred;
  if (key <= pred->key)
    pred = list->head;
#endif

retry:
  while (ismarked(LOAD(&pred->next)) || key <= pred->key) {
    INC(list->trav);
    pred = LOAD(&pred->prev);
  }
  curr = getpointer(LOAD(&pred->next));
  INC(list->trav);
#else // DOUBLY
retry:
#ifdef TEXTBOOK
  pred = list->head;
#else
  pred = list->pred;
  if (ismarked(LOAD(&pred->next)) || key <= pred->key)
    pred = list->head;
#endif

  curr = getpointer(LOAD(&pred->next));
  INC(list->trav);
#endif // DOUBLY
  assert(pred->key < key);

  do {
    succ = LOAD(&curr->next);
    while (ismarked(succ)) {
      succ = getpointer(succ);
      if (!CAS(&pred->next, &curr, succ)) {
        INC(list->fail);
#ifdef TEXTBOOK
        INC(list->rtry);
        goto retry;
#else
        next = LOAD(&pred->next);
        if (ismarked(next)) {
          INC(list->rtry);
          goto retry;
        }
        succ = next;
#endif
      }
#ifdef DOUBLY
      else
        STORE(&succ->prev, pred);
#endif

      curr = getpointer(succ);
      succ = LOAD(&succ->next);
      INC(list->trav);
    }
#ifdef DOUBLY
    if (LOAD(&curr->prev) != pred)
      STORE(&curr->prev, pred);
#endif

    if (key <= curr->key) {
      assert(pred->key < curr->key);
      list->pred = pred;
      list->curr = curr;
      return;
    }
    pred = curr;
    curr = getpointer(LOAD(&curr->next));
    INC(list->trav);
  } while (1);
}

int add(long key, list_t *list)
{
  node_t *pred, *curr, *node;

  node = (node_t*)malloc(sizeof(node_t));
  assert(node != NULL);
  node->key = key;

#ifndef CURSOR
  list->pred = list->head;
#endif
  do {
    pos(key, list);
    pred = list->pred;
    curr = list->curr;
    if (curr->key == key) {
      free(node);
      return 0; // already there
    }

    node->next = curr;
#ifdef DOUBLY
    node->prev = pred;
#endif

    if (CAS(&pred->next, &curr, node)) {
      INC(list->adds);
#ifdef DOUBLY
      STORE(&curr->prev, node);
#endif

      return 1;
    }
    INC(list->fail);
  } while (1);
}

int rem(long key, list_t *list)
{
  node_t *pred, *succ, *node;
  node_t *markedsucc;

  do {
    pos(key, list);
    pred = list->pred;
    node = list->curr;
    if (node->key != key)
      return 0; // not there

#ifdef TEXTBOOK
    succ = getpointer(LOAD(&node->next)); // unmarked
    markedsucc = setmark(succ);

    if (!CAS(&node->next, &succ, markedsucc)) {
      INC(list->fail);
      continue;
    }
#else
#ifdef FETCH
    // x86 supports atomic-or, but not atomic-fetch-or, i.e., we cannot get the old value.
    // If we do not use the return value of the fetch-or, the compiler is smart enough to
    // use an atomic-or. If we use the return value, the atomic-fetch-or is emulated via
    // a CAS loop.
    // In this case it would be sufficient to use a pure atomic-or, although it has the
    // drawback that we cannot tell WHO has set the bit, so we would also have to adapt
    // the management of the free-list.

    succ = (node_t*)FAO((_Atomic uint64_t*)&node->next, MARK_BIT);
    if (ismarked(succ)) return 0;
#else    
    succ = LOAD(&node->next);
    do {
      if (ismarked(succ))
        return 0;
      markedsucc = setmark(succ);
      if (CAS(&node->next, &succ, markedsucc))
        break;
      INC(list->fail);
    } while (1);
#endif
#endif
    
    if (!CAS(&pred->next, &node, succ))
      node = list->curr; // beware!
#ifdef DOUBLY
    STORE(&succ->prev, pred);
#endif

    node->free = list->free;
    list->free = node;
    INC(list->rems);

    return 1;
  } while (1);
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
  while (key < curr->key) {
    curr = LOAD(&curr->prev);
    INC(list->cons);
  }
#else // DOUBLY
#ifdef CURSOR
  curr = list->pred;
  if (key < curr->key)
    curr = list->head;
#else
  curr = list->head;
#endif
#endif // DOUBLY
  assert(curr->key <= key);

  while (key > curr->key) {
    curr = getpointer(LOAD(&curr->next));
    INC(list->cons);
  }

#ifdef CURSOR
  list->pred = curr;
#endif

  return (curr->key == key && !ismarked(LOAD(&curr->next)));
}
