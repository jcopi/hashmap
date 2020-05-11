#include <hashmap.h>

#include <stdlib.h>
#include <string.h>

struct probe_result hm_int_probe (HashMap* hm, uint32_t hash, char* key)
{
    struct probe_result result;
    
    // Compute table index for this key
    size_t index = MASK(hash, hm->capacity), i;
    uint8_t probe = 0;

    while (probe <= hm->max_probe) {
        i = MASK(index + probe, hm->capacity);
        // Probe linearly until an empty space is found
        // or the probe limit is exceeded
        if (!(hm->table[i].used)) {
            // Empty space found, the provided key does not exist in the map
            result.found = FALSE;
            result.smaller_found = FALSE;
            result.limit_exceeded = FALSE;
            result.empty_found = TRUE;
            result.probe_len = probe;
            result.index = i;

            return result;
        }
        if (hm->table[i].dist < probe) {
            // An entry with a smaller probe value found
            // The provided key does not exist in the map
            result.found = FALSE;
            result.smaller_found = TRUE;
            result.limit_exceeded = FALSE;
            result.empty_found = FALSE;
            result.probe_len = probe;
            result.index = i;

            return result;
        }
        if (hm->table[i].dist == probe && strcmp(hm->table[i].key, key) == 0) {
            // The provided key is found in the map
            result.found = TRUE;
            result.smaller_found = FALSE;
            result.limit_exceeded = FALSE;
            result.empty_found = FALSE;
            result.probe_len = probe;
            result.index = i;

            return result;
        }
        
        ++probe;
    }

    result.found = FALSE;
    result.smaller_found = FALSE;
    result.limit_exceeded = TRUE;
    result.empty_found = FALSE;
    result.probe_len = probe;
    result.index = i;

    return result;
}

int hm_int_insert_nocopy (HashMap* hm, uint32_t hash, char* key, void* value)
{
    struct probe_result probe;
    probe = hm_int_probe(hm, hash, key);
    // If the value already exists only the value needs to be updated
    if (probe.found) {
        // The user will provide a deallocator if the values will need deallocation
        if (NULL != hm->deallocator) {
            hm->deallocator(hm->table[probe.index].value);
        }
        hm->table[probe.index].value = value;

        return 0;
    } else if (probe.empty_found) {
        // If probing found an empty location it can be populated with the provided information
        hm->table[probe.index].used = TRUE;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key;
        hm->table[probe.index].value = value;

        hm->size++;

        return 0;
    } else if (probe.smaller_found) {
        // An proper insertion point was found that requires robinhood hashing
        char* new_key = hm->table[probe.index].key;
        void* new_value = hm->table[probe.index].value;
        uint32_t new_hash = hm_int_hash(new_key, strlen(new_key));
        //uint8_t new_dist = hm->table[probe.index].dist;
               
        hm->table[probe.index].used = TRUE;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key;
        hm->table[probe.index].value = value;

        // An oppotunity exists to tune performance
        // The next probing sequence could be made to start at the previous 'dist'
        return hm_int_insert_nocopy(hm, new_hash, new_key, new_value);
    } else {
        int grow_result = hm_int_grow(hm);
        if (0 != grow_result) {
            return grow_result;
        }

        return hm_int_insert_nocopy(hm, hash, key, value);
    }
}

int hm_int_insert_copy (HashMap* hm, uint32_t hash, char* key, void* value)
{
    struct probe_result probe;
    probe = hm_int_probe(hm, hash, key);
    // If the value already exists only the value needs to be updated
    if (probe.found) {
        // The user will provide a deallocator if the values will need deallocation
        if (NULL != hm->deallocator) {
            hm->deallocator(hm->table[probe.index].value);
        }
        hm->table[probe.index].value = value;

        return 0;
    } else if (probe.empty_found) {
        // If probing found an empty location it can be populated with the provided information
        size_t len = strlen(key) + 1; // Include Null Terminator
        char* key_copy = calloc(len, sizeof (char));
        if (key_copy == NULL) {
            return -1;
        }
        strncpy(key_copy, key, len);

        hm->table[probe.index].used = TRUE;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key_copy;
        hm->table[probe.index].value = value;

        hm->size++;

        return 0;
    } else if (probe.smaller_found) {
        // An proper insertion point was found that requires robinhood hashing
        size_t len = strlen(key);
        char* key_copy = calloc(len, sizeof (char));
        if (key_copy == NULL) {
            return -1;
        }
        strncpy(key_copy, key, len);

        char* new_key = hm->table[probe.index].key;
        void* new_value = hm->table[probe.index].value;
        uint32_t new_hash = hm_int_hash(new_key, strlen(new_key));
        //uint8_t new_dist = hm->table[probe.index].dist;
               
        hm->table[probe.index].used = TRUE;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key_copy;
        hm->table[probe.index].value = value;

        // An oppotunity exists to tune performance
        // The next probing sequence could be made to start at the previous 'dist'
        return hm_int_insert_nocopy(hm, new_hash, new_key, new_value);
    } else {
        int grow_result = hm_int_grow(hm);
        if (0 != grow_result) {
            return grow_result;
        }

        return hm_int_insert_copy(hm, hash, key, value);
    }
}

