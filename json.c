#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define JSON_THINGS_SIZE 128
#define JSON_STRINGS_SIZE 2048
#define JSON_COMPOUNDS_SIZE 128

typedef enum {
    JSON_UNPARSED,
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_thing_type_t;

typedef struct json_compound_t {
    const char *key;
    struct json_thing_t *thing;
    struct json_compound_t *next;
} json_compound_t;

typedef struct json_thing_t {
    json_thing_type_t type;

    union {
        const char *string;
        int32_t integer;
        bool boolean;
        json_compound_t *compound;
    } val;
} json_thing_t;

typedef struct {
    void *mem_things;
    void *mem_strings;
    void *mem_compounds;

    json_thing_t *things;
    uint16_t things_rem;
    uint16_t things_total;

    char *strings;
    uint16_t strings_rem;
    uint16_t strings_total;

    json_compound_t *compounds;
    uint16_t compounds_rem;
    uint16_t compounds_total;
} json_t;

json_t *json_init(void)
{
    void *mem = malloc(sizeof(json_t)
                       + sizeof(json_thing_t) * JSON_THINGS_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    void *mem2 = malloc(JSON_STRINGS_SIZE);
    if (mem2 == NULL) {
        free(mem);
        fprintf(stderr, "%d\n", __LINE__); return NULL;

    }

    void *mem3 = malloc(sizeof(json_compound_t) * JSON_COMPOUNDS_SIZE);
    if (mem3 == NULL) {
        free(mem);
        free(mem2);
        fprintf(stderr, "%d\n", __LINE__); return NULL;

    }

    json_t *json = (json_t*)mem;

    json->mem_things = mem;
    json->mem_strings = mem2;
    json->mem_compounds = mem3;

    json->things = (json_thing_t*)((char*)mem + sizeof(json_t));
    json->things_rem = JSON_THINGS_SIZE;
    json->things_total = JSON_THINGS_SIZE;

    json->strings = mem2;
    json->strings_rem = JSON_STRINGS_SIZE;
    json->strings_total = JSON_STRINGS_SIZE;

    json->compounds = mem3;
    json->compounds_rem = JSON_COMPOUNDS_SIZE;
    json->compounds_total = JSON_COMPOUNDS_SIZE;

    return json;
}

json_thing_t *json_allocate_thing(json_t *json)
{
    if (json->things_rem == 0) {
        // TODO: reallocate.
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    json_thing_t *thing = json->things;
    json->things += 1;
    json->things_rem -= 1;

    return thing;
}

json_compound_t *json_allocate_compound(json_t *json)
{
    if (json->compounds_rem == 0) {
        // TODO: reallocate.
        fprintf(stderr, "%d\n", __LINE__); return NULL;

    }

    json_compound_t *compound = json->compounds;
    json->compounds += 1;
    json->compounds_rem -= 1;

    compound->key = NULL;
    compound->thing = NULL;
    compound->next = NULL;

    return compound;
}

json_thing_t *json_get_singleton(json_t *json, const char **json_str,
                                 int8_t type)
{
    json_thing_t *thing = json_allocate_thing(json);
    if (thing == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;

    }

    switch (type) {
    case 0:
        thing->type = JSON_NULL;
        (*json_str) += 4;
        break;

    case 1:
        thing->type = JSON_BOOL;
        thing->val.boolean = true;
        (*json_str) += 4;
        break;

    case -1:
        thing->type = JSON_BOOL;
        thing->val.boolean = false;
        (*json_str) += 5;
        break;
    }

    return thing;
}

json_thing_t *json_get_unparsed(json_t *json, const char **json_str,
                                const char *stop_chars)
{
    json_thing_t *thing = json_allocate_thing(json);
    if (thing == NULL) {
        fprintf(stderr, "%d\n", __LINE__);
        return NULL;
    }

    size_t sclen = strlen(stop_chars);

    if (sclen) {
        while (**json_str != '\0') {
            for (uint8_t i = 0; i < sclen; ++i) {
                if (**json_str == stop_chars[i]) {
                    goto END;
                }
            }

            (*json_str)++;
        }
    }

 END:
    thing->type = JSON_UNPARSED;

    return thing;
}

const char *json_get_string0(json_t *json, const char **json_str)
{
    const char *org = json->strings;

    (*json_str)++;

    bool escape = false;
    for (; **json_str != '\0'; (*json_str)++) {
        if (escape) {
            switch (**json_str) {
            case '\\':
                *(json->strings) = '\\';
                break;

            case 't':
                *(json->strings) = '\t';
                break;

            case 'r':
                *(json->strings) = '\r';
                break;

            case 'n':
                *(json->strings) = '\n';
                break;

            case '"':
                *(json->strings) = '"';
                break;

            default:
                *(json->strings) = **json_str;
                break;
            }

            json->strings += 1;
            json->strings_rem -= 1;
            escape = false;
        } else {
            switch (**json_str) {
            case '\\':
                escape = true;
                break;

            case '"':
                (*json_str)++;
                goto END;

            default:
                *(json->strings) = **json_str;

                json->strings += 1;
                json->strings_rem -= 1;
                break;
            }
        }

        if (json->strings_rem < 2) {
            // TODO: reallocate.
            fprintf(stderr, "%d\n", __LINE__);
            return NULL;
        }
    }

 END:
    *(json->strings) = '\0';
    json->strings += 1;
    json->strings_rem -= 1;

    return org;
}

json_thing_t *json_get_string(json_t *json, const char **json_str)
{
    json_thing_t *thing = json_allocate_thing(json);
    if (thing == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    const char *str = json_get_string0(json, json_str);
    if (str == NULL) {
        return NULL;
    }

    thing->type = JSON_STRING;
    thing->val.string = str;

    return thing;
}

json_thing_t *json_parse(json_t *json, const char **json_str,
                         const char *stop_chars);

json_thing_t *json_get_object(json_t *json, const char **json_str)
{
    json_thing_t *obj_head = json_allocate_thing(json);
    if (obj_head == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    obj_head->type = JSON_OBJECT;
    obj_head->val.compound = NULL;

    (*json_str)++;

    json_compound_t *last_compound = NULL;
    json_compound_t *compound = NULL;

    bool is_key = true;
    bool is_key_unquoted = true;

    while (**json_str != '\0') {
        if (is_key) {
            if (compound == NULL) {
                compound = json_allocate_compound(json);
                if (compound == NULL) {
                    fprintf(stderr, "%d\n", __LINE__);
                    return NULL;
                }

                compound->key = json->strings;
            }

            switch (**json_str) {
            case '}':
                (*json_str)++;
                goto END;

            case '"':
                is_key_unquoted = false;

                // TODO: Handle string is NULL.
                compound->key = json_get_string0(json, json_str);
                continue;

            case ':':
                if (is_key_unquoted) {
                    // TODO: detect out of memory.
                    *(json->strings) = '\0';
                    json->strings += 1;
                    json->strings_rem -= 1;
                }

                is_key = false;
                (*json_str)++;
                break;

            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ',':
                (*json_str)++;
                continue;

            default:
                is_key_unquoted = true;
                *(json->strings) = **json_str;

                // TODO: detect out of memory.
                json->strings += 1;
                json->strings_rem -= 1;

                (*json_str)++;
                continue;
            }
        }

        json_thing_t *thing = json_parse(json, json_str, ",}\t\r\n ");
        if (thing == NULL) {
            fprintf(stderr, "%d\n", __LINE__);
            return NULL;
        }

        compound->thing = thing;

        if (last_compound != NULL) {
            last_compound->next = compound;
        } else {
            obj_head->val.compound = compound;
        }

        last_compound = compound;
        compound = NULL;
        is_key = true;
        is_key_unquoted = true;
    }

 END:
    return obj_head;
}

json_thing_t *json_get_array(json_t *json, const char **json_str)
{
    json_thing_t *array_head = json_allocate_thing(json);
    if (array_head == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    array_head->type = JSON_ARRAY;
    array_head->val.compound = NULL;

    (*json_str)++;

    json_compound_t *last_compound = NULL;

    while (**json_str != '\0') {
        switch (**json_str) {
            json_thing_t *thing;
            json_compound_t *compound;

        case ']':
            (*json_str)++;
            goto END;

        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
            (*json_str)++;
            break;

        default:
            thing = json_parse(json, json_str, ",]\t\r\n ");
            if (thing == NULL) {
                fprintf(stderr, "%d\n", __LINE__); return NULL;
            }

            compound = json_allocate_compound(json);
            if (compound == NULL) {
                fprintf(stderr, "%d\n", __LINE__); return NULL;
            }

            compound->thing = thing;

            if (last_compound != NULL) {
                last_compound->next = compound;
            } else {
                array_head->val.compound = compound;
            }

            last_compound = compound;

            break;
        }
    }

 END:
    return array_head;
}

json_thing_t *json_get_number(json_t *json, const char **json_str,
                              const char *stop_chars)
{
    json_thing_t *thing = json_allocate_thing(json);
    if (thing == NULL) {
        fprintf(stderr, "%d\n", __LINE__); return NULL;
    }

    thing->type = JSON_NUMBER;
    thing->val.integer = 0;

    int8_t sign = 1;
    if (**json_str == '-') {
        sign = -1;
        (*json_str)++;
    } else if (**json_str == '+') {
        (*json_str)++;
    }

    for (; **json_str != '\0'; (*json_str)++) {
        if (**json_str >= '0' && **json_str <= '9') {
            thing->val.integer *= 10;
            thing->val.integer += (**json_str - '0');
        } else {
            size_t sclen = strlen(stop_chars);
            for (size_t i = 0; i < sclen; ++i) {
                if (**json_str == stop_chars[i]) {
                    goto END;
                }
            }

            // Unexpected character.
            thing->type = JSON_UNPARSED;

            // Start eating until a stop char is found.
            for (; **json_str != '\0'; (*json_str)++) {
                for (size_t i = 0; i < sclen; ++i) {
                    if (**json_str == stop_chars[i]) {
                        goto END2;
                    }
                }
            }
        }
    }

 END:
    thing->val.integer *= sign;

 END2:
    return thing;
}

json_thing_t *json_parse(json_t *json, const char **json_str,
                         const char *stop_chars)
{
    while (**json_str != '\0') {
        switch (**json_str) {
        case '{':
            return json_get_object(json, json_str);

        case '[':
            return json_get_array(json, json_str);

        case '"':
            return json_get_string(json, json_str);

        case '+':
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return json_get_number(json, json_str, stop_chars);

        case 'n':
        case 't':
        case 'f':
            if (strncmp(*json_str, "null", 4) == 0) {
                return json_get_singleton(json, json_str, 0);
            } else if (strncmp(*json_str, "true", 4) == 0) {
                return json_get_singleton(json, json_str, 1);
            } else if (strncmp(*json_str, "false", 5) == 0) {
                return json_get_singleton(json, json_str, -1);
            } else {
                return json_get_unparsed(json, json_str, stop_chars);
            }

        case '\t':
        case '\r':
        case '\n':
        case ' ':
            (*json_str)++;
            break;

        default:
            (*json_str)++;
            return json_get_unparsed(json, json_str, stop_chars);
        }
    }

    fprintf(stderr, "%d\n", __LINE__); return NULL;
}

void json_print_thing(const json_thing_t *thing) {
    switch (thing->type) {
    case JSON_UNPARSED:
        printf("<*UNPARSED*>");
        break;

    case JSON_NULL:
        printf("null");
        break;

    case JSON_BOOL:
        printf("%s", thing->val.boolean ? "true" : "false");
        break;

    case JSON_NUMBER:
        printf("%d", thing->val.integer);
        break;

    case JSON_STRING:
        putchar('"');
        for (const char *c = thing->val.string; *c != '\0'; ++c) {
            switch (*c) {
            case '\\':
                putchar('\\');
                putchar('\\');
                break;

            case '\t':
                putchar('\\');
                putchar('t');
                break;

            case '\r':
                putchar('\\');
                putchar('r');
                break;

            case '\n':
                putchar('\\');
                putchar('n');
                break;

            case '"':
                putchar('\\');
                putchar('"');
                break;

            default:
                putchar(*c);
            }
        }
        putchar('"');
        break;

    case JSON_ARRAY:
        putchar('[');

        for (const json_compound_t *compound = thing->val.compound;
             compound != NULL; compound = compound->next)
        {
            json_print_thing(compound->thing);
            if (compound->next != NULL) {
                putchar(',');
                putchar(' ');
            }
        }

        putchar(']');
        break;

    case JSON_OBJECT:
        putchar('{');

        for (const json_compound_t *compound = thing->val.compound;
             compound != NULL; compound = compound->next)
        {
            printf("\"%s\": ", compound->key);
            json_print_thing(compound->thing);
            if (compound->next != NULL) {
                putchar(',');
                putchar(' ');
            }
        }

        putchar('}');
        break;
    }
}

void json_print_stats(const json_t *json)
{
    size_t size_things = json->things_total - json->things_rem;
    size_t size_compounds = json->compounds_total - json->compounds_rem;
    size_t size_things_b = size_things * sizeof(json_thing_t);
    size_t size_strings_b = json->strings_total - json->strings_rem;
    size_t size_compounds_b = size_compounds * sizeof(json_compound_t);

    printf("%zu things (%zu bytes), %zu bytes of strings, "
           "%zu compounds (%zu bytes).\n%zu bytes in total.\n",
           size_things, size_things_b, size_strings_b, size_compounds,
           size_compounds_b,
           size_things_b + size_strings_b + size_compounds_b);
}

int main(int argc, char *argv[]) {
    size_t alloc_size = 2048;
    char *buf = NULL;

    size_t tr = 0;
    do {
        buf = (char*)realloc(buf, alloc_size);
        if (buf == NULL) {
            return 1;
        }

        size_t r = fread(buf + tr, 1, alloc_size - tr, stdin);
        if (r != alloc_size && !feof(stdin)) {
            return 2;
        }

        tr += r;

        alloc_size *= 2;
    } while (!feof(stdin));

    const char *json_str = buf;
    json_t *json = json_init();
    json_thing_t *thing = json_parse(json, &json_str, "\t\r\n ");
    json_print_thing(thing);
    putchar('\n');

    if (argc >= 2 && strcmp(argv[1], "-v") == 0 && thing != NULL) {
        json_print_stats(json);
    }
}
