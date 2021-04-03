#pragma once

#include "containers/string.h"
#include "containers/darray.h"
#include "containers/dictionary.h"

// @Todo: Figure out how Script notes work in Final Draft
// @Todo: Boneyards can only work if lines start with /*.
//        Maybe make it if it's in the same line too?
typedef enum _Elem_Type
{
    ELEM_SCENE_HEADING,
    ELEM_ACTION,
    ELEM_CHARACTER,
    ELEM_DIALOGUE,
    ELEM_PARENTHETICAL,
    ELEM_TRANSITION,
    ELEM_CENTERED_TEXT,
    ELEM_BONEYARD,
    ELEM_PAGE_BREAK,
} Elem_Type;

typedef struct _Elem
{
    Elem_Type type;
    String data;
} Elem;

typedef struct _Parser
{
    String content;
    int idx;
    Dict(String) title_page_details;
    DArray(Elem) elements;

    DArray(String) characters;
    DArray(String) scene_intros;
    DArray(String) locations;
    DArray(String) times_of_day;
    DArray(String) transitions;

    int prev_line_empty;
    int next_line_empty;
    int line_all_caps;
} Parser;

Parser parser_make(String content);
void parser_free(Parser* parser);
void parser_parse(Parser* parser);

char* elem_type_as_string(Elem e);