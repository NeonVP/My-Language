#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "Colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#ifdef _DEBUG

#define my_assert( arg, message )                                                                            \
    do {                                                                                                     \
        if ( !( arg ) ) {                                                                                    \
            fprintf( stderr, COLOR_BRIGHT_RED "Error in function `%s` %s:%d: %s \n" COLOR_RESET, __func__,   \
                     __FILE__, __LINE__, message );                                                          \
            abort();                                                                                         \
        }                                                                                                    \
    } while ( 0 )

#define PRINT( str, ... )                                                                                    \
    do {                                                                                                     \
        time_t rawtime;                                                                                      \
        struct tm *timeinfo;                                                                                 \
        time( &rawtime );                                                                                    \
        timeinfo = localtime( &rawtime );                                                                    \
        char buffer[20];                                                                                     \
        strftime( buffer, sizeof( buffer ), "%H:%M:%S", timeinfo );                                          \
        fprintf( stderr, "[%s] [%s:%d] [%s] " COLOR_CYAN "[DEBUG]" COLOR_RESET " " str "\n" COLOR_RESET,     \
                 buffer, __FILE__, __LINE__, __func__, ##__VA_ARGS__ );                                      \
    } while ( 0 )

#define ON_DEBUG( ... ) __VA_ARGS__

#else // !_DEBUG

#define my_assert( arg, message ) ( (void)( arg ) )

#define PRINT( str, ... )

#define ON_DEBUG( ... )

#endif // _DEBUG

#define PRINT_ERROR( format, ... ) fprintf( stderr, COLOR_BRIGHT_RED format COLOR_RESET "\n", ##__VA_ARGS__ )

#endif // DEBUG_UTILS_H