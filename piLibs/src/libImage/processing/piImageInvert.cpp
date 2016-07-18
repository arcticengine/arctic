#include "../piImage.h"

namespace piLibs {

static bool imageInvert_rgba_i( piImage *dst, const piImage *src, float amount )
{
    unsigned int i;
    unsigned int num = src->GetXRes()*src->GetYRes()*src->GetZRes();


    unsigned int iamount = (unsigned int)(amount*65536.0f);

    unsigned char *srcPtr = (unsigned char*)src->GetData();
    unsigned char *dstPtr = (unsigned char*)dst->GetData();
    for( i=0; i<num; i++ )
    {
        int r = srcPtr[0];
        int g = srcPtr[1];
        int b = srcPtr[2];
        int a = srcPtr[3];
        srcPtr += 4;

        dstPtr[0] = r + (((255-2*r)*iamount)>>16);
        dstPtr[1] = g + (((255-2*g)*iamount)>>16);
        dstPtr[2] = b + (((255-2*b)*iamount)>>16);
        dstPtr[3] = a;
        dstPtr += 4;
    }

    return true;
}


bool piImage_Invert( piImage *dst, const piImage *src, float amount )
{
    if( src->GetFormat()==piImage::FORMAT_I_RGBA )
    {
        return imageInvert_rgba_i( dst, src, amount );
    }

    return false;
}


}