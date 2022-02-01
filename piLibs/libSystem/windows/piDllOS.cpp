#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../piDll.h"

namespace piLibs {

piDLL piDLL_Load( const wchar_t *name )
{
    HMODULE h;

	h = LoadLibrary( name );

	return( (piDLL)h );
}

void *piDLL_GetFunction( piDLL h, const wchar_t *name )
{
	char tmp[1024];
	int len = (int)wcslen( name );
	WideCharToMultiByte(CP_ACP,0,name,len,tmp,1023,0,0);
	tmp[len] = 0;


    return GetProcAddress( (HMODULE)h, tmp );
}

void piDLL_Free( piDLL h )
{
    if( h ) FreeLibrary( (HMODULE)h );
}

}