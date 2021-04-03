#include "fdx.h"

#include <stdio.h>

#include "fountain.h"
#include "filestuff.h"
#include "format.h"

static const char* get_elem_fmt_type(Elem e)
{
    switch (e.type)
    {
        case ELEM_SCENE_HEADING: return "Scene Heading";
        case ELEM_ACTION:        return "Action";
        case ELEM_CHARACTER:     return "Character";
        case ELEM_DIALOGUE:      return "Dialogue";
        case ELEM_PARENTHETICAL: return "Parenthetical";
        case ELEM_TRANSITION:    return "Transition";
        case ELEM_CENTERED_TEXT: return "General";

        // Ignored
        default: return "General";
    }
}

static const char* get_elem_fmt_alignment(Elem e)
{
    switch (e.type)
    {
        case ELEM_SCENE_HEADING: return "Left";
        case ELEM_ACTION:        return "Left";
        case ELEM_CHARACTER:     return "Left";
        case ELEM_DIALOGUE:      return "Left";
        case ELEM_PARENTHETICAL: return "Left";
        case ELEM_TRANSITION:    return "Right";
        case ELEM_CENTERED_TEXT: return "Center";

        // Ignored
        default: return "Left";
    }
}

void append_escaped(String* string, String other)
{
    int size = string_length(other);
    int last_idx = 0;
    for (int i = 0; i < size; i++)
    {
        switch (other[i])
        {
            #define ESCAPE_CHAR(ch, escaped) \
            case ch:\
            {\
                other[i] = '\0';\
                string_append(string, other + last_idx);\
                string_append(string, escaped);\
                other[i] = ch;\
                last_idx = i + 1;\
            } break

            ESCAPE_CHAR('\"', "&quot;");
            ESCAPE_CHAR('\'', "&apos;");
            ESCAPE_CHAR('<', "&lt;");
            ESCAPE_CHAR('>', "&gt;");
            ESCAPE_CHAR('&', "&amp;");

            #undef ESCAPE_CHAR
        }
    }

    string_append(string, other + last_idx);
}

void generate_fdx(Parser* parser, String filepath)
{
    String screenplay_content = NULL;
    da_foreach(Elem, elem, parser->elements)
    {
        // Handle page breaks properly later
        if (elem->type == ELEM_BONEYARD || elem->type == ELEM_PAGE_BREAK)
            continue;

        char buffer[128];
        sprintf(buffer, elem_fmt_start, get_elem_fmt_type(*elem), get_elem_fmt_alignment(*elem));

        string_append(&screenplay_content, buffer);
        append_escaped(&screenplay_content, elem->data);
        string_append(&screenplay_content, elem_fmt_end);
    }

    #define FILL_SMARTTYPE_SECTION(str, prop, prop_name, section_name) \
    if (da_size(parser->prop) == 0)\
        str = string_make(default_##prop);\
    else\
    {\
        str= string_make("    <"section_name">\n");\
        da_foreach(String, s, parser->prop)\
        {\
            string_append(&str, "      <"prop_name">");\
            append_escaped(&str, *s);\
            string_append(&str, "</"prop_name">\n");\
        }\
        string_append(&str, "    </"section_name">\n");\
    }

    String smarttype_characters = NULL;
    FILL_SMARTTYPE_SECTION(smarttype_characters, characters, "Character", "Characters");

    // @Todo: Implement extensions later
    String smarttype_extensions = string_make(default_extensions);

    String smarttype_scene_intros = NULL;
    FILL_SMARTTYPE_SECTION(smarttype_scene_intros, scene_intros, "SceneIntro", "SceneIntros");

    String smarttype_locations = NULL;
    FILL_SMARTTYPE_SECTION(smarttype_locations, locations, "Location", "Locations");

    String smarttype_times_of_day = NULL;
    FILL_SMARTTYPE_SECTION(smarttype_times_of_day, times_of_day, "TimeOfDay", "TimesOfDay");

    String smarttype_transitions = NULL;
    FILL_SMARTTYPE_SECTION(smarttype_transitions, transitions, "Transition", "Transitions");

    FILE* file = fopen(filepath, "wb");
    fprintf(file, file_fmt,
            screenplay_content,
            smarttype_characters,
            smarttype_extensions,
            smarttype_scene_intros,
            smarttype_locations,
            smarttype_times_of_day,
            smarttype_transitions);
    fclose(file);
}