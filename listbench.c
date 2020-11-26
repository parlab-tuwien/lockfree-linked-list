/* (C) Jesper Larsson Traff, May 2020 */

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <omp.h>

#include "linkedlist.h"

#define N 10000

//#define PRIVATE

#define MICRO 1000000.0
#define MILLI 1000.0
#define KOPS  1000

//#define TEST(_A) assert(_A) // just _A when shared with overlap
#define TEST(_A) if (!(_A)) printf("Line %d: t %d key %ld\n",__LINE__,t,key)
//#define TEST(_A) assert( _A)

// stress linearity benchmark
void benchmark1(int n, int p, int ar, int ao, int rr, int ro, int verbose,
		int latex)
{
  double time;
  int disjoint;

  disjoint = (ar==rr&&ao==ro)&&((ar==p)||(ar==1&&ao==n));

  time = 0.0;
  
  // performance counters
#ifdef COUNTERS
  unsigned long long tops, adds, rems, cons, trav, fail, rtry;

  tops = 0;

  adds = 0;
  rems = 0;
  cons = 0;
  trav = 0;
  fail = 0;
  rtry = 0;
#endif
  
#ifndef PRIVATE
  node_t head, tail; // shared list
#endif    

#ifndef PRIVATE
#pragma omp parallel shared(head) shared(tail) reduction(max:time) reduction(+:tops,adds,rems,cons,trav,fail)
#else
#pragma omp parallel reduction(max:time) reduction(+:tops,adds,rems,cons,trav,fail)
#endif  
  {
    double start, stop;
#ifdef PRIVATE
    node_t head, tail;
#endif
    list_t list;

#ifdef COUNTERS
    int ops = 0;
#endif
    int t = omp_get_thread_num();
    long key;

    init(&head,&tail,&list);
    
#pragma omp barrier
    start = omp_get_wtime();
    
    int i;
    int ok;
    for (i=0; i<n; i++) {
      key = i;
      key = key*ar+t*ao+t%ar;
      ok = !con(key,&list); INC(ops);
      TEST(!disjoint||ok);  
      ok = add(key,&list);  INC(ops);
      TEST(!disjoint||ok);
      ok = con(key,&list);  INC(ops);
      TEST(!disjoint||ok);
      ok = !add(key,&list); INC(ops);
      TEST(!disjoint||ok);
    }

    for (i=n-1; i>=0; i--) {
      key = i;
      key = key*ar+t*ao+t%ar;
      ok = con(key,&list);  INC(ops);
      TEST(!disjoint||ok);
      ok = rem(key,&list);  INC(ops);
      TEST(!disjoint||ok);
      ok = !con(key,&list); INC(ops);
      TEST(!disjoint||ok);
      ok = !rem(key,&list); INC(ops);
      TEST(!disjoint||ok);
    }

    for (i=0; i<n; i++) {
      key = i;
      key = key*ar+t*ao+t%ar;
      ok = !con(key,&list); INC(ops);
      TEST(!disjoint||ok);
    }

    stop = omp_get_wtime();
#pragma omp barrier
    if (time<stop-start) time = stop-start;
    
    tops += ops;

    adds += list.adds;
    rems += list.rems;
    cons += list.cons;
    trav += list.trav;
    fail += list.fail;
    rtry += list.rtry;
    if (verbose) {
      printf("DET Thread %d: ops %d adds %llu rems %llu cons %llu trav %llu fail %llu rtry %llu\n",
	     t,ops,list.adds,list.rems,list.cons,list.trav,list.fail,list.rtry);
    }

    clean(&list);
  }

  printf("DET Threads: %d\n",p);
  if (latex) {
    printf("Time (ms) & Total ops & Throughput (Kops/s) & adds & rems & cons& trav & fail & rtry \\\\\n");
    printf("%.2f & %llu & %.2f & %llu & %llu & %llu & %llu & %llu & %llu \\\\\n",
	   time*MILLI,tops,((double)tops/time)/KOPS,
	   adds,rems,cons,trav,fail,rtry);
  } else {
    printf("Time (ms) %.2f Total ops %llu Throughput (Kops/s) %.2f\n",
	   time*MILLI,tops,((double)tops/time)/KOPS);
    printf("adds %llu rems %llu cons %llu trav %llu fail %llu rtry %llu\n",
	   adds,rems,cons,trav,fail,rtry);
  }
}

