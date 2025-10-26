/** @file jsmn.h
 *
 * @brief Minimalistic JSON parser for embedded systems.
 *
 * JSMN (pronounced like 'jasmine') is a minimalistic JSON parser in C.
 * It can be easily integrated into resource-limited projects or embedded systems.
 *
 * @par
 * MIT License
 *
 * Copyright (c) 2010 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef JSMN_H
#define JSMN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

/**
 * JSON type identifier. Basic types are:
 *   o Object
 *   o Array
 *   o String
 *   o Other primitive: number, boolean (true/false) or null
 */
typedef enum
{
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1 << 0,
    JSMN_ARRAY = 1 << 1,
    JSMN_STRING = 1 << 2,
    JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

/**
 * JSMN error codes
 */
enum jsmnerr
{
    JSMN_ERROR_NOMEM = -1,  /* Not enough tokens were provided */
    JSMN_ERROR_INVAL = -2,  /* Invalid character inside JSON string */
    JSMN_ERROR_PART = -3    /* The string is not a full JSON packet */
};

/**
 * JSON token description.
 * type     type (object, array, string etc.)
 * start    start position in JSON data string
 * end      end position in JSON data string
 * size     number of child tokens
 */
typedef struct jsmntok
{
    jsmntype_t type;
    int32_t start;
    int32_t end;
    int32_t size;
#ifdef JSMN_PARENT_LINKS
    int32_t parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct jsmn_parser
{
    uint32_t pos;      /* offset in the JSON string */
    uint32_t toknext;  /* next token to allocate */
    int32_t toksuper;  /* superior token node, e.g. parent object or array */
} jsmn_parser_t;

/**
 * Create JSON parser over an array of tokens
 */
JSMN_API void jsmn_init(jsmn_parser_t * p_parser);

/**
 * Run JSON parser. It parses a JSON data string into an array of tokens,
 * each describing a single JSON object.
 */
JSMN_API int32_t jsmn_parse(jsmn_parser_t * p_parser, 
                            char const * const p_js, 
                            size_t const len,
                            jsmntok_t * p_tokens, 
                            uint32_t const num_tokens);

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */

/*** end of file ***/
