#include <stdio.h>

#include "converter/filestuff.h"
#include "converter/fountain.h"
#include "converter/fdx.h"
#include "converter/helpers.h"

// #define DEBUG

const char ff_help_string[] =
"Convert .fountain file to .fdx.\n"
"   usage: %s <in-path> <out-path>\n"
;

#ifdef DEBUG
int main()
{
    int argc = 2;
    char* argv[] = {
        "fftest",
        "tests/Brick & Steel.fountain"
    };
#else
int main(int argc, char* argv[])
{
#endif
    if (argc < 2 || string_cmp(argv[1], "help"))
    {
        printf(ff_help_string, argv[0]);
        return 0;
    }

    if (!is_fountain(argv[1]))
    {
        printf("What file is this? %s\n", argv[1]);
        return 1;
    }

    String outfile;
    if (argc == 2)
        outfile = convert_extension(argv[1]);
    else
        outfile = argv[2];

    Parser parser = parser_make(load_file(argv[1]));
    parser_parse(&parser);
    generate_fdx(&parser, outfile);

    printf("%s\n", outfile);
}