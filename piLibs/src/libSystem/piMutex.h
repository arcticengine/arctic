#pragma once

namespace piLibs {


typedef void * piMutex;

piMutex piMutex_Init( void );
void piMutex_Free( piMutex mutex );
void piMutex_Lock( piMutex mutex );
void piMutex_UnLock( piMutex mutex );

/*
#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <winbase.h>

#if _MSC_VER<1300  
//#define ATOMIC_Inc32(a) (_InterlockedIncrement((void*)(a)))
#define ATOMIC_Inc32(a) (InterlockedIncrement((long*)(a)))
#else
#define ATOMIC_Inc32(a) (InterlockedIncrementAcquire((long*)(a)))
#endif

#endif

#ifdef ALTIX
#include <ia64intrin.h>
#define ATOMIC_Inc32(a) (_InterlockedIncrement((a)))
#endif

#ifdef MSDOS
#define ATOMIC_Inc32(a) ((++(*((int*)(a)) )))
#endif


#ifdef IBM
#include <asm-ppc64/atomic.h>
#define ATOMIC_Inc32(a) (atomic_inc_return((atomic_t*)(a)))
#endif

#ifdef ONYX4
#include <sys/atomic_ops.h>
//#define ATOMIC_Inc32(a) (atomicAddInt((a),1)
#define ATOMIC_Inc32(a) (__add_and_fetch((a),1))
#endif


//-----------------------
#ifdef LINUX
#ifdef PC

 #include <asm/atomic.h>

 #ifdef atomic_inc_return
   #define ATOMIC_Inc32(a) (atomic_inc_return((atomic_t*)(a)))
 #else
   static __inline__ int atomic_inc_return(atomic_t *v)
   {
	    int ret;
        __asm__ __volatile__(
            LOCK "movl %0,%%eax;incl %0"
      //        LOCK "incl %0"
            :"=m" (v->counter), "=a"(ret)
            :"m" (v->counter));

        return ret;
   }
 #endif

#endif
#endif
*/

} // namespace piLibs