#ifndef _SIMDEMU_H_
#define _SIMDEMU_H_

#include "types.h"
#include <math.h>


typedef struct
{
    float x, y, z, w;
}simd4f;

typedef struct
{
    unsigned int x, y, z, w;
}simd4i;


typedef union
{
    simd4f  ssef;
    simd4i  ssei;
    float   f[4];
    unsigned int i[4];
}sseFloat4;


typedef union
{
    simd4i      ssei;
    simd4f      ssef;
    unsigned int i[4];
}sseInt4;

#ifdef WINDOWS
    #define SIMDALIGN16 __declspec(align(16))
#endif
#ifdef LINUX
    #define SIMDALIGN16 __attribute__ ((aligned(16))) 
#endif
#ifdef IRIX
    #define SIMDALIGN16
#endif


MAKEINLINE simd4f simdf_add( const simd4f &a, const simd4f &b )
{
	simd4f res;
	res.x = a.x + b.x;
	res.y = a.y + b.y;
	res.z = a.z + b.z;
    res.w = a.w + b.w;
	return res;
}

MAKEINLINE simd4f simdf_sub( const simd4f &a, const simd4f &b )
{
	simd4f res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
    res.w = a.w - b.w;
	return res;
}

MAKEINLINE simd4f simdf_mul( const simd4f &a, const simd4f &b )
{
	simd4f res;
	res.x = a.x * b.x;
	res.y = a.y * b.y;
	res.z = a.z * b.z;
    res.w = a.w * b.w;
	return res;
}

MAKEINLINE simd4f simdf_madd( const simd4f &a, const simd4f &b, const simd4f &c )
{
	simd4f res;
	res.x = a.x * b.x + c.x;
	res.y = a.y * b.y + c.y;
	res.z = a.z * b.z + c.z;
    res.w = a.w * b.w + c.w;
	return res;
}

MAKEINLINE simd4f simdf_set1( const float x )
{
	simd4f res;
	res.x = x;
	res.y = x;
	res.z = x;
        res.w = x;
	return res;
}

MAKEINLINE simd4f simdf_set4( const float x, const float y, const float z, const float w )
{
	simd4f res;
	res.x = w;
	res.y = z;
	res.z = y;
    res.w = x;
	return res;
}


MAKEINLINE simd4f simdf_setzero( void )
{
	simd4f res;
	res.x = 0.0f;
	res.y = 0.0f;
	res.z = 0.0f;
        res.w = 0.0f;
	return res;
}


MAKEINLINE simd4f simdf_rcp( const simd4f &a )
{
	simd4f res;
	res.x = 1.0f/a.x;
	res.y = 1.0f/a.y;
	res.z = 1.0f/a.z;
    res.w = 1.0f/a.w;
	return res;
}

MAKEINLINE simd4f simdf_rcpimp( const simd4f &a )
{
	simd4f res;
	res.x = 1.0f/a.x;
	res.y = 1.0f/a.y;
	res.z = 1.0f/a.z;
    res.w = 1.0f/a.w;
	return res;
}

MAKEINLINE simd4f simdf_rsqrt( const simd4f &a )
{
	simd4f res;
	res.x = 1.0f/sqrtf(a.x);
	res.y = 1.0f/sqrtf(a.y);
	res.z = 1.0f/sqrtf(a.z);
    res.w = 1.0f/sqrtf(a.w);
	return res;
}

MAKEINLINE simd4f simdf_rsqrtimp( const simd4f &a )
{
	simd4f res;
	res.x = 1.0f/sqrtf(a.x);
	res.y = 1.0f/sqrtf(a.y);
	res.z = 1.0f/sqrtf(a.z);
    res.w = 1.0f/sqrtf(a.w);
	return res;
}

MAKEINLINE simd4f simdf_or( const simd4f &a, const simd4f &b )
{
	simd4f res;
	simd4i *pi = (simd4i*)&res;
	simd4i *pa = (simd4i*)&a;
	simd4i *pb = (simd4i*)&b;
	pi->x = pa->x | pb->x;
	pi->y = pa->y | pb->y;
	pi->z = pa->z | pb->z;
	pi->w = pa->w | pb->w;
	return res;
}

MAKEINLINE simd4f simdf_and( const simd4f &a, const simd4f &b )
{
	simd4f res;
	simd4i *pi = (simd4i*)&res;
	simd4i *pa = (simd4i*)&a;
	simd4i *pb = (simd4i*)&b;
	pi->x = pa->x & pb->x;
	pi->y = pa->y & pb->y;
	pi->z = pa->z & pb->z;
	pi->w = pa->w & pb->w;
	return res;
}

MAKEINLINE simd4f simdf_andnot( const simd4f &a, const simd4f &b )
{
	simd4f res;
	simd4i *pi = (simd4i*)&res;
	simd4i *pa = (simd4i*)&a;
	simd4i *pb = (simd4i*)&b;
	pi->x = (~pa->x) & pb->x;
	pi->y = (~pa->y) & pb->y;
	pi->z = (~pa->z) & pb->z;
	pi->w = (~pa->w) & pb->w;
	return res;
}

MAKEINLINE int simdf_movemask( const simd4f &a )
{
     int res;
     simd4i *pa = (simd4i*)&a;
     res  = ((pa->x>>31)<<0);
     res |= ((pa->y>>31)<<1);
     res |= ((pa->z>>31)<<2);
     res |= ((pa->w>>31)<<3);
     return( res );
}

MAKEINLINE simd4f simdf_min( const simd4f &a, const simd4f &b )
{
	simd4f res;
	res.x = (a.x<b.x)?a.x:b.x;
	res.y = (a.y<b.y)?a.y:b.y;
	res.z = (a.z<b.z)?a.z:b.z;
	res.w = (a.w<b.w)?a.w:b.w;
	return res;
}

