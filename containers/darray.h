/*
    PURE C DYNAMIC ARRAY
    This implementation is heavily based on Sean Barrett's dynamic array implementation.

    To create the implementaion use:
        #define DARRAY_IMPL
    before you include this file in *one* C or C++ file.

    Default growth rate is 1.5.
    Growth rate of the dynamic array can be changed by using:
        #define DARRAY_GROWTH_RATE <value>
    before creating the implementation to change growth rate to <value>.

    Default starting capacity is 2.
    Starting capacity of the dynamic array can be changed by using:
        #define DARRAY_START_CAP <value>
    before creating the implementation to change starting capacity to <value>.

    Assertions in the implementation can be removed by using:
        #define CONTAINER_NO_ASSERT
    before creating the implemenation.

    Example:
        #define DARRAY_IMPL
        #define DARRAY_START_CAP 5
        #define DARRAY_GROWTH_RATE 2
        #include "containers/darray.h"

    The definitions can be in any order as long as all of them are above the #include.
*/

#ifndef DARRAY_H
#define DARRAY_H

#ifndef DARRAY_GROWTH_RATE
#define DARRAY_GROWTH_RATE 1.5
#endif // DARRAY_GROWTH_RATE

#ifndef DARRAY_START_CAP
#define DARRAY_START_CAP 2
#endif // DARRAY_START_CAP

#define DArray(type) type*
#define DA_Itr(type) type*

#define da_make(arr)                 da_make_impl((void**)&arr, DARRAY_START_CAP, sizeof(*arr))
#define da_copy(dest, src)           da_copy_impl((void**)&dest, (void*)src, sizeof(*src))
#define da_move(dest, src)           da_move_impl((void**)&dest, (void**)&src, sizeof(*src))
#define da_free(arr)                 da_free_impl((void**)&arr)

#define da_make_with_cap(arr, cap)   da_make_impl((void**)&arr, cap, sizeof(*arr))
#define da_resize(arr, cap)          da_resize_impl((void**)&arr, cap, sizeof(*arr))

#define da_begin(arr)                da_get_itr_impl((void*)arr, 0, sizeof(*arr))
#define da_end(arr)                  da_get_itr_impl((void*)arr, da_size(arr), sizeof(*arr))

#define da_size(arr)                 da_size_impl((void*)arr)
#define da_cap(arr)                  da_cap_impl((void*)arr)

#define da_push_back(arr, value)     da_push_back_impl(arr, value)
#define da_pop_back(arr)             da_pop_back_impl(arr)
#define da_insert(arr, index, value) da_insert_impl(arr, index, value)
#define da_erase_at(arr, index)      da_erase_at_impl(arr, index)
#define da_erase_swap(arr, index)    da_erase_swap_impl(arr, index)

#define da_foreach(type, it, arr)    for (DA_Itr(type) it = (DA_Itr(type))da_begin(arr); it != (DA_Itr(type))da_end(arr); it++)

void da_make_impl(void** arr, size_t cap, size_t type_size);
void da_copy_impl(void** dest, void* src, size_t type_size);
void da_move_impl(void** dest, void** src, size_t type_size);
void da_free_impl(void** arr);

void da_resize_impl(void** arr, size_t new_cap, size_t type_size);

DA_Itr(void) da_get_itr_impl(void* arr, size_t index, size_t type_size);

size_t da_size_impl(void* arr);
size_t da_cap_impl(void* arr);

typedef struct
{
    size_t cap;
    size_t size;
    char buffer[];
} DA_Internal;

#define da_data(arr) ((DA_Internal*)(arr) - 1)

#define da_push_back_impl(arr, value) \
    do {                                                            \
        hd_assert(arr != NULL);                                     \
        DA_Internal* da = da_data(arr);                             \
        size_t size = da->size, cap = da->cap;                      \
                                                                    \
        if (cap == 0)    da_resize(arr, DARRAY_START_CAP);          \
        if (size >= cap) da_resize(arr, DARRAY_GROWTH_RATE * cap);  \
                                                                    \
        arr[da_data(arr)->size++] = (value);                        \
    } while (0)

#define da_pop_back_impl(arr) \
    do {                                    \
        hd_assert(arr != NULL);             \
        DA_Internal* da = da_data(arr);     \
        size_t size = da->size;             \
                                            \
        if (size > 0) da->size--;           \
    } while(0)

