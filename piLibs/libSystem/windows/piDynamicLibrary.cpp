#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../piDynamicLibrary.h"

namespace piLibs {

piDynamicLibrary piDynamicLibrary_Load( const wchar_t *name )
{
    HMODULE h;

	h = LoadLibrary( name );

	return( (piDynamicLibrary)h );
}

void *piDynamicLibrary_GetFunction( piDynamicLibrary h, const char *name )
{
    return GetProcAddress( (HMODULE)h, name );
}

void piDynamicLibrary_Unload( piDynamicLibrary h )
{
    if( h )
		FreeLibrary( (HMODULE)h );
}

}