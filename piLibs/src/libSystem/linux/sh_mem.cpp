#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "../sh_mem.h"

static int string_to_key( char *str )
{
    int i, l, k;

    l = strlen(str);
    k = 1;
    for( i=0; i<l; i++ )
        {
        k += str[i]<<(i&31);
        }

    return( k );
}
/*
int SHMEM_CreateAndMap( SHMEM *shmem, char *name, long amount )
{
    void *ptr;
    int key = string_to_key( name );
    int h = shmget( key, amount,0);
    if( h<0 )
        {
         h = shmget( key, amount, IPC_CREAT|IPC_EXCL|0666 );
	if( h<0 )
		return( 0 );
        }

	ptr = shmat( h, (void *) 0, 0 );
	if (ptr == (void*) -1)
        return( 0 );

    shmem->data = (long)h;
    shmem->ptr = ptr;

    return( 1 );
}
*/
int SHMEM_CreateAndMap2( SHMEM *shmem, char *name, long amount )
{
    void  *ptr;
    int   h;
    int   key = string_to_key( name );

    h = shmget( key, amount, IPC_CREAT|IPC_EXCL|0666 );
    if( h<0 )
        {
         h = shmget( key, amount, 0 );
         if( h<0 )
            return( 0 );
        }

    ptr = shmat( h, (void *) 0, 0 );
    if (ptr == (void*) -1)
        return( 0 );

    shmem->data = (long)h;
    shmem->ptr = ptr;

    return( 1 );
}


int SHMEM_Map( SHMEM *shmem, char *name, long amount )
{
    void *ptr;
    int key = string_to_key( name );
    int h = shmget( key, amount, 0 );
    if( h<0 )
        return( 0 );

    ptr = shmat( h, (void *) 0, 0 );
    if (ptr == (void*) -1)
        return( 0 );

    shmem->data = (long)h;
    shmem->ptr = ptr;

    return( 1 );
}

void SHMEM_UnMapAndClose( SHMEM *shmem )
{
    shmdt( shmem->ptr );
}
