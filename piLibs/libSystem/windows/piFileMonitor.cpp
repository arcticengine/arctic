#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
//----------------------------------------------------------------------------

#include "../piFileMonitor.h"

namespace piLibs {

typedef struct
{
    wchar_t  pFileName[256];
    int      pCreationTime_Low;
    int      pCreationTime_High;
    int      pLastWriteTime_Low;
    int      pLastWriteTime_High;
}iFileMonitor;

piFileMonitor piFileMonitor_Init( const wchar_t *filename )
{
    HANDLE                     hFile;
    BY_HANDLE_FILE_INFORMATION fileInformation;

	iFileMonitor *file = (iFileMonitor*)malloc( sizeof(iFileMonitor) );
	if( !file )
		return 0;

    wcscpy( file->pFileName, filename );

    hFile = CreateFile( file->pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( !hFile )
    {
        free( file );
        return( 0 );
    }

    if( !GetFileInformationByHandle( hFile, &fileInformation ) )
    {
        CloseHandle( hFile );
        free( file );
        return( 0 );
    }

    CloseHandle( hFile );

    file->pCreationTime_Low  = fileInformation.ftCreationTime.dwLowDateTime;
    file->pCreationTime_High = fileInformation.ftCreationTime.dwHighDateTime;

    file->pLastWriteTime_Low  = fileInformation.ftLastWriteTime.dwLowDateTime;
    file->pLastWriteTime_High = fileInformation.ftLastWriteTime.dwHighDateTime;

    return file;

}

int piFileMonitor_Changed( piFileMonitor vme, int *res )
{
	iFileMonitor *file = (iFileMonitor*)vme;
    HANDLE                     hFile;
    BY_HANDLE_FILE_INFORMATION fileInformation;   

    res[0] = 0;

    hFile = CreateFile( file->pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( !hFile )
        return( 0 );

    if( !GetFileInformationByHandle( hFile, &fileInformation ) )
        return( 0 );

    CloseHandle( hFile );

    FILETIME ct, wt;
    ct.dwLowDateTime  = file->pCreationTime_Low;
    ct.dwHighDateTime = file->pCreationTime_High;

    wt.dwLowDateTime  = file->pLastWriteTime_Low;
    wt.dwHighDateTime = file->pLastWriteTime_High;

    const int compres1 = CompareFileTime( &fileInformation.ftCreationTime,  &ct  );
    const int compres2 = CompareFileTime( &fileInformation.ftLastWriteTime, &wt );

    if( compres1>0 || compres2>0 )
    {
        file->pCreationTime_Low  = fileInformation.ftCreationTime.dwLowDateTime;
        file->pCreationTime_High = fileInformation.ftCreationTime.dwHighDateTime;

        file->pLastWriteTime_Low  = fileInformation.ftLastWriteTime.dwLowDateTime;
        file->pLastWriteTime_High = fileInformation.ftLastWriteTime.dwHighDateTime;

        res[0] = 1;
    }

    return( 1 );
}

void piFileMonitor_Destroy( piFileMonitor vme )
{
	iFileMonitor *me = (iFileMonitor*)vme;

    free( me );
}

}