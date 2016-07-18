#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include "../mutex.h" 

int MUTEX_Init( MUTEX *mutex )
{
    mutex->data = (void*)malloc( sizeof(pthread_mutex_t) );
    if( !mutex->data )
    	return( 0 );

    if( pthread_mutex_init( (pthread_mutex_t*)mutex->data, 0 )!=0 )
    	return( 0 );

    return( 1 );
}

void MUTEX_Free( MUTEX *mutex )
{
    pthread_mutex_destroy( (pthread_mutex_t*)mutex->data );

    free( mutex->data );
}

void MUTEX_Lock( MUTEX *mutex )
{
    pthread_mutex_lock( (pthread_mutex_t*)mutex->data );
}

void MUTEX_UnLock( MUTEX *mutex )
{
    pthread_mutex_unlock( (pthread_mutex_t*)mutex->data );
}
