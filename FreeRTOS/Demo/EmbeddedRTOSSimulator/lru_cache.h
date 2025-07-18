#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdint.h>
#include <stddef.h>

#define LRU_CACHE_SIZE 8

typedef struct {
    int key;
    int value;
    uint32_t last_used;
} lru_entry_t;

typedef struct {
    lru_entry_t entries[LRU_CACHE_SIZE];
    uint32_t use_counter;
} lru_cache_t;

void lru_cache_init(void);
void lru_cache_put(int key, int value);
int lru_cache_get(int key, int *found);
void lru_cache_clear(void);
size_t lru_cache_count(void);

#endif // LRU_CACHE_H