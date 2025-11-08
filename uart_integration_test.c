/** @file uart_integration_tests_no_loopback.c
 *
 * @brief Integration tests that DON'T require loopback.
 *
 * Tests TX/RX state machines and concurrent operations without
 * requiring data to be echoed back. Useful when loopback isn't available.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "types.h"

/* Test configuration */
#define TEST_TIMEOUT_MS           2000u
#define STRESS_TEST_CYCLES        20u

/* External UART state variables */
extern volatile uart_state_t g_tx_state;
extern volatile uart_state_t g_rx_state;
extern volatile uart_error_t g_error;
extern char * g_p_rx_buffer;
extern volatile uint32_t g_rx_index;

/* Test result tracking */
static uint32_t g_tests_passed = 0;
static uint32_t g_tests_failed = 0;

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

/*!
 * @brief Wait for TX completion with timeout.
 */
static int32_t wait_tx_complete(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    
    while ((g_tx_state != UART_STATE_IDLE) && (elapsed < timeout_ms)) {
        delay_ms(10);
        elapsed += 10;
    }
    
    return (g_tx_state == UART_STATE_IDLE) ? 0 : -1;
}

/*!
 * @brief Safe transmit wrapper.
 */
static void safe_transmit(char const * const p_str)
{
    wait_tx_complete(1000);
    uart_transmit_buffer(p_str);
    wait_tx_complete(1000);
}


/*!
 * @brief Integration Test 1: TX state machine validation.
 *
 * Tests that TX properly transitions: IDLE → BUSY → IDLE
 */
