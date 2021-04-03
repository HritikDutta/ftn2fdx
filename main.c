#include <stdio.h>

#include "converter/filestuff.h"
#include "converter/fountain.h"
#include "converter/fdx.h"

#define DEBUG

#ifdef DEBUG
int main()
{
    char* argv[] = {
        "fftest",
        "tests/Brick & Steel.fountain"
    };
#else
int main(int argc, char* argv[])
{
    if (argc < 1)
        return 0;
#endif

    Parser parser = parser_make(load_file(argv[1]));
    parser_parse(&parser);

    generate_fdx(&parser);

    // parser_free(&parser);
    // string_free(&content);
}