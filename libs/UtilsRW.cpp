#include <errno.h>
#include <assert.h>

#include "UtilsRW.h"
#include "DebugUtils.h"

int MakeDirectory( const char* path ) {    
    if ( mkdir( path, 0700 ) == -1 ) {
        if ( errno != EEXIST ) {
            PRINT_ERROR( "Directory `%s` was not created. \n", path );
            return -1;
        }
    }

    PRINT( "Directory `%s` was created.", path );
    return 0;
}

off_t DetermineTheFileSize( const char* filename ) {
    my_assert( filename, "Null pointer on `filename`" );

    struct stat file_stat;
    int check_stat = stat( filename, &file_stat );
    assert( check_stat == 0 );

    return file_stat.st_size;
}


char* ReadToBuffer( const char* filename ) {
    my_assert( filename, "Null pointer on `filename`" );

    off_t file_size = DetermineTheFileSize( filename );
    if ( file_size == 0 ) {
        PRINT_ERROR( "The file `%s` is empty!", filename );
    }

    char* buffer = ( char* ) calloc ( ( size_t ) ( file_size + 1 ), sizeof( *buffer ) );
    assert( buffer && "Memory allocation error for `buffer`" );

    FILE* file = fopen( filename, "r" );
    assert( file && "Error opening file" );

    size_t result_of_read = fread( buffer, sizeof( char ), ( size_t ) file_size, file );
    assert( result_of_read != 0 && "Fail read to buffer \n" );

    int result_of_fclose = fclose( file );
    if ( result_of_fclose ) {
        PRINT_ERROR( "Error closing file `%s` \n", filename );
    }

    return buffer;
}

