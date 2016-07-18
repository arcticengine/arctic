#ifndef _SIMD_H_
#define _SIMD_H_


//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
#ifdef NOSIMD

#include "simd_emu.h"

#else


#include "types.h"


#ifdef WINDOWS
#include <xmmintrin.h>
#include <emmintrin.h>
//#include <pmmintrin.h>
#endif
#ifdef ALTIX
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
#ifdef IBM
#include <math.h>
#endif

//---------------

#ifdef IBM
typedef vector float simd_4f;
#else
typedef __m128  simd4f;
typedef __m128i simd4i;
#endif

#define simd_prefetch_T0(addr) _mm_prefetch(((char *)(addr)),_MM_HINT_T0) 

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
#else
    #define SIMDALIGN16 __attribute__ ((aligned(16))) 
#endif


#ifdef IBM
#define simdf_add(a,b)       vec_add((a),(b))
#define simdf_sub(a,b)       vec_sub((a),(b))
#define simdf_mad(a,b,c)     vec_madd((a),(b),(c))
#define simdf_mul(a,b)       vec_mul((a),(b))
#define simdf_rcp(a)         vec_re((a))
#define simdf_rsqrt(a)       vec_rsqrte((a))
#define simdf_and(a,b)       vec_and(a, b)  
#define simdf_andnot(a,b)    vec_andc(a, b)  
#define simdf_xor(a,b)       vec_xor((a),(b))  
#define simdf_nor(a,b)       vec_nor((a),(b))  
#define simdf_or(a,b)        vec_no((a),(b))  
#define simdf_min(a,b)       vec_min((a),(b))
#define simdf_max(a,b)       vec_max((a),(b))
#define simdf_sel(a,b,c)     vec_sel(a,b,c)
#define simdf_load(p)        vec_ld(0,(p))
#define simdf_store(p,v)     vec_st((v),0,(p))


#else


#define simdf_add(a,b)       _mm_add_ps((a),(b))
#define simdf_sub(a,b)       _mm_sub_ps((a),(b))
#define simdf_madd(a,b,c)    _mm_add_ps(_mm_mul_ps((a),(b)),(c))
#define simdf_msub(a,b,c)    _mm_sub_ps(_mm_mul_ps((a),(b)),(c))
#define simdf_mul(a,b)       _mm_mul_ps((a),(b))
#define simdf_div(a,b)       _mm_div_ps((a),(b))
#define simdf_rcp(a)         _mm_rcp_ps((a))
#define simdf_rsqrt(a)       _mm_rsqrt_ps((a))
#define simdf_and(a,b)       _mm_and_ps((a),(b))  
#define simdf_andnot(a,b)    _mm_andnot_ps((a),(b))  
#define simdf_xor(a,b)       _mm_xor_ps((a),(b))  
#define simdf_or(a,b)        _mm_or_ps((a),(b))
#define simdf_min(a,b)       _mm_min_ps((a),(b))
#define simdf_max(a,b)       _mm_max_ps((a),(b))
#define simdf_sel(a,b,c)     _mm_or_ps(_mm_andnot_ps((c),(a)), _mm_and_ps((c),(b)))
#define simdf_load(p)        _mm_load_ps((p))
#define simdf_store(p,v)     _mm_store_ps((p),(v))
#define simdf_set1(a)        _mm_set_ps1( (a) )
#define simdf_setzero()      _mm_setzero_ps()
#define simdf_set4(a,b,c,d)  _mm_set_ps( (a),(b),(c),(d) )
#define simdf_cmpeq(a,b)     _mm_cmpeq_ps( (a), (b) )
#define simdf_cmpge(a,b)     _mm_cmpge_ps( (a), (b) )
#define simdf_cmpgt(a,b)     _mm_cmpgt_ps( (a), (b) )
#define simdf_cmplt(a,b)     _mm_cmplt_ps( (a), (b) )
#define simdf_movemask(a)    _mm_movemask_ps( (a) )
//------------------------------------------------
#define simdi_add(a,b)       _mm_add_epi32((a),(b))
#define simdi_sub(a,b)       _mm_sub_epi32((a),(b))
#define simdi_min(a,b)       _mm_min_epi32((a),(b))
#define simdi_and(a,b)       _mm_and_si128((a),(b))
#define simdi_andnot(a,b)    _mm_andnot_si128((a),(b))  
#define simdi_xor(a,b)       _mm_xor_si128((a),(b))
#define simdi_or(a,b)        _mm_or_si128((a),(b))
#define simdi_sel(a,b,c)     _mm_or_si128(_mm_andnot_si128((c),(a)), _mm_and_si128((c),(b)))
#define simdi_set1(a)        _mm_set1_epi32( (a) )
#define simdi_setzero()      _mm_setzero_si128()
//#define simdi_set4(a,b,c,d)  _mm_set_epi32( (a),(b),(c),(d) )
#define simdi_set4(a,b,c,d)  _mm_set_epi32( (a),(b),(c),(d) )
#define simdi_shl(a,b)       _mm_slli_si128( (a), (b) )
#define simdi_shr(a,b)       _mm_srli_si128( (a), (b) )
#define simdi_shr32(a,b)     _mm_srli_epi32( (a), (b) )
#define simdi_shl32(a,b)     _mm_slli_epi32( (a), (b) )
#define simdi_sar32(a,b)     _mm_srai_epi32( (a), (b) )
#define simdi_sal32(a,b)     _mm_slai_epi32( (a), (b) )
#define simdi_cmpeq(a,b)     _mm_cmpeq_epi32( (a), (b) )
#define simdi_cmpgt(a,b)     _mm_cmpgt_epi32( (a), (b) )
#define simdi_cmplt(a,b)     _mm_cmplt_epi32( (a), (b) )
#define simdf_to_simdi(a)    _mm_cvtps_epi32( (a) )
#define simdi_to_simdf(a)    _mm_cvtepi32_ps( (a) )
#define simdf_to_simdi_t(a)    _mm_cvttps_epi32( (a) )
//------------------------------------------------
#if 0
MAKEINLINE simd4i simdi_mul( simd4i a, simd4i b )
{
    
  simd4i c = _mm_shuffle_epi32( a, _MM_SHUFFLE(2,3,0,1) );
  //simd4i t1 = _mm_mul_epu32( a, b );
  //simd4i t2 = _mm_mul_epu32( c, b );
  simd4i t1 = _mm_mul_epu32( a, b );
  simd4i t2 = _mm_mul_epu32( c, b );
  t1 = _mm_shuffle_epi32(t1,_MM_SHUFFLE(3,1,2,0));
  t2 = _mm_shuffle_epi32(t2,_MM_SHUFFLE(3,1,2,0));
  return _mm_unpacklo_epi32( t1, t2 );
  
/*
    __m128i hi = _mm_mulhi_epi16(a, b);
    __m128i lo = _mm_mullo_epi16(a, b);
    return simdi_or( lo, simdi_shl(hi,16) );*/

}
#else
MAKEINLINE simd4i simdi_mul( simd4i a, simd4i b )
{
   __m128i mask32 = _mm_set_epi32(0, 0xffffffff, 0, 0xffffffff);
   __m128i x, y;
   x = _mm_mul_epu32(a, b);
   x = _mm_and_si128(x, mask32);
   a = _mm_shuffle_epi32(a, 0x39);
   b = _mm_shuffle_epi32(b, 0x39);
   y = _mm_mul_epu32(a, b);
   y = _mm_and_si128(y, mask32);
   y = _mm_slli_si128(y,4);
   return _mm_add_epi32(x, y);
} 
#endif
//------------------------------------------------