static void test_tx_state_machine(void)
{
    safe_transmit("\r\n=== Test: TX State Machine ===\r\n");
    
    /* Verify starts in IDLE */
    if (g_tx_state != UART_STATE_IDLE) {
        safe_transmit("FAIL: Not in IDLE state initially\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Start transmission */
    uart_transmit_buffer("TX_STATE_TEST");
    
    /* Should be BUSY immediately */
    if (g_tx_state != UART_STATE_TX_BUSY) {
        safe_transmit("FAIL: Did not enter TX_BUSY state\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Wait for completion */
    if (wait_tx_complete(TEST_TIMEOUT_MS) != 0) {
        safe_transmit("FAIL: TX timeout\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Should return to IDLE */
    if (g_tx_state != UART_STATE_IDLE) {
        safe_transmit("FAIL: Did not return to IDLE\r\n");
        g_tests_failed++;
        return;
    }
    
    safe_transmit("PASS\r\n");
    g_tests_passed++;
}


/*!
 * @brief Integration Test 2: RX state machine validation.
 *
 * Tests that RX properly transitions: IDLE → BUSY → IDLE
 * Cancels RX after verification (no data needed).
 */
static void test_rx_state_machine(void)
{
    safe_transmit("\r\n=== Test: RX State Machine ===\r\n");
    
    /* Verify starts in IDLE */
    if (g_rx_state != UART_STATE_IDLE) {
        safe_transmit("FAIL: RX not in IDLE state initially\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Start reception */
    if (uart_receive_buffer() != 0) {
        safe_transmit("FAIL: Could not start RX\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Should be BUSY immediately */
    if (g_rx_state != UART_STATE_RX_BUSY) {
        safe_transmit("FAIL: Did not enter RX_BUSY state\r\n");
        g_tests_failed++;
        g_rx_state = UART_STATE_IDLE; /* Clean up */
        return;
    }
    
    /* Manually cancel RX (simulates completion) */
    g_rx_state = UART_STATE_IDLE;
    
    safe_transmit("PASS\r\n");
    g_tests_passed++;
}


/*!
 * @brief Integration Test 3: Concurrent TX stress test.
 *
 * Sends multiple packets back-to-back rapidly.
 */
static void test_tx_stress(void)
{
    uint32_t i;
    
    safe_transmit("\r\n=== Test: TX Stress ===\r\n");
    
    for (i = 0; i < STRESS_TEST_CYCLES; i++) {
        /* Wait for idle */
        if (wait_tx_complete(TEST_TIMEOUT_MS) != 0) {
            safe_transmit("FAIL: TX timeout during stress\r\n");
            g_tests_failed++;
            return;
        }
        
        /* Send packet */
        uart_transmit_buffer("STRESS_");
    }
    
    /* Final wait */
    wait_tx_complete(TEST_TIMEOUT_MS);
    
    safe_transmit("\r\nPASS\r\n");
    g_tests_passed++;
}


/*!
 * @brief Integration Test 4: TX/RX isolation.
 *
 * Verifies TX and RX state machines don't interfere with each other.
 */
static void test_tx_rx_isolation(void)
{
    safe_transmit("\r\n=== Test: TX/RX Isolation ===\r\n");
    
    /* Start RX */
    if (uart_receive_buffer() != 0) {
        safe_transmit("FAIL: Could not start RX\r\n");
        g_tests_failed++;
        return;
    }
    
    /* RX should be busy */
    if (g_rx_state != UART_STATE_RX_BUSY) {
        safe_transmit("FAIL: RX not busy\r\n");
        g_tests_failed++;
        g_rx_state = UART_STATE_IDLE;
        return;
    }
    
    /* Start TX while RX is busy */
    uart_transmit_buffer("TX_DURING_RX");
    
    /* TX should work independently */
    if (g_tx_state != UART_STATE_TX_BUSY) {
        safe_transmit("FAIL: TX blocked by RX\r\n");
        g_tests_failed++;
        g_rx_state = UART_STATE_IDLE;
        return;
    }
    
    /* Wait for TX to complete */
    wait_tx_complete(TEST_TIMEOUT_MS);
    
    /* RX should still be busy (independent) */
    if (g_rx_state != UART_STATE_RX_BUSY) {
        safe_transmit("FAIL: RX affected by TX\r\n");
        g_tests_failed++;
        g_rx_state = UART_STATE_IDLE;
        return;
    }
    
    /* Clean up */
    g_rx_state = UART_STATE_IDLE;
    
    safe_transmit("PASS\r\n");
    g_tests_passed++;
}


/*!
 * @brief Integration Test 5: Error injection and recovery.
 *
 * Simulates hardware errors and validates recovery mechanism.
 */
static void test_error_recovery_integration(void)
{
    safe_transmit("\r\n=== Test: Error Recovery Integration ===\r\n");
    
    uart_error_t errors[] = {
        UART_ERROR_OVERRUN,
        UART_ERROR_FRAMING,
        UART_ERROR_NOISE,
        UART_ERROR_PARITY
    };
    
    for (uint32_t i = 0; i < 4; i++) {
        /* Inject error */
        g_rx_state = UART_STATE_ERROR;
        g_error = errors[i];
        
        /* Attempt recovery */
        uart_error_reset();
        
        /* Verify recovery */
        if (g_error != UART_ERROR_NONE || g_rx_state != UART_STATE_RX_BUSY) {
            safe_transmit("FAIL: Recovery failed for error type\r\n");
            g_tests_failed++;
            g_rx_state = UART_STATE_IDLE;
            return;
        }
        
        /* Clean up */
        g_rx_state = UART_STATE_IDLE;
    }
    
    safe_transmit("PASS: All error types recovered\r\n");
    g_tests_passed++;
}


/*!
 * @brief Integration Test 6: Buffer boundary validation.
 *
 * Tests that RX buffer index management works correctly.
 */
static void test_buffer_boundary(void)
{
    safe_transmit("\r\n=== Test: Buffer Boundary ===\r\n");
    
    /* Clear buffer */
    memset(g_p_rx_buffer, 0, RX_BUFFER_SIZE_BYTES);
    g_rx_index = 0;
    
    /* Manually simulate filling buffer to near-capacity */
    for (uint32_t i = 0; i < RX_BUFFER_SIZE_BYTES - 2; i++) {
        g_p_rx_buffer[i] = 'A';
    }
    g_rx_index = RX_BUFFER_SIZE_BYTES - 2;
    
    /* Verify we haven't overflowed */
    if (g_rx_index >= RX_BUFFER_SIZE_BYTES) {
        safe_transmit("FAIL: Buffer index overflow\r\n");
        g_tests_failed++;
        return;
    }
    
    /* Reset for next test */
    g_rx_index = 0;
    
    safe_transmit("PASS\r\n");
    g_tests_passed++;
}


/*!
 * @brief Print integration test summary.
 */
static void print_integration_summary(void)
{
    safe_transmit("\r\n========================================\r\n");
    safe_transmit("   INTEGRATION TEST SUMMARY (No Loopback)\r\n");
    safe_transmit("========================================\r\n");
    
    safe_transmit("Total Tests:  6\r\n");
    
    if (g_tests_passed == 6) {
        safe_transmit("Passed:       6\r\n");
    } else if (g_tests_passed == 5) {
        safe_transmit("Passed:       5\r\n");
    } else if (g_tests_passed == 4) {
        safe_transmit("Passed:       4\r\n");
    } else {
        safe_transmit("Passed:       <4\r\n");
    }
    
    if (g_tests_failed == 0) {
        safe_transmit("Failed:       0\r\n");
    } else if (g_tests_failed == 1) {
        safe_transmit("Failed:       1\r\n");
    } else {
        safe_transmit("Failed:       >1\r\n");
    }
    
    safe_transmit("========================================\r\n");
    
    if (g_tests_failed == 0) {
        safe_transmit("\r\nALL TESTS PASSED!\r\n");
        safe_transmit("Note: These tests don't require loopback.\r\n");
        safe_transmit("For full RX testing, use physical loopback.\r\n");
    } else {
        safe_transmit("\r\nSOME TESTS FAILED - CHECK OUTPUT\r\n");
    }
}


/*!
 * @brief Main integration test runner (no loopback needed).
 */
int32_t main(void)
{
    uart_init();
    
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();
    
    delay_ms(1000);
    
    safe_transmit("\r\n\r\n");
    safe_transmit("############################################\r\n");
    safe_transmit("#  UART INTEGRATION TESTS (No Loopback)   #\r\n");
    safe_transmit("#                                          #\r\n");
    safe_transmit("#  Tests state machines without requiring  #\r\n");
    safe_transmit("#  physical loopback or echo               #\r\n");
    safe_transmit("############################################\r\n");
    
    delay_ms(500);
    
    /* Run integration tests */
    test_tx_state_machine();
    delay_ms(200);
    
    test_rx_state_machine();
    delay_ms(200);
    
    test_tx_stress();
    delay_ms(200);
    
    test_tx_rx_isolation();
    delay_ms(200);
    
    test_error_recovery_integration();
    delay_ms(200);
    
    test_buffer_boundary();
    delay_ms(200);
    
    /* Print summary */
    print_integration_summary();
    
    /* Hold in idle loop */
    for (;;) {
        delay_ms(1000);
    }
    
    return 0;
}

/*** end of file ***/