MAKEINLINE simd4f simdf_max( const simd4f &a, const simd4f &b )
{
	simd4f res;
	res.x = (a.x>b.x)?a.x:b.x;
	res.y = (a.y>b.y)?a.y:b.y;
	res.z = (a.z>b.z)?a.z:b.z;
	res.w = (a.w>b.w)?a.w:b.w;
	return res;
}

MAKEINLINE simd4i simdf_to_simdi( const simd4f &a )
{
	simd4i res;
	res.x = (int)a.x;
	res.y = (int)a.y;
	res.z = (int)a.z;
	res.w = (int)a.w;
	return res;
}

MAKEINLINE simd4f simdi_to_simdf( const simd4i &a )
{
	simd4f res;
	res.x = (float)a.x;
	res.y = (float)a.y;
	res.z = (float)a.z;
	res.w = (float)a.w;
	return res;
}

//------------------------------------------------

MAKEINLINE simd4i simdi_or( const simd4i &a, const simd4i &b )
{
	simd4i res;
	res.x = a.x | b.x;
	res.y = a.y | b.y;
	res.z = a.z | b.z;
	res.w = a.w | b.w;
	return res;
}

MAKEINLINE simd4i simdi_and( const simd4i &a, const simd4i &b )
{
	simd4i res;
	res.x = a.x & b.x;
	res.y = a.y & b.y;
	res.z = a.z & b.z;
	res.w = a.w & b.w;
	return res;
}

MAKEINLINE simd4i simdi_andnot( const simd4i &a, const simd4i &b )
{
	simd4i res;
	res.x = (~a.x) & b.x;
	res.y = (~a.y) & b.y;
	res.z = (~a.z) & b.z;
	res.w = (~a.w) & b.w;
	return res;
}

MAKEINLINE simd4i simdi_set1( const int x )
{
	simd4i res;
	res.x = x;
	res.y = x;
	res.z = x;
    res.w = x;
	return res;
}

MAKEINLINE simd4i simdi_set4( const int x, const int y, const int z, const int w )
{
	simd4i res;
	res.x = w;
	res.y = z;
	res.z = y;
    res.w = x;
	return res;
}

MAKEINLINE simd4i simdi_setzero( void )
{
	simd4i res;
	res.x = 0;
	res.y = 0;
	res.z = 0;
    res.w = 0;
	return res;
}

MAKEINLINE simd4i simdi_cmp4eq( const simd4i & a )
{
     simd4i res;
     
     res.x = 0xffffffff;
     res.y = (unsigned int)-(a.y==a.x);
    res.z = (unsigned int)-(a.z==a.x);
    res.w = (unsigned int)-(a.w==a.x);
     return( res );
}


MAKEINLINE simd4i simdi_shl( const simd4i &a, const int n )
{
	simd4i res;
	const int m = n<<3;
	res.x = a.x << m;
	res.y = a.y << m;
	res.z = a.z << m;
	res.w = a.w << m;
	return res;
}

MAKEINLINE simd4i simdi_shr( const simd4i &a, const int n )
{
	simd4i res;
	const int m = n<<3;
	res.x = a.x >> m;
	res.y = a.y >> m;
	res.z = a.z >> m;
	res.w = a.w >> m;
	return res;
}

MAKEINLINE simd4i simdi_add( const simd4i &a, const simd4i &b )
{
	simd4i res;
	res.x = a.x + b.x;
	res.y = a.y + b.y;
	res.z = a.z + b.z;
    res.w = a.w + b.w;
	return res;
}

MAKEINLINE simd4i simdi_sub( const simd4i &a, const simd4i &b )
{
	simd4i res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
    res.w = a.w - b.w;
	return res;
}

MAKEINLINE simd4i simdi_cmpeq( const simd4i &a, const simd4i &b )
{
    simd4i res;

    res.x = -(a.x==b.x);
    res.y = -(a.y==b.y);
    res.z = -(a.z==b.z);
    res.w = -(a.w==b.w);
    return( res );

}


MAKEINLINE simd4i simdi_cmpgt( const simd4i &a, const simd4i &b )
{
    simd4i res;

    res.x = -(a.x>b.x);
    res.y = -(a.y>b.y);
    res.z = -(a.z>b.z);
    res.w = -(a.w>b.w);
    return( res );

}

MAKEINLINE simd4i simdi_shr32( const simd4i &a, const int n )
{
    simd4i res;

    res.x = a.x>>n;
    res.y = a.y>>n;
    res.z = a.z>>n;
    res.w = a.w>>n;
    return( res );
}

MAKEINLINE simd4i simdi_shl32( const simd4i &a, const int n )
{
    simd4i res;

    res.x = a.x<<n;
    res.y = a.y<<n;
    res.z = a.z<<n;
    res.w = a.w<<n;
    return( res );
}

MAKEINLINE simd4i simdi_not( simd4i &a )
{ 
    simd4i res;

    res.x = ~a.x;
    res.y = ~a.y;
    res.z = ~a.z;
    res.w = ~a.w;
    return( res );
}



#define simdf_sel(a,b,c)     simdf_or(simdf_andnot((c),(a)), simdf_and((c),(b)))
#define simdi_sel(a,b,c)     simdi_or(simdi_andnot((c),(a)), simdi_and((c),(b)))


MAKEINLINE simd4f simdf_settrue( void )
{
    simd4f res;
    simd4i *pi = (simd4i*)&res;
    pi->x = 0xffffffff;
    pi->y = 0xffffffff;
    pi->z = 0xffffffff;
    pi->w = 0xffffffff;
    return( res );
}

#endif
