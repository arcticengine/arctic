#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "piTxtLogger.h"

namespace piLibs {

piTxtLogger::piTxtLogger():piLogger()
{
}
piTxtLogger::~piTxtLogger()
{
}


static void printHeader( FILE *fp, const piLogStartInfo *info )
{
    fwprintf( fp, L"===================================================\n" );
    fwprintf( fp, L"date  : %s\n", info->date );
    fwprintf( fp, L"\n" );
    fwprintf( fp, L"Memory: %d / %d Megabytes \n", info->free_memory_MB, info->total_memory_MB );
    fwprintf( fp, L"CPU   : %s\n", info->processor );
    fwprintf( fp, L"Units : %d\n", info->number_cpus );
    fwprintf( fp, L"Speed : %d Mhz\n", info->mhz );
    fwprintf( fp, L"OS    : %s\n", info->os );
    fwprintf( fp, L"GPU   : %s, %s\n", info->gpuVendor, info->gpuModel );
    fwprintf( fp, L"VRam  : %d Megabytes \n", info->total_videomemory_MB );
    fwprintf( fp, L"Screen: %d x %d\n", info->mScreenResolution[0], info->mScreenResolution[1] );
	fwprintf( fp, L"Multitouch Integrated: %d\n", info->mIntegratedMultitouch );
    fwprintf( fp, L"Monitors: %d\n", info->mNumMonitors);
    fwprintf( fp, L"===================================================\n");
	fflush( fp );
}

bool piTxtLogger::Init( const wchar_t *path, const piLogStartInfo *info )
{
    //path
    mFp = _wfopen( path, L"wt" );
    if( !mFp )
        return false;

	printHeader( mFp, info );

	return true;
}

void piTxtLogger::End( void )
{
	fclose( mFp );
}

void piTxtLogger::Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str )
{
    if( !mFp ) return;

    switch( type )
        {
        case 1:
            fwprintf( mFp, L"[%d]  %s::%s (%d) :", threadId, file, func, line );
            fwprintf( mFp, str  );
            fwprintf( mFp, L"\n" );
            break;

        case 2:
            fwprintf( mFp, L"[%d]  %s::%s (%d) :", threadId, file, func, line );
            fwprintf( mFp, str  );
            fwprintf( mFp, L"\n" );
            break;

        case 3:
            fwprintf( mFp, L"[%d]  %s::%s (%d) :", threadId, file, func, line );
            fwprintf( mFp, str  );
            fwprintf( mFp, L"\n" );
            break;

        case 4:
            fwprintf( mFp, str  );
            fwprintf( mFp, L"\n" );
            break;

        case 5:
            fwprintf( mFp, str  );
            fwprintf( mFp, L"\n" );
            break;
        }
 
    fflush( mFp );

}

} // namespace piLibs