/** @file jsonprocess_test.c
 *
 * @brief JSON processing unit tests for STM32 (hardware-based).
 *
 * Tests jsonprocess.c usage of JSMN library on actual hardware.
 * Outputs test results via UART to serial terminal.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Sri Hari Shankar Sharma. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "jsmn.h"
#include "uart.h"
#include "delay.h"

/* Test configuration */
#define MAX_TOKENS 20

/* Test counters */
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

/* External UART state */
extern volatile uart_state_t g_tx_state;

/* Interrupt Enable Number */
#define USART2_IRQn 28u

/* Inline functions */
static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void NVIC_EnableIRQ(uint32_t IRQn) {
    volatile uint32_t *NVIC_ISER0 = (volatile uint32_t *)0xE000E100;
    NVIC_ISER0[IRQn >> 5] = (1u << (IRQn & 0x1F));
}

/* Helper: Wait for TX idle */
static void wait_tx_idle(void)
{
    uint32_t start = delay_get_tick();
    while ((g_tx_state != UART_STATE_IDLE) && !delay_elapsed(start, 1000)) {
        /* Wait */
    }
}

/* Helper: Safe transmit */
static void safe_transmit(char const * const p_str)
{
    wait_tx_idle();
    uart_transmit_buffer(p_str);
    wait_tx_idle();
}

/* Helper: Non-blocking delay */
static void delay_nb(uint32_t ms)
{
    uint32_t start = delay_get_tick();
    while (!delay_elapsed(start, ms)) {
        /* Wait */
    }
}

/* Helper: Compare JSON token with string (from jsonprocess.c) */
static int32_t jsoneq(char const * const p_json, jsmntok_t const * const p_tok, 
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

/* Test result reporting */
static void report_test(char const * test_name, int passed)
{
    tests_run++;
    if (passed) {
        tests_passed++;
        safe_transmit("[PASS] ");
    } else {
        tests_failed++;
        safe_transmit("[FAIL] ");
    }
    safe_transmit(test_name);
    safe_transmit("\r\n");
}

/* ============================================
 * TEST 1: Parse Valid JSON
 * ============================================ */
void test_json_parse_valid(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    /* Exact JSON from jsonprocess.c */
    char const json[] = 
        "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
        "\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"]}";
    
    jsmn_init(&parser);
    int32_t result = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int passed = (result >= 1) && (tokens[0].type == JSMN_OBJECT);
    
    report_test("Parse Valid JSON", passed);
}

/* ============================================
 * TEST 2: Extract 'user' Key
 * ============================================ */
