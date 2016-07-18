#include <math.h>
#include "../piImage.h"
#include "../../libSystem/piTypes.h"

namespace piLibs {

__inline static uint32 lerpBilin( uint32 c0, uint32 c1, uint32 c2, uint32 c3, uint32 fu, uint32 fv )
{
    const uint32 a0 = (c0>>24) & 0xff;
    const uint32 r0 = (c0>>16) & 0xff;
    const uint32 g0 = (c0>> 8) & 0xff;
    const uint32 b0 = (c0>> 0) & 0xff;
    const uint32 a1 = (c1>>24) & 0xff;
    const uint32 r1 = (c1>>16) & 0xff;
    const uint32 g1 = (c1>> 8) & 0xff;
    const uint32 b1 = (c1>> 0) & 0xff;
    const uint32 a2 = (c2>>24) & 0xff;
    const uint32 r2 = (c2>>16) & 0xff;
    const uint32 g2 = (c2>> 8) & 0xff;
    const uint32 b2 = (c2>> 0) & 0xff;
    const uint32 a3 = (c3>>24) & 0xff;
    const uint32 r3 = (c3>>16) & 0xff;
    const uint32 g3 = (c3>> 8) & 0xff;
    const uint32 b3 = (c3>> 0) & 0xff;

    const uint32 fuv = (fu*fv)>>16;

    const uint32 a = a0 + (( (a1-a0)*fu + (a2-a0)*fv + (a0+a3-a1-a2)*fuv ) >> 16 );
    const uint32 r = r0 + (( (r1-r0)*fu + (r2-r0)*fv + (r0+r3-r1-r2)*fuv ) >> 16 );
    const uint32 g = g0 + (( (g1-g0)*fu + (g2-g0)*fv + (g0+g3-g1-g2)*fuv ) >> 16 );
    const uint32 b = b0 + (( (b1-b0)*fu + (b2-b0)*fv + (b0+b3-b1-b2)*fuv ) >> 16 );

    return (a<<24)|(r<<16)|(g<<8)|b;
}

static bool imageRotoZoomOffset_rgba_i( piImage *dst, const piImage *src, float rotate, float scale, float ofx, float ofy )
{
    unsigned int i, j;
    const unsigned int xres = src->GetXRes();
    const unsigned int yres = src->GetYRes();


    if( scale>255.0f ) scale=255.0f;
    if( scale<0.001f ) scale=0.001f;

    unsigned int *srcPtr = (unsigned int*)src->GetData();
    unsigned int *dstPtr = (unsigned int*)dst->GetData();

    const unsigned int offx = (unsigned int)(65536.0f*(ofx+0.5f)*(float)xres);
    const unsigned int offy = (unsigned int)(65536.0f*(ofy+0.5f)*(float)yres);

    const unsigned int co = (unsigned int)(scale*65536.0f*cosf(rotate));
    const unsigned int si = (unsigned int)(scale*65536.0f*sinf(rotate));

    const unsigned int xres2 = xres / 2;
    const unsigned int yres2 = yres / 2;

    for( j=0; j<yres; j++ )
    for( i=0; i<xres; i++ )
    {
        const   signed int u = i - xres2;
        const   signed int v = j - yres2;

        const unsigned int nu = offx + u*co - v*si;
        const unsigned int nv = offy + u*si + v*co;

        const unsigned int iu = nu>>16;
        const unsigned int iv = nv>>16;

        const unsigned int fu = nu & 0x0000ffff;
        const unsigned int fv = nv & 0x0000ffff;

        unsigned int iu0 = iu + 0;
        unsigned int iv0 = iv + 0;
        unsigned int iu1 = iu + 1;
        unsigned int iv1 = iv + 0;
        unsigned int iu2 = iu + 0;
        unsigned int iv2 = iv + 1;
        unsigned int iu3 = iu + 1;
        unsigned int iv3 = iv + 1;

        iu0 %= xres;
        iv0 %= yres;
        iu1 %= xres;
        iv1 %= yres;
        iu2 %= xres;
        iv2 %= yres;
        iu3 %= xres;
        iv3 %= yres;

        const unsigned int off0 = xres*iv0 + iu0;
        const unsigned int off1 = xres*iv1 + iu1;
        const unsigned int off2 = xres*iv2 + iu2;
        const unsigned int off3 = xres*iv3 + iu3;

        const unsigned int c0 = srcPtr[off0];
        const unsigned int c1 = srcPtr[off1];
        const unsigned int c2 = srcPtr[off2];
        const unsigned int c3 = srcPtr[off3];

        *dstPtr++ = lerpBilin( c0, c1, c2, c3, fu, fv );
    }

    return true;
}


bool piImage_RotoZoomOffset( piImage *dst, const piImage *src, float rotate, float scale, float ofx, float ofy )
{
    if( src->GetType() != piImage::TYPE_2D )
        return false;

    if (src->GetFormat() == piImage::FORMAT_I_RGBA)
        return imageRotoZoomOffset_rgba_i( dst, src, rotate, scale, ofx, ofy );

    return false;
}

}