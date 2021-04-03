#ifndef CONTAINER_STRING_H
#define CONTAINER_STRING_H

typedef char* String;

String string_make(char* cstr);
void   string_copy(String* dest, String src);
void   string_free(String* str);

String string_make_till_char(char* cstr, char delim);
String string_make_till_n(char* cstr, size_t n);
void   string_replace(String* str, char* cstr);

String string_get_line(String contents, size_t* index);
void   string_resize(String* str, size_t new_len);

inline size_t string_length(String str);
inline int    string_cmp(String s1, String s2);

void string_append(String* dest, char* other);
void string_to_lower(String* str);

#endif // CONTAINER_STRING_H

// #define STRING_IMPL
#ifdef STRING_IMPL

#ifndef STRING_IMPLEMENTED
#define STRING_IMPLEMENTED

#include <stdlib.h>
#include <string.h>

#include "hd_assert.h"

typedef struct
{
    size_t length;
    char   buffer[];
} String_Internal;

#define string_data(str) ((String_Internal*)(str) - 1)

String string_make(char* cstr)
{
    size_t len = strlen(cstr) + 1;
    String_Internal* s = (String_Internal*) malloc(len * sizeof(char) + sizeof(String_Internal));
    hd_assert(s != NULL);
    
    s->length = len;
    strcpy(s->buffer, cstr);
    return s->buffer;
}

void string_copy(String* dest, String src)
{
    hd_assert(src != NULL);
    String_Internal* src_str = string_data(src);
    size_t byte_size = (src_str->length + 1) * sizeof(char) + sizeof(String_Internal);
    
    String_Internal* dest_str;
    if (*dest) dest_str = (String_Internal*) realloc(string_data(*dest), byte_size);
    else       dest_str = (String_Internal*) malloc(byte_size);

    hd_assert(dest_str != NULL);
    dest_str->length = src_str->length;
    strcpy(dest_str->buffer, src_str->buffer);

    *dest = dest_str->buffer;
}

void string_free(String* str)
{
    hd_assert(*str != NULL);
    String_Internal* s = string_data(*str);
    free(s);
    *str = NULL;
}

String string_make_till_char(char* cstr, char delim)
{
    char* end = strchr(cstr, delim);

    if (!end)
        return string_make(cstr);

    size_t len = end - cstr;
    size_t byte_size = len * sizeof(char) + sizeof(String_Internal);
    String_Internal* s = (String_Internal*) malloc(byte_size);
    hd_assert(s != NULL);

    s->length = len;
    strncpy(s->buffer, cstr, len);
    s->buffer[len] = '\0';
    return s->buffer;
}

String string_make_till_n(char* cstr, size_t n)
{
    hd_assert(n < strlen(cstr) + 1);

    size_t byte_size = (n + 1) * sizeof(char) + sizeof(String_Internal);
    String_Internal* s = (String_Internal*) malloc(byte_size);
    hd_assert(s != NULL);

    s->length = n + 1;
    strncpy(s->buffer, cstr, n);
    s->buffer[n] = '\0';
    return s->buffer;
}

String string_get_line(String contents, size_t* index)
{
    if (!contents)
        return NULL;

    if (contents[*index] == '\0')
        return NULL;
    
    char temp[1024];
    size_t cpy_idx = 0;
    while (contents[*index] != '\0' &&
           contents[*index] != '\n')
    {
        temp[cpy_idx] = contents[*index];
        cpy_idx++;
        (*index)++;
    }

    if (contents[*index] != '\0')
        (*index)++;

    temp[cpy_idx] = '\n';
    temp[cpy_idx + 1] = '\0';

    return string_make(temp);
}

void string_replace(String* str, char* cstr)
{
    hd_assert(*str);
    size_t length = strlen(cstr);
    size_t byte_size = (length + 1) * sizeof(char) + sizeof(String_Internal);
    String_Internal* s = (String_Internal*) realloc(string_data(*str), byte_size);

    s->length = length;
    strcpy(s->buffer, cstr);
    *str = s->buffer;
}

// Dangerous...
void string_resize(String* str, size_t new_len)
{
    size_t byte_size = (new_len + 1) * sizeof(char) + sizeof(String_Internal);

    String_Internal* s;
    if (*str) s = (String_Internal*) realloc(string_data(*str), byte_size);
    else      s = (String_Internal*) malloc(byte_size);

    hd_assert(s != NULL);
    s->length = new_len - 1;
    *str = s->buffer;
}

inline size_t string_length(String str)
{
    if (!str) return 0;
    return string_data(str)->length;
}

inline int string_cmp(String s1, String s2)
{
    if (s1 && s2)
        return strcmp(s1, s2) == 0;
    else if (!s1 && !s2)
        return 1;

    return 0;    
}

void string_append(String* dest, char* other)
{
    if (*dest == NULL)
    {
        *dest = string_make(other);
    }
    else
    {
        size_t prev_len = string_length(*dest);
        string_resize(dest, prev_len + strlen(other) + 1);
        String_Internal* s = string_data(*dest);
        strcat(*dest, other);
    }
}

void string_to_lower(String* str)
{
    for (size_t i = 0; i < string_length(*str); i++)
    {
        if ((*str)[i] >= 'A' && (*str)[i] <= 'Z')
            (*str)[i] += 'a' - 'A';
    }
}

#endif // STRING_IMPLEMENTED

#endif // STRING_IMPL