int hm_int_remove (HashMap* hm, uint32_t hash, char* key)
{
    struct probe_result probe;
    probe = hm_int_probe(hm, hash, key);

    if (probe.found) {
        // The entry is found delete it
        free(hm->table[probe.index].key);
        if (NULL != hm->deallocator) {
            hm->deallocator(hm->table[probe.index].value);
        }
        hm->table[probe.index] = (struct node){0};
        hm->size--;

        size_t index = probe.index, pindex = probe.index, probe_len = 0;
        for (;;) {
            probe_len++;
            index = MASK(probe.index + probe_len, hm->capacity);

            if (FALSE == hm->table[index].used || 0 == hm->table[index].dist) {
                break;
            } else {
                hm->table[pindex] = hm->table[index];
                hm->table[pindex].dist--;
                hm->table[index] = (struct node){0};
            }

            pindex = index;
        }

        return hm_int_shrink(hm);
    }

    return 0;
}

int hm_int_grow (HashMap* hm)
{
    size_t next_cap = NEXT_CAPACITY(hm->capacity);
    uint8_t prev_max_probe = hm->max_probe;
    uint8_t next_max_probe = hm->max_probe + 1; //hm_int_log2(next_cap);

    struct node* prev_table = hm->table;
    size_t prev_size = hm->size;
    size_t prev_capacity = hm->capacity;

    hm->table = calloc(next_cap, sizeof (struct node));
    if (hm->table == NULL) {
        // Allocation failed put everything back as it was previously
        hm->table = prev_table;
        return -1;
    }

    hm->capacity = next_cap;
    hm->max_probe = next_max_probe;
    hm->size = 0;
    size_t i, j;
    for (i = 0, j = 0; i < prev_capacity && j < prev_size; ++i) {
        if (prev_table[i].used) {
            ++j;
            uint32_t hash = hm_int_hash(prev_table[i].key, strlen(prev_table[i].key));
            int result = hm_int_insert_nocopy(hm, hash, prev_table[i].key, prev_table[i].value);
            if (result != 0) {
                // Failed to insert an item, put map back the way it was
                free(hm->table);
                hm->table = prev_table;
                hm->size = prev_size;
                hm->capacity = prev_capacity;
                hm->max_probe = prev_max_probe;
                
                return -1;
            }
        }
    }

    free(prev_table);
    return 0;
}

int hm_int_shrink (HashMap* hm) {
    if (hm->size >= SHRINK_SIZE(hm->capacity) || hm->capacity <= INIT_CAPACITY) {
        return 0;
    }

    size_t next_cap = PREVIOUS_CAPACITY(hm->capacity);
    uint8_t prev_max_probe = hm->max_probe;
    uint8_t next_max_probe = hm->max_probe - 1; //hm_int_log2(next_cap);

    struct node* prev_table = hm->table;
    size_t prev_size = hm->size;
    size_t prev_capacity = hm->capacity;

    hm->table = calloc(next_cap, sizeof (struct node));
    if (hm->table == NULL) {
        // Allocation failed put everything back as it was previously
        hm->table = prev_table;
        return -1;
    }

    hm->capacity = next_cap;
    hm->max_probe = next_max_probe;
    hm->size = 0;
    size_t i, j;
    for (i = 0, j = 0; i < prev_capacity && j < prev_size; ++i) {
        if (prev_table[i].used) {
            ++j;
            uint32_t hash = hm_int_hash(prev_table[i].key, strlen(prev_table[i].key));
            int result = hm_int_insert_nocopy(hm, hash, prev_table[i].key, prev_table[i].value);
            if (result != 0) {
                // Failed to insert an item, put map back the way it was
                free(hm->table);
                hm->table = prev_table;
                hm->size = prev_size;
                hm->capacity = prev_capacity;
                hm->max_probe = prev_max_probe;
                
                return -1;
            }
        }
    }

    free(prev_table);
    return 0;
}

uint32_t hm_int_hash (char* key, size_t length)
{
    return fasthash32(key, length, HASH_SEED);
}

int hm_init (HashMap* hm)
{
    *hm = (HashMap){0};
    hm->capacity = INIT_CAPACITY;
    hm->max_probe = INIT_MAX_PROBE;

    hm->table = calloc(hm->capacity, sizeof (struct node));
    if (NULL == hm->table) {
        return -1;
    }

    return 0;
}

void hm_destroy (HashMap* hm) {
    size_t i;
    for (i = 0; i < hm->capacity; i++) {
        if (hm->table[i].used) {
            free(hm->table[i].key);
            if (NULL != hm->deallocator) {
                hm->deallocator(hm->table[i].value);
            }
        }
    }

    free(hm->table);
    *hm = (HashMap){0};
}

int hm_set (HashMap* hm, char* key, void* value)
{
    return hm_int_insert_nocopy(hm, hm_int_hash(key, strlen(key)), key, value);
}

int hm_set_copy (HashMap* hm, char* key, void* value)
{
    return hm_int_insert_copy(hm, hm_int_hash(key, strlen(key)), key, value);
}

int hm_delete (HashMap* hm, char* key)
{
    return hm_int_remove(hm, hm_int_hash(key, strlen(key)), key);
}

void* hm_get (HashMap* hm, char* key)
{
    struct probe_result probe;
    probe = hm_int_probe(hm, hm_int_hash(key, strlen(key)), key);
    if (probe.found) {
        return hm->table[probe.index].value;
    }

    return NULL;
}