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
	//to do
	pthread_mutex_t lock;
	bool lock_initialized;
	bool lock_isLocked;
};

struct hash_table_v2 {
	struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

void hash_table_v2_destroy(struct hash_table_v2 *hash_table)
{
	bool failure_exit=false;
	int errno_;
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		struct list_head *list_head = &entry->list_head;
		if(entry->lock_initialized){
			if(entry->lock_isLocked){
				if(pthread_mutex_unlock(&entry->lock)!=0){
					failure_exit=true;
					errno_=errno;
				}
				else if(pthread_mutex_destroy(&entry->lock)!=0){
					failure_exit=true;
					errno_=errno;
				}
			}
			else if(pthread_mutex_destroy(&entry->lock)!=0){
                   failure_exit=true;
				   errno_=errno;
			};
		}
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
struct hash_table_v2 *hash_table_v2_create()
{
	struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
	assert(hash_table != NULL);
	for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
		struct hash_table_entry *entry = &hash_table->entries[i];
		SLIST_INIT(&entry->list_head);
		if(pthread_mutex_init(&entry->lock,NULL)!=0){
			hash_table_v2_destroy(hash_table);
			exit(errno);
		}
		entry->lock_initialized=true;
		entry->lock_isLocked=false;
	}
	return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v2 *hash_table,
                                                     const char *key)
{
	assert(key != NULL);
	uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
	struct hash_table_entry *entry = &hash_table->entries[index];
	return entry;
}

static struct list_entry *get_list_entry(struct hash_table_v2 *hash_table,
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

bool hash_table_v2_contains(struct hash_table_v2 *hash_table,
                            const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	return list_entry != NULL;
}


//wrappers
void Pthread_mutex_lock_v2(struct hash_table_entry* entry,struct hash_table_v2* hash_table){
	if(pthread_mutex_lock(&(entry->lock))!=0){
		hash_table_v2_destroy(hash_table);
		exit(errno);
	}
}

void Pthread_mutex_unlock_v2(struct hash_table_entry* entry,struct hash_table_v2* hash_table){
	if(pthread_mutex_unlock(&(entry->lock))!=0){
		hash_table_v2_destroy(hash_table);
		exit(errno);
	}
}
void hash_table_v2_add_entry(struct hash_table_v2 *hash_table,
                             const char *key,
                             uint32_t value)
{

	struct list_entry* temp = calloc(1, sizeof(struct list_entry));
	temp->key = key;
	temp->value = value;

	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;

	//To do
	Pthread_mutex_lock_v2(hash_table_entry, hash_table);
	hash_table_entry->lock_isLocked=true;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);

	/* Update the value if it already exists */
	if (list_entry != NULL) {
		list_entry->value = value;
		hash_table_entry->lock_isLocked=false;
		free(temp);
		Pthread_mutex_unlock_v2(hash_table_entry,hash_table);
		return;
	}
	list_entry=temp;
	SLIST_INSERT_HEAD(list_head, list_entry, pointers);
	hash_table_entry->lock_isLocked=false;
	Pthread_mutex_unlock_v2(hash_table_entry,hash_table);
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table,
                                 const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	assert(list_entry != NULL);
	return list_entry->value;
}









