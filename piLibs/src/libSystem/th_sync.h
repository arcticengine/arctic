#ifndef _THREADSYNC_H_
#define _THREADSYNC_H_


#include "thread.h"

typedef struct
{
    int    numthreads;
    void   *sthread;
	int	   finished;	// internal... HIDE IT! (only windows!)
    void   *data;
}THREADSYNC;

int  THREADSYNC_Init( THREADSYNC *me, int num );
void THREADSYNC_Set( THREADSYNC *me, int which, DOWORKCB func, void *dat );
void THREADSYNC_End( THREADSYNC *me );
void THREADSYNC_Launch( THREADSYNC *me );

int  THREADSYNC_GetRunOn( void );
int  THREADSYNC_GetOSID( void );

#endif
