#include "image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "../../kernel/file.h"
#include "../../kernel/sstr.h"

static char *extractstring( char *str, FILE *fp )
{
    int i=0;
    unsigned char   byte;

    while( fread( &byte, 1, 1, fp ) )
        {
        if( byte==0x0a || byte==0x20 )
            break;
        str[i++] = byte;
        }
    str[i] = 0;

    return( str );
}



int PFM_Load( IMAGE *bmp, const char *name )
{
    FILE        *fp;
    char        str[256];
    int         xres, yres;
    float       expo;

    fp = FILE_Open( name, "rb" );
    if( !fp )
        return( 0 );

    if( strcmp( extractstring( str, fp ), "PF") )
        return( 0 );

    xres = atoi( extractstring( str, fp ) );
    yres = atoi( extractstring( str, fp ) );
    expo = (float)atof( extractstring( str, fp ) );


    if( !IMAGE_Make( bmp, xres, yres, 1, IMAGE_FORMAT_F_RGB ) )
        {
        fclose( fp );
        return( 0 );
        }

    fread( bmp->data, 3*sizeof(float), xres*yres, fp );

    fclose( fp );

    return( 1 );
}


int PFM_Save( IMAGE *bmp, const char *name )
{
    FILE        *fp;
    char        str[256];
    //int         xres, yres;
//    float       expo;

    if( bmp->format!=IMAGE_FORMAT_F_RGB )
        return( 0 );

    fp = FILE_Open( name, "wb" );
    if( !fp )
        return( 0 );

    
    ssprintf( str, 255, "PF%c%d%c%d%c%f%c", 0xa, bmp->xres, 0xa, bmp->yres, 0xa, -1.0f, 0xa );
    fwrite( str, 1, strlen(str), fp );

    fwrite( bmp->data, 3*sizeof(float), bmp->xres*bmp->yres, fp );

    fclose( fp );

    return( 1 );
}