/** @file uart_test.c
 *
 * @brief Host unit tests for UART driver (mocked hardware).
 *
 * Link this test with the real uart.c (which defines the actual globals).
 * The test provides mock storage and redirects the driver's pointers to mocks.
 *
 * Build (host):
 *   gcc -Wall -Wextra -std=c11 -O0 -o test_uart uart.c uart_test.c
 *   ./test_uart
 *
 * Or (ARM toolchain) to produce an ELF for target:
 *   arm-none-eabi-gcc -g -O0 -mcpu=cortex-m0 -mthumb -o uart_test.elf uart.c uart_test.c
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ============================================
 * TEST CONFIGURATION
 * ============================================ */

/* Uncomment if you want to build uart.c differently for unit tests:
   #define UNIT_TEST
*/

/* Define boolean type used by driver if not provided by uart.h */
typedef uint8_t bool_t;
#define TRUE  1u
#define FALSE 0u

/* ============================================
 * MOCK HARDWARE STORAGE (test-owned)
 * ============================================ */

/* Mock storage for registers (actual driver uses pointers to these) */
static uint32_t mock_USART_ISR = 0;
static uint32_t mock_USART_RDR = 0;
static uint32_t mock_USART_TDR = 0;
static uint32_t mock_USART_CR1 = 0;
static uint32_t mock_USART_ICR = 0;

static uint32_t mock_RCC_APBENR1 = 0;
static uint32_t mock_RCC_IOPENR = 0;
static uint32_t mock_GPIOx_MODER = 0;
static uint32_t mock_GPIOx_AFRL = 0;
static uint32_t mock_USART_BRR = 0;
static uint32_t mock_SYST_CSR = 0;
static uint32_t mock_SYST_RVR = 0;

/* ============================================
 * CONSTANTS (match driver)
 * ============================================ */

#define USART_CR1_TXEIE_BIT    7u
#define USART_CR1_RXNEIE_BIT   5u
#define USART_ISR_TXE_BIT      7u
#define USART_ISR_RXNE_BIT     5u
#define USART_ISR_ORE_BIT      3u
#define USART_ISR_FE_BIT       2u
#define USART_ISR_NF_BIT       1u
#define USART_ISR_PE_BIT       0u

#define RX_BUFFER_SIZE_BYTES   100u

/* ============================================
 * DRIVER TYPES (should match uart.c / uart.h)
 * If you have uart.h, include it instead and remove these typedefs.
 * ============================================ */

typedef enum {
    UART_STATE_IDLE = 0,
    UART_STATE_TX_BUSY,
    UART_STATE_RX_BUSY,
    UART_STATE_ERROR
} uart_state_t;

typedef enum {
    UART_ERROR_NONE = 0,
    UART_ERROR_OVERRUN,
    UART_ERROR_FRAMING,
    UART_ERROR_PARITY,
    UART_ERROR_NOISE
} uart_error_t;

/* ============================================
 * EXTERN DECLARATIONS (symbols owned by uart.c)
 * Do NOT provide initializers here — they must be defined in uart.c.
 * ============================================ */

/* Peripheral register pointer symbols declared in uart.c */
extern volatile uint32_t *USART_ISR;
extern volatile uint32_t *USART_RDR;
extern volatile uint32_t *USART_TDR;
extern volatile uint32_t *USART_CR1;
extern volatile uint32_t *USART_ICR;

extern volatile uint32_t *RCC_APBENR1;
extern volatile uint32_t *RCC_IOPENR;
extern volatile uint32_t *GPIOx_MODER;
extern volatile uint32_t *GPIOx_AFRL;
extern volatile uint32_t *USART_BRR;
extern volatile uint32_t *SYST_CSR;
extern volatile uint32_t *SYST_RVR;

extern volatile uint32_t *USART2;
extern volatile uint32_t *RCC;
extern volatile uint32_t *GPIOA;

