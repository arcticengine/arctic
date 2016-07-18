#pragma once

#include "../../libSystem/piTypes.h"

namespace piLibs {

#ifdef BIGENDIAN

#define MASK_RB    0xff00ff00
#define MASK_G     0x00ff0000
#define MASK_BLEND 0xfffefefe

#else

#define MASK_RB    0x00ff00ff
#define MASK_G     0x0000ff00
#define MASK_BLEND 0xfefefeff

#endif


uint32 piIColor_Make( int r, int g, int b )
{
    #ifdef BIGENDIAN
//			#ifdef IBM
//				*buffer++ = 0x00000000|(b<<16)|(g<<8)|r;
//			#else
        return( (b<<24)|(g<<16)|(r<<8)|0x00 );
//			#endif
    #else
        return( 0x00000000|(r<<16)|(g<<8)|b );
    #endif

}

uint32 piIColor_FastBlend( uint32 c1, uint32 c2 )
{
    c1 &= MASK_BLEND;
    c2 &= MASK_BLEND;

    return( (c1+c2)>>1 );
}

uint32 piIColor_Mix( uint32 c1, uint32 c2, int alpha )
{
    unsigned int    a, b, d, e;

    a = c1 & MASK_RB;
    b = c2 & MASK_RB;
    d = a + (((b-a)*alpha)>>8);
    d = d & MASK_RB;

    a = c1 & MASK_G;
    b = c2 & MASK_G;
    e = a + (((b-a)*alpha)>>8);
    e = e & MASK_G;

    return( d | e );
}

uint32 piIColor_Scale( uint32 c, int alpha )
{
    unsigned int    a, d, e;

    a = c & MASK_RB;
    d = (a*alpha)>>8;
    d = d & MASK_RB;

    a = c & MASK_G;
    e = (a*alpha)>>8;
    e = e & MASK_G;

    return( d | e );
}

void piIColor_Lerp( uint32 *c1, uint32 c2, int alpha )
{
    signed int    a, b, d, e;

    a = (*c1) & MASK_RB;
    b = c2 & MASK_RB;
    d = a + (((b-a)*alpha)>>8);
    d = d & MASK_RB;

    a = (*c1) & MASK_G;
    b = c2 & MASK_G;
    e = a + (((b-a)*alpha)>>8);
    e = e & MASK_G;

    *c1 = d | e;
}




uint32 piIColor_GetGray( int r, int g, int b )
{
	int	res;

	//res = 20224*r + 39937*g + 5374*b;
    res = 19595*r  + 38470*g + 7471*b;

	return( res>>16 );
}

uint32 piIColor_LerpBilin( uint32 a, uint32 b, uint32 c, uint32 d, int ax, int ay )
{
    const int     axy = (ax*ay)>>8;

    signed int aa, bb, cc, dd, rb, gg;

    aa = a & MASK_RB;
    bb = b & MASK_RB;
    cc = c & MASK_RB;
    dd = d & MASK_RB;

    rb = aa + (( (bb-aa)*ax + (cc-aa)*ay + (aa+dd-bb-cc)*axy ) >> 8 );
    rb &= MASK_RB;

    aa = a & MASK_G;
    bb = b & MASK_G;
    cc = c & MASK_G;
    dd = d & MASK_G;

    gg = aa + (( (bb-aa)*ax + (cc-aa)*ay + (aa+dd-bb-cc)*axy ) >> 8 );
    gg &= MASK_G;

    return( rb|gg );
}

} // namespace piLibs