#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../piMesh.h"
#include "../../libSystem/piStr.h"

namespace piLibs {


static int separa_palabras( char *str, char *ptrs[], char *del, int maxpalabras )
{

    if( str[0]==';' ) return( 0 );
    if( str[0]==0 )   return( 0 );
    if( str[0]==13 )  return( 0 );
    if( str[0]==10 )  return( 0 );

    int i = 0;
    char    *ctx = 0;
    char *p = pistrtok( str, del, &ctx );
    while( p && i<maxpalabras )
    {
        ptrs[i++] = p;
        p = pistrtok( 0, del, &ctx );
    }

    return( i );
}

static int readline( char *str, int max, FILE *fp )
{
    size_t l;

	if( !fgets(str,max,fp) )
        return( 0 );

    l = strlen(str)-1;
    while( str[l]==10 || str[l]==13 )
        str[l] = 0;

    return( 1 );
}


int piRenderMeshPly_Read( piMesh *me, const char *name, bool calcNormals )
{
    int     n, nv, nf;
    char    str[256];
	char	*ptrs[16];

    FILE *fp = fopen( name, "rt" );
    if( !fp )
        return( 0 );

    readline( str, 255, fp ); if( strcmp(str,"ply") ) { fclose(fp); return(0); }
	fgets(str,255,fp);
	fgets(str,255,fp);

	readline(str,255,fp); 
    n = separa_palabras(str, ptrs, " \t", 16 );
    if( strcmp(ptrs[0], "element") ) { fclose(fp); return(0); }
    if( strcmp(ptrs[1], "vertex") ) { fclose(fp); return(0); }
    nv = atoi( ptrs[2] );

    do
    {
	    readline(str,255,fp); 
        n = separa_palabras(str, ptrs, " \t", 16 );
    }while( strcmp(ptrs[0],"property")==0 );

    n = separa_palabras(str, ptrs, " \t", 16 );
    if( strcmp(ptrs[0], "element") ) { fclose(fp); return(0); }
    if( strcmp(ptrs[1], "face") ) { fclose(fp); return(0); }
    nf = atoi( ptrs[2] );

    do
    {
	    readline(str,255,fp); 
        n = separa_palabras(str, ptrs, " \t", 16 );
    }while( strcmp(ptrs[0],"property")==0 );

    if( strcmp(str,"end_header") ) { fclose(fp); return(0); }

    const piMeshVertexFormat vf = { 5*sizeof(float), 2, 0, {{3, piRMVEDT_Float, false},
                                                            {2, piRMVEDT_Float, false} } };
    if( !me->Init( 1, nv, &vf, piRMVEDT_Polys, 1, nf ) )
	{
		fclose( fp );
        return( 0 );
    }

    for( int i=0; i<nv; i++ )
    {
	    readline(str,255,fp);

        float *v  = (float*)me->GetVertexData( 0, i, 0 );
        float *nt = (float*)me->GetVertexData( 0, i, 1 );

		n = separa_palabras(str, ptrs, " \t", 16 );

		v[0] = (float)atof( ptrs[0] );
		v[1] = (float)atof( ptrs[1] );
		v[2] = (float)atof( ptrs[2] );

        nt[0] = (float)atof( ptrs[3] );
        nt[1] = (float)atof( ptrs[4] );
        nt[2] = (float)atof( ptrs[5] );
	}

    //-------------------

    for( int i=0; i<nf; i++ )
    {
	    readline(str,255,fp);

		n = separa_palabras(str, ptrs, " \t", 16 );

        me->mFaceData.mIndexArray[0].mBuffer[i].mNum = 3;
      	me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[0] = atoi( ptrs[1] );
      	me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[1] = atoi( ptrs[2] );
      	me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[2] = atoi( ptrs[3] );
	}

    fclose( fp );


    //-------------------
/*
    DMESH_CalcNormals( me, 0, 3*sizeof(float), 0,
                           1, 5*sizeof(float), 0, 
                           20.0f );
*/
    return( 1 );
}


}