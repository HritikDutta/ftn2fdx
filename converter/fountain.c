#include "fountain.h"

#define STRING_IMPL
#include "containers/string.h"

#define DARRAY_IMPL
#include "containers/darray.h"

#define DICTIONARY_IMPL
#include "containers/dictionary.h"

Elem elem_make(Elem_Type type)
{
    Elem e = { type, 0 };

    if (type != ELEM_PAGE_BREAK && type != ELEM_BONEYARD)
        da_make(e.texts);
    
    return e;
}

void elem_process(Elem* elem, String line, int* emphasis_flags)
{
    int start_idx = 0;
    for (int i = 0; line[i]; i++)
    {
        if (line[i] == '_')
        {
            if (i > start_idx && line[i - 1] != '*' && line[i - 1] != '_')
            {
                Text t = { *emphasis_flags, NULL };
            
                line[i] = '\0';
                t.text = string_make(line + start_idx);
                line[i] = '_';

                da_push_back(elem->texts, t);
            }

            start_idx = i + 1;
            *emphasis_flags ^= EMPHASIS_UNDERLINED;
            continue;
        }

        if (line[i] == '*')
        {
            if (i > start_idx && line[i - 1] != '*' && line[i - 1] != '_')
            {
                Text t = { *emphasis_flags, NULL };
            
                line[i] = '\0';
                t.text = string_make(line + start_idx);
                line[i] = '*';

                da_push_back(elem->texts, t);
            }

            if (line[i + 1] == '*')
            {
                start_idx = i + 2;
                *emphasis_flags ^= EMPHASIS_BOLD;
                i++;
            }
            else
            {
                start_idx = i + 1;
                *emphasis_flags ^= EMPHASIS_ITALICIZED;
            }

            continue;
        }
    }

    
    // Add the remaing line
    Text t = { *emphasis_flags, NULL };
    t.text = string_make(line + start_idx);
    da_push_back(elem->texts, t);
}

void elem_free(Elem* elem)
{
    da_foreach(Text, t, elem->texts)
    {
        if (t->text)
            string_free(&t->text);
    }

    da_free(elem->texts);
}

Parser parser_make(String content)
{
    Parser p = { 0 };
    p.content = content;
    dict_make(p.title_page_details);
    da_make(p.elements);

    da_make(p.characters);
    da_make(p.scene_intros);
    da_make(p.locations);
    da_make(p.times_of_day);
    da_make(p.transitions);

    return p;
}

static void free_string_array(DArray(String)* arr)
{
    da_foreach(String, str, (*arr))
    {
        if (*str)
            string_free(str);
    }

    da_free((*arr));
}

void parser_free(Parser* parser)
{
    if (parser->content)
        string_free(&parser->content);

    dict_foreach(String, it, parser->title_page_details)
    {
        if (it->key)
        {
            string_free(&it->key);

            if (it->value)
                string_free(&it->value);
        }
    }

    dict_free(parser->title_page_details);

    da_foreach(Elem, elem, parser->elements)
        elem_free(elem);

    da_free(parser->elements);

    free_string_array(&parser->characters);
    free_string_array(&parser->scene_intros);
    free_string_array(&parser->locations);
    free_string_array(&parser->times_of_day);
    free_string_array(&parser->transitions);
}

static int is_ws(char ch)
{
    return ch == ' '  ||
           ch == '\t' ||
           ch == '\r' ||
           ch == '\n';
}

static char peek(Parser* parser, int offset)
{
    if (parser->idx + offset >= string_length(parser->content) + 1)
        return 0;

    return parser->content[parser->idx + offset];
}

static char consume(Parser* parser)
{
    if (parser->idx >= string_length(parser->content) + 1)
        return 0;

    return parser->content[parser->idx++];
}

static void consume_n(Parser* parser, int n)
{
    if (parser->idx + n >= string_length(parser->content))
        parser->idx = string_length(parser->content);

    parser->idx += n;
}

static void consume_ws(Parser* parser)
{
    while (is_ws(peek(parser, 0)))
        consume(parser);
}

static void consume_line(Parser* parser)
{
    while (peek(parser, 0))
    {
        char ch = consume(parser);
        if (ch == '\n')
            break;
    }
}

static int line_is_empty(Parser* parser)
{
    int i = 0;
    char ch;
    while (ch = peek(parser, i))
    {
        if (ch == '\n')
            return 1;

        if (ch != ' '  &&
            ch != '\t' &&
            ch != '\r')
            return 0;

        i++;
    }

    // No non-ws char encountered in final line
    return 1;
}

