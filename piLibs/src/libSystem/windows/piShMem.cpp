#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <winbase.h>


#include "../piShMem.h"

namespace piLibs {

int piShMem_Init( piShMem *shmem, const wchar_t *name, long amount )
{
    HANDLE      h;
    void        *ptr;

    h = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, name );
    if( !h )
        {
        h = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size 
                 amount,                // buffer size  
                 name);                 // name of mapping object
        if( !h || h==INVALID_HANDLE_VALUE )
            return( 0 );
        }

    ptr = MapViewOfFile( h, FILE_MAP_ALL_ACCESS, 0, 0, amount );
    if( !ptr )
        return( 0 );

    shmem->data = h;
    shmem->ptr = ptr;

    return( 1 );
}


int piShMem_Map( piShMem *shmem, const wchar_t *name, long amount )
{
    HANDLE      h;
    void        *ptr;

    h = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, name );
    if( !h )
        return( 0 );

    ptr = MapViewOfFile( h, FILE_MAP_ALL_ACCESS, 0, 0, amount );
    if( !ptr )
        return( 0 );

    shmem->data = (void*)h;
    shmem->ptr = ptr;

    return( 1 );
}



void piShMem_End( piShMem *shmem )
{
    if( !shmem )
        return;

    if( shmem->ptr )
    {
        UnmapViewOfFile( shmem->ptr );
        shmem->ptr = 0;
    }

    HANDLE h = (HANDLE)shmem->data;
    if( h ) 
    {
        CloseHandle( h );
        shmem->data = 0;
    }
}

}