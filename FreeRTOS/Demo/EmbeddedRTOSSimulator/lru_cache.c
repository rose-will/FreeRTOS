#include "lru_cache.h"
#include <stdio.h>
#include <string.h>

static lru_cache_t g_cache;

void lru_cache_init(void) {
    memset(&g_cache, 0, sizeof(g_cache));
    printf("[LRUCache] Initialized (size=%d).\n", LRU_CACHE_SIZE);
}

void lru_cache_put(int key, int value) {
    // Check if key exists
    for (int i = 0; i < LRU_CACHE_SIZE; ++i) {
        if (g_cache.entries[i].key == key) {
            g_cache.entries[i].value = value;
            g_cache.entries[i].last_used = ++g_cache.use_counter;
            printf("[LRUCache] Updated key=%d value=%d\n", key, value);
            return;
        }
    }
    // Find empty slot
    for (int i = 0; i < LRU_CACHE_SIZE; ++i) {
        if (g_cache.entries[i].last_used == 0) {
            g_cache.entries[i].key = key;
            g_cache.entries[i].value = value;
            g_cache.entries[i].last_used = ++g_cache.use_counter;
            printf("[LRUCache] Inserted key=%d value=%d\n", key, value);
            return;
        }
    }
    // Replace LRU
    int lru_idx = 0;
    uint32_t lru_val = g_cache.entries[0].last_used;
    for (int i = 1; i < LRU_CACHE_SIZE; ++i) {
        if (g_cache.entries[i].last_used < lru_val) {
            lru_val = g_cache.entries[i].last_used;
            lru_idx = i;
        }
    }
    g_cache.entries[lru_idx].key = key;
    g_cache.entries[lru_idx].value = value;
    g_cache.entries[lru_idx].last_used = ++g_cache.use_counter;
    printf("[LRUCache] Replaced LRU idx=%d with key=%d value=%d\n", lru_idx, key, value);
}

int lru_cache_get(int key, int *found) {
    for (int i = 0; i < LRU_CACHE_SIZE; ++i) {
        if (g_cache.entries[i].key == key && g_cache.entries[i].last_used != 0) {
            g_cache.entries[i].last_used = ++g_cache.use_counter;
            *found = 1;
            printf("[LRUCache] Hit key=%d value=%d\n", key, g_cache.entries[i].value);
            return g_cache.entries[i].value;
        }
    }
    *found = 0;
    printf("[LRUCache] Miss key=%d\n", key);
    return 0;
}

void lru_cache_clear(void) {
    memset(&g_cache, 0, sizeof(g_cache));
    printf("[LRUCache] Cleared.\n");
}

size_t lru_cache_count(void) {
    size_t count = 0;
    for (int i = 0; i < LRU_CACHE_SIZE; ++i) {
        if (g_cache.entries[i].last_used != 0) ++count;
    }
    return count;
}