/** @file jsmn.c
 *
 * @brief JSMN parser implementation - minimalistic JSON parser.
 *
 * @par
 * MIT License - Copyright (c) 2010 Serge Zaitsev
 * See jsmn.h for full license text.
 */

#include <stddef.h>
#include <stdint.h>
#include "jsmn.h"
#include "types.h"  /* For bool_t type */

#ifdef __cplusplus
extern "C" {
#endif

/* Character code constants */
#define CHAR_QUOTE           '\"'
#define CHAR_BACKSLASH       '\\'
#define CHAR_FORWARD_SLASH   '/'
#define CHAR_BACKSPACE       'b'
#define CHAR_FORM_FEED       'f'
#define CHAR_CARRIAGE_RET    'r'
#define CHAR_NEWLINE         'n'
#define CHAR_TAB             't'
#define CHAR_UNICODE         'u'
#define CHAR_BRACE_OPEN      '{'
#define CHAR_BRACE_CLOSE     '}'
#define CHAR_BRACKET_OPEN    '['
#define CHAR_BRACKET_CLOSE   ']'
#define CHAR_COLON           ':'
#define CHAR_COMMA           ','
#define CHAR_SPACE           ' '
#define CHAR_NULL            '\0'
#define CHAR_LF              '\n'
#define CHAR_CR              '\r'
#define CHAR_HTAB            '\t'
#define CHAR_MINUS           '-'
#define CHAR_DIGIT_0         '0'
#define CHAR_DIGIT_9         '9'
#define CHAR_LOWER_T         't'
#define CHAR_LOWER_F         'f'
#define CHAR_LOWER_N         'n'
#define CHAR_UPPER_A         'A'
#define CHAR_UPPER_F         'F'
#define CHAR_LOWER_A         'a'
#define CHAR_LOWER_F_END     'f'

/* ASCII code ranges */
#define ASCII_PRINTABLE_MIN  32
#define ASCII_PRINTABLE_MAX  127
#define ASCII_DIGIT_0        48
#define ASCII_DIGIT_9        57
#define ASCII_UPPER_A        65
#define ASCII_UPPER_F        70
#define ASCII_LOWER_A        97
#define ASCII_LOWER_F        102

/* Unicode escape sequence length */
#define UNICODE_HEX_DIGITS   4


/*!
 * @brief Allocate next available token from token array.
 *
 * @param[in,out] p_parser Pointer to parser state.
 * @param[in] p_tokens Pointer to token array.
 * @param[in] num_tokens Maximum number of tokens available.
 *
 * @return Pointer to allocated token, or NULL if no tokens available.
 */
static jsmntok_t *
jsmn_alloc_token (jsmn_parser_t * p_parser, jsmntok_t * p_tokens,
                  size_t const num_tokens)
{
    jsmntok_t * p_tok = NULL;
    
    if (p_parser->toknext < num_tokens)
    {
        p_tok = &p_tokens[p_parser->toknext];
        p_parser->toknext++;
        
        p_tok->start = -1;
        p_tok->end = -1;
        p_tok->size = 0;
#ifdef JSMN_PARENT_LINKS
        p_tok->parent = -1;
#endif
    }
    
    return p_tok;
}


/*!
 * @brief Fill token with type and boundary information.
 *
 * @param[out] p_token Pointer to token to fill.
 * @param[in] type JSON type of this token.
 * @param[in] start Start position in JSON string.
 * @param[in] end End position in JSON string.
 */
static void
jsmn_fill_token (jsmntok_t * p_token, jsmntype_t const type,
                 int32_t const start, int32_t const end)
{
    p_token->type = type;
    p_token->start = start;
    p_token->end = end;
    p_token->size = 0;
}


/*!
 * @brief Parse JSON primitive (number, boolean, null).
 *
 * @param[in,out] p_parser Pointer to parser state.
 * @param[in] p_js Pointer to JSON string.
 * @param[in] len Length of JSON string.
 * @param[out] p_tokens Pointer to token array.
 * @param[in] num_tokens Maximum number of tokens.
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t
jsmn_parse_primitive (jsmn_parser_t * p_parser, char const * const p_js,
                      size_t const len, jsmntok_t * p_tokens,
                      size_t const num_tokens)
{
    jsmntok_t * p_token = NULL;
    uint32_t start = p_parser->pos;
    bool_t b_found = FALSE;  /* Changed to bool_t for Keil */

    for (; (p_parser->pos < len) && (CHAR_NULL != p_js[p_parser->pos]); 
         p_parser->pos++)
    {
        switch (p_js[p_parser->pos])
        {
#ifndef JSMN_STRICT
            case CHAR_COLON:
#endif
            case CHAR_HTAB:
            case CHAR_CR:
            case CHAR_LF:
            case CHAR_SPACE:
            case CHAR_COMMA:
            case CHAR_BRACKET_CLOSE:
            case CHAR_BRACE_CLOSE:
            {
                b_found = TRUE;  /* Changed to TRUE for Keil */
                break;
            }
            
            default:
            {
                /* Continue parsing primitive */
                break;
            }
        }
        
        if (b_found)
        {
            break;
        }
        
        /* Check for invalid characters in primitive */
        if ((p_js[p_parser->pos] < ASCII_PRINTABLE_MIN) || 
            (p_js[p_parser->pos] >= ASCII_PRINTABLE_MAX))
        {
            p_parser->pos = start;
            return JSMN_ERROR_INVAL;
        }
    }

#ifdef JSMN_STRICT
    /* In strict mode primitive must be followed by delimiter */
    if (!b_found)
    {
        p_parser->pos = start;
        return JSMN_ERROR_PART;
    }
#endif

    if (NULL == p_tokens)
    {
        p_parser->pos--;
        return 0;
    }
    
    p_token = jsmn_alloc_token(p_parser, p_tokens, num_tokens);
    
    if (NULL == p_token)
    {
        p_parser->pos = start;
        return JSMN_ERROR_NOMEM;
    }
    
    jsmn_fill_token(p_token, JSMN_PRIMITIVE, (int32_t)start, 
                    (int32_t)p_parser->pos);
                    
#ifdef JSMN_PARENT_LINKS
    p_token->parent = p_parser->toksuper;
#endif
    
    p_parser->pos--;
    return 0;
}


