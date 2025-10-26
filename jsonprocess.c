/** @file jsonprocess.c
 *
 * @brief JSON parsing and UART transmission using JSMN library.
 *
 * Parses embedded JSON string and transmits key-value pairs via UART.
 * Demonstrates token extraction from JSON using JSMN's zero-allocation parser.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "uart.h"
#include "jsonprocess.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum tokens expected in JSON string */
#define MAX_JSON_TOKENS         15u

/* Maximum buffer size for formatted output strings */
#define OUTPUT_BUFFER_SIZE      200u

/* Delay between UART transmissions in milliseconds */
#define TX_DELAY_MS             500u

/* Error codes for JSON processing */
#define JSON_ERR_PARSE_FAILED   1
#define JSON_ERR_NO_OBJECT      2
#define JSON_SUCCESS            0

/* External UART state variables */
extern volatile uart_state_t g_tx_state;

/* Hardcoded JSON test string */
static char const JSON_STRING[] =
    "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
    "\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"]}";


/*!
 * @brief Compare JSON token string with target string.
 *
 * @param[in] p_json Pointer to complete JSON string.
 * @param[in] p_tok Pointer to token to compare.
 * @param[in] p_s Target string for comparison.
 *
 * @return 0 if strings match, -1 otherwise.
 */
static int32_t
jsoneq (char const * const p_json, jsmntok_t const * const p_tok, 
        char const * const p_s)
{
    int32_t result = -1;
    size_t target_len = strlen(p_s);
    int32_t token_len = p_tok->end - p_tok->start;

    if ((JSMN_STRING == p_tok->type) && 
        ((int32_t)target_len == token_len) &&
        (0 == strncmp(p_json + p_tok->start, p_s, (size_t)token_len)))
    {
        result = 0;
    }

    return result;
}


/*!
 * @brief Initialize JSON processing subsystem.
 *
 * Initializes UART peripheral required for JSON output transmission.
 */
void
json_process_init (void)
{
    (void)uart_init();
}


/*!
 * @brief Parse and transmit JSON data via UART.
 *
 * Parses hardcoded JSON string using JSMN, extracts key-value pairs,
 * and transmits formatted output via UART. Handles arrays and primitives.
 *
 * @return 0 on success, positive error code on failure.
 */
int32_t
json_process (void)
{
    char output_buffer[OUTPUT_BUFFER_SIZE];
    int32_t i;
    int32_t parse_result;
    jsmn_parser parser;
    jsmntok_t tokens[MAX_JSON_TOKENS];

    /* Initialize JSMN parser */
    jsmn_init(&parser);
    
    parse_result = jsmn_parse(&parser, JSON_STRING, strlen(JSON_STRING), 
                              tokens, MAX_JSON_TOKENS);
    
    /* Enable USART2 interrupts */
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();

    /* Check if parsing succeeded */
    if (parse_result < 0)
    {
        (void)sprintf(output_buffer, "Failed to parse JSON: %ld\r\n", 
                      (long)parse_result);

        if (UART_STATE_IDLE == g_tx_state)
        {
            (void)uart_transmit_buffer(output_buffer);
            delay_ms(TX_DELAY_MS);
        }
        
        return JSON_ERR_PARSE_FAILED;
    }

    /* Verify top-level element is an object */
    if ((parse_result < 1) || (JSMN_OBJECT != tokens[0].type))
    {
        (void)sprintf(output_buffer, "Object expected\r\n");

        if (UART_STATE_IDLE == g_tx_state)
        {
            (void)uart_transmit_buffer(output_buffer);
            delay_ms(TX_DELAY_MS);
        }
        
        return JSON_ERR_NO_OBJECT;
    }

    /* Process all keys in root object */
    for (i = 1; i < parse_result; i++)
    {
        if (0 == jsoneq(JSON_STRING, &tokens[i], "user"))
        {
            (void)sprintf(output_buffer, "- User: %.*s\r\n", 
                          tokens[i + 1].end - tokens[i + 1].start,
                          JSON_STRING + tokens[i + 1].start);
            
            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer(output_buffer);
                delay_ms(TX_DELAY_MS);
            }
            i++;
        }
        else if (0 == jsoneq(JSON_STRING, &tokens[i], "admin"))
        {
            (void)sprintf(output_buffer, "- Admin: %.*s\r\n", 
                          tokens[i + 1].end - tokens[i + 1].start,
                          JSON_STRING + tokens[i + 1].start);

            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer(output_buffer);
                delay_ms(TX_DELAY_MS);
            }
            i++;
        }
        else if (0 == jsoneq(JSON_STRING, &tokens[i], "uid"))
        {
            (void)sprintf(output_buffer, "- UID: %.*s\r\n", 
                          tokens[i + 1].end - tokens[i + 1].start,
                          JSON_STRING + tokens[i + 1].start);

            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer(output_buffer);
                delay_ms(TX_DELAY_MS);
            }
            i++;
        }
        else if (0 == jsoneq(JSON_STRING, &tokens[i], "groups"))
        {
            int32_t j;
            
            (void)sprintf(output_buffer, "- Groups:\r\n");

            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer(output_buffer);
                delay_ms(TX_DELAY_MS);
            }
            
            /* Verify groups is an array */
            if (JSMN_ARRAY != tokens[i + 1].type)
            {
                continue;
            }
            
            /* Process array elements */
            for (j = 0; j < tokens[i + 1].size; j++)
            {
                jsmntok_t const * const p_group = &tokens[i + j + 2];
                
                (void)sprintf(output_buffer, "  * %.*s\r\n", 
                              p_group->end - p_group->start, 
                              JSON_STRING + p_group->start);
                
                if (UART_STATE_IDLE == g_tx_state)
                {
                    (void)uart_transmit_buffer(output_buffer);
                    delay_ms(TX_DELAY_MS);
                }
            }
            
            i += tokens[i + 1].size + 1;
        }
        else
        {
            (void)sprintf(output_buffer, "Unexpected key: %.*s\r\n", 
                          tokens[i].end - tokens[i].start,
                          JSON_STRING + tokens[i].start);
            
            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer(output_buffer);
                delay_ms(TX_DELAY_MS);
            }
        }
    }
    
    return JSON_SUCCESS;
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/
