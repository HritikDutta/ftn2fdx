#include "helpers.h"

#include <string.h>
#include "containers/string.h"

int is_fountain(char* filepath)
{
    int last_dot = -1;
    for (int i = 0; filepath[i]; i++)
    {
        if (filepath[i] == '.')
            last_dot = i;
    }

    return last_dot > -1 && strcmp(filepath + last_dot, ".fountain") == 0;
}

String convert_extension(char* filepath)
{
    int last_dot = -1;
    for (int i = 0; filepath[i]; i++)
    {
        if (filepath[i] == '.')
            last_dot = i;
    }

    String fdx_file = string_make_till_n(filepath, last_dot);
    string_append(&fdx_file, ".fdx");
    return fdx_file;
}