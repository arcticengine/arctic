#include <windows.h>

#include "../piCriticalSection.h"

namespace piLibs {

bool piCriticalSection_Init(piCriticalSection *me)
{
    me->data = (void*)malloc( sizeof(CRITICAL_SECTION) );
    if( !me->data )
    	return false;

	InitializeCriticalSection((CRITICAL_SECTION*)me->data);

    return true;
}

void piCriticalSection_End(piCriticalSection *me)
{
	DeleteCriticalSection((CRITICAL_SECTION*)me->data);
    free( me->data );
}


void piCriticalSection_Enter(piCriticalSection *me)
{
	EnterCriticalSection((CRITICAL_SECTION*)me->data);
}

void piCriticalSection_Leave(piCriticalSection *me)
{
	LeaveCriticalSection((CRITICAL_SECTION*)me->data);
}

}