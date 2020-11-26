A more Pragmatic Implementation of the Lock-free, Ordered, Linked List
====

The lock-free ordered, linked list is an important, standard example
of a concurrent data structure. An obvious, practical drawback of
textbook implementations is that failed compare-and-swap (`CAS`)
operations lead to retraversal of the entire list (retries), which
is particularly harmful for a linear-time data structure. We
alleviate this drawback by first observing that failed `CAS`
operations under some conditions do not require a full retry, and
second by maintaining approximate backwards pointers that are used
to find a closer starting position in the list for operation
retry. Experiments with both a worst-case deterministic benchmark,
and a standard, randomized, mixed-operation throughput benchmark on
three shared-memory systems (Intel Xeon, AMD EPYC, SPARC-T5) show
practical improvements ranging from significant to dramatic several
orders of magnitude.

Publications
----
*   [A more Pragmatic Implementation of the Lock-free, Ordered, Linked List](https://arxiv.org/abs/2010.15755)  
J. Träff, M. Pöter / Report for CoRR - Computing Research Repository; Report No. arXiv:2010.15755, 2020

Building and running
----
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4
```

The build process generates six different executables:
* `ldraconic` - this implements the list as proposed by Harris (also referred to as "textbook implementation" in the paper).
* `ldoubly` - this implements the list with approximate backward pointers and retry from head of list.
* `ldoubly_cursor` - as `ldoubly` with per thread retry from the last recorded position (cursor) in the list.
* `lsingly` - this implements the list with the mild improvements described in the paper.
* `lsingly_cursor` - as `lsingly` with per thread retry from the last recorded position (cursor) in the list.
* `lsingly_cursor_fetch` - as `lsingly_cursor` but uses `fetch_or` to set the delete mark on the next pointer.

Each of the executables takes a number of arguments:
* `-p <threads>` - the number of threads; optional, defaults to `omp_get_max_threads()`
* `-B [D|S]` - the benchmark to run - D = deterministic; S = steady (randomized). If omitted, both are run, starting with deterministc.

Additional arguments for deterministic benchmark:
* `-n <elements>` - the number of elements; optional, defaults to 10000
* `-r <add factor>` - factor to calculate effective key in insert operations; optional, defaults to number of threads
* `-o <add offset>` - offset to calculate effective key in insert operations; optional, defaults to 0
* `-R <remove factor>` - factor to calculate effective key in remove operations; optional, defaults to number of threads
* `-O <remove offset>` - offset to calculate effective key in remove operations; optional, defaults to 0

Additional arguments for randomized (steady) benchmark:
* `-S <seed>` - randomization seed
* `-f <prefill>` - the number of items for prefill; optional, defaults to 10000
* `-U <keyrange>` - the key range; optional, defaults to 10*prefill
* `-A <add propability>` - propability of insert operation in percent (0-100); optional, defaults to 10
* `-R <remove propability>` - propability of remove operation in percent (0-100); optional, defaults to 10
* `-c <ops>` - number of operations; optional, defaults to 10000

The `run_benchmark.sh` script runs each executable in three different configurations:
1. Deterministic benchmark with `k(i)=i`, p=[threads], n=100000
2. Deterministic benchmark with `k(i)=t+ip`, p=[threads], n=10000
3. Random operation mix benchmark, p=[threads], c=1000000, f=1000, U=10000$; operation Mix 10% add, 10% remove, 80% lookup.

The number of threads can be configured by setting the `THREAD` environment variable, e.g., `THREADS=16 run_benchmark.sh`.

These settings correspond to the ones used to produce the result tables in the paper.

The `run_steady_benchmark.sh` scripts executes the randomized (steady) benchmark for each executable with varying numbers
of threads and the following parameters: c=50000, f=16384, U=32768; operation mix 25% add, 25% remove, 50% lookup.   
Each configuration is run five times, and all the results are collected in the result file `steady_results.csv`.
These settings correspond to the ones used to produce the scalability charts in the paper. Please note, that you have
to edit the shell script directly in order to change the varying thread numbers.
