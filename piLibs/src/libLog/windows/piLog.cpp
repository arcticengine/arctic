#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../libSystem/piTypes.h"
#include "../../libSystem/piSystemInfo.h"
#include "../../libSystem/piThread.h"
#include "../../libSystem/piStr.h"

#include "../piLog.h"
#include "../piTxtLogger.h"
#include "../piCnsLogger.h"

namespace piLibs {

static void piLogStartInfo_Get( piLogStartInfo *info )
{
    memset( info, 0, sizeof(piLogStartInfo) );

	uint64 fm, tm, vm;
    piSystemInfo_getFreeRAM( &fm, &tm );
    vm = piSystemInfo_getVideoMemory();

    info->number_cpus = piSystemInfo_getCPUs();
    info->free_memory_MB = (unsigned int)(fm >> 20L);
	info->total_memory_MB = (unsigned int)(tm >> 20L);
    piSystemInfo_getTime( info->date, 511 );
    piSystemInfo_getProcessor( info->processor, 511, &info->mhz );
    piSystemInfo_getOS( info->os, 511 );
    piSystemInfo_getGfxCardIdentification( info->gpuVendor, 64, info->gpuModel, 512 );
    info->total_videomemory_MB = (unsigned int)(vm >> 20L);
    piSystemInfo_getScreenResolution( info->mScreenResolution );
	info->mIntegratedMultitouch = piSystemInfo_getIntegratedMultitouch();
    info->mNumMonitors = piSystemInfo_getNumMonitors();
}

//-------------------------------------------------------------

piLog::piLog()
{
}

piLog::~piLog()
{
}

bool piLog::Init( const wchar_t *path, int loggers )
{
	piLogStartInfo info;

    piLogStartInfo_Get( &info );

	mNumLoggers = 0;
	mMessageCounter = 0;

	for( int i=0; i<3; i++ )
	{
		if( (loggers & (1<<i))==0 ) continue;

		piLogger *lo = 0;
		if( i==0 ) lo = new piTxtLogger();
		if( i==1 ) lo = new piCnsLogger();
		//if( i==2 ) lo = new piXmlLogger();
		if( !lo )
			continue;

		if( !lo->Init(path,&info) )
			return false;
		
		mLoggers[mNumLoggers++] = lo;
	}

    return true;
}

void piLog::End( void )
{
	for( int i=0; i<mNumLoggers; i++ )
	{
		piLogger *lo = mLoggers[i];
		lo->End();
		delete lo;
	}
}

void piLog::Printf( const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *format, ... )
{
	va_list arglist;

    if( mNumLoggers<1 ) return;

    va_start( arglist, format );

	int maxLen = pivscwprintf( format, arglist ) + 1;
	wchar_t *tmpstr = (wchar_t*)_malloca( maxLen*sizeof(wchar_t) );
	if (!tmpstr) return;
	
	
	pivwsprintf(tmpstr, maxLen, format, arglist);

	va_end( arglist );

    for( int i=0; i<mNumLoggers; i++ )
    {
		piLogger *lo = mLoggers[i];
        lo->Printf( mMessageCounter, (int)(uint64)piThread_GetOSID(), file, func, line, type, tmpstr );
    }

    mMessageCounter++;

	_freea( tmpstr );
}


}
 
#if 0
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../piCriticalSection.h"
#include "../piShMem.h"
#include "../piStr.h"
#include "../piThread.h"

#include "logger.h"
#include "logger_txt.h"
#include "logger_cns.h"
#include "logger_xml.h"

typedef struct
{
    int                 counter;
    piCriticalSection 	mtx;
    LOGGER              logger[8];
    int                 numloggers;
}piILogSys;

void log_start( void );


bool piLog_Init( const wchar_t *filename, int loggers )
{
    if( logsys ) return true;

    piwsprintf( shmemareaname, 255, L"%s_%s", SHMEMNAME, filename );

    if( !piShMem_Init( &shmem, shmemareaname, sizeof(piILogSys) ) )
        return false;


    logsys = (piILogSys*)shmem.ptr;
    
    if( !logsys )
        return false;

    logsys->numloggers = 0;

    if( loggers & 1 )
    {
        if( !LOGGERTXT_New( logsys->logger+logsys->numloggers, filename ) ) 
            return false;
        logsys->numloggers++;
    }

    if( loggers & 2 )
    {
        if( !LOGGERCNS_New( logsys->logger+logsys->numloggers, filename ) ) 
            return( 0 );
        logsys->numloggers++;
    }

    if( loggers & 4 )
    {
        if( !LOGGERXML_New( logsys->logger+logsys->numloggers, filename ) ) 
            return false;
        logsys->numloggers++;
    }

    if( !piCriticalSection_Init( &logsys->mtx ) )
        return false;


    logsys->counter = 0;
    log_start();

    return true;
}

void piLog_End( void )
{ 
    if( !logsys )
        return;

    for( int i=0; i<logsys->numloggers; i++ )
        logsys->logger[i].end( logsys->logger+i );

    piCriticalSection_End( &logsys->mtx );
    piShMem_End( &shmem );
}


//-------------------------------



void piLog_Printf( const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *format, ... )
{
    wchar_t tmpstr[4096];

    if( !logsys )
    {
        if( !piShMem_Map( &shmem, shmemareaname, sizeof(piILogSys) ) )
        {
            return;
        }
        logsys = (piILogSys*)shmem.ptr;
    }

    int num = logsys->numloggers;

    if( num<1 ) return;

    piCriticalSection_Enter( &logsys->mtx );

    va_list arglist;
    va_start( arglist, format );

    pivwsprintf( tmpstr, 4095, format, arglist );

	va_end( arglist );


    for( int i=0; i<num; i++ )
    {
        LOGGER *log = logsys->logger + i;
        log->print( log, logsys->counter, (int)(uint64)piThread_GetOSID(), file, func, line, type, tmpstr );
    }

    logsys->counter++;

    piCriticalSection_Leave( &logsys->mtx );
}



//=================================================
#include "../piSysteminfo->h"

void log_start( void )
{
    LOG_START_INFO  info;
 
    memset( &info, 0, sizeof(LOG_START_INFO) );

    info->number_cpus = piSystemInfo_getCPUs();
    piSystemInfo_getFreeRAM( &info->free_memory, &info->total_memory );
    piSystemInfo_getTime( info->date, 511 );
    piSystemInfo_getProcessor( info->processor, 511, &info->mhz );
    piSystemInfo_getOS( info->os, 511 );
    piSystemInfo_getGfxCardIdentification( info->gpuVendor, 64, info->gpuModel, 512 );
    info->total_videomemory = piSystemInfo_getVideoMemory();



    piCriticalSection_Enter( &logsys->mtx );

    for( int i=0; i<logsys->numloggers; i++ )
    {
        LOGGER *log = logsys->logger + i;
        log->start( log, &info );
    }
    piCriticalSection_Leave( &logsys->mtx );
}

#endif

