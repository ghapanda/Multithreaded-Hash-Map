# Hash Hash Hash

In this lab, we make insertion to a hash table thread-safe

## Building

```shell
make
```

## Running

```shell
./hash-table-test -t <number of threads/cores> -s <number of entries to be inserted>
```

## First Implementation

In the `hash_table_v1_add_entry` function, I added one pthread*mutex_t lock to hash_table_v1 struct. Since we are only using one lock, we have to lock the entire hash table upon entry insertion. This means that multiple threads cannot insert entries to the hash table simultaneously, and there is no parallelism. Hence, all insertions are sequential plus the overhead of context switch from one thread to another as insertions are sequential and critical code. As a result, the performance ends up being slower than the base. This mutex is locked in `hash_table_v1_add_entry` before checking whether an entry with the desired key already exists or not. This is crucial to ensure that we are not inserting two entries with the same key into the hash table. The mutex is unlocked after `SLIST_INSERT_HEAD(list_head, list_entry, pointers)`; During each call to pthread_mutex\*\* functions, we check if it was successful and if not `hash_table_v1_destroy(struct hash_table_v1 *hash_table)` is called to free the memory occupied by the hash table and destroy the lock.

### Performance

```shell
 ./hash-table-tester -t 4  -s 100000
Generation: 55,650 usec
Hash table base: 408,527 usec
  - 0 missing
Hash table v1: 1,013,009 usec
  - 0 missing
Hash table v2: 108,087 usec
  - 0 missing
```

Version 1 is a little slower/faster than the base version. As we are only using one lock, we have to lock the entire hash table upon entry insertion. This means that multiple threads cannot insert entries to the hash table simultaneously, and there is no parallelism. Hence, all insertions are sequential plus the overhead of context switch from one thread to another. As a result, the performance ends up being slower than the base.

In addition to the locking issue, if V1 creates and destroys threads dynamically, it incurs thread definition-related overhead. This includes the cost of resource allocation, initializing thread contexts, and registering threads with the operating system. Combined with the lack of parallelism, this further degrades performance.

## Second Implementation

In the `hash_table_v2_add_entry` function, I added a mutex to each `hash_table_entry` struct and this mutex is being locked right after acquiring the list_head and prior to checking whether the key already exists or not.This is to prevent the case where thread 1 finds the `list_entry` as null and goes for inserting the new entry but gets interrupted and thread 2 inserts an entry with the exact same key, but thread 1 does not do the if statement checking again and we will end up having two entries with the same key. However, in order to minimize the amount of code in the critical section and exploit parallelism, node creation and call to calloc for a new entry and setting the key and values is moved outside of the critical section. This new node is assigned to the pointer `temp` and inside the critical section we only have to set 'list_entry' pointer to 'temp' or free() 'temp' if the key already exists.

### Performance

```shell
 ./hash-table-tester -t 4  -s 100000
Generation: 55,650 usec
Hash table base: 408,527 usec
  - 0 missing
Hash table v1: 1,013,009 usec
  - 0 missing
Hash table v2: 108,087 usec
  - 0 missing
```

TODO more results, speedup measurement, and analysis on v2
I added a mutex to `hash_table_entry` struct. Upon insertion into each bucket (aka hash_table_entry), that entry is being locked. Hence, multiple threads can insert to the hash table simultaneously as long as it is not in the same bucket/linked list. As a result parallelism is levereged and we see a better performance.

## Cleaning up

```shell
make clean
```
