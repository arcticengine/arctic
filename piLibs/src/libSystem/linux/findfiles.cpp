#include <stdio.h>


#include <malloc.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../array.h"
#include "../file.h"
#include "../findfiles.h"
#include "../log/log.h"

/*
int FINDFILE_Start( ARRAY *array, char *mask )
{
    DIR *h;
    FILEDATA    *filedata;
    struct dirent *dd;

    if( !ARRAY_Init( array, 1024, sizeof(FILEDATA) ) )
        return( 0 );

    //sprintf( mask, "%s/%s", dir, ext );
    
    h = opendir( dir );
    if( !h )
        return( 0 );

    char *dir = FILE_ExtractPath( mask );

    dd = readdir( h );
    while( dd )
        {
        if( fnmatch( ext, dd->d_name, FNM_NOESCAPE )==0 )
            {
            filedata = (FILEDATA*)ARRAY_Alloc( array, 1 );
            if( !filedata )
                dd = 0;

            sprintf( filedata->name, "%s/%s", dir, dd->d_name );
            }

        if( dd )
            dd = readdir( h );
        }

    closedir( h );

    return( 1 );
}
*/
void add_files( ARRAY *array, char *mask, int recursive )
{
    DIR *h;
    FILEDATA    *filedata;
    struct dirent *dd;
    char *dir;
    char *nam;
    char *ext;
    char str[1024];

    // first, recurse directories
    if( recursive )
        {
        dir = FILE_ExtractPath( mask );
        nam = FILE_ExtractName( mask );
        ext = FILE_GetExtension( mask );

	dir[ strlen(dir)-1 ] =0;
        sprintf( str, "%s", dir );

        h = opendir( str );
        if( !h )
            return;

        dd = readdir( h );
        while( dd )
	    {
            sprintf( str, "%s/%s", dir, dd->d_name );
            int h2 = open( str, O_DIRECTORY );
            if( h2==-1 ) 
		{
		}
            else 
                {
                close( h2 );
		if( dd->d_name[0]!='.' )
                    {
                    sprintf( str, "%s/%s/%s", dir, dd->d_name, nam );
                    add_files( array, str, recursive );
                    }
                }

            if( dd )
                dd = readdir( h );
	    }

        closedir( h );

        free( dir );
        free( nam );
        }
    // ----------------------

    dir = FILE_ExtractPath( mask );
    nam = FILE_ExtractName( mask );
    ext = FILE_GetExtension( mask );

    dir[ strlen(dir)-1 ] =0;

    h = opendir( dir );
    if( !h )
        return;


    dd = readdir( h );
    while( dd )
        {
        sprintf( str, "%s/%s", dir, dd->d_name );

	char ext2[64];
	sprintf( ext2, "*%s", ext );

        if( fnmatch( ext2, str, FNM_NOESCAPE )==0 )
            {
            filedata = (FILEDATA*)ARRAY_Alloc( array, 1 );
            if( !filedata )
                dd = 0;

            sprintf( filedata->name, "%s/%s", dir, dd->d_name );
            }

        if( dd )
            dd = readdir( h );
        }

    closedir( h );

    free( dir );
    free( nam );
}

int FINDFILE_Start( ARRAY *array, char *mask, int recursive )
{
    if( !ARRAY_Init( array, 16384, sizeof(FILEDATA) ) )
        return( 0 );

    add_files( array, mask, recursive );

    if( array->num==0 )
        return( 0 );

    return( 1 );
}
void FINDFILE_End ( ARRAY *array )
{   
    ARRAY_End( array );
}
