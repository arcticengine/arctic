#include <stdio.h>
#include <string.h>
#include <malloc.h>

int pcx_load( char *nombre, unsigned char *buffer, unsigned char *paleta )
{
    FILE            *fp;
    unsigned char   *aux, *ptr;
    long            filesize;
    long            i;
    unsigned char   Len, Check;
    int             CntLines, CntUBYTEs;

    //---------------

    fp = fopen(nombre, "rb");
    if( !fp )
        {
        return( 0 );
        }
 
    fseek( fp, 0, 2 );
    filesize = ftell( fp );
    fseek( fp, 0 ,0 );

    aux = (unsigned char *)malloc( filesize );
    if( !aux )
        {
        fclose( fp );
        return( 0 );
        }
    fread( aux, 1, filesize, fp );
    fclose( fp );

    //---------------

    ptr  = aux;
    ptr += 128; // cabecera

    for( CntLines=0; CntLines<480; CntLines++ )
        {
        CntUBYTEs = 0;
        while( CntUBYTEs<640 )
            {
            Check = *ptr++;
            if( (Check & 192)==192 )
                {
                Len = Check&63;
                Check = *ptr++;
                CntUBYTEs += Len;
                for( i=0; i<Len; i++ )
                    *buffer++ = Check;
                }
            else
                {
                CntUBYTEs++;
                *buffer++ = Check;
                }
            }
        }

    ptr++;

    memcpy( paleta, ptr, 768 );

    free( aux );
    return( 1 );
}
