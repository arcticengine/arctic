#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <conio.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "../piCnsLogger.h"

namespace piLibs {

piCnsLogger::piCnsLogger():piLogger()
{
}
piCnsLogger::~piCnsLogger()
{
}

typedef struct
{
    bool    mIsNewConsole;     
    HANDLE  mCns;
}MyData;


static void printF_con( MyData *me, const wchar_t *format, ... )
{
    wchar_t    tmpstr[1024];
    unsigned long res;

    va_list arglist;
    va_start( arglist, format );
    int len = vswprintf_s( tmpstr, 1023, format, arglist );
	va_end( arglist );

    WriteConsole( me->mCns, tmpstr, len, &res, 0  );
}

static void printL_con( MyData *me, const wchar_t *str )
{
    unsigned long res;
    WriteConsole( me->mCns, str, (int)wcslen(str), &res, 0  );
}

static void printHeader( MyData *me, const piLogStartInfo *info )
{
    printL_con( me, L"===================================================\n" );
    printF_con( me, L"date      : %s\n", info->date );
    printF_con( me, L"\n" );
    printF_con( me, L"Memory    : %d / %d Megabytes \n", info->free_memory_MB, info->total_memory_MB );
    printF_con( me, L"CPU       : %s\n", info->processor );
    printF_con( me, L"Units     : %d\n", info->number_cpus );
    printF_con( me, L"Speed     : %d Mhz\n", info->mhz );
    printF_con( me, L"OS        : %s\n", info->os );
    printF_con( me, L"GPU       : %s, %s\n", info->gpuVendor, info->gpuModel );
    printF_con( me, L"VRam      : %d Megabytes \n", info->total_videomemory_MB );
    printF_con( me, L"Screen    : %d x %d\n", info->mScreenResolution[0], info->mScreenResolution[1] );
    printF_con( me, L"Monitors  : %d\n", info->mNumMonitors);
    printF_con( me, L"Multitouch: %d\n", info->mIntegratedMultitouch);
    printL_con( me, L"===================================================\n" );
}

bool piCnsLogger::Init( const wchar_t *path, const piLogStartInfo *info )
{
    MyData *me = (MyData*)malloc( sizeof(MyData) );
    if( !me )
        return false;

    mData = (void*)me;

    me->mIsNewConsole = (AllocConsole()!=0);
    SetConsoleTitle( L"console" );
    me->mCns = GetStdHandle( STD_OUTPUT_HANDLE );

    //HWND h = FindWindow(NULL, L"console"); SetWindowPos(h, HWND_NOTOPMOST, 0, 0, 100, 100, SWP_NOSIZE|SWP_NOOWNERZORDER);

	printHeader( me, info );

	return true;
}

void piCnsLogger::End( void )
{
    MyData *me = (MyData*)mData;
    if( me->mIsNewConsole ) FreeConsole();

    free( mData );
}

void piCnsLogger::Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str )
{
	int strLen = (int)wcslen(str);
	int maxLen = strLen; if (maxLen < 256) maxLen = 256;
	int bufLen = maxLen + 1;
	wchar_t *aux = (wchar_t*)_malloca(bufLen*sizeof(wchar_t));
	if( !aux ) return;

    unsigned long res;

    MyData *me = (MyData*)mData;

    switch( type )
    {
        case 1:
		{
            SetConsoleTextAttribute(me->mCns, FOREGROUND_RED | FOREGROUND_INTENSITY);
			int len = swprintf(aux, bufLen, L"[%d]  %s::%s (%d) :", threadId, file, func, line);
			WriteConsole(me->mCns, aux, len, &res, 0);
			SetConsoleTextAttribute(me->mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			WriteConsole(me->mCns, str, strLen, &res, 0);
			WriteConsole(me->mCns, L"\n", 1, &res, 0);
			break;
		}

		case 2:
		{
            SetConsoleTextAttribute(me->mCns, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			int len = swprintf(aux, bufLen, L"[%d]  %s::%s (%d) :", threadId, file, func, line);
			WriteConsole(me->mCns, aux, len, &res, 0);
			SetConsoleTextAttribute(me->mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			WriteConsole(me->mCns, str, strLen, &res, 0);
			WriteConsole(me->mCns, L"\n", 1, &res, 0);
			break;
		}

		case 3:
		{
            SetConsoleTextAttribute(me->mCns, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			int len = swprintf(aux, bufLen, L"[%d]  %s::%s (%d) :", threadId, file, func, line);
			WriteConsole(me->mCns, aux, len, &res, 0);
			SetConsoleTextAttribute(me->mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			WriteConsole(me->mCns, str, strLen, &res, 0);
			WriteConsole(me->mCns, L"\n", 1, &res, 0);
			break;
		}

		case 4:
		{
            SetConsoleTextAttribute(me->mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			WriteConsole(me->mCns, str, strLen, &res, 0);
			WriteConsole(me->mCns, L"\n", 1, &res, 0);
			break;
		}

		case 5:
		{
            SetConsoleTextAttribute(me->mCns, FOREGROUND_BLUE | FOREGROUND_GREEN);
			WriteConsole(me->mCns, str, strLen, &res, 0);
			WriteConsole(me->mCns, L"\n", 1, &res, 0);
			break;
		}
    }

	_freea( aux );
}


}