// random mix
void benchmark2(int n, int p, int f, int U, int pa, int pr, unsigned seed,
		int verbose, int latex, int csv)
{
  double time;

  time = 0.0;
  
  // performance counters
#ifdef COUNTERS
  unsigned long long tops, adds, rems, cons, trav, fail, rtry;

  tops = 0;
  
  adds = 0;
  rems = 0;
  cons = 0;
  trav = 0;
  fail = 0;
  rtry = 0;
#endif
  
#ifndef PRIVATE
  node_t head, tail; // shared list
#endif

#ifdef PRIVATE
#pragma omp parallel reduction(max:time) reduction(+:tops,adds,rems,cons,trav,fail)
#else
#pragma omp parallel shared(head) shared(tail) reduction(max:time) reduction(+:tops,adds,rems,cons,trav,fail)
#endif
  {
    double start, stop;
#ifdef PRIVATE
    node_t head, tail;
#endif

    list_t list;
    int i;

    int t = omp_get_thread_num();

#ifdef COUNTERS
    int ops = 0;
#endif
    
#if defined(sun) || defined(__sun)
    // Solaris does not support random_r
    srand(seed + t);
#else
    struct random_data rbuf;
    char rstate[32];
    
    rbuf.state = NULL;
    initstate_r(seed+t,rstate,32,&rbuf);
#endif

    long key;

    init(&head,&tail,&list);

    // prefill
#ifndef PRIVATE
#pragma single nowait
#endif
    {
      int k;
      for (i=0; i<f; i++) {
#if defined(sun) || defined(__sun)      
        k = rand();
#else
        random_r(&rbuf,&k);
#endif
        key = k%U;
        add(key,&list);
      }
    
      list.adds = 0;
      list.rems = 0;
      list.cons = 0;
      list.trav = 0;
      list.fail = 0;
      list.rtry = 0;
    }
    
#pragma omp barrier
    start = omp_get_wtime();
    
    int op, k;
    for (i=0; i<n; i++) {
#if defined(sun) || defined(__sun)      
      k = rand();
#else
      random_r(&rbuf,&k);
#endif
      key = k%U;

#if defined(sun) || defined(__sun)      
      op = rand();
#else
      random_r(&rbuf,&op);
#endif
      op = op%100;
      if (op<pa) {
	add(key,&list); INC(ops);
      } else if (op<pa+pr) {
	rem(key,&list); INC(ops);
      } else {
	con(key,&list); INC(ops);
      }
    }
    
    stop = omp_get_wtime();
#pragma omp barrier
    if (time<stop-start) time = stop-start;

    tops += ops;
    
    adds += list.adds;
    rems += list.rems;
    cons += list.cons;
    trav += list.trav;
    fail += list.fail;
    rtry += list.rtry;
    if (verbose) {
      printf("STEADY Thread %d: ops %d adds %llu rems %llu cons %llu trav %llu fail %llu rtry %llu\n",
	     t,ops,list.adds,list.rems,list.cons,list.trav,list.fail,list.rtry);
    }

#ifndef PRIVATE
#pragma omp single
#endif
    {
      node_t *node = list.head->next;
      while (node!=&tail) {
	node_t *curr = node;
	node = node->next;
	free(curr);
      }
    }
    
    clean(&list);
  }

#if defined(TEXTBOOK)
  char* benchmark = "draconic";
#elif defined(CURSOR)
#if defined(DOUBLY)
  char* benchmark = "doubly_cursor";
#else
  char* benchmark = "singly_cursor";
#endif
#elif defined(DOUBLY)
  char* benchmark = "doubly";
#else
  char* benchmark = "singly";
#endif

  printf("STEADY Threads: %d\n",p);
  if (latex) {
    printf("Time (ms) & Total ops & Throughput (Kops/s) & adds & rems & cons& trav & fail & rtry \\\\\n");
    printf("%.2f & %llu & %.2f & %llu & %llu & %llu & %llu & %llu & %llu \\\\\n",
	   time*MILLI,tops,((double)tops/time)/KOPS,
	   adds,rems,cons,trav,fail,rtry);
  } else if (csv) {
    printf("Time (ms);Total ops;Throughput (Kops/s);adds;rems;cons;trav;fail;rtry;threads;benchmark\n");
    printf("%.2f;%llu;%.2f;%llu;%llu;%llu;%llu;%llu;%llu;%d;%s\n",
      time*MILLI, tops, ((double)tops/time)/KOPS, adds, rems, cons, trav, fail, rtry, p, benchmark);
  } else {
    printf("Time (ms) %.2f Total ops %llu Throughput (Kops/s) %.2f\n",
	   time*MILLI,tops,((double)tops/time)/KOPS);
    printf("adds %llu rems %llu cons %llu trav %llu fail %llu rtry %llu\n",
	   adds,rems,cons,trav,fail,rtry);
  }
}