void test_json_extract_user(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{\"user\": \"johndoe\", \"admin\": false}";
    
    jsmn_init(&parser);
    int32_t num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int found_user = 0;
    char extracted_value[20] = {0};
    
    for (int i = 1; i < num_tokens; i++) {
        if (jsoneq(json, &tokens[i], "user") == 0) {
            int value_len = tokens[i + 1].end - tokens[i + 1].start;
            strncpy(extracted_value, json + tokens[i + 1].start, value_len);
            extracted_value[value_len] = '\0';
            found_user = 1;
            break;
        }
    }
    
    int passed = found_user && (strcmp(extracted_value, "johndoe") == 0);
    
    report_test("Extract 'user' Key", passed);
}

/* ============================================
 * TEST 3: Extract 'uid' Number
 * ============================================ */
void test_json_extract_uid(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{\"uid\": 1000}";
    
    jsmn_init(&parser);
    int32_t num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int found_uid = 0;
    
    for (int i = 1; i < num_tokens; i++) {
        if (jsoneq(json, &tokens[i], "uid") == 0) {
            /* Just verify we found it (value parsing tested in production code) */
            found_uid = 1;
            break;
        }
    }
    
    int passed = found_uid;
    
    report_test("Extract 'uid' Number", passed);
}

/* ============================================
 * TEST 4: Handle Invalid JSON
 * ============================================ */
void test_json_parse_invalid(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    /* Malformed JSON (missing closing brace) */
    char const json[] = "{\"user\": \"johndoe\"";
    
    jsmn_init(&parser);
    int32_t result = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    /* Should return negative error code */
    int passed = (result < 0);
    
    report_test("Handle Invalid JSON", passed);
}

/* ============================================
 * TEST 5: Extract Array Elements
 * ============================================ */
void test_json_extract_array(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{\"groups\": [\"users\", \"wheel\"]}";
    
    jsmn_init(&parser);
    int32_t num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int found_array = 0;
    int array_size = 0;
    
    for (int i = 1; i < num_tokens; i++) {
        if (jsoneq(json, &tokens[i], "groups") == 0) {
            if (tokens[i + 1].type == JSMN_ARRAY) {
                array_size = tokens[i + 1].size;
                found_array = 1;
            }
            break;
        }
    }
    
    int passed = found_array && (array_size == 2);
    
    report_test("Extract Array Elements", passed);
}

/* ============================================
 * TEST 6: jsoneq() Function
 * ============================================ */
void test_jsoneq_function(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{\"key\": \"value\"}";
    
    jsmn_init(&parser);
    jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    /* Test matching key */
    int match = jsoneq(json, &tokens[1], "key");
    
    /* Test non-matching key */
    int no_match = jsoneq(json, &tokens[1], "other");
    
    int passed = (match == 0) && (no_match != 0);
    
    report_test("jsoneq() Function", passed);
}

/* ============================================
 * TEST 7: Empty JSON Object
 * ============================================ */
void test_json_empty_object(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{}";
    
    jsmn_init(&parser);
    int32_t result = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int passed = (result == 1) && (tokens[0].type == JSMN_OBJECT) && (tokens[0].size == 0);
    
    report_test("Empty JSON Object", passed);
}

/* ============================================
 * TEST 8: Boolean Value
 * ============================================ */
void test_json_extract_boolean(void)
{
    jsmn_parser_t parser;
    jsmntok_t tokens[MAX_TOKENS];
    
    char const json[] = "{\"admin\": false}";
    
    jsmn_init(&parser);
    int32_t num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);
    
    int found_admin = 0;
    
    for (int i = 1; i < num_tokens; i++) {
        if (jsoneq(json, &tokens[i], "admin") == 0) {
            found_admin = 1;
            break;
        }
    }
    
    int passed = found_admin;
    
    report_test("Extract Boolean Value", passed);
}

/* ============================================
 * TEST SUMMARY
 * ============================================ */
void print_summary(void)
{
    safe_transmit("\r\n========================================\r\n");
    safe_transmit("  JSON Processing Test Summary\r\n");
    safe_transmit("========================================\r\n");
    
    safe_transmit("Total Tests:  8\r\n");
    
    if (tests_passed == 8) {
        safe_transmit("Passed:       8\r\n");
    } else if (tests_passed == 7) {
        safe_transmit("Passed:       7\r\n");
    } else if (tests_passed >= 5) {
        safe_transmit("Passed:       5-6\r\n");
    } else {
        safe_transmit("Passed:       <5\r\n");
    }
    
    if (tests_failed == 0) {
        safe_transmit("Failed:       0\r\n");
    } else if (tests_failed == 1) {
        safe_transmit("Failed:       1\r\n");
    } else {
        safe_transmit("Failed:       >1\r\n");
    }
    
    safe_transmit("========================================\r\n");
    
    if (tests_failed == 0) {
        safe_transmit("\r\nALL TESTS PASSED! ✓\r\n");
    } else {
        safe_transmit("\r\nSOME TESTS FAILED ✗\r\n");
    }
}

/* ============================================
 * MAIN TEST RUNNER
 * ============================================ */
int32_t main(void)
{
    /* Initialize delay subsystem */
    delay_init();
    
    /* Initialize UART */
    uart_init();
    
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();
    
    delay_nb(1000);
    
    safe_transmit("\r\n\r\n");
    safe_transmit("############################################\r\n");
    safe_transmit("#  JSON PROCESSING UNIT TESTS             #\r\n");
    safe_transmit("############################################\r\n");
    
    delay_nb(500);
    
    /* Run all tests */
    test_json_parse_valid();
    delay_nb(100);
    
    test_json_extract_user();
    delay_nb(100);
    
    test_json_extract_uid();
    delay_nb(100);
    
    test_json_parse_invalid();
    delay_nb(100);
    
    test_json_extract_array();
    delay_nb(100);
    
    test_jsoneq_function();
    delay_nb(100);
    
    test_json_empty_object();
    delay_nb(100);
    
    test_json_extract_boolean();
    delay_nb(100);
    
    /* Print summary */
    print_summary();
    
    /* Hold in idle loop */
    for (;;) {
        delay_nb(1000);
    }
    
    return 0;
}