#define da_insert_impl(arr, index, value) \
    do {                                                                \
        hd_assert(arr != NULL);                                         \
        DA_Internal* da = da_data(arr);                                 \
        size_t size = da->size, cap = da->cap;                          \
                                                                        \
        if (cap == 0)        da_resize(arr, DARRAY_START_CAP);          \
        if (size + 1 >= cap) da_resize(arr, DARRAY_GROWTH_RATE * cap);  \
                                                                        \
        for (int i = size; i > index; i--)                              \
            arr[i] = arr[i - 1];                                        \
                                                                        \
        arr[index] = value;                                             \
        da_data(arr)->size++;                                           \
    } while(0)

#define da_erase_at_impl(arr, index) \
    do {                                        \
        hd_assert(arr != NULL);                 \
        DA_Internal* da = da_data(arr);         \
        size_t size = da->size;                 \
        hd_assert(size > 0 && index < size);    \
                                                \
        for (int i = index; i < size - 1; i++)  \
            arr[i] = arr[i + 1];                \
                                                \
        da->size--;                             \
    } while(0)

#define da_erase_swap_impl(arr, index) \
    do {                                        \
        hd_assert(arr != NULL);                 \
        DA_Internal* da = da_data(arr);         \
        size_t size = da->size;                 \
        hd_assert(size > 0 && index < size);    \
                                                \
        arr[index] = arr[size - 1];             \
        da->size--;                             \
    } while(0)

#endif // DARRAY_H

#ifdef DARRAY_IMPL

#ifndef DARRAY_IMPLEMENTED
#define DARRAY_IMPLEMENTED

/* METHOD IMPLEMENTATIONS */

/*
    DArray memory layout:
    [cap, size, value, value value, ...]
               ^ --> array pointer
*/

#include <stdlib.h>
#include <string.h>
#include "hd_assert.h"

void da_make_impl(void** arr, size_t cap, size_t type_size)
{
    size_t byte_size = cap * type_size + sizeof(DA_Internal);
    DA_Internal* da = (DA_Internal*) malloc(byte_size);
    hd_assert(da != NULL);

    da->cap  = cap;
    da->size = 0;

    *arr = da->buffer;
}

void da_copy_impl(void** dest, void* src, size_t type_size)
{
    hd_assert(src != NULL);
    DA_Internal* src_da = da_data(src);
    size_t byte_size = src_da->cap * type_size + sizeof(DA_Internal);
    
    DA_Internal* dest_da;
    if (*dest) dest_da = (DA_Internal*) realloc(da_data(*dest), byte_size);
    else       dest_da = (DA_Internal*) malloc(byte_size);

    hd_assert(dest_da != NULL);
    
    size_t filled_byte_size = src_da->size * type_size + sizeof(DA_Internal);
    memcpy(dest_da, src_da, filled_byte_size);

    *dest = dest_da->buffer;
}

void da_move_impl(void** dest, void** src, size_t type_size)
{
    hd_assert(*src != NULL);
    DA_Internal* dest_da = da_data(*src);
    
    *src  = NULL;
    *dest = dest_da->buffer;
}

void da_free_impl(void** arr)
{
    hd_assert(*arr != NULL);
    DA_Internal* da = da_data(*arr);
    free(da);
    *arr = NULL;
}


void da_resize_impl(void** arr, size_t new_cap, size_t type_size)
{
    DA_Internal* da = NULL;
    size_t size = 0;
    
    if (*arr)
    {
        da   = da_data(*arr);
        size = da->size;
    }

    size_t byte_size = new_cap * type_size + sizeof(DA_Internal);
    DA_Internal* new_da = (DA_Internal*) realloc(da, byte_size);
    
    new_da->size = size;
    new_da->cap  = new_cap;

    *arr = new_da->buffer;
}

DA_Itr(void) da_get_itr_impl(void* arr, size_t index, size_t type_size)
{
    hd_assert(arr != NULL);
    return (char*)arr + (index * type_size);
}

inline size_t da_size_impl(void* arr)
{
    if (!arr) return 0;
 
    return da_data(arr)->size;
}

inline size_t da_cap_impl(void* arr)
{
    if (!arr) return 0;
 
    return da_data(arr)->cap;
}

#endif // DARRAY_IMPLEMENTED

#endif // DARRAY_IMPL