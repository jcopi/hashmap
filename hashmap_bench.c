#include <hashmap.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_SIZE (128)
#define INSERT_RESULT_FILE ("hm_bench_insertion_timing.csv")
#define FIND_EXISTING_RESULT_FILE ("hm_bench_find_timing.csv")
#define REMOVE_RESULT_FILE ("hm_bench_removal_timing.csv")

#ifdef _WIN32
#include <windows.h>
uint64_t get_time()
{
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (uint64_t)(1e9*((double)t.QuadPart/(double)f.QuadPart));
}
#else
#include <time.h>
uint64_t get_time()
{
    struct timespec time1;
    clock_gettime(CLOCK_REALTIME, &time1);
    return (time1.tv_sec * 1e9) + time1.tv_nsec; 
}
#endif

int main (int argc, const char** argv)
{
    if (argc != 2) {
        printf("Not enough arguments provided. A key list must be provided\n");
        return 1;
    }

    const char* keyfile = argv[1];

    char** keys;
    intptr_t* values;

    size_t keycnt = 0;

    keys = NULL;
    values = NULL;

    // read the list of keys and generate a list of key-value pairs to test the hashmap with
    FILE *keyfp;
    keyfp = fopen(keyfile, "r");

    if (keyfp == NULL) {        
        fprintf(stderr, "Failed to open key list. Exiting...\n");
        exit(1);
    }

    srand(42);
    char buffer[MAX_KEY_SIZE];

    while (fgets(buffer, MAX_KEY_SIZE, keyfp)) {
        keycnt++;
        keys = realloc(keys, sizeof (char*) * keycnt);
        values = realloc(values, sizeof (intptr_t) * keycnt);
        if (keys == NULL || values == NULL) {
            fprintf(stderr, "Out of memory.\n");
            exit(1);
        }
        size_t l = strlen(buffer);
        if (buffer[l - 1] == '\n') buffer[l - 1] = '\0';
        keys[keycnt - 1] = strdup(buffer);
        if (keys[keycnt - 1] == NULL) {
            fprintf(stderr, "Out of memory.\n");
            exit(1);
        }
        values[keycnt - 1] = rand();
    }

    fclose(keyfp);

    // Allocate a list of times, and load factors to document results
    uint64_t* times = calloc(keycnt, sizeof (uint64_t));
    float* load_factors = calloc(keycnt, sizeof (float));
    if (times == NULL || load_factors == NULL) {
        fprintf(stderr, "Failed to allocate timing buffers for testing. Exiting...\n");
        exit(1);
    }

    intptr_t sum = 0;

    HashMap hashmap;
    HashMap* hm = &hashmap;

    if (0 != hm_init(hm)) goto exitcond;

    for (size_t i = 0; i < keycnt; i++) {
        uint64_t start, end;
        start = get_time();
        hm_set(hm, keys[i], (void*)values[i]);
        end  = get_time();

        times[i] = end - start;
        load_factors[i] = (float)hm->size / (float)hm->capacity;
    }
    FILE *insfp;
    insfp = fopen(INSERT_RESULT_FILE, "w");
    if (insfp == NULL) {
        fprintf(stderr, "Failed to open output file. Exiting...\n");
        goto exitcond;
    }
    for (size_t i = 0; i < keycnt; i++) {
        fprintf(insfp, "%lu, %lu, %f\n", i, times[i], load_factors[i]);
    }
    fclose(insfp);

    for (size_t i = 0; i < keycnt; i++) {
        uint64_t start, end;
        intptr_t v;
        start = get_time();
        v = (intptr_t)hm_get(hm, keys[i]);
        end  = get_time();

        sum += v;
        if (v != values[i]) {
            fprintf(stderr, "Hashmap looked up the wrong value for '%s'\n", keys[i]);
        }

        times[i] = end - start;
        load_factors[i] = (float)hm->size / (float)hm->capacity;
    }
    printf("%li\n", sum);
    FILE *findfp;
    findfp = fopen(FIND_EXISTING_RESULT_FILE, "w");
    if (findfp == NULL) {
        fprintf(stderr, "Failed to open output file. Exiting...\n");
        goto exitcond;
    }
    for (size_t i = 0; i < keycnt; i++) {
        fprintf(findfp, "%lu, %lu, %f\n", i, times[i], load_factors[i]);
    }
    fclose(findfp);

    for (size_t i = 0; i < keycnt; i++) {
        uint64_t start, end;
        start = get_time();
        hm_delete(hm, keys[i]);
        end  = get_time();

        times[i] = end - start;
        load_factors[i] = (float)hm->size / (float)hm->capacity;
    }
    FILE *delfp;
    delfp = fopen(REMOVE_RESULT_FILE, "w");
    if (delfp == NULL) {
        fprintf(stderr, "Failed to open output file. Exiting...\n");
        goto exitcond;
    }
    for (size_t i = 0; i < keycnt; i++) {
        fprintf(delfp, "%lu, %lu, %f\n", i, times[i], load_factors[i]);
    }
    fclose(delfp);

exitcond:
    hm_destroy(hm);

    free(keys);
    free(values);
    free(times);
    free(load_factors);

    return 0;
}