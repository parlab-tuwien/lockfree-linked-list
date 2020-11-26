/* (C) Jesper Larsson Traff, May 2020 */
/* Improved lock-free linked list implementations */

#define COUNTERS

#ifdef COUNTERS
#define INC(_c) ((_c)++)
#else
#define INC(_c)
#endif

typedef struct _node {
  _Atomic(struct _node *) next;
  _Atomic(struct _node *) prev;
  struct _node *free; // for freelist
  char padding[40]; // fill the cacheline
  long key;
} node_t;

typedef struct _list {
  node_t *head, *tail; // sentinels, possibly shared
  
  node_t *curr; // private cursor (last operation)
  node_t *pred; // predecessor of cursor
  
  node_t *free; // private free list
  
#ifdef COUNTERS
  unsigned long long adds, rems, cons, trav, fail, rtry;
#endif
} list_t;
  
void init(node_t *head, node_t *tail, list_t* list);
void clean(list_t *list);

int add(long key, list_t *list);
int rem(long key, list_t *list);
int con(long key, list_t *list);
