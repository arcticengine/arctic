#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

namespace piLibs {

struct complexf
{
    float x, y;

	complexf() {}

	explicit complexf( float a )
    {
        x = a;
        y = 0.0f;
    }

	explicit complexf( float a, float b )
    {
        x = a;
        y = b;
    }

	explicit complexf( float const * const v )
    {
        x = v[0];
        y = v[1];
    }

	complexf & operator =( complexf  const & v ) { x = v.x;  y = v.y;  return *this; }
	complexf & operator+=( float     const & s ) { x += s;             return *this; }
	complexf & operator+=( complexf  const & v ) { x += v.x; y += v.y; return *this; }
	complexf & operator-=( float     const & s ) { x -= s;             return *this; }
	complexf & operator-=( complexf  const & v ) { x -= v.x; y -= v.y; return *this; }

	complexf & operator*=( float     const & s ) { x *= s;   y *= s;   return *this; }
	complexf & operator*=( complexf  const & b ) { float ax = x; float ay = y; x = ax*b.x - ay*b.y; y = ax*b.y + ay*b.x; return *this; }
	complexf & operator/=( float     const & s ) { x /= s;   y /= s;   return *this; }
	complexf & operator/=( complexf  const & b ) {  float ax = x; float ay = y; float d = 1.0f/(b.x*b.x + b.y*b.y); x = (ax*b.x + ay*b.y)*d; y = (-ax*b.y + ay*b.x)*d; return *this; }
};

inline complexf operator+( complexf  const & v, float const & s ) { return complexf( v.x + s, v.y ); }
inline complexf operator+( float const & s, complexf  const & v ) { return complexf( s + v.x, v.y ); }
inline complexf operator+( complexf  const & a, complexf  const & b ) { return complexf( a.x + b.x, a.y + b.y ); }
inline complexf operator-( complexf  const & v, float const & s ) { return complexf( v.x - s, v.y ); }
inline complexf operator-( float const & s, complexf  const & v ) { return complexf( s - v.x, v.y ); }
inline complexf operator-( complexf  const & a, complexf  const & b ) { return complexf( a.x - b.x, a.y - b.y ); }
inline complexf operator*( complexf  const & v, float const & s ) { return complexf( v.x * s, v.y * s ); }
inline complexf operator*( float const & s, complexf  const & v ) { return complexf( s * v.x, s * v.y ); }
inline complexf operator*( complexf  const & a, complexf  const & b ) { return complexf( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x ); }
inline complexf operator/( complexf  const & v, float const & s ) { return complexf( v.x / s, v.y / s ); }
inline complexf operator/( float const & s, complexf  const & b ) { float d = 1.0f/(b.x*b.x + b.y*b.y); return complexf( (s*b.x)*d, (-s*b.y)*d ); }
inline complexf operator/( complexf  const & a, complexf  const & b ) { float d = 1.0f/(b.x*b.x + b.y*b.y); return complexf( (a.x*b.x + a.y*b.y)*d, (-a.x*b.y + a.y*b.x)*d ); }


inline complexf floor( complexf const & v )
{
    return complexf( floorf(v.x), floorf(v.y) );
}


inline float modulo( complexf const & z )
{
    return sqrtf( z.x*z.x + z.y*z.y );
}

inline float modulo2( complexf const & z )
{
    return z.x*z.x + z.y*z.y;
}


inline complexf sqrt( const complexf & z )
{
    complexf res;

    if( z.x==0.0f )
    {
        res.x = sqrtf( fabsf(z.y)*.5f );
        if( z.x>0.0f )
            res.y = z.y*0.5f/res.x;
        else
            res.y = 0.0f;
    }
    else if( z.x>0.0f )
    {
        const float m = sqrtf(z.x*z.x + z.y*z.y);
        res.x = sqrtf( (res.x+m)*.5f );
        res.y = z.y*.5f/res.x;
    }
    else
    {
        const float m = sqrtf(z.x*z.x + z.y*z.y);
        res.y = sqrtf( (-z.x+m)*.5f );
        if( z.y<0.0f )
            res.y = -res.y;
        res.x = z.y*0.5f/res.y;
    }
    return res;
}


inline complexf conjugate( const complexf & z )
{
    return complexf(z.x,-z.y);
}

inline complexf squared( const complexf & z )
{
    return complexf( z.x*z.x - z.y*z.y, 2.0f*z.x*z.y );
}

inline complexf cubed( const complexf & z )
{
    const float a = z.x*z.x - z.y*z.y;
    const float b = 2.0f*z.x*z.y;
    return complexf( a*z.x - b*z.y, a*z.y + b*z.x );
}

inline void toPolar( const complexf & z, float *r, float *a )
{
    *a = atan2f( z.y, z.x );
    *r = sqrtf( z.x*z.x + z.y*z.y );
}

inline float arg( const complexf & z )
{
    return atan2f( z.y, z.x );
}

inline complexf ipower( const complexf & z, int power )
{
         if( power==0 ) return complexf(1.0f,0.0f);
    else if( power==1 ) return z;
    else if( power==2 ) return squared(z);
    else if( power==3 ) return cubed(z);
    else if( power==4 ) return squared(squared(z));
    else if( power==6 ) return squared(cubed(z));
    else if( power==8 ) return squared(squared(squared(z)));
    else if( power==9 ) return cubed(cubed(z));
    else if( power==27 ) return cubed(cubed(cubed(z)));

    complexf res = z;
    for( int i=1; i<power; i++ ) res = res * z;
    return res;
}


inline complexf log( const complexf & z )
{
    return complexf( 0.5f*logf(modulo2(z)), arg( z ) );
}

__inline complexf exp( const complexf & z )
{
    const float mo = expf( z.x );
    return complexf( mo*cosf( z.y ), mo*sinf( z.y ) );
}

__inline complexf sin( const complexf & z )
{
    return complexf( sinf(z.x)*coshf(z.y), cosf(z.x)*sinhf(z.y) );
}

__inline complexf cos( const complexf & z )
{
    return complexf( cosf(z.x)*coshf(z.y), -sinf(z.x)*sinhf(z.y) );
}

} // namespace piLibs
