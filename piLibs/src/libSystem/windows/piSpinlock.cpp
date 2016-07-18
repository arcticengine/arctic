//#define WINVER 0x0500
#define _WIN32_WINNT 0x0403 // otherwise InitializeCriticalSectionAndSpinCount is not defined


#include <windows.h>

#include "../piSpinlock.h"

namespace piLibs {

bool piSpinLock_Init(piSpinLock *me,unsigned int count)
{
    me->data = (void*)malloc( sizeof(CRITICAL_SECTION) );
    if( !me->data )
    	return false;

    #if _MSC_VER<1300
    return 0;
    #else
	InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION*)me->data,count);
    #endif

    return true;
}

void piSpinLock_Free(piSpinLock *me)
{
    free( me->data );
	DeleteCriticalSection((CRITICAL_SECTION*)me->data);
}


void piSpinLock_Enter(piSpinLock *me)
{
	EnterCriticalSection((CRITICAL_SECTION*)me->data);
}

void piSpinLock_Leave(piSpinLock *me)
{
	LeaveCriticalSection((CRITICAL_SECTION*)me->data);
}

}