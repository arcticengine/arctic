#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <winbase.h>

#include "../log/log.h"
#include "../th_sync.h"
#include "../memleaks.h"

typedef struct
{
    //HANDLE		mtx;


    HANDLE		mtx2;

    HANDLE		th;
	THREADSYNC	*thsync;
    //--- this for the user
    void	  *data;
    DOWORKCB  doworkcb;
}STHREAD;

typedef struct
{
    HANDLE hhh1;
    HANDLE hhhs[256];
}THREADSYNC_DATA;

static void create_name( char *name, long time )
{
	static unsigned int mutexCount = 0;
	name[ 0] = 'm';
	name[ 1] = 'u';

	name[ 2] = (char)( '0' + ((time/100000)%10) );
	name[ 3] = (char)( '0' + ((time/10000)%10) );
	name[ 4] = (char)( '0' + ((time/1000)%10) );
	name[ 5] = (char)( '0' + ((time/100)%10) );
	name[ 6] = (char)( '0' + ((time/10)%10) );
	name[ 7] = (char)( '0' + ((time/1)%10) );

	name[ 8] = (char)( '0' + ((mutexCount/10000)%10) );
	name[ 9] = (char)( '0' + ((mutexCount/1000)%10) );
	name[10] = (char)( '0' + ((mutexCount/100)%10) );
	name[11] = (char)( '0' + ((mutexCount/10)%10) );
	name[12] = (char)( '0' + ((mutexCount/1)%10) );
	name[13] = 'x';
	name[14] = 0;
	mutexCount++;	
}

static HANDLE create_semaphore( int num, int state, int time )
{
	char	name[32];
	/*
	SECURITY_ATTRIBUTES attr;
	attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	attr.lpSecurityDescriptor = 0;
    attr.bInheritHandle = true;
    */
	create_name( name, time ); 

	return( CreateSemaphore(NULL, state, num, name ) );
}








//---------------------------


static DWORD __stdcall th_iteration( void *vme )
{
    STHREAD         *me = (STHREAD*)vme;
    THREADSYNC      *syn = me->thsync;
    THREADSYNC_DATA *thsdata = (THREADSYNC_DATA*)syn->data;

//    LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "C [%d]: before loop 1", me->th );

    ReleaseSemaphore( me->mtx2, 1, NULL );

//    LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "C [%d]: waiting for job", me->th );

    //WaitForSingleObject( me->mtx2, INFINITE );



    for( ;; )
    	{
#if 0
    	// wait...
		WaitForSingleObject( me->mtx, INFINITE );

		int kk = me->doworkcb( me->data );

		ReleaseMutex( me->mtx2 );
#else



        WaitForSingleObject( thsdata->hhh1, INFINITE );

        if( syn->finished )
            break;

        //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "C [%d]: working...", me->th );

		int kk = me->doworkcb( me->data );

        //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "C [%d]: work done", me->th );

        ReleaseSemaphore( me->mtx2, 1, NULL );

        //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "C [%d]: waiting for job", me->th );

      

#endif

    	}

    ReleaseSemaphore( me->mtx2, 1, NULL );

    return( 0 );
}




int THREADSYNC_Init( THREADSYNC *me, int num )
{
    int             i;
	unsigned long	thid;
    STHREAD         *sth;
    long            time = timeGetTime();
    THREADSYNC_DATA *thsdata;

	me->finished = 0;
    me->numthreads = num;
    me->sthread = (void*)malloc( num*sizeof(STHREAD) );
    if( !me->sthread )
        return( 0 );

    if( num==1 )
        return( 1 );

    me->data = malloc( sizeof(THREADSYNC_DATA) );
    if( !me->data )
        return( 0 );

    thsdata = (THREADSYNC_DATA*)me->data;


    // create semaphore and mutexes
#if 1
    thsdata->hhh1 = create_semaphore( num, 0, time );
#endif

    sth = (STHREAD*)me->sthread;
    for( i=0; i<me->numthreads; i++ )
    	{
		sth->thsync = me;
#if 0
		//sth->mtx  = mutex_create( time );
		//sth->mtx2 = mutex_create( time );
		//mutex_lock( sth->mtx );
#else

        thsdata->hhhs[i] = create_semaphore( 1, 0, time );
        sth->mtx2 = thsdata->hhhs[i];

#endif

    
		sth->th = CreateThread( NULL, 0, th_iteration, sth, 0, &thid );
    	if( !sth->th )
    	    return( 0 );

		//SetThreadPriority( sth->th, THREAD_PRIORITY_HIGHEST );
		sth++;
    	}


	// WHY?????? Make sure we dont launch anything before threads start to work...
/*
	while( okey<me->numthreads ) 
	    {
	    Sleep(4);
	    };
*/

    // wait until all the threads are ready to start :)
    //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "Waiting the children to be ready..." );
    
    WaitForMultipleObjects( me->numthreads, thsdata->hhhs, true, INFINITE );

    //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "Ok, initialization done" );

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
    THREADSYNC_DATA *thsdata = (THREADSYNC_DATA*)me->data;

    if( me->numthreads>1 )
        {
	    me->finished = 1;

        // allow the threads to iterate one more time
#if 0
        for( i=0; i<me->numthreads; i++ )
    	    ReleaseMutex( sth[i].mtx );
#else
        ReleaseSemaphore( thsdata->hhh1, me->numthreads, NULL );
#endif


        // wait until all of them finish
#if 1
        for( i=0; i<me->numthreads; i++ )
    	    WaitForSingleObject( sth[i].mtx2, INFINITE );
    
#else
        WaitForMultipleObjects( me->numthreads, thsdata->hhhs, true, INFINITE );
#endif

        CloseHandle( thsdata->hhh1 );

        for( i=0; i<me->numthreads; i++ )
    	    {

    	    //mutex_destroy( sth[i].mtx );
    	    CloseHandle( sth[i].mtx2 );

		    CloseHandle( sth[i].th );
    	    }
	
        if( me->data )
            free( me->data );
        }

    if( me->sthread )
        free( me->sthread );

}


void THREADSYNC_Launch( THREADSYNC *me )
{
    //int     i;
    STHREAD *sth = (STHREAD*)me->sthread;
    THREADSYNC_DATA *thsdata = (THREADSYNC_DATA*)me->data;



    if( me->numthreads>1 )
        {

#if 0
        int i;

        for( i=0; i<me->numthreads; i++ )
    	    ReleaseMutex( sth[i].mtx );

        for( i=0; i<me->numthreads; i++ )
    	    WaitForSingleObject( sth[i].mtx2, INFINITE );
#else

        //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "Launch 1. Waking up children..." );

        ReleaseSemaphore( thsdata->hhh1, me->numthreads, NULL );

        //LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "Launch 2. Waiting..." );

        WaitForMultipleObjects( me->numthreads, thsdata->hhhs, true, INFINITE );
#endif






        }
    else
        {
        sth[0].doworkcb( sth[0].data );
        }



}


int THREADSYNC_GetRunOn( void )
{
    int cpu = 0;
    return( cpu );
}

int THREADSYNC_GetOSID( void )
{
    return( (int)GetCurrentThread() );
}
