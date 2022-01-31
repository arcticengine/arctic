#include <malloc.h>
#include <math.h>

#include "../piImage.h"

namespace piLibs {


int piImage_Normalmap( piImage *img, const piImage *src, float scale )
{
	if( src->GetFormat()==piImage::FORMAT_I_RGBA )
    {
        const int xres = src->GetXRes();
        const int yres = src->GetYRes();

        const unsigned char *srcPtr = (unsigned char*)src->GetData();
        unsigned char *dstPtr = (unsigned char*)img->GetData();

        for( int j=0; j<yres; j++ )
        for( int i=0; i<xres; i++ )
        {
            const int ia = (i-1+xres) % xres;
            const int ib = (i+1     ) % xres;
            const int ja = (j-1+yres) % yres;
            const int jb = (j+1     ) % yres;

            float nx = (float)srcPtr[ (xres*j  + ia)*4 + 3] - (float)srcPtr[ (xres*j  + ib)*4 + 3];
            float ny = (float)srcPtr[ (xres*ja + i )*4 + 3] - (float)srcPtr[ (xres*jb + i )*4 + 3];
            float nz = scale*255.0f/(float)xres;

            const float im = 1.0f/sqrtf( nx*nx + ny*ny + nz*nz );
            nx *= im;
            ny *= im;
            nz *= im;

            dstPtr[0] = (int)(128.0f + 127.0f*nx);
            dstPtr[1] = (int)(128.0f + 127.0f*ny);
            dstPtr[2] = (int)(128.0f + 127.0f*nz);
            dstPtr[3] = srcPtr[(xres*j+i)*4+3];
            
            dstPtr += 4;
        }
    }
    else if (src->GetFormat() == piImage::FORMAT_F_RGBA)
    {
        const int xres = src->GetXRes();
        const int yres = src->GetYRes();

        const float *srcPtr = (float*)src->GetData();
        float *dstPtr = (float*)img->GetData();

        for( int j=0; j<yres; j++ )
        for( int i=0; i<xres; i++ )
        {
            const int ia = (i-1+xres) % xres;
            const int ib = (i+1     ) % xres;
            const int ja = (j-1+yres) % yres;
            const int jb = (j+1     ) % yres;

            float nx = srcPtr[ (xres*j  + ia)*4 + 3] - srcPtr[ (xres*j  + ib)*4 + 3];
            float ny = scale;
            float nz = srcPtr[ (xres*ja + i )*4 + 3] - srcPtr[ (xres*jb + i )*4 + 3];

            const float im = 1.0f/sqrtf( nx*nx + ny*ny + nz*nz );
            nx *= im;
            ny *= im;
            nz *= im;

            dstPtr[0] = nx;
            dstPtr[1] = ny;
            dstPtr[2] = nz;
            dstPtr[3] = srcPtr[(xres*j+i)*4+3];
            
            dstPtr += 4;
        }
    }
    else if (src->GetFormat() == piImage::FORMAT_I_GREY)
    {
        const int xres = src->GetXRes();
        const int yres = src->GetYRes();

        const unsigned char *srcPtr = (unsigned char*)src->GetData();
        unsigned char *dstPtr = (unsigned char*)img->GetData();

        for( int j=0; j<yres; j++ )
        for( int i=0; i<xres; i++ )
        {
            const int ia = (i-1+xres) % xres;
            const int ib = (i+1     ) % xres;
            const int ja = (j-1+yres) % yres;
            const int jb = (j+1     ) % yres;

            float nx = (float)srcPtr[ (xres*j  + ia)] - (float)srcPtr[ (xres*j  + ib)];
            float ny = (float)srcPtr[ (xres*ja + i )] - (float)srcPtr[ (xres*jb + i )];
            float nz = scale*255.0f/(float)xres;

            const float im = 1.0f/sqrtf( nx*nx + ny*ny + nz*nz );
            nx *= im;
            ny *= im;
            nz *= im;

            dstPtr[0] = (int)(128.0f + 127.0f*nx);
            dstPtr[1] = (int)(128.0f + 127.0f*ny);
            dstPtr[2] = (int)(128.0f + 127.0f*nz);
            dstPtr[3] = srcPtr[(xres*j+i)];
            
            dstPtr += 4;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}
}