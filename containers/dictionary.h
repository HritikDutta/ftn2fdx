#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdlib.h>

#include "hd_assert.h"
#include "string.h"

#ifndef DICT_GROWTH_RATE
#define DICT_GROWTH_RATE 2
#endif // DICT_GROWTH_RATE

#ifndef DICT_START_CAP
#define DICT_START_CAP 1024
#endif // DICT_START_CAP

#ifndef DICT_MAX_LOAD
#define DICT_MAX_LOAD 0.75
#endif // DICT_MAX_LOAD

#define Dict(type) \
    struct              \
    {                   \
        size_t cap;     \
        size_t filled;  \
                        \
        struct          \
        {               \
            String key; \
            type value; \
        }* buckets;     \
    }

#define _gen_name_internal(n, c) n ##c
#define _gen_name(n, c) _gen_name_internal(n, c)

#define Dict_Bkt(type) \
    struct _gen_name(bkt, __COUNTER__) \
    {                   \
        String key;     \
        type value;     \
    }*

typedef struct
{
    String key;
    char value[];
} Dict_Bucket_Internal;

typedef struct
{
    size_t index;
    void* ptr;
} Dict_Itr;

#define dict_bucket_at(buckets, index, bkt_size) (void*)((char*) buckets + index * bkt_size)

#define dict_make(dict) \
    do                                                                \
    {                                                                 \
        dict.cap = DICT_START_CAP;                                    \
        dict.filled = 0;                                              \
        dict.buckets = calloc(DICT_START_CAP, sizeof(*dict.buckets)); \
        hd_assert(dict.buckets != NULL);                              \
    } while (0)
    
#define dict_resize(dict, _cap) \
    do                                                                                              \
    {                                                                                               \
        size_t new_cap = _cap;                                                                      \
        Dict_Bucket_Internal* new_bkts = calloc(new_cap, sizeof(*dict.buckets));                    \
        hd_assert(new_bkts != NULL);                                                                \
                                                                                                    \
        for (int i = 0; i < dict.cap; i++)                                                          \
        {                                                                                           \
            if (dict.buckets[i].key == NULL)                                                        \
                continue;                                                                           \
                                                                                                    \
            size_t index = dict_string_hasher(dict.buckets[i].key) % new_cap;                       \
            size_t start = index;                                                                   \
                                                                                                    \
            do                                                                                      \
            {                                                                                       \
                Dict_Bucket_Internal* bkt = dict_bucket_at(new_bkts, index, sizeof(*dict.buckets)); \
                                                                                                    \
                if (bkt->key == NULL)                                                               \
                {                                                                                   \
                    bkt->key = dict.buckets[i].key;                                                 \
                    memcpy(bkt->value, &dict.buckets[i].value, sizeof(dict.buckets[i].value));      \
                    break;                                                                          \
                }                                                                                   \
                                                                                                    \
                index = (index + 1) % new_cap;                                                      \
            } while (index != start);                                                               \
        }                                                                                           \
                                                                                                    \
        dict.cap = new_cap;                                                                         \
        free(dict.buckets);                                                                         \
        dict.buckets = (void*) new_bkts;                                                            \
    } while (0)

#define dict_put(dict, _key, _value) \
    do {                                                          \
        if (dict.buckets == NULL)                                 \
            dict_make(dict);                                      \
        else if (((dict.filled + 1) / dict.cap) >= DICT_MAX_LOAD) \
            dict_resize(dict, dict.cap * DICT_GROWTH_RATE);       \
                                                                  \
        size_t index = dict_string_hasher(_key) % dict.cap;       \
        size_t start = index;                                     \
                                                                  \
        do                                                        \
        {                                                         \
            if (dict.buckets[index].key == NULL)                  \
            {                                                     \
                dict.buckets[index].key = string_make(_key);      \
                dict.buckets[index].value = _value;               \
                dict.filled++;                                    \
                break;                                            \
            }                                                     \
                                                                  \
            if (string_cmp(dict.buckets[index].key, _key))        \
            {                                                     \
                dict.buckets[index].value = _value;               \
                break;                                            \
            }                                                     \
                                                                  \
            index++;                                              \
        } while (index != start);                                 \
                                                                  \
    } while (0)

#define dict_remove(dict, _key) \
    do                                                                                                              \
    {                                                                                                               \
        hd_assert(dict.buckets);                                                                                    \
        String k = dict.buckets[(dict_find_bucket(dict.buckets, dict.cap, sizeof(*dict.buckets), _key)).index].key; \
        dict.buckets[(dict_find_bucket(dict.buckets, dict.cap, sizeof(*dict.buckets), _key)).index].key = NULL;     \
        string_free(&k);                                                                                            \
    } while (0)

#define dict_free(dict) \
    do                                        \
    {                                         \
        if (dict.buckets) free(dict.buckets); \
        dict.cap = dict.filled = 0;           \
        dict.buckets = NULL;                  \
    } while (0)

#define dict_find(dict, _key) ((dict.buckets) ? (dict_find_bucket(dict.buckets, dict.cap, sizeof(*dict.buckets), _key)).ptr : NULL)
#define dict_begin(dict) ((dict.buckets) ? dict_bucket_at(dict.buckets, 0, sizeof(*dict.buckets)) : NULL)
#define dict_end(dict)   ((dict.buckets) ? dict_bucket_at(dict.buckets, dict.cap, sizeof(*dict.buckets)) : NULL)

#define dict_get(dict, _key) ((dict.buckets) ? dict.buckets[(dict_find_bucket(dict.buckets, dict.cap, sizeof(*dict.buckets), _key)).index].value : 0)

// To be consistent with other data structures
#define dict_cap(dict)    (dict.cap)
#define dict_filled(dict) (dict.filled)

#define dict_next_bucket(bkt, dict) dict_next_bucket_impl(&bkt, dict_end(dict), sizeof(*dict.buckets))

#define dict_foreach(type, it, dict) for (Dict_Bkt(type) it = dict_begin(dict); \
                                          it != dict_end(dict);                \
                                          dict_next_bucket(it, dict))

size_t dict_string_hasher(String key);
Dict_Itr dict_find_bucket(void* buckets, size_t cap, size_t bkt_size, String key);
void dict_next_bucket_impl(void** bkt, void* end, size_t stride);

#endif // DICTIONARY_H


#ifdef DICTIONARY_IMPL

inline size_t dict_string_hasher(String key)
{
    size_t prime = 16794649U;
    size_t val = (size_t) key[0];

    for (int i = 0; key[i] != '\0'; i++)
    {
        val ^= (size_t) key[i];
        val *= prime;
    }

    return val;
}

Dict_Itr dict_find_bucket(void* buckets, size_t cap, size_t bkt_size, String key)
{
    size_t index = dict_string_hasher(key) % cap;
    size_t start = index;

    do
    {
        Dict_Bucket_Internal* bkt = dict_bucket_at(buckets, index, bkt_size);

        if (bkt->key == NULL)
            break;
        
        if (string_cmp(bkt->key, key))
            return (Dict_Itr){ index, bkt };

        index = (index + 1) % cap;
    } while (index != start);

    return (Dict_Itr){ cap, dict_bucket_at(buckets, cap, bkt_size) };
}

void dict_next_bucket_impl(void** bkt, void* end, size_t stride)
{
    void* itr = *bkt;

    while (1)
    {
        itr = (char*) itr + stride;

        if (itr == end)
            break;

        Dict_Bucket_Internal* b = (Dict_Bucket_Internal*) itr;
        if (b->key != NULL)
            break;
    }

    *bkt = itr;
}

#endif // DICTIONRY_IMPL