/*!
 * @brief Parse JSON string token.
 *
 * @param[in,out] p_parser Pointer to parser state.
 * @param[in] p_js Pointer to JSON string.
 * @param[in] len Length of JSON string.
 * @param[out] p_tokens Pointer to token array.
 * @param[in] num_tokens Maximum number of tokens.
 *
 * @return 0 on success, negative error code on failure.
 */
static int32_t
jsmn_parse_string (jsmn_parser_t * p_parser, char const * const p_js,
                   size_t const len, jsmntok_t * p_tokens,
                   size_t const num_tokens)
{
    jsmntok_t * p_token = NULL;
    uint32_t start = p_parser->pos;
    bool_t b_valid_hex = TRUE;  /* Changed to bool_t for Keil */

    /* Skip starting quote */
    p_parser->pos++;

    for (; (p_parser->pos < len) && (CHAR_NULL != p_js[p_parser->pos]); 
         p_parser->pos++)
    {
        char c = p_js[p_parser->pos];

        /* Quote: end of string */
        if (CHAR_QUOTE == c)
        {
            if (NULL == p_tokens)
            {
                return 0;
            }
            
            p_token = jsmn_alloc_token(p_parser, p_tokens, num_tokens);
            
            if (NULL == p_token)
            {
                p_parser->pos = start;
                return JSMN_ERROR_NOMEM;
            }
            
            jsmn_fill_token(p_token, JSMN_STRING, (int32_t)(start + 1u), 
                           (int32_t)p_parser->pos);
                           
#ifdef JSMN_PARENT_LINKS
            p_token->parent = p_parser->toksuper;
#endif
            return 0;
        }

        /* Backslash: escaped symbol expected */
        if ((CHAR_BACKSLASH == c) && ((p_parser->pos + 1u) < len))
        {
            p_parser->pos++;
            
            switch (p_js[p_parser->pos])
            {
                /* Allowed escaped symbols */
                case CHAR_QUOTE:
                case CHAR_FORWARD_SLASH:
                case CHAR_BACKSLASH:
                case CHAR_BACKSPACE:
                case CHAR_FORM_FEED:
                case CHAR_CARRIAGE_RET:
                case CHAR_NEWLINE:
                case CHAR_TAB:
                {
                    break;
                }
                
                /* Unicode escape sequence \uXXXX */
                case CHAR_UNICODE:
                {
                    p_parser->pos++;
                    
                    for (uint32_t i = 0u; 
                         (i < UNICODE_HEX_DIGITS) && 
                         (p_parser->pos < len) && 
                         (CHAR_NULL != p_js[p_parser->pos]);
                         i++)
                    {
                        /* Verify hex character (0-9, A-F, a-f) */
                        b_valid_hex = (((p_js[p_parser->pos] >= ASCII_DIGIT_0) && 
                                       (p_js[p_parser->pos] <= ASCII_DIGIT_9)) ||
                                      ((p_js[p_parser->pos] >= ASCII_UPPER_A) && 
                                       (p_js[p_parser->pos] <= ASCII_UPPER_F)) ||
                                      ((p_js[p_parser->pos] >= ASCII_LOWER_A) && 
                                       (p_js[p_parser->pos] <= ASCII_LOWER_F)));
                        
                        if (!b_valid_hex)
                        {
                            p_parser->pos = start;
                            return JSMN_ERROR_INVAL;
                        }
                        
                        p_parser->pos++;
                    }
                    p_parser->pos--;
                    break;
                }
                
                /* Unexpected escaped symbol */
                default:
                {
                    p_parser->pos = start;
                    return JSMN_ERROR_INVAL;
                }
            }
        }
    }
    
    p_parser->pos = start;
    return JSMN_ERROR_PART;
}


