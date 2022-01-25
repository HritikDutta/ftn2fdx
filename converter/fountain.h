#pragma once

#include "containers/string.h"
#include "containers/darray.h"
#include "containers/dictionary.h"

// @Todo: Figure out how Script notes work in Final Draft
// @Todo: Boneyards can only work if lines start with /*.
//        Maybe make it if it's in the same line too?
typedef enum _Elem_Type
{
    ELEM_TP_DETAIL,
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

typedef enum _Emphasis_Type
{
    EMPHASIS_NONE       = 0x00,
    EMPHASIS_ITALICIZED = 0x01,
    EMPHASIS_BOLD       = 0x02,
    EMPHASIS_UNDERLINED = 0x04,
} Emphasis_Type;

typedef struct _Text
{
    int emphasis_flags;
    String text;
} Text;

typedef struct _Elem
{
    Elem_Type type;
    DArray(Text) texts;
} Elem;

Elem elem_make(Elem_Type type);
void elem_process(Elem* elem, String line, int* emphasis_flags);
void elem_free(Elem* elem);

typedef struct _Parser
{
    String content;
    int idx;
    Dict(Elem)   title_page_details;
    DArray(Elem) elements;

    DArray(String) characters;
    DArray(String) scene_intros;
    DArray(String) locations;
    DArray(String) times_of_day;
    DArray(String) transitions;

    int prev_line_empty;
    int next_line_empty;
    int line_all_caps;
    int emphasis_flags;
} Parser;

Parser parser_make(String content);
void parser_free(Parser* parser);
void parser_parse(Parser* parser);

char* elem_type_as_string(Elem e);