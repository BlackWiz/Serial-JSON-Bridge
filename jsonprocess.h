/** @file jsonprocess.h
 *
 * @brief JSON parsing and processing interface for embedded UART communication.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#ifndef JSONPROCESS_H
#define JSONPROCESS_H

#include <stdint.h>

/* Public API functions */
void json_process_init(void);
int32_t json_process(void);

#endif /* JSONPROCESS_H */

/*** end of file ***/
