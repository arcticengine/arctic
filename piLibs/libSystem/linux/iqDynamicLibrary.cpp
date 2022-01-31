#include <dlfcn.h>
#include "../log/log.h"
long  DLL_Load( char *name )
{
    void *h;

    h = dlopen( name, RTLD_LAZY|RTLD_LOCAL );

    if( !h ) LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "DLL_Load(%s) : %s\n", name, dlerror() );

    
    return( (long)h );
}

void *DLL_GetFunction( long h, char *name )
{
    void *f;
    
    f = dlsym( (void*)h, name );
    
    if( !h ) LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "DLL_GetFunction(%s) : %s\n", name, dlerror()  );
    
    return( f );

}

void  DLL_Free( long h )
{
    if( h )
        dlclose( (void*)h );

}
