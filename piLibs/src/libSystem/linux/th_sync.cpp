#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <sys/errno.h>
#include <malloc.h>


#include "../th_sync.h"

#include "../log/log.h"

typedef struct
{
    unsigned int    id;
    pthread_mutex_t mtx;
    pthread_mutex_t mtx2;
    pthread_t    th;
    //--- this for the user
    void	    *data;
    DOWORKCB	    doworkcb;
    int             readytodie;     
}STHREAD;

static void *th_iteration( void *vme )
{
    STHREAD *me = (STHREAD*)vme;
    int     finished = 0;
    int cpu;
/*
    cpu = me->id % 4;

    if( pthread_setrunon_np( cpu ) != 0 )
        LOG_Printf( "Error in pthread_setrunon_np(%d)\n", cpu );
    else
    	LOG_Printf( "Thread %x (th %d) assigned to cpu %d\n", me->id, me->th, cpu );
*/

    // LOG_Printf( "Thread %x (th %d)\n", me->id, me->th );

    pthread_mutex_lock( &me->mtx2 );

    while( !finished )
    	{
    	// wait...
    	pthread_mutex_lock( &me->mtx );
    	
	if( !me->readytodie )
        finished = me->doworkcb( me->data );

    	pthread_mutex_unlock( &me->mtx2 );
    	}


    pthread_exit( 0 );

    return( 0 );
}

int THREADSYNC_Init( THREADSYNC *me, int num )
{
    int             i;
    pthread_attr_t  attr;
    STHREAD         *sth;
    

    me->numthreads = num;

    me->sthread = (void*)malloc( num*sizeof(STHREAD) );
    if( !me->sthread )
        return( 0 );

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    //pthread_attr_setscope( &attr, PTHREAD_SCOPE_BOUND_NP );
    pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );

	
    sth = (STHREAD*)me->sthread;
    for( i=0; i<me->numthreads; i++ )
    	{
    	sth->id = i;
	sth->readytodie = 0;
    	pthread_mutex_init( &sth->mtx, 0 );
    	pthread_mutex_lock( &sth->mtx );
    	pthread_mutex_init( &sth->mtx2, 0 );

    	if( pthread_create( &sth->th, &attr, th_iteration, (void*)sth ) )
    	    return( 0 );

    	// LOG_Printf( "Init thread %d\n", i );

    	sth++;
    	}

    pthread_attr_destroy( &attr );

    return( 1 );
}


void THREADSYNC_Set( THREADSYNC *me, int which, DOWORKCB func, void *data )
{
    STHREAD *sth = (STHREAD*)me->sthread;

    sth[which].data = data;
    sth[which].doworkcb = func;
}

void THREADSYNC_End( THREADSYNC *me )
{
    int		i;
    STHREAD *sth = (STHREAD*)me->sthread;

    for( i=0; i<me->numthreads; i++ )
        {
	sth[i].readytodie = 1;
    	pthread_mutex_unlock( &sth[i].mtx );
	}

    for( i=0; i<me->numthreads; i++ )
    	pthread_mutex_lock( &sth[i].mtx2 );

    for( i=0; i<me->numthreads; i++ )
    	{
    	pthread_mutex_destroy( &sth[i].mtx );
    	pthread_mutex_destroy( &sth[i].mtx2 );
    	}


    if( me->sthread )
        free( me->sthread );
}

void THREADSYNC_Launch( THREADSYNC *me )
{
    int     i;
    STHREAD *sth = (STHREAD*)me->sthread;

    for( i=0; i<me->numthreads; i++ )
    	pthread_mutex_unlock( &sth[i].mtx );

    for( i=0; i<me->numthreads; i++ )
    	pthread_mutex_lock( &sth[i].mtx2 );
}


int  THREADSYNC_GetRunOn( void )
{
    int cpu, res;
/*
    res = pthread_getrunon_np( &cpu );

    if( res==EPERM  ) return( -1 ); // caller is not _SCOPE_SYSTEM or _SCOPE_BOUND_NP
    if( res==EINVAL ) return( -2 ); // caller has no affinisy set

    return( cpu );
    */
    return( 0 );
}

int THREADSYNC_GetOSID( void )
{
    return( (unsigned int)pthread_self() );
}
