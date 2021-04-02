#include "filestuff.h"

#include <stdio.h>
#include <string.h>
#include "containers/string.h"

String load_file(const String filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    String contents = NULL;
    string_resize(&contents, len);
    
    fread(contents, sizeof(char), len, file);
    contents[len] = '\0';

    fclose(file);

    return contents;
}

int write_file(const String filepath, String contents)
{
    FILE* file = fopen(filepath, "wb");
    if (!file)
        return 0;

    fputs(contents, file);

    fclose(file);
    return 1;
}