/* Driver globals (defined in uart.c) */
extern volatile char const *g_p_tx_buffer;
extern char g_rx_buffer_storage[]; /* size declared in uart.c */
extern char *g_p_rx_buffer;
extern volatile uint32_t g_tx_length;
extern volatile uint32_t g_tx_index;
extern volatile uint32_t g_rx_index;
extern volatile uart_state_t g_tx_state;
extern volatile uart_state_t g_rx_state;
extern volatile uart_error_t g_error;

/* ============================================
 * COPY FUNCTIONS TO TEST (from uart.c)
 * ============================================ */

static inline void uart_process_tx(void)
{
    if ((NULL != g_p_tx_buffer) && (g_tx_index < g_tx_length))
    {
        *USART_TDR = (uint32_t)g_p_tx_buffer[g_tx_index];
        g_tx_index++;
    }
    else
    {
        *USART_CR1 &= ~(1u << USART_CR1_TXEIE_BIT);
        g_p_tx_buffer = NULL;
        g_tx_state = UART_STATE_IDLE;
    }
}

static inline bool_t uart_has_error(void)
{
    return (((*USART_ISR & (1u << USART_ISR_ORE_BIT)) != 0u) ||
            ((*USART_ISR & (1u << USART_ISR_FE_BIT)) != 0u) ||
            ((*USART_ISR & (1u << USART_ISR_NF_BIT)) != 0u) ||
            ((*USART_ISR & (1u << USART_ISR_PE_BIT)) != 0u));
}

static inline uart_error_t uart_handle_rx_error(void)
{
    *USART_CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
    g_rx_state = UART_STATE_ERROR;
    
    if ((*USART_ISR & (1u << USART_ISR_ORE_BIT)) != 0u)
    {
        *USART_ICR |= (1u << USART_ISR_ORE_BIT);
        return UART_ERROR_OVERRUN;
    }
    else if ((*USART_ISR & (1u << USART_ISR_FE_BIT)) != 0u)
    {
        *USART_ICR |= (1u << USART_ISR_FE_BIT);
        return UART_ERROR_FRAMING;
    }
    else if ((*USART_ISR & (1u << USART_ISR_PE_BIT)) != 0u)
    {
        *USART_ICR |= (1u << USART_ISR_PE_BIT);
        return UART_ERROR_PARITY;
    }
    else if ((*USART_ISR & (1u << USART_ISR_NF_BIT)) != 0u)
    {
        *USART_ICR |= (1u << USART_ISR_NF_BIT);
        return UART_ERROR_NOISE;
    }
    
    return UART_ERROR_NONE;
}

static inline uart_error_t uart_process_rx(void)
{
    if (uart_has_error())
    {
        return uart_handle_rx_error();
    }
    
    g_error = UART_ERROR_NONE;
    
    if (g_rx_index < (RX_BUFFER_SIZE_BYTES - 1u))
    {
        g_p_rx_buffer[g_rx_index] = (char)*USART_RDR;
        g_rx_index++;

        if ((g_rx_index > 0u) && 
            (('\n' == g_p_rx_buffer[g_rx_index - 1u]) || 
             ('\r' == g_p_rx_buffer[g_rx_index - 1u])))
        {
            g_p_rx_buffer[g_rx_index] = '\0';
            g_rx_state = UART_STATE_IDLE;
            *USART_CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
        }
    }
    else
    {
        *USART_CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
        g_rx_state = UART_STATE_IDLE;
    }
    
    return UART_ERROR_NONE;
}

/* ============================================
 * TEST UTILITIES
 * ============================================ */

