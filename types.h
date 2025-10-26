/** @file types.h
 *
 * @brief Common type definitions for Keil µVision compatibility.
 *
 * Provides boolean type and other common types that may not be
 * available in all C89/C90 compilers (like Keil ARMCC).
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Boolean type definition for C89/Keil compatibility */
#ifndef __cplusplus
    #if !defined(__bool_true_false_are_defined)
        /* C99 stdbool.h not available - define our own */
        typedef uint8_t bool_t;
        
        #ifndef FALSE
            #define FALSE  0u
        #endif
        
        #ifndef TRUE
            #define TRUE   1u
        #endif
        
        /* For code compatibility, map bool to bool_t */
        #define bool   bool_t
        #define false  FALSE
        #define true   TRUE
    #else
        /* C99 stdbool.h is available */
        #include <stdbool.h>
        typedef bool bool_t;
    #endif
#endif

#endif /* TYPES_H */

/*** end of file ***/
