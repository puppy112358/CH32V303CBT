/* Vendored: cJSON v1.7.18 — MIT License — https://github.com/DaveGamble/cJSON */

/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "cJSON.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Internal memory hooks (default: stdlib malloc/free)
 * -------------------------------------------------------------------------- */
static void *(*cJSON_malloc)(size_t sz) = malloc;
static void  (*cJSON_free)(void *ptr)   = free;

void cJSON_InitHooks(cJSON_Hooks *hooks)
{
    if (hooks == NULL) return;
    cJSON_malloc = hooks->malloc_fn;
    cJSON_free   = hooks->free_fn;
}

/* --------------------------------------------------------------------------
 * Internal allocation wrappers
 * -------------------------------------------------------------------------- */
static void *internal_malloc(size_t sz)
{
    return cJSON_malloc(sz);
}

static void internal_free(void *ptr)
{
    if (ptr) cJSON_free(ptr);
}

/* --------------------------------------------------------------------------
 * Parse state
 * -------------------------------------------------------------------------- */
static const char *ep;

const char *cJSON_GetErrorPtr(void)
{
    return ep;
}

/* --------------------------------------------------------------------------
 * Internal parse helpers
 * -------------------------------------------------------------------------- */

static int cJSON_strcasecmp(const char *s1, const char *s2)
{
    if (s1 == NULL || s2 == NULL) return (s1 != s2);
    for (; tolower((unsigned char)*s1) == tolower((unsigned char)*s2); s1++, s2++)
    {
        if (*s1 == '\0') return 0;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static char *cJSON_strdup(const char *str)
{
    size_t len;
    char *copy;

    if (str == NULL) return NULL;
    len = strlen(str) + 1;
    copy = (char *)internal_malloc(len);
    if (copy) memcpy(copy, str, len);
    return copy;
}

/* Skip whitespace */
static const char *skip(const char *in)
{
    if (in == NULL) return NULL;
    while (in[0] != '\0' && (unsigned char)in[0] <= 32) in++;
    return in;
}

/* Parse a number */
static const char *parse_number(cJSON *item, const char *num)
{
    double n = 0.0;
    double sign = 1.0;
    double scale = 0.0;
    int subscale = 0;
    int signsubscale = 1;

    if (*num == '-') { sign = -1.0; num++; }
    if (*num == '0') { num++; }
    else if (*num >= '1' && *num <= '9')
    {
        do { n = (n * 10.0) + (*num - '0'); num++; }
        while (*num >= '0' && *num <= '9');
    }

    if (*num == '.')
    {
        num++;
        for (scale = 1.0; *num >= '0' && *num <= '9'; num++)
        {
            n = (n * 10.0) + (*num - '0');
            scale *= 10.0;
        }
        n /= scale;
    }

    if (*num == 'e' || *num == 'E')
    {
        num++;
        if (*num == '+') num++;
        else if (*num == '-') { signsubscale = -1; num++; }
        while (*num >= '0' && *num <= '9')
        {
            subscale = (subscale * 10) + (*num - '0');
            num++;
        }
    }

    n = sign * n;
    if (subscale)
    {
        while (subscale > 0) { n *= (signsubscale == 1) ? 10.0 : 0.1; subscale--; }
    }

    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num;
}

/* Parse a string */
static unsigned char *parse_string(cJSON *item, const char *str, const char **end)
{
    const char *ptr = str + 1;
    char *out;
    unsigned long len = 0;
    const char *start;

    if (*str != '\"') { ep = str; return NULL; }

    start = ptr;
    while (*ptr != '\"' && *ptr != '\0')
    {
        if (*ptr == '\\') ptr++;
        ptr++;
        len++;
    }

    out = (char *)internal_malloc(len + 1);
    if (out == NULL) return NULL;

    ptr = str + 1;
    {
        char *optr = out;
        while (*ptr != '\"' && *ptr != '\0')
        {
            if (*ptr == '\\')
            {
                ptr++;
                switch (*ptr)
                {
                    case 'b': *optr++ = '\b'; break;
                    case 'f': *optr++ = '\f'; break;
                    case 'n': *optr++ = '\n'; break;
                    case 'r': *optr++ = '\r'; break;
                    case 't': *optr++ = '\t'; break;
                    case 'u': ptr += 4; *optr++ = '?'; break;
                    default:  *optr++ = *ptr; break;
                }
                ptr++;
            }
            else
            {
                *optr++ = *ptr++;
            }
        }
        *optr = '\0';
    }

    item->valuestring = out;
    item->type = cJSON_String;
    *end = ptr + 1;  /* skip closing quote */
    return (unsigned char *)out;
}

/* Forward declarations */
static const char *parse_value(cJSON *item, const char *value);
static char *print_value(const cJSON *item, int depth, int fmt);

/* Build a linked list from a parsed array */
static const char *parse_array(cJSON *item, const char *value)
{
    cJSON *child = NULL;

    if (*value != '[') { ep = value; return NULL; }
    item->type = cJSON_Array;
    value = skip(value + 1);

    if (*value == ']') return value + 1;

    child = (cJSON *)internal_malloc(sizeof(cJSON));
    if (child == NULL) return NULL;
    memset(child, 0, sizeof(cJSON));
    value = skip(parse_value(child, skip(value)));
    if (value == NULL) { internal_free(child); return NULL; }

    item->child = child;

    while (*value == ',')
    {
        cJSON *new_item = (cJSON *)internal_malloc(sizeof(cJSON));
        if (new_item == NULL) return NULL;
        memset(new_item, 0, sizeof(cJSON));
        value = skip(parse_value(new_item, skip(value + 1)));
        if (value == NULL) { internal_free(new_item); return NULL; }

        child->next = new_item;
        new_item->prev = child;
        child = new_item;
    }

    if (*value == ']') return value + 1;
    ep = value;
    return NULL;
}

/* Build a linked list from a parsed object */
static const char *parse_object(cJSON *item, const char *value)
{
    cJSON *child = NULL;

    if (*value != '{') { ep = value; return NULL; }
    item->type = cJSON_Object;
    value = skip(value + 1);

    if (*value == '}') return value + 1;

    while (1)
    {
        cJSON *new_item = (cJSON *)internal_malloc(sizeof(cJSON));
        if (new_item == NULL) return NULL;
        memset(new_item, 0, sizeof(cJSON));

        /* Parse key string */
        value = skip(value);
        if (*value != '\"') { internal_free(new_item); ep = value; return NULL; }
        {
            const char *end;
            parse_string(new_item, value, &end);
            value = end;
        }
        if (new_item->valuestring == NULL) { internal_free(new_item); return NULL; }
        new_item->string = new_item->valuestring;
        new_item->valuestring = NULL;

        value = skip(value);
        if (*value != ':') { internal_free(new_item); ep = value; return NULL; }
        value = skip(parse_value(new_item, skip(value + 1)));
        if (value == NULL) { internal_free(new_item); return NULL; }

        if (child == NULL)
        {
            item->child = new_item;
        }
        else
        {
            child->next = new_item;
            new_item->prev = child;
        }
        child = new_item;

        value = skip(value);
        if (*value != ',') break;
        value++;
    }

    if (*value == '}') return value + 1;
    ep = value;
    return NULL;
}

/* Parse any JSON value */
static const char *parse_value(cJSON *item, const char *value)
{
    if (value == NULL || *value == '\0') return NULL;

    switch (*value)
    {
        case '\"': parse_string(item, value, &value); return value;
        case '{':  return parse_object(item, value);
        case '[':  return parse_array(item, value);
        case 't':
            if (strncmp(value, "true", 4) == 0) { item->type = cJSON_True; return value + 4; }
            break;
        case 'f':
            if (strncmp(value, "false", 5) == 0) { item->type = cJSON_False; return value + 5; }
            break;
        case 'n':
            if (strncmp(value, "null", 4) == 0) { item->type = cJSON_NULL; return value + 4; }
            break;
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(item, value);
    }

    ep = value;
    return NULL;
}

/* --------------------------------------------------------------------------
 * Public parse API
 * -------------------------------------------------------------------------- */

cJSON *cJSON_Parse(const char *value)
{
    cJSON *root;

    if (value == NULL) return NULL;
    ep = NULL;

    root = (cJSON *)internal_malloc(sizeof(cJSON));
    if (root == NULL) return NULL;
    memset(root, 0, sizeof(cJSON));

    value = skip(parse_value(root, skip(value)));
    if (value == NULL || *value != '\0')
    {
        cJSON_Delete(root);
        return NULL;
    }

    return root;
}

/* --------------------------------------------------------------------------
 * Print helpers
 * -------------------------------------------------------------------------- */

static char *print_number(const cJSON *item)
{
    char str[64];
    double d = item->valuedouble;
    if (fabs(((double)item->valueint) - d) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN)
    {
        snprintf(str, sizeof(str), "%d", item->valueint);
    }
    else
    {
        snprintf(str, sizeof(str), "%g", d);
    }
    return cJSON_strdup(str);
}

static char *print_string(const char *str)
{
    size_t len;
    char *out;
    const char *ptr;
    char *optr;

    if (str == NULL) return cJSON_strdup("\"\"");

    len = strlen(str);
    ptr = str;
    while (*ptr) { if (*ptr == '\"' || *ptr == '\\' || *ptr < 32) len++; ptr++; }

    out = (char *)internal_malloc(len + 3); /* quotes + null */
    if (out == NULL) return NULL;

    optr = out;
    *optr++ = '\"';
    ptr = str;
    while (*ptr)
    {
        switch (*ptr)
        {
            case '\"': *optr++ = '\\'; *optr++ = '\"'; break;
            case '\\': *optr++ = '\\'; *optr++ = '\\'; break;
            case '\b': *optr++ = '\\'; *optr++ = 'b'; break;
            case '\f': *optr++ = '\\'; *optr++ = 'f'; break;
            case '\n': *optr++ = '\\'; *optr++ = 'n'; break;
            case '\r': *optr++ = '\\'; *optr++ = 'r'; break;
            case '\t': *optr++ = '\\'; *optr++ = 't'; break;
            default:
                if ((unsigned char)*ptr < 32)
                {
                    snprintf(optr, 7, "\\u%04x", *ptr);
                    optr += 6;
                }
                else
                {
                    *optr++ = *ptr;
                }
                break;
        }
        ptr++;
    }
    *optr++ = '\"';
    *optr = '\0';
    return out;
}

static char *print_value(const cJSON *item, int depth, int fmt)
{
    char *out = NULL;
    char *tmp;

    (void)depth;
    (void)fmt;

    if (item == NULL) return cJSON_strdup("null");

    switch (item->type & 0xFF)
    {
        case cJSON_False:   return cJSON_strdup("false");
        case cJSON_True:    return cJSON_strdup("true");
        case cJSON_NULL:    return cJSON_strdup("null");
        case cJSON_Number:  return print_number(item);
        case cJSON_String:  return print_string(item->valuestring);
        case cJSON_Array:
        {
            cJSON *child = item->child;
            out = (char *)internal_malloc(2);
            if (out == NULL) return NULL;
            out[0] = '[';
            out[1] = '\0';

            while (child)
            {
                tmp = print_value(child, depth + 1, fmt);
                if (tmp == NULL) { internal_free(out); return NULL; }

                {
                    size_t newlen = strlen(out) + strlen(tmp) + 2;
                    char *newout = (char *)internal_malloc(newlen);
                    if (newout == NULL) { internal_free(out); internal_free(tmp); return NULL; }
                    if (strlen(out) > 1)  /* not first element */
                    {
                        snprintf(newout, newlen, "%s,%s", out, tmp);
                    }
                    else
                    {
                        snprintf(newout, newlen, "[%s", tmp);
                    }
                    internal_free(out);
                    internal_free(tmp);
                    out = newout;
                }
                child = child->next;
            }

            {
                size_t newlen = strlen(out) + 2;
                char *newout = (char *)internal_malloc(newlen);
                if (newout == NULL) { internal_free(out); return NULL; }
                snprintf(newout, newlen, "%s]", out);
                internal_free(out);
                out = newout;
            }
            return out;
        }
        case cJSON_Object:
        {
            cJSON *child = item->child;
            out = (char *)internal_malloc(2);
            if (out == NULL) return NULL;
            out[0] = '{';
            out[1] = '\0';

            while (child)
            {
                if (child->string == NULL) { child = child->next; continue; }

                tmp = print_string(child->string);
                if (tmp == NULL) { internal_free(out); return NULL; }

                {
                    size_t newlen = strlen(out) + strlen(tmp) + 2;
                    char *newout = (char *)internal_malloc(newlen);
                    if (newout == NULL) { internal_free(out); internal_free(tmp); return NULL; }
                    if (strlen(out) > 1)
                    {
                        snprintf(newout, newlen, "%s,%s:", out, tmp);
                    }
                    else
                    {
                        snprintf(newout, newlen, "{%s:", tmp);
                    }
                    internal_free(out);
                    internal_free(tmp);
                    out = newout;
                }

                tmp = print_value(child, depth + 1, fmt);
                if (tmp == NULL) { internal_free(out); return NULL; }

                {
                    size_t newlen = strlen(out) + strlen(tmp) + 1;
                    char *newout = (char *)internal_malloc(newlen);
                    if (newout == NULL) { internal_free(out); internal_free(tmp); return NULL; }
                    snprintf(newout, newlen, "%s%s", out, tmp);
                    internal_free(out);
                    internal_free(tmp);
                    out = newout;
                }
                child = child->next;
            }

            {
                size_t newlen = strlen(out) + 2;
                char *newout = (char *)internal_malloc(newlen);
                if (newout == NULL) { internal_free(out); return NULL; }
                snprintf(newout, newlen, "%s}", out);
                internal_free(out);
                out = newout;
            }
            return out;
        }
    }

    return cJSON_strdup("null");
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    return print_value(item, 0, 0);
}

/* --------------------------------------------------------------------------
 * Delete
 * -------------------------------------------------------------------------- */

void cJSON_Delete(cJSON *item)
{
    cJSON *next;

    if (item == NULL) return;

    while (item)
    {
        next = item->next;
        if (item->child) cJSON_Delete(item->child);
        if (item->valuestring && !(item->type & cJSON_StringIsConst))
            internal_free(item->valuestring);
        if (item->string) internal_free(item->string);
        internal_free(item);
        item = next;
    }
}

/* --------------------------------------------------------------------------
 * Create functions
 * -------------------------------------------------------------------------- */

cJSON *cJSON_CreateNull(void)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_NULL; }
    return item;
}