/*!
 * @brief Parse complete JSON string and fill token array.
 *
 * @param[in,out] p_parser Pointer to parser state.
 * @param[in] p_js Pointer to JSON string to parse.
 * @param[in] len Length of JSON string.
 * @param[out] p_tokens Pointer to token array (can be NULL for counting).
 * @param[in] num_tokens Maximum number of tokens available.
 *
 * @return Number of tokens parsed on success, negative error code on failure.
 */
JSMN_API int32_t
jsmn_parse (jsmn_parser_t * p_parser, char const * const p_js, 
            size_t const len, jsmntok_t * p_tokens, 
            uint32_t const num_tokens)
{
    int32_t r = 0;
    int32_t i = 0;
    jsmntok_t * p_token = NULL;
    int32_t count = (int32_t)p_parser->toknext;
    jsmntype_t type;

    for (; (p_parser->pos < len) && (CHAR_NULL != p_js[p_parser->pos]); 
         p_parser->pos++)
    {
        char c = p_js[p_parser->pos];
        
        switch (c)
        {
            case CHAR_BRACE_OPEN:
            case CHAR_BRACKET_OPEN:
            {
                count++;
                
                if (NULL == p_tokens)
                {
                    break;
                }
                
                p_token = jsmn_alloc_token(p_parser, p_tokens, num_tokens);
                
                if (NULL == p_token)
                {
                    return JSMN_ERROR_NOMEM;
                }
                
                if (-1 != p_parser->toksuper)
                {
                    jsmntok_t * p_t = &p_tokens[p_parser->toksuper];
                    
#ifdef JSMN_STRICT
                    /* In strict mode object/array cannot be a key */
                    if (JSMN_OBJECT == p_t->type)
                    {
                        return JSMN_ERROR_INVAL;
                    }
#endif
                    p_t->size++;
                    
#ifdef JSMN_PARENT_LINKS
                    p_token->parent = p_parser->toksuper;
#endif
                }
                
                p_token->type = (CHAR_BRACE_OPEN == c) ? JSMN_OBJECT : JSMN_ARRAY;
                p_token->start = (int32_t)p_parser->pos;
                p_parser->toksuper = (int32_t)(p_parser->toknext - 1u);
                break;
            }
            
            case CHAR_BRACE_CLOSE:
            case CHAR_BRACKET_CLOSE:
            {
                if (NULL == p_tokens)
                {
                    break;
                }
                
                type = (CHAR_BRACE_CLOSE == c) ? JSMN_OBJECT : JSMN_ARRAY;
                
#ifdef JSMN_PARENT_LINKS
                if (p_parser->toknext < 1u)
                {
                    return JSMN_ERROR_INVAL;
                }
                
                p_token = &p_tokens[p_parser->toknext - 1u];
                
                for (;;)
                {
                    if ((-1 != p_token->start) && (-1 == p_token->end))
                    {
                        if (type != p_token->type)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        
                        p_token->end = (int32_t)(p_parser->pos + 1u);
                        p_parser->toksuper = p_token->parent;
                        break;
                    }
                    
                    if (-1 == p_token->parent)
                    {
                        if ((type != p_token->type) || (-1 == p_parser->toksuper))
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        break;
                    }
                    
                    p_token = &p_tokens[p_token->parent];
                }
#else
                for (i = (int32_t)(p_parser->toknext - 1u); i >= 0; i--)
                {
                    p_token = &p_tokens[i];
                    
                    if ((-1 != p_token->start) && (-1 == p_token->end))
                    {
                        if (type != p_token->type)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        
                        p_parser->toksuper = -1;
                        p_token->end = (int32_t)(p_parser->pos + 1u);
                        break;
                    }
                }
                
                /* Error if unmatched closing bracket */
                if (-1 == i)
                {
                    return JSMN_ERROR_INVAL;
                }
                
                for (; i >= 0; i--)
                {
                    p_token = &p_tokens[i];
                    
                    if ((-1 != p_token->start) && (-1 == p_token->end))
                    {
                        p_parser->toksuper = i;
                        break;
                    }
                }
#endif
                break;
            }
            
            case CHAR_QUOTE:
            {
                r = jsmn_parse_string(p_parser, p_js, len, p_tokens, num_tokens);
                
                if (r < 0)
                {
                    return r;
                }
                
                count++;
                
                if ((-1 != p_parser->toksuper) && (NULL != p_tokens))
                {
                    p_tokens[p_parser->toksuper].size++;
                }
                break;
            }
            
            case CHAR_HTAB:
            case CHAR_CR:
            case CHAR_LF:
            case CHAR_SPACE:
            {
                break;
            }
            
            case CHAR_COLON:
            {
                p_parser->toksuper = (int32_t)(p_parser->toknext - 1u);
                break;
            }
            
            case CHAR_COMMA:
            {
                if ((NULL != p_tokens) && 
                    (-1 != p_parser->toksuper) &&
                    (JSMN_ARRAY != p_tokens[p_parser->toksuper].type) &&
                    (JSMN_OBJECT != p_tokens[p_parser->toksuper].type))
                {
#ifdef JSMN_PARENT_LINKS
                    p_parser->toksuper = p_tokens[p_parser->toksuper].parent;
#else
                    for (i = (int32_t)(p_parser->toknext - 1u); i >= 0; i--)
                    {
                        if ((JSMN_ARRAY == p_tokens[i].type) || 
                            (JSMN_OBJECT == p_tokens[i].type))
                        {
                            if ((-1 != p_tokens[i].start) && 
                                (-1 == p_tokens[i].end))
                            {
                                p_parser->toksuper = i;
                                break;
                            }
                        }
                    }
#endif
                }
                break;
            }
            
#ifdef JSMN_STRICT
            /* In strict mode primitives are numbers and booleans */
            case CHAR_MINUS:
            case CHAR_DIGIT_0:
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case CHAR_DIGIT_9:
            case CHAR_LOWER_T:
            case CHAR_LOWER_F:
            case CHAR_LOWER_N:
            {
                /* Primitives cannot be object keys */
                if ((NULL != p_tokens) && (-1 != p_parser->toksuper))
                {
                    jsmntok_t const * const p_t = &p_tokens[p_parser->toksuper];
                    
                    if ((JSMN_OBJECT == p_t->type) ||
                        ((JSMN_STRING == p_t->type) && (0 != p_t->size)))
                    {
                        return JSMN_ERROR_INVAL;
                    }
                }
#else
            default:
            {
#endif
                r = jsmn_parse_primitive(p_parser, p_js, len, p_tokens, 
                                        num_tokens);
                
                if (r < 0)
                {
                    return r;
                }
                
                count++;
                
                if ((-1 != p_parser->toksuper) && (NULL != p_tokens))
                {
                    p_tokens[p_parser->toksuper].size++;
                }
                break;
            }

#ifdef JSMN_STRICT
            /* Unexpected char in strict mode */
            default:
            {
                return JSMN_ERROR_INVAL;
            }
#endif
        }
    }

    if (NULL != p_tokens)
    {
        for (i = (int32_t)(p_parser->toknext - 1u); i >= 0; i--)
        {
            /* Unmatched opened object or array */
            if ((-1 != p_tokens[i].start) && (-1 == p_tokens[i].end))
            {
                return JSMN_ERROR_PART;
            }
        }
    }

    return count;
}


/*!
 * @brief Initialize JSMN parser to default state.
 *
 * @param[out] p_parser Pointer to parser structure to initialize.
 */
JSMN_API void
jsmn_init (jsmn_parser_t * p_parser)
{
    p_parser->pos = 0u;
    p_parser->toknext = 0u;
    p_parser->toksuper = -1;
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/
