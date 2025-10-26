/** @file main.c
 *
 * @brief Main application entry point for JSON-UART bridge.
 *
 * Initializes JSON processing subsystem and continuously processes
 * embedded JSON data for UART transmission.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include "jsonprocess.h"

/*!
 * @brief Main application entry point.
 *
 * Initializes JSON processing and enters infinite processing loop.
 *
 * @return Never returns (embedded system main loop).
 */
int32_t
main (void)
{
    json_process_init();
    
    for (;;)
    {
        (void)json_process();
    }
    
    /* Never reached */
    return 0;
}

/*** end of file ***/