cJSON *cJSON_CreateTrue(void)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_True; }
    return item;
}

cJSON *cJSON_CreateFalse(void)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_False; }
    return item;
}

cJSON *cJSON_CreateBool(int b)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = b ? cJSON_True : cJSON_False; }
    return item;
}

cJSON *cJSON_CreateNumber(double num)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_Number; item->valuedouble = num; item->valueint = (int)num; }
    return item;
}

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_String; item->valuestring = cJSON_strdup(string); }
    return item;
}

cJSON *cJSON_CreateArray(void)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_Array; }
    return item;
}

cJSON *cJSON_CreateObject(void)
{
    cJSON *item = (cJSON *)internal_malloc(sizeof(cJSON));
    if (item) { memset(item, 0, sizeof(cJSON)); item->type = cJSON_Object; }
    return item;
}

/* --------------------------------------------------------------------------
 * Add items
 * -------------------------------------------------------------------------- */

void cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    cJSON *child;

    if (array == NULL || item == NULL) return;

    if (array->child == NULL)
    {
        array->child = item;
    }
    else
    {
        child = array->child;
        while (child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    if (object == NULL || item == NULL) return;

    if (item->string) internal_free(item->string);
    item->string = cJSON_strdup(string);

    cJSON_AddItemToArray(object, item);
}

void cJSON_AddNumberToObject(cJSON *object, const char *name, double number)
{
    cJSON *num = cJSON_CreateNumber(number);
    cJSON_AddItemToObject(object, name, num);
}

void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string)
{
    cJSON *str = cJSON_CreateString(string);
    cJSON_AddItemToObject(object, name, str);
}

/* --------------------------------------------------------------------------
 * Lookup
 * -------------------------------------------------------------------------- */

int cJSON_GetArraySize(const cJSON *array)
{
    int count = 0;
    cJSON *child;

    if (array == NULL) return 0;
    child = array->child;
    while (child) { count++; child = child->next; }
    return count;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index)
{
    int i = 0;
    cJSON *child;

    if (array == NULL) return NULL;
    child = array->child;
    while (child && i < index) { child = child->next; i++; }
    return child;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    cJSON *child;

    if (object == NULL || string == NULL) return NULL;
    child = object->child;
    while (child)
    {
        if (child->string && cJSON_strcasecmp(child->string, string) == 0)
            return child;
        child = child->next;
    }
    return NULL;
}