MAKEINLINE simd4i simdi_cmp4eq( simd4i a )
{ 
    const simd4i tmp = _mm_shuffle_epi32(a,0);//simdi_set1( a );
    return _mm_cmpeq_epi32( tmp, a );
}

MAKEINLINE simd4f simdif_cmp4eq( simd4i a )
{ 
    sseInt4 tmp2;
    const simd4i tmp = _mm_shuffle_epi32(a,0);//simdi_set1( a );
    tmp2.ssei = _mm_cmpeq_epi32( tmp, a );
    return tmp2.ssef;
}

MAKEINLINE simd4i simdi_not( simd4i a )
{ 
    return simdi_xor( a, simdi_set1(-1) );
}
MAKEINLINE simd4f simdi_settrue( void )
{
    sseFloat4 tmp;
    tmp.ssei = simdi_set1(-1);
    return tmp.ssef;
}

MAKEINLINE simd4f simdf_abs( simd4f a )
{
    sseFloat4 b;

    b.ssei = simdi_set1( 0x7fffffff ); 
    return simdf_and( a, b.ssef );
}

MAKEINLINE simd4f simdf_rcpimp( simd4f t )
{
    simd4f xmm0, xmm1;
    // newton-rapson
    xmm0 = simdf_rcp( t );           // z ~= 1/x
    xmm1 = simdf_mul( xmm0, xmm0 );  // z^2
    xmm0 = simdf_add( xmm0, xmm0 );  // 2z
    xmm1 = simdf_mul( xmm1, t );     // xz^2
    return simdf_sub( xmm0, xmm1 );  // 2z - xz^2
}

MAKEINLINE simd4f simdf_rcpimp2( simd4f t )
{
    simd4f xmm0, xmm1;
    xmm0 = simdf_rcp( t );           // z ~= 1/x

    // newton-rapson
    xmm1 = simdf_mul( xmm0, xmm0 );  // z^2
    xmm0 = simdf_add( xmm0, xmm0 );  // 2z
    xmm1 = simdf_mul( xmm1, t );     // xz^2
    xmm0 = simdf_sub( xmm0, xmm1 );  // 2z - xz^2

    // newton-rapson
    xmm1 = simdf_mul( xmm0, xmm0 );  // z^2
    xmm0 = simdf_add( xmm0, xmm0 );  // 2z
    xmm1 = simdf_mul( xmm1, t );     // xz^2
    return simdf_sub( xmm0, xmm1 );  // 2z - xz^2

}


MAKEINLINE simd4f simdf_rsqrtimp( simd4f im )
{
    simd4f tmp;

    const simd4f m0p5 = simdf_set1( -0.5f );
	const simd4f p3p0 = simdf_set1(  3.0f );
    const simd4f ra   = simdf_rsqrt( im );
    // newton-rapson
    tmp = simdf_mul(  im,  ra );
    tmp = simdf_msub( tmp, ra, p3p0 );
    tmp = simdf_mul(  tmp, ra );
    return simdf_mul( tmp, m0p5 );
}
#endif

	


#endif
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#endif
