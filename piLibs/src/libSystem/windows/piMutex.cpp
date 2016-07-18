#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <malloc.h>
#include <process.h>
#include "../piMutex.h" 

namespace piLibs {

piMutex piMutex_Init( void )
{
	HANDLE h = CreateMutex( NULL, false, NULL );

    return (piMutex)h;
}

void piMutex_Free( piMutex vme )
{
    CloseHandle( (HANDLE)vme );
}

void piMutex_Lock( piMutex vme )
{
	WaitForSingleObject( (HANDLE)vme, INFINITE );
}

void piMutex_UnLock( piMutex vme )
{
	ReleaseMutex( (HANDLE)vme );
}

}