static int line_is_indented(Parser* parser)
{
    if (parser->idx == 0)
        return peek(parser, 0) == '\t' ||
               (peek(parser, 0) == ' '  &&
                peek(parser, 1) == ' '  &&
                peek(parser, 2) == ' ');

    int offset = 0;
    while (offset < parser->idx)
    {
        if (peek(parser, -offset) == '\n')
        {
            return peek(parser, 1 - offset) == '\t' ||
                   (peek(parser, 1 - offset) == ' '  &&
                    peek(parser, 2 - offset) == ' '  &&
                    peek(parser, 3 - offset) == ' ');
        }

        offset++;
    }

    return 0;
}

static int line_is_all_caps(Parser* parser)
{
    int offset = 0;
    while (peek(parser, offset) && peek(parser, offset) != '\n')
    {
        if (peek(parser, offset) >= 'a' && peek(parser, offset) <= 'z')
            return 0;

        offset++;
    }

    return 1;
}

static int next_line_is_empty(Parser* parser)
{
    int prev_idx = parser->idx;
    consume_line(parser);
    int is_empty = line_is_empty(parser);
    parser->idx = prev_idx;
    return is_empty;
}

static String get_title_page_key(Parser* parser)
{
    int offset = 0;
    while (peek(parser, offset) && peek(parser, offset) != '\n')
    {
        if (peek(parser, offset) == ':')
        {
            char* start = parser->content + parser->idx;
            consume_n(parser, offset + 1);
            return string_make_till_n(start, offset);
        }

        offset++;
    }

    return NULL;
}

static String get_line(Parser* parser)
{
    DArray(char) buffer = NULL;
    da_make(buffer);

    while (peek(parser, 0) && peek(parser, 0) != '\n' && peek(parser, 0) != '\r')
        da_push_back(buffer, consume(parser));

    da_push_back(buffer, '\0');
    consume(parser);

    String line = string_make(buffer);
    da_free(buffer);
    return line;
}

// This won't check if the delim exists ahead
static String get_till_char(Parser* parser, char delim)
{
    return string_make_till_char(parser->content + parser->idx, delim);
}

static void parse_title_page(Parser* parser)
{
    consume_ws(parser);
    
    while (!line_is_empty(parser))
    {
        String key = get_title_page_key(parser);
        // If no title page details are provided
        if (key == NULL)
            break;

        String value = NULL;
        if (line_is_empty(parser))
        {
            consume_line(parser);
            int first_line = 1;
            while (line_is_indented(parser) && !line_is_empty(parser))
            {
                if (!first_line)
                    string_append(&value, "\n");
                else
                    first_line = 0;

                consume_ws(parser);
                String line = get_line(parser);
                consume_line(parser);

                string_append(&value, line);
                string_free(&line);
            }
        }
        else
        {
            consume_ws(parser);
            value = get_line(parser);
            consume_line(parser);
        }

        Elem e = elem_make(ELEM_TP_DETAIL);
        elem_process(&e, value, &parser->emphasis_flags);
        dict_put(parser->title_page_details, key, e);
    }
}

static int line_starts_with(Parser* parser, String substr)
{
    // Not doing unnecessary bound checks for parser
    for (int index = 0; substr[index]; index++)
    {
        if (peek(parser, index) != substr[index])
            return 0;
    }

    return 1;
}

static int line_ends_with(Parser* parser, String substr)
{
    int start_index = parser->idx;
    int len = strlen(substr);

    consume_line(parser);

    for (int index = 0; substr[index]; index++)
    {
        if (peek(parser, -(len - index + 1)) != substr[index])
        {
            parser->idx = start_index;
            return 0;
        }
    }

    parser->idx = start_index;
    return 1;
}

static int line_wrapped_with(Parser* parser, char left, char right)
{
    if (peek(parser, 0) == left)
    {
        int prev_index = parser->idx;
        consume_line(parser);
        int offset = 1;
        while (parser->idx - offset > prev_index)
        {
            char ch = peek(parser, -offset);
            offset++;

            if (is_ws(ch))
                continue;
            
            parser->idx = prev_index;
            return ch == right;
        }
    }

    return 0;
}

