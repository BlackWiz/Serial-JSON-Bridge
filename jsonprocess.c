/** @file jsonprocess.c
 *
 * @brief JSON parsing and UART transmission using JSMN library (non-blocking).
 *
 * Parses embedded JSON string and transmits key-value pairs via UART
 * using non-blocking delays to prevent CPU freezing.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "uart.h"
#include "delay.h"
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

/* JSON processor state machine */
typedef enum {
    JSON_STATE_IDLE = 0,
    JSON_STATE_PARSING,
    JSON_STATE_TRANSMITTING,
    JSON_STATE_WAITING,
    JSON_STATE_COMPLETE
} json_state_t;

/* Global state for non-blocking operation */
static json_state_t g_json_state = JSON_STATE_IDLE;
static int32_t g_current_token = 1;  /* Start at 1 to skip root object */
static uint32_t g_delay_start = 0;
static jsmn_parser_t g_parser;
static jsmntok_t g_tokens[MAX_JSON_TOKENS];
static int32_t g_parse_result = 0;

/* Interrupt Enable Number */
#define USART2_IRQn 28u

/* Inline functions for interrupt control */
static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void NVIC_EnableIRQ(uint32_t IRQn) {
    volatile uint32_t *NVIC_ISER0 = (volatile uint32_t *)0xE000E100;
    NVIC_ISER0[IRQn >> 5] = (1u << (IRQn & 0x1F));
}

/* External UART state variables */
extern volatile uart_state_t g_tx_state;

/* Hardcoded JSON test string */
static char const JSON_STRING[] =
    "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
    "\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"]}";


/*!
 * @brief Compare JSON token string with target string.
 */
