#ifndef _PASSMNGR_HASHMAP
#define _PASSMNGR_HASHMAP

#include <stdint.h>

#include "fast-hash/fasthash.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

// This shall be an open addressing hash map
// the hashing algorithm employed shall be robinhood hashing with a probe length limit
// The map shall maintain power of 2 capacities and a probe length limit of log2(capacity)
// Care shall be taken to reduce the number of allocations.

// Define the expression to calculate the next and previous
// The capacities are powers of 2, handling potential 0 lengths
// The masking function is directly related to the capacity restrictions
#define NEXT_CAPACITY(X)     ((!(X)) + ((X) * 2))
#define PREVIOUS_CAPACITY(X) ((X) / 2)
#define SHRINK_SIZE(C)       ((3 * PREVIOUS_CAPACITY(C)) / 4)
#define INIT_CAPACITY        (16)
#define INIT_MAX_PROBE       (4)
#define MASK(X, C)           ((X) & ((C) - 1))

#define HASH_SEED            (0xDEADBEEF)

struct node {
    char* key;
    void* value;
    uint8_t dist; // : 7;
    uint8_t used; // : 1;
};

struct probe_result {
    uint8_t found          ;//: 2;
    uint8_t limit_exceeded ;//: 2;
    uint8_t smaller_found  ;//: 2;
    uint8_t empty_found    ;//: 2;
    uint8_t probe_len;
    size_t  index;
};

typedef struct _hash_map {
    struct node* table;
    
    size_t capacity;
    size_t size;
    uint8_t max_probe;

    void (*deallocator)(void*);
} HashMap;

int         hm_init     (HashMap* hm);
void        hm_destroy  (HashMap* hm);
int         hm_set      (HashMap* hm, char* key, void* value);
int         hm_set_copy (HashMap* hm, char* key, void* value);
int         hm_delete   (HashMap* hm, char* key);
void*       hm_get      (HashMap* hm, char* key);

struct probe_result hm_int_probe         (HashMap* hm, uint32_t hash, char* key);
int                 hm_int_insert_copy   (HashMap* hm, uint32_t hash, char* key, void* value);
int                 hm_int_insert_nocopy (HashMap* hm, uint32_t hash, char* key, void* value);
int                 hm_int_remove        (HashMap* hm, uint32_t hash, char* key);
int                 hm_int_grow          (HashMap* hm);
int                 hm_int_shrink        (HashMap* hm);
uint32_t            hm_int_hash          (char* key, size_t length);

#endif