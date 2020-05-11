#include <hashmap.h>

#include <stdint.h>
#include <string.h>

#define TV_1 ((intptr_t)42)
#define TV_2 ((intptr_t)36)

#define TK_1 ("first key")
#define TK_2 ("second key")

int main ()
{
    HashMap hm;
    HashMap* hashmap = &hm;

    hm_init(hashmap);

    hm_set(hashmap, strdup(TK_1), (void*)TV_1);
    hm_set(hashmap, strdup(TK_2), (void*)TV_2);
    if ((intptr_t)hm_get(hashmap, TK_1) != TV_1) return 1;
    if ((intptr_t)hm_get(hashmap, TK_2) != TV_2) return 1;
    hm_delete(hashmap, TK_1);
    hm_delete(hashmap, TK_2);
    if (hm_get(hashmap, TK_1) != NULL) return 1;
    if (hm_get(hashmap, TK_2) != NULL) return 1;

    hm_destroy(hashmap);

    return 0;
}