void setup_test(void)
{
    /* Redirect the driver's peripheral pointers to our mock storage */
    USART_ISR   = &mock_USART_ISR;
    USART_RDR   = &mock_USART_RDR;
    USART_TDR   = &mock_USART_TDR;
    USART_CR1   = &mock_USART_CR1;
    USART_ICR   = &mock_USART_ICR;

    RCC_APBENR1 = &mock_RCC_APBENR1;
    RCC_IOPENR  = &mock_RCC_IOPENR;
    GPIOx_MODER  = &mock_GPIOx_MODER;
    GPIOx_AFRL   = &mock_GPIOx_AFRL;
    USART_BRR    = &mock_USART_BRR;
    SYST_CSR     = &mock_SYST_CSR;
    SYST_RVR     = &mock_SYST_RVR;

    /* Reset driver-visible globals (driver defines them; we set initial values) */
    g_tx_state = UART_STATE_IDLE;
    g_rx_state = UART_STATE_IDLE;
    g_tx_index = 0;
    g_rx_index = 0;
    g_error = UART_ERROR_NONE;
    g_p_tx_buffer = NULL;
    g_tx_length = 0;

    /* Clear mock registers/storage */
    mock_USART_ISR = 0;
    mock_USART_RDR = 0;
    mock_USART_TDR = 0;
    mock_USART_CR1 = 0;
    mock_USART_ICR = 0;

    /* Clear driver's RX buffer (size from RX_BUFFER_SIZE_BYTES) */
    memset(g_rx_buffer_storage, 0, RX_BUFFER_SIZE_BYTES);
}

/* ============================================
 * TEST CASES
 * (same tests you had; they operate on driver via externs)
 * ============================================ */

void test_tx_sends_first_byte(void)
{
    printf("TEST: TX sends first byte... ");
    setup_test();

    char data[] = "A";
    g_p_tx_buffer = data;
    g_tx_length = 1;
    g_tx_index = 0;
    g_tx_state = UART_STATE_TX_BUSY;

    uart_process_tx();

    assert(mock_USART_TDR == 'A');
    assert(g_tx_index == 1);
    assert(g_tx_state == UART_STATE_TX_BUSY);

    printf("PASS\n");
}

void test_tx_sends_multiple_bytes(void)
{
    printf("TEST: TX sends multiple bytes... ");
    setup_test();

    char data[] = "HELLO";
    g_p_tx_buffer = data;
    g_tx_length = 5;
    g_tx_state = UART_STATE_TX_BUSY;

    for (size_t i = 0; i < 5; i++)
    {
        g_tx_index = i;
        uart_process_tx();
        assert(mock_USART_TDR == (uint32_t)data[i]);
        assert(g_tx_index == i + 1);
    }

    printf("PASS\n");
}

void test_tx_completes_transmission(void)
{
    printf("TEST: TX completes transmission... ");
    setup_test();

    char data[] = "X";
    g_p_tx_buffer = data;
    g_tx_length = 1;
    g_tx_index = 1;  /* Past end */
    g_tx_state = UART_STATE_TX_BUSY;
    mock_USART_CR1 = (1u << USART_CR1_TXEIE_BIT);

    uart_process_tx();

    assert(g_tx_state == UART_STATE_IDLE);
    assert(g_p_tx_buffer == NULL);
    assert((mock_USART_CR1 & (1u << USART_CR1_TXEIE_BIT)) == 0);

    printf("PASS\n");
}

void test_rx_receives_single_byte(void)
{
    printf("TEST: RX receives single byte... ");
    setup_test();

    g_rx_state = UART_STATE_RX_BUSY;
    mock_USART_RDR = 'X';
    mock_USART_ISR = 0;

    uart_error_t err = uart_process_rx();

    assert(err == UART_ERROR_NONE);
    assert(g_p_rx_buffer[0] == 'X');
    assert(g_rx_index == 1);
    assert(g_rx_state == UART_STATE_RX_BUSY);

    printf("PASS\n");
}

void test_rx_detects_newline(void)
{
    printf("TEST: RX detects newline... ");
    setup_test();

    g_rx_state = UART_STATE_RX_BUSY;
    mock_USART_RDR = '\n';
    mock_USART_ISR = 0;
    mock_USART_CR1 = (1u << USART_CR1_RXNEIE_BIT);

    uart_error_t err = uart_process_rx();

    assert(err == UART_ERROR_NONE);
    assert(g_p_rx_buffer[0] == '\n');
    assert(g_p_rx_buffer[1] == '\0');
    assert(g_rx_state == UART_STATE_IDLE);
    assert((mock_USART_CR1 & (1u << USART_CR1_RXNEIE_BIT)) == 0);

    printf("PASS\n");
}