int main(int argc, char *argv[])
{
  int i;

  int p; // number of threads
  
  int n, f, c;
  int ar, ao, rr, ro; // add and remove factors and offsets
  int pa, pr; // percentage (integer) of adds and removes
  int verbose, latex, csv;

  int U;
  unsigned seed;
  char benchmark = '_'; // _ = both; D = deterministic; S = steady
  
  n = N;
  f = N;
  c = N;
  
  p = -1;

  ar = -1;
  ao = -1;
  rr = -1;
  ro = -1;

  U = -1;
  pa = 10; pr = 10; // 10% add, 10% rem
  
  verbose = 0;
  latex = 0;
  csv=0;
  
  for (i=1; i<argc&&argv[i][0]=='-'; i++) {
    if (argv[i][1]=='h') {
      printf("Quadratic benchmark:\n");
      printf("-n\tNumber of elements\n");
      printf("Randomized throughput benchmark:\n");
      printf("-c\tNumber of operations\n");
    }
    if (argv[i][1]=='n') i++,sscanf(argv[i],"%d",&n); // number elements (deterministic benchmark)
    if (argv[i][1]=='p') i++,sscanf(argv[i],"%d",&p); // number threads

    if (argv[i][1]=='r') i++,sscanf(argv[i],"%d",&ar); // add factor
    if (argv[i][1]=='o') i++,sscanf(argv[i],"%d",&ao); // add offset
    if (argv[i][1]=='R') i++,sscanf(argv[i],"%d",&rr); // remove factor
    if (argv[i][1]=='O') i++,sscanf(argv[i],"%d",&ro); // remove offset

    if (argv[i][1]=='c') i++,sscanf(argv[i],"%d",&c); // number operations (randomized benchmark)
    if (argv[i][1]=='f') i++,sscanf(argv[i],"%d",&f); // prefill
    if (argv[i][1]=='U') i++,sscanf(argv[i],"%d",&U); // key-range (default 10*prefill)
    if (argv[i][1]=='A') i++,sscanf(argv[i],"%d",&pa);
    if (argv[i][1]=='R') i++,sscanf(argv[i],"%d",&pr);

    if (argv[i][1]=='S') i++,sscanf(argv[i],"%d",&seed);
    if (argv[i][1]=='V') verbose = 1;
    if (argv[i][1]=='L') latex = 1;
    if (argv[i][1]=='C') csv = 1;

    if (argv[i][1]=='B') {
       i++;
      benchmark = argv[i][0];
      if (benchmark != 'D' && benchmark != 'S')
        benchmark = '_';
    }
  }

  if (p<=0) p = omp_get_max_threads(); // default
  else omp_set_num_threads(p);

  if (benchmark == 'D' || benchmark == '_') {
    if (ar==-1) ar = p;
    if (ao==-1) ao = 0;
    if (rr==-1) rr = p;
    if (ro==-1) ro = 0;
    benchmark1(n,p,ar,ao,rr,ro,verbose,latex);
  }

  if (benchmark == 'S' || benchmark == '_') {
    assert(pa+pr<=100);
    if (U==-1) U = 10*f;
    benchmark2(c,p,f,U,pa,pr,seed,verbose,latex,csv);
  }
  
  return 0;
}
