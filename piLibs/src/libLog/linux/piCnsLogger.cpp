
#ifdef WINDOWS
#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <conio.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "../piStr.h"
#include "../piLog.h"
#include "logger.h"

typedef struct
{
    #ifdef WINDOWS
    HANDLE hh;
    int    thereWasNotConsole;
    #else
    int    dummy;
    #endif
}LOGGERCNS_DATA;

int iLOGGERCNS_Init( LOGGER *vme, const wchar_t *title )
{
    LOGGERCNS_DATA *me = (LOGGERCNS_DATA *)(vme->data);

    #ifdef WINDOWS
    me->thereWasNotConsole = AllocConsole();
    SetConsoleTitle( title );
    me->hh = GetStdHandle( STD_OUTPUT_HANDLE );
    #endif

    return( 1 );
}

void iLOGGERCNS_End( LOGGER *vme )
{
    LOGGERCNS_DATA *me = (LOGGERCNS_DATA *)(vme->data);

    
    #ifdef WINDOWS
    //_getch();
    if( me->thereWasNotConsole ) FreeConsole();
    #endif
    free( me );
}

void iLOGGERCNS_Print( LOGGER *vme, int id, int th, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str )
{
    LOGGERCNS_DATA *me = (LOGGERCNS_DATA *)(vme->data);

    #ifdef WINDOWS
    wchar_t      aux[512];
    unsigned long res;

    switch( type )
        {
        case LT_ERROR:
            SetConsoleTextAttribute( me->hh, FOREGROUND_RED|FOREGROUND_INTENSITY );
            piwsprintf( aux, 511, L"[%d]  %s::%s (%d) :", th, file, func, line );
            WriteConsole( me->hh, aux, piwstrlen(aux), &res, 0  );
            SetConsoleTextAttribute( me->hh, FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN );
            WriteConsole( me->hh, str, piwstrlen(str), &res, 0  );
            WriteConsole( me->hh, L"\n", 1, &res, 0  );
            break;

        case LT_WARNING:
            SetConsoleTextAttribute( me->hh, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY );
            piwsprintf( aux, 511, L"[%d]  %s::%s (%d) :", th, file, func, line );
            WriteConsole( me->hh, aux, piwstrlen(aux), &res, 0  );
            SetConsoleTextAttribute( me->hh, FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN );
            WriteConsole( me->hh, str, piwstrlen(str), &res, 0  );
            WriteConsole( me->hh, L"\n", 1, &res, 0  );
            break;

        case LT_ASSUME:
            SetConsoleTextAttribute( me->hh, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY );
            piwsprintf( aux, 511, L"[%d]  %s::%s (%d) :", th, file, func, line );
            WriteConsole( me->hh, aux, piwstrlen(aux), &res, 0  );
            SetConsoleTextAttribute( me->hh, FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN );
            WriteConsole( me->hh, str, piwstrlen(str), &res, 0  );
            WriteConsole( me->hh, L"\n", 1, &res, 0  );
            break;

        case LT_MESSAGE:
            SetConsoleTextAttribute( me->hh, FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN );
            WriteConsole( me->hh, str, piwstrlen(str), &res, 0  );
            WriteConsole( me->hh, L"\n", 1, &res, 0  );
            break;

        case LT_DEBUG:
            SetConsoleTextAttribute( me->hh, FOREGROUND_BLUE|FOREGROUND_GREEN );
            WriteConsole( me->hh, str, piwstrlen(str), &res, 0  );
            WriteConsole( me->hh, L"\n", 1, &res, 0  );
            break;
        }

    #else
    switch( type )
        {
        case LT_ERROR:
            printf( "[%d]  %s::%s (%d) :", th, file, func, line );
            printf( str  );
            printf( "\n" );
            break;

        case LT_WARNING:
            printf( "[%d]  %s::%s (%d) :", th, file, func, line );
            printf( str  );
            printf( "\n" );
            break;

        case LT_ASSUME:
            printf( "[%d]  %s::%s (%d) :", th, file, func, line );
            printf( str  );
            printf( "\n" );
            break;

        case LT_MESSAGE:
            printf( str  );
            printf( "\n" );
            break;

        case LT_DEBUG:
            printf( str  );
            printf( "\n" );
            break;
        }
    fflush( stdout );
    #endif

}

#ifdef WINDOWS
void print_con( LOGGERCNS_DATA *me, const wchar_t *format, ... )
{
    wchar_t    tmpstr[512];
    unsigned long res;

    va_list arglist;
    va_start( arglist, format );
    pivwsprintf( tmpstr, 511, format, arglist );
	va_end( arglist );

    WriteConsole( me->hh, tmpstr, piwstrlen(tmpstr), &res, 0  );
}
#else

#include <stdarg.h>

void print_con(LOGGERCNS_DATA *me, const wchar_t *format, ... )
{
    va_list arglist;
    va_start( arglist, format );

    vprintf( format, arglist );

	va_end( arglist );
}
#endif

void ILOGGERCNS_Start( LOGGER *vme, const LOG_START_INFO *info )
{
    LOGGERCNS_DATA *me = (LOGGERCNS_DATA *)(vme->data);
    
    print_con( me, L"===================================================\n" );
    print_con( me, L"date  : %s\n", info->date );
    print_con( me, L"\n" );
#ifdef WINDOWS
    print_con( me, L"Memory: %I64d / %I64d Megabytes \n", info->free_memory>>20, info->total_memory>>20 );
#else
    print_con( me, L"Memory: %d / %d Megabytes \n", info->free_memory>>20L, info->total_memory>>20L );
#endif
    print_con( me, L"CPU   : %s\n", info->processor );
    print_con( me, L"Units : %d\n", info->number_cpus );
    print_con( me, L"Speed : %d Mhz\n", info->mhz );
    print_con( me, L"OS    : %s\n", info->os );
    print_con( me, L"GPU   : %s, %s\n", info->gpuVendor, info->gpuModel );
#ifdef WINDOWS
    print_con( me, L"VRam  : %I64d Megabytes\n", info->total_videomemory>>20 );
#else
    print_con( me, "VRam  : %d Megabytes\n", info->total_videomemory>>20L );
#endif



    print_con( me, L"===================================================\n" );
}


int LOGGERCNS_New( LOGGER *logger, const wchar_t *title )
{
    if( !logger )
        return( 0 );

    logger->data = (LOGGERCNS_DATA*)malloc( sizeof(LOGGERCNS_DATA) );
    if( !logger->data )
        return( 0 );

    memset( logger->data, 0, sizeof(LOGGERCNS_DATA) );

    if( !iLOGGERCNS_Init( logger, title ) )
        return( 0 );

    logger->end   = iLOGGERCNS_End;
    logger->print = iLOGGERCNS_Print;
    logger->start = ILOGGERCNS_Start;

    return( 1 );

}
