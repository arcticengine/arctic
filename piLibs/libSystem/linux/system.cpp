#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "../types.h"

void SYSTEM_getFreeRAM( uint64 *avail, uint64 *total )
{
	long page_size = sysconf(_SC_PAGE_SIZE);
	*avail = page_size * sysconf(_SC_AVPHYS_PAGES);
	*total = page_size * sysconf(_SC_PHYS_PAGES);
}

int SYSTEM_getCPUs( void )
{
 return( sysconf( _SC_NPROCESSORS_CONF ) );
}

void SYSTEM_getOS( char *str, int length )
{
	strcpy( str,"Linux");
}

void SYSTEM_getProcessor( char *str, int *mhz )
{
    strcpy( str, "Unknown" );
	*mhz = 0;
}


void SYSTEM_getTime( char *str )
{
    const int str_max = 1000;
    struct timeval now;
    struct tm time;
    memset(&time,0,sizeof(time));

    gettimeofday(&now, 0);
    localtime_r(&now.tv_sec,&time);

    strftime(str, str_max, "%H:%M:%S of %a %d/%m/%y", &time);
}

