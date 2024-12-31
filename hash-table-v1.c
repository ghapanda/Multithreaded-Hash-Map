#include "hash-table-base.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <pthread.h>

#include <errno.h>





struct list_entry {
	const char *key;
	uint32_t value;
	SLIST_ENTRY(list_entry) pointers;
};

SLIST_HEAD(list_head, list_entry);

struct hash_table_entry {
	struct list_head list_head;
};

struct hash_table_v1 {
	struct hash_table_entry entries[HASH_TABLE_CAPACITY];
	//to do
	pthread_mutex_t lock;
	bool lock_initialized;
	bool lock_islocked;
};




void hash_table_v1_destroy(struct hash_table_v1 *hash_table)
{
	//to do
	bool failure_exit=false;
	int errno_;
	if(hash_table->lock_initialized){
		if(hash_table->lock_islocked){
			if(pthread_mutex_unlock(&hash_table->lock)!=0){
				failure_exit=true;
				errno_=errno;
			}
			else if(pthread_mutex_destroy(&(hash_table->lock))!=0){
               failure_exit=true;
			   errno_=errno;
			}
		}
		else if(pthread_mutex_destroy(&(hash_table->lock))!=0){
			failure_exit=true;
			errno_=errno;
		}
		
	}
	
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		struct list_head *list_head = &entry->list_head;
		struct list_entry *list_entry = NULL;
		while (!SLIST_EMPTY(list_head)) {
			list_entry = SLIST_FIRST(list_head);
			SLIST_REMOVE_HEAD(list_head, pointers);
			free(list_entry);
		}
	}
	free(hash_table);
	if(failure_exit){
		exit(errno_);
	}
}



struct hash_table_v1 *hash_table_v1_create()
{
	struct hash_table_v1 *hash_table = calloc(1, sizeof(struct hash_table_v1));
	assert(hash_table != NULL);
	hash_table->lock_initialized=false;
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		SLIST_INIT(&entry->list_head);
	}
    //to do
	if(pthread_mutex_init(&(hash_table->lock),NULL)!=0){
	    hash_table_v1_destroy(hash_table);
        exit(errno);
	}
	hash_table->lock_initialized=true;
	hash_table->lock_islocked=false;
	return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v1 *hash_table,
                                                     const char *key)
{
	assert(key != NULL);
	uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
	struct hash_table_entry *entry = &hash_table->entries[index];
	return entry;
}

static struct list_entry *get_list_entry(struct hash_table_v1 *hash_table,
                                         const char *key,
                                         struct list_head *list_head)
{
	assert(key != NULL);

	struct list_entry *entry = NULL;
	
	SLIST_FOREACH(entry, list_head, pointers) {
	  if (strcmp(entry->key, key) == 0) {
	    return entry;
	  }
	}
	return NULL;
}

bool hash_table_v1_contains(struct hash_table_v1 *hash_table,
                            const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	return list_entry != NULL;
}



//wrappers
void Pthread_mutex_lock(struct hash_table_v1* hash_table){
	if(pthread_mutex_lock(&(hash_table->lock))!=0){
		hash_table_v1_destroy(hash_table);
		exit(errno);
	}
}

void Pthread_mutex_unlock(struct hash_table_v1* hash_table){
	if(pthread_mutex_unlock(&(hash_table->lock))!=0){
		hash_table_v1_destroy(hash_table);
		exit(errno);
	}
}
void hash_table_v1_add_entry(struct hash_table_v1 *hash_table,
                             const char *key,
                             uint32_t value)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
     //to do
	
	struct list_head *list_head = &hash_table_entry->list_head;
	Pthread_mutex_lock(hash_table);
	hash_table->lock_islocked=true;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);

	/* Update the value if it already exists */
	if (list_entry != NULL) {
		list_entry->value = value;
		hash_table->lock_islocked=false;
		Pthread_mutex_unlock(hash_table);
		return;
	}

	list_entry = calloc(1, sizeof(struct list_entry));
	list_entry->key = key;
	list_entry->value = value;
	
	SLIST_INSERT_HEAD(list_head, list_entry, pointers);
	hash_table->lock_islocked=false;
	Pthread_mutex_unlock(hash_table);
}

uint32_t hash_table_v1_get_value(struct hash_table_v1 *hash_table,
                                 const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	assert(list_entry != NULL);
	return list_entry->value;
}