// @Todo: This should also work for lowercase letters
static int is_scene_heading(Parser* parser)
{
    if (!parser->prev_line_empty || !parser->line_all_caps || !parser->next_line_empty)
        return 0;

    if (peek(parser, 0) == '.' && peek(parser, 1) != '.')
    {
        consume(parser);
        return 1;
    }

    return line_starts_with(parser, "EXT.")     ||
           line_starts_with(parser, "INT.")     ||  // Takes care of INT./EXT. case too
           line_starts_with(parser, "EST.")     ||
           line_starts_with(parser, "INT/EXT.") ||
           line_starts_with(parser, "I/E.");
}

static int is_character(Parser* parser)
{
    if (!parser->prev_line_empty || parser->next_line_empty)
        return 0;

    int allow_lowercase = 0;
    if (peek(parser, 0) == '@')
    {
        allow_lowercase = 1;
        consume(parser);
    }

    int offset = 0, found_char = 0;
    while (peek(parser, offset) && peek(parser, offset) != '\n' && peek(parser, offset) != '(')
    {
        char ch = peek(parser, offset);

        if (ch >= 'A' && ch <= 'Z')
        {
            found_char = 1;
        }

        if (ch >= 'a' && ch <= 'z')
        {
            if (allow_lowercase)
                found_char = 1;
            else
                return 0;
        }

        offset++;
    }

    return found_char;
}

static int is_dialogue(Parser* parser)
{
    int last_elem_idx = da_size(parser->elements) - 1;
    if (last_elem_idx < 0)
        return 0;

    Elem_Type prev_elem_type = parser->elements[last_elem_idx].type;

    // If previous element was a character or a parenthetical
    if (prev_elem_type == ELEM_CHARACTER ||
        prev_elem_type == ELEM_PARENTHETICAL)
        return 1;

    // If previous element was a dialogue and the previous line wasn't empty
    if (prev_elem_type == ELEM_DIALOGUE &&
        !parser->prev_line_empty)
        return 1;

    // @Todo: Look for { number spaces } for empty lines

    return 0;
}

static int is_parenthetical(Parser* parser)
{
    int last_elem_idx = da_size(parser->elements) - 1;
    if (last_elem_idx < 0)
        return 0;

    Elem_Type prev_elem_type = parser->elements[last_elem_idx].type;

    // If previous element was a character or a parenthetical
    if (prev_elem_type == ELEM_CHARACTER ||
        prev_elem_type == ELEM_PARENTHETICAL ||
        prev_elem_type == ELEM_DIALOGUE)
    {
        return line_wrapped_with(parser, '(', ')');
    }

    return 0;
}

static int is_transition(Parser* parser)
{
    // This will be checked after centered text so this
    // condition will be enough
    if (peek(parser, 0) == '>')
    {
        consume(parser);
        consume_ws(parser);
        return 1;
    }

    if (!parser->prev_line_empty || !parser->next_line_empty || !parser->line_all_caps)
        return 0;

    return line_ends_with(parser, "TO:");
}

static int is_centered_text(Parser* parser)
{
    return line_wrapped_with(parser, '>', '<');
}

static void push_unique_string_or_free(DArray(String)* list, String* str)
{
    int size = da_size((*list));
    for (int i = 0; i < size; i++)
    {
        if (string_cmp((*list)[i], *str))
        {
            string_free(str);
            return;
        }
    }

    da_push_back((*list), *str);
}

static void push_character_name(Parser* parser, String line)
{
    int last_idx = 0;
    for (int i = 0; line[i]; i++)
    {
        if (line[i] == '(')
            break;

        if (!is_ws(line[i]))
            last_idx = i;
    }

    String name = string_make_till_n(line, last_idx + 1);
    push_unique_string_or_free(&parser->characters, &name);
}

static void push_scene_heading_details(Parser* parser, String line)
{
    int start_idx = 0;
    while (!is_ws(line[start_idx]))
        start_idx++;

    // There is a scene intro
    if (line[start_idx] != '.')
    {
        String scene_intro = string_make_till_n(line, start_idx + 1);
        push_unique_string_or_free(&parser->scene_intros, &scene_intro);
    }
    else
        start_idx = 0;

    while (is_ws(line[start_idx]))
        start_idx++;

    int i, last_idx = 0;
    for (i = 0; line[i]; i++)
    {
        if (line[i] == '-')
            break;

        if (!is_ws(line[i]))
            last_idx = i;
    }

    String location = string_make_till_n(line + start_idx , last_idx - start_idx + 1);
    push_unique_string_or_free(&parser->locations, &location);

    if (line[i])
    {
        start_idx = i + 1;
        while (is_ws(line[start_idx]))
            start_idx++;

        String time_of_day = string_make(line + start_idx);
        push_unique_string_or_free(&parser->times_of_day, &time_of_day);
    }
}

