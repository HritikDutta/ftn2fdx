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
    int last_idx = 0;
    for (int i = 0; i < other[i]; i++)
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

static int count_lines(String str)
{
    if (!str || str[0] == '\0')
        return 0;

    int count = 0;

    for(int i = 0; str[i]; i++)
    {
        if (str[i] == '\n')
            count++;
    }

    return count + 1;
}

static void append_lines(String* dest, String lines, String alignment)
{
    char buffer[128];

    int last_idx = 0;
    for (int i = 0; lines[i]; i++)
    {
        if (lines[i] == '\n')
        {
            int prev_was_ws = lines[i] == '\r';
            lines[i - prev_was_ws] = '\0';

            sprintf(buffer, title_page_elem_fmt_start, alignment);
            string_append(dest, buffer);
            append_escaped(dest, lines + last_idx);
            string_append(dest, title_page_elem_fmt_end);
            
            if (prev_was_ws)
                lines[i - 1] = '\r';
            else
                lines[i] = '\n';

            last_idx = i + 1;
        }
    }

    sprintf(buffer, title_page_elem_fmt_start, alignment);
    string_append(dest, buffer);
    append_escaped(dest, lines + last_idx);
    string_append(dest, title_page_elem_fmt_end);
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

    if (!screenplay_content)
        screenplay_content = string_make("");

    #define FILL_SMARTTYPE_SECTION(str, prop, prop_name, section_name) \
    if (da_size(parser->prop) == 0)                     \
        str = string_make(default_##prop);              \
    else                                                \
    {                                                   \
        str= string_make("    <"section_name">\n");     \
        da_foreach(String, s, parser->prop)             \
        {                                               \
            string_append(&str, "      <"prop_name">"); \
            append_escaped(&str, *s);                   \
            string_append(&str, "</"prop_name">\n");    \
        }                                               \
        string_append(&str, "    </"section_name">\n"); \
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

    #undef FILL_SMARTTYPE_SECTION

    String title_page_content = NULL;
    const int total_lines = 60;     // Painstakingly counted

    int title_start_idx   = -1;
    int credit_start_idx  = -1;
    int author_start_idx  = -1;
    int contact_start_idx = -1;

    // Determine a few things beforehand to make a proper title page layout
    Dict_Bkt(String) title_bkt = dict_find(parser->title_page_details, "Title");
    if (title_bkt != dict_end(parser->title_page_details))
        title_start_idx = (total_lines / 3) - (count_lines(title_bkt->value) / 2);

    Dict_Bkt(String) credit_bkt = dict_find(parser->title_page_details, "Credit");
    if (credit_bkt != dict_end(parser->title_page_details))
        credit_start_idx = (title_start_idx > 0) ? (title_start_idx + 2) : ((total_lines / 3) + 2);

    Dict_Bkt(String) author_bkt = dict_find(parser->title_page_details, "Author");
    if (author_bkt == dict_end(parser->title_page_details))
        author_bkt = dict_find(parser->title_page_details, "Authors");

    if (author_bkt != dict_end(parser->title_page_details))
    {
        int last_idx = max(title_start_idx, credit_start_idx);
        author_start_idx = (last_idx > 0) ? (last_idx + 2) : (total_lines / 3) + 2;
    }

    Dict_Bkt(String) contact_bkt = dict_find(parser->title_page_details, "Contact");
    if (contact_bkt != dict_end(parser->title_page_details))
        contact_start_idx = total_lines - count_lines(contact_bkt->value);

    for (int i = 0; i < total_lines; i++)
    {
        if (i == title_start_idx)
        {
            append_lines(&title_page_content, title_bkt->value, "Center");
            i += count_lines(title_bkt->value);
            continue;
        }

        if (i == credit_start_idx)
        {
            append_lines(&title_page_content, credit_bkt->value, "Center");
            i += count_lines(credit_bkt->value);
            continue;
        }

        if (i == author_start_idx)
        {
            append_lines(&title_page_content, author_bkt->value, "Center");
            i += count_lines(author_bkt->value);
            continue;
        }

        if (i == contact_start_idx)
        {
            append_lines(&title_page_content, contact_bkt->value, "Left");
            i += count_lines(contact_bkt->value);
            continue;
        }

        string_append(&title_page_content, title_page_empty_elem);
    }

    FILE* file = fopen(filepath, "wb");
    fprintf(file, file_fmt,
            screenplay_content,
            title_page_content,
            smarttype_characters,
            smarttype_extensions,
            smarttype_scene_intros,
            smarttype_locations,
            smarttype_times_of_day,
            smarttype_transitions);
    fclose(file);
}