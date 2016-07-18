#include <unistd.h>
#include <sys/time.h>

static struct timeval to;

int TIMER_Init( void )
{
	if( gettimeofday(&to, 0)!=0 )
		return( 0 );

    return( 1 );
}

double TIMER_GetTime( void )
{
    double long t;
    double ret;
    struct timeval now, res;

    gettimeofday(&now, 0);

	timersub( &now, &to, &res );

    t = (res.tv_sec*1000) + (res.tv_usec/1000);
    ret = .001*(double)t;

    return( ret );
}

void TIMER_End( void )
{
}

void TIMER_Sleep( long miliseconds )
{
	struct timeval tv;
	tv.tv_usec = (miliseconds % 1000) * 1000;
	tv.tv_sec = miliseconds / 1000;
	select(0, 0, 0, 0, &tv);
}