static int32_t
jsoneq(char const * const p_json, jsmntok_t const * const p_tok, 
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
 */
void json_process_init(void)
{
    /* Initialize delay subsystem */
    delay_init();
    
    /* Initialize UART */
    (void)uart_init();
    
    /* Enable USART2 interrupts */
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();
    
    /* Parse JSON once at initialization */
    jsmn_init(&g_parser);
    g_parse_result = jsmn_parse(&g_parser, JSON_STRING, strlen(JSON_STRING), 
                                g_tokens, MAX_JSON_TOKENS);
    
    /* Reset state machine */
    g_json_state = JSON_STATE_IDLE;
    g_current_token = 1;
}


/*!
 * @brief Process JSON with non-blocking state machine.
 *
 * Call this repeatedly in main loop. It progresses through parsing
 * and transmission states without blocking.
 *
 * @return 0 if still processing, positive error code on failure,
 *         1 when complete.
 */
int32_t json_process(void)
{
    char output_buffer[OUTPUT_BUFFER_SIZE];
    int32_t i;

    switch (g_json_state)
    {
        case JSON_STATE_IDLE:
        {
            /* Check if parsing succeeded */
            if (g_parse_result < 0)
            {
                (void)sprintf(output_buffer, "Failed to parse JSON: %ld\r\n", 
                              (long)g_parse_result);
                
                if (UART_STATE_IDLE == g_tx_state)
                {
                    (void)uart_transmit_buffer(output_buffer);
                    g_json_state = JSON_STATE_COMPLETE;
                }
                return JSON_ERR_PARSE_FAILED;
            }

            /* Verify top-level element is an object */
            if ((g_parse_result < 1) || (JSMN_OBJECT != g_tokens[0].type))
            {
                (void)sprintf(output_buffer, "Object expected\r\n");
                
                if (UART_STATE_IDLE == g_tx_state)
                {
                    (void)uart_transmit_buffer(output_buffer);
                    g_json_state = JSON_STATE_COMPLETE;
                }
                return JSON_ERR_NO_OBJECT;
            }
            
            /* Start processing tokens */
            g_json_state = JSON_STATE_TRANSMITTING;
            g_current_token = 1;
            break;
        }

        case JSON_STATE_TRANSMITTING:
        {
            /* Only transmit if UART is idle */
            if (UART_STATE_IDLE != g_tx_state)
            {
                break;  /* Wait for previous transmission to complete */
            }
            
            /* Check if we've processed all tokens */
            if (g_current_token >= g_parse_result)
            {
                g_json_state = JSON_STATE_COMPLETE;
                break;
            }
            
            i = g_current_token;
            
            /* Process current token */
            if (0 == jsoneq(JSON_STRING, &g_tokens[i], "user"))
            {
                (void)sprintf(output_buffer, "- User: %.*s\r\n", 
                              g_tokens[i + 1].end - g_tokens[i + 1].start,
                              JSON_STRING + g_tokens[i + 1].start);
                (void)uart_transmit_buffer(output_buffer);
                g_current_token += 2;
                
                /* Start non-blocking delay */
                g_delay_start = delay_start();
                g_json_state = JSON_STATE_WAITING;
            }
            else if (0 == jsoneq(JSON_STRING, &g_tokens[i], "admin"))
            {
                (void)sprintf(output_buffer, "- Admin: %.*s\r\n", 
                              g_tokens[i + 1].end - g_tokens[i + 1].start,
                              JSON_STRING + g_tokens[i + 1].start);
                (void)uart_transmit_buffer(output_buffer);
                g_current_token += 2;
                
                g_delay_start = delay_start();
                g_json_state = JSON_STATE_WAITING;
            }
            else if (0 == jsoneq(JSON_STRING, &g_tokens[i], "uid"))
            {
                (void)sprintf(output_buffer, "- UID: %.*s\r\n", 
                              g_tokens[i + 1].end - g_tokens[i + 1].start,
                              JSON_STRING + g_tokens[i + 1].start);
                (void)uart_transmit_buffer(output_buffer);
                g_current_token += 2;
                
                g_delay_start = delay_start();
                g_json_state = JSON_STATE_WAITING;
            }
            else if (0 == jsoneq(JSON_STRING, &g_tokens[i], "groups"))
            {
                int32_t j;
                
                (void)sprintf(output_buffer, "- Groups:\r\n");
                (void)uart_transmit_buffer(output_buffer);
                
                g_delay_start = delay_start();
                g_json_state = JSON_STATE_WAITING;
                
                /* Verify groups is an array */
                if (JSMN_ARRAY != g_tokens[i + 1].type)
                {
                    g_current_token++;
                    break;
                }
                
                /* Process array elements */
                for (j = 0; j < g_tokens[i + 1].size; j++)
                {
                    /* This still blocks for array elements - see note below */
                    jsmntok_t const * const p_group = &g_tokens[i + j + 2];
                    
                    /* Wait for UART idle */
                    while (UART_STATE_IDLE != g_tx_state) {
                        /* Spin wait - could be improved */
                    }
                    
                    (void)sprintf(output_buffer, "  * %.*s\r\n", 
                                  p_group->end - p_group->start, 
                                  JSON_STRING + p_group->start);
                    (void)uart_transmit_buffer(output_buffer);
                    
                    /* Non-blocking delay between array elements */
                    uint32_t array_delay = delay_start();
                    while (!delay_elapsed(array_delay, TX_DELAY_MS)) {
                        /* Allow other processing here */
                    }
                }
                
                g_current_token += g_tokens[i + 1].size + 2;
            }
            else
            {
                (void)sprintf(output_buffer, "Unexpected key: %.*s\r\n", 
                              g_tokens[i].end - g_tokens[i].start,
                              JSON_STRING + g_tokens[i].start);
                (void)uart_transmit_buffer(output_buffer);
                g_current_token++;
                
                g_delay_start = delay_start();
                g_json_state = JSON_STATE_WAITING;
            }
            break;
        }

        case JSON_STATE_WAITING:
        {
            /* Non-blocking delay - check if time has elapsed */
            if (delay_elapsed(g_delay_start, TX_DELAY_MS))
            {
                g_json_state = JSON_STATE_TRANSMITTING;
            }
            /* Otherwise, return and let main loop do other work */
            break;
        }

        case JSON_STATE_COMPLETE:
        {
            /* Reset for next iteration if needed */
            return JSON_SUCCESS;
        }

        default:
        {
            g_json_state = JSON_STATE_IDLE;
            break;
        }
    }
    
    /* Still processing */
    return 0;
}


/*!
 * @brief Reset JSON processor to process data again.
 */
void json_process_reset(void)
{
    g_json_state = JSON_STATE_IDLE;
    g_current_token = 1;
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/