void test_rx_detects_carriage_return(void)
{
    printf("TEST: RX detects carriage return... ");
    setup_test();

    g_rx_state = UART_STATE_RX_BUSY;
    mock_USART_RDR = '\r';
    mock_USART_ISR = 0;
    mock_USART_CR1 = (1u << USART_CR1_RXNEIE_BIT);

    uart_error_t err = uart_process_rx();

    assert(err == UART_ERROR_NONE);
    assert(g_p_rx_buffer[0] == '\r');
    assert(g_p_rx_buffer[1] == '\0');
    assert(g_rx_state == UART_STATE_IDLE);

    printf("PASS\n");
}

void test_rx_buffer_overflow_protection(void)
{
    printf("TEST: RX buffer overflow protection... ");
    setup_test();

    g_rx_state = UART_STATE_RX_BUSY;
    g_rx_index = RX_BUFFER_SIZE_BYTES - 1;
    mock_USART_RDR = 'Z';
    mock_USART_ISR = 0;
    mock_USART_CR1 = (1u << USART_CR1_RXNEIE_BIT);

    uart_error_t err = uart_process_rx();

    assert(err == UART_ERROR_NONE);
    assert(g_rx_state == UART_STATE_IDLE);
    assert((mock_USART_CR1 & (1u << USART_CR1_RXNEIE_BIT)) == 0);

    printf("PASS\n");
}

void test_has_error_detects_overrun(void)
{
    printf("TEST: Has error detects overrun... ");
    setup_test();

    mock_USART_ISR = (1u << USART_ISR_ORE_BIT);

    bool_t has_err = uart_has_error();

    assert(has_err == TRUE);

    printf("PASS\n");
}

void test_has_error_detects_framing(void)
{
    printf("TEST: Has error detects framing error... ");
    setup_test();

    mock_USART_ISR = (1u << USART_ISR_FE_BIT);

    bool_t has_err = uart_has_error();

    assert(has_err == TRUE);

    printf("PASS\n");
}

void test_handle_error_clears_overrun(void)
{
    printf("TEST: Handle error clears overrun flag... ");
    setup_test();

    g_rx_state = UART_STATE_RX_BUSY;
    mock_USART_ISR = (1u << USART_ISR_ORE_BIT);
    mock_USART_CR1 = (1u << USART_CR1_RXNEIE_BIT);

    uart_error_t err = uart_handle_rx_error();

    assert(err == UART_ERROR_OVERRUN);
    assert(g_rx_state == UART_STATE_ERROR);
    assert((mock_USART_CR1 & (1u << USART_CR1_RXNEIE_BIT)) == 0);
    assert((mock_USART_ICR & (1u << USART_ISR_ORE_BIT)) != 0);

    printf("PASS\n");
}

void test_rx_full_message(void)
{
    printf("TEST: RX receives complete message... ");
    setup_test();

    char message[] = "TEST\n";
    g_rx_state = UART_STATE_RX_BUSY;
    mock_USART_CR1 = (1u << USART_CR1_RXNEIE_BIT);

    for (size_t i = 0; i < strlen(message); i++)
    {
        mock_USART_RDR = message[i];
        mock_USART_ISR = 0;
        uart_process_rx();
    }

    assert(g_rx_state == UART_STATE_IDLE);
    assert(strcmp(g_p_rx_buffer, "TEST\n") == 0);
    assert((mock_USART_CR1 & (1u << USART_CR1_RXNEIE_BIT)) == 0);

    printf("PASS\n");
}

/* ============================================
 * TEST RUNNER
 * ============================================ */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  UART Driver Unit Tests (host)\n");
    printf("========================================\n\n");

    /* TX tests */
    test_tx_sends_first_byte();
    test_tx_sends_multiple_bytes();
    test_tx_completes_transmission();

    /* RX tests */
    test_rx_receives_single_byte();
    test_rx_detects_newline();
    test_rx_detects_carriage_return();
    test_rx_buffer_overflow_protection();
    test_rx_full_message();

    /* Error handling tests */
    test_has_error_detects_overrun();
    test_has_error_detects_framing();
    test_handle_error_clears_overrun();

    printf("\n========================================\n");
    printf("  All Tests Passed! ✓\n");
    printf("========================================\n\n");

    return 0;
}
