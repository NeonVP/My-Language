#ifndef ERRORS_H
#define ERRORS_H

#include <stdio.h>
#include <stdlib.h>
#include "Colors.h"

#ifdef _DEBUG
    #define my_assert(arg, message)                                                                       \
        if ( !( arg ) ) {                                                                                     \
            fprintf( stderr, COLOR_BRIGHT_RED "Error in function `%s` %s:%d: %s \n" COLOR_RESET,                 \
                    __func__, __FILE__, __LINE__, message );                                              \
            abort();                                                                                      \
        }

    #define PRINT(str, ...) fprintf( stderr, "%s:%d " DEBUG " " str "\n" COLOR_RESET, __FILE__, __LINE__, ##__VA_ARGS__ );

    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define my_assert(arg, message) ((void) (arg));

    #define PRINT(str, ...)

    #define PRINT_EXECUTING   
    #define PRINT_STATUS_OK   
    #define PRINT_STATUS_FAIL 

    #define ON_DEBUG(...)
#endif //_DEBUG

#define PRINT_ERROR( format, ... ) fprintf( stderr, COLOR_BRIGHT_RED format COLOR_RESET, ##__VA_ARGS__ );

#endif //ERRORS_H