static void parse_screenplay(Parser* parser)
{
    int len = string_length(parser->content);

    parser->prev_line_empty = 1;
    while (parser->idx < len)
    {
        if (line_is_empty(parser))
            parser->prev_line_empty = 1;

        consume_ws(parser);

        if (parser->idx >= len)
            return;

        if (peek(parser, 0) == '!')
        {
            consume(parser);
            goto action;
        }

        // Predetermine a few things
        parser->line_all_caps = line_is_all_caps(parser);
        parser->next_line_empty = next_line_is_empty(parser);

        if (line_starts_with(parser, "==="))
        {
            Elem e = elem_make(ELEM_PAGE_BREAK);
            da_push_back(parser->elements, e);
            
            consume_line(parser);
            parser->prev_line_empty = 1;
            continue;
        }

        if (line_starts_with(parser, "/*"))
        {
            Elem e = elem_make(ELEM_BONEYARD);
            da_push_back(parser->elements, e);

            while (peek(parser, 0))
            {
                if (peek(parser, 0) == '*' && peek(parser, 1) == '/')
                    break;

                consume(parser);
            }

            consume(parser);
            consume(parser);

            continue;
        }

        if (is_centered_text(parser))
        {
            consume(parser);        // Consume the first '>'
            consume_ws(parser);
            String str = get_till_char(parser, '<');    // @Todo: Also trim off whitespaces at the end
            consume_line(parser);

            Elem e = elem_make(ELEM_CENTERED_TEXT);
            da_push_back(parser->elements, e);
            elem_process(&e, str, &parser->emphasis_flags);

            string_free(&str);
            parser->prev_line_empty = 0;
            continue;
        }

        if (is_parenthetical(parser))
        {
            String str = get_line(parser);

            Elem e = elem_make(ELEM_PARENTHETICAL);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);

            string_free(&str);
            parser->prev_line_empty = 0;
            continue;
        }

        if (is_dialogue(parser))
        {
            String str = get_line(parser);

            Elem e = elem_make(ELEM_DIALOGUE);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);
            
            string_free(&str);
            parser->prev_line_empty = 0;
            continue;
        }

        if (is_transition(parser))
        {
            String str = get_line(parser);
            consume_line(parser);   // Consume the empty line after this

            Elem e = elem_make(ELEM_TRANSITION);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);

            String transition = NULL;
            push_unique_string_or_free(&parser->transitions, &str);

            parser->prev_line_empty = 0;
            continue;
        }

        if (is_scene_heading(parser))
        {
            String str = get_line(parser);
            consume_line(parser);   // Consume the empty line after this

            Elem e = elem_make(ELEM_SCENE_HEADING);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);
            
            push_scene_heading_details(parser, str);
            string_free(&str);
            parser->prev_line_empty = 0;
            continue;
        }

        if (is_character(parser))
        {
            String str = get_line(parser);

            Elem e = elem_make(ELEM_CHARACTER);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);
         
            push_character_name(parser, str);

            string_free(&str);
            parser->prev_line_empty = 0;
            continue;
        }

    action: // Use a different way where it collects everything between elements
        
        {
            String str = get_line(parser);

            Elem e = elem_make(ELEM_ACTION);
            elem_process(&e, str, &parser->emphasis_flags);
            da_push_back(parser->elements, e);

            string_free(&str);
            parser->prev_line_empty = 0;
        }
    }
}

void parser_parse(Parser* parser)
{
    // Just in case
    parser->idx = 0;
    parse_title_page(parser);
    parse_screenplay(parser);
}

char* elem_type_as_string(Elem e)
{
    switch (e.type)
    {
        case ELEM_TP_DETAIL:     return "TP_DETAIL";
        case ELEM_SCENE_HEADING: return "SCENE_HEADING";
        case ELEM_ACTION:        return "ACTION       ";
        case ELEM_CHARACTER:     return "CHARACTER    ";
        case ELEM_DIALOGUE:      return "DIALOGUE     ";
        case ELEM_PARENTHETICAL: return "PARENTHETICAL";
        case ELEM_TRANSITION:    return "TRANSITION   ";
        case ELEM_CENTERED_TEXT: return "CENTERED_TEXT";
        case ELEM_BONEYARD:      return "BONEYARD     ";
        case ELEM_PAGE_BREAK:    return "PAGE_BREAK   ";
        default: return "NOT_FOUND";
    }
}