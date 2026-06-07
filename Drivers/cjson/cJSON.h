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

#ifndef cJSON__h
#define cJSON__h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* cJSON Types */
#define cJSON_Invalid (0)
#define cJSON_False   (1 << 0)
#define cJSON_True    (1 << 1)
#define cJSON_NULL    (1 << 2)
#define cJSON_Number  (1 << 3)
#define cJSON_String  (1 << 4)
#define cJSON_Array   (1 << 5)
#define cJSON_Object  (1 << 6)
#define cJSON_Raw     (1 << 7)

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* The cJSON structure */
typedef struct cJSON
{
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;

    int type;
    char *valuestring;
    int valueint;
    double valuedouble;

    char *string;
} cJSON;

/* Hooks for custom memory allocators */
typedef struct cJSON_Hooks
{
    void *(*malloc_fn)(size_t sz);
    void  (*free_fn)(void *ptr);
} cJSON_Hooks;

/* Supply malloc/free function pointers to cJSON */
void cJSON_InitHooks(cJSON_Hooks *hooks);

/* Parse a JSON string and return the root cJSON object */
cJSON *cJSON_Parse(const char *value);

/* Render a cJSON entity to text (unformatted — compact, single-line) */
char *cJSON_PrintUnformatted(const cJSON *item);

/* Delete a cJSON entity and all children */
void cJSON_Delete(cJSON *item);

/* Returns the number of items in an array (or object) */
int cJSON_GetArraySize(const cJSON *array);

/* Get item "string" from object. Case insensitive */
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);

/* For analysing failed parses — returns a pointer to the parse error */
const char *cJSON_GetErrorPtr(void);

/* Create basic types */
cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(int b);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);

/* Add item to array/object */
void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);

/* Helper: add number/string to object */
void cJSON_AddNumberToObject(cJSON *object, const char *name, double number);
void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);

/* Get array item at index */
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);

#ifdef __cplusplus
}
#endif

#endif /* cJSON__h */
