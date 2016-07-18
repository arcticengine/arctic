#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../piFindFiles.h"
#include "../piFileName.h"
#include "../piStr.h"

namespace piLibs {

typedef struct
{
    int         mNum;
    int         mMax;
    piFileData *mFiles;
}iIqFindFiles;

static void add_files( iIqFindFiles *me, const wchar_t *mask, int recursive )
{
	HANDLE h;
    piFileData    *filedata;
    WIN32_FIND_DATA   fd;
    wchar_t *dir;
    wchar_t *nam;


    // first, recurse directories
    if( recursive )
    {
        dir = piFileName_ExtractPath( mask );
        nam = piFileName_ExtractName( mask );

        wchar_t str[1024];
        piwsprintf( str, 1023, L"%s*.*", dir );
	    h = FindFirstFile( str, &fd );
        if( h==INVALID_HANDLE_VALUE )
            return;

	    while( h != INVALID_HANDLE_VALUE )
        {
            if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0]!='.' )
            {
                piwsprintf( str, 1023, L"%s%s\\%s", dir, fd.cFileName, nam );
                add_files( me, str, recursive );
            }

            if( h != INVALID_HANDLE_VALUE )
            {
		        if( !FindNextFile(h, &fd) )
			        h = INVALID_HANDLE_VALUE;
            }
        }

        FindClose( h );

        free( dir );
        free( nam );
    }
    // ----------------------

	h = FindFirstFile( mask, &fd );

    if( h==INVALID_HANDLE_VALUE )
        return;

    dir = piFileName_ExtractPath( mask );

	while( h != INVALID_HANDLE_VALUE )
    {
		if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0]!='.' )
        {
            if( me->mNum >= me->mMax )
                h = INVALID_HANDLE_VALUE;
            else
            {
                filedata = me->mFiles  + me->mNum;
                me->mNum ++;
                piwsprintf( filedata->mName, 127, L"%s\\%s", dir, fd.cFileName );
            }

        }

        if( h != INVALID_HANDLE_VALUE )
        {
		    if( !FindNextFile(h, &fd) )
			    h = INVALID_HANDLE_VALUE;
        }
    }

    FindClose( h );

    free( dir );
}





piFindFiles piFindFiles_Init( void )
{
    iIqFindFiles *me = (iIqFindFiles*)malloc( sizeof(iIqFindFiles) );
    if( !me )
        return 0;

    me->mNum = 0;
    me->mMax = 8192;
    me->mFiles = (piFileData*)malloc( me->mMax*sizeof(piFileData) );
    if( !me->mFiles )
    {
        free( me );
        return( 0 );
    }

    return (piFindFiles)me;
}


void piFindFiles_Start( piFindFiles vme, const wchar_t *mask, int recursive )
{
    iIqFindFiles *me = (iIqFindFiles*)vme;
    if( !me ) return;

    me->mNum = 0;
    add_files( me, mask, recursive );
}


void piFindFiles_End( piFindFiles vme )
{   
    iIqFindFiles *me = (iIqFindFiles*)vme;
    free( me->mFiles );
}


int piFindFiles_GetLength( piFindFiles vme )
{
    iIqFindFiles *me = (iIqFindFiles*)vme;
    return me->mNum;
}

const piFileData *piFindFiles_Get( piFindFiles vme, int i )
{
    iIqFindFiles *me = (iIqFindFiles*)vme;
    return me->mFiles + i;
}

}