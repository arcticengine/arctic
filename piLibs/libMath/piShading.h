#pragma once

#include <math.h>

namespace piLibs {


static inline float clamp01( float x )
{
    if( x<0.0f ) return 0.0f;
    if( x>1.0f ) return 1.0f;
    return x;
}

static inline float fminf( float a, float b ) { return (a<b)?a:b; }
static inline float fmaxf( float a, float b ) { return (a>b)?a:b; }


static float mix( float a, float b, float x )
{
    return a + (b-a)*x;
}

static float sign( float x )
{
    if( x<0.0f ) return -1.0f;
    if( x>0.0f ) return  1.0f;
    return 0.0f;
}


static inline float smoothstep( float a, float b, float x )
{
    if( x<a ) return 0.0f;
    if( x>b ) return 1.0f;
    x = (x-a)/(b-a);
    return x*x*(3.0f-2.0f*x);
}

static inline float smoothstep( float a, float b, float x, int *reg )
{
    if( x<a ) { *reg=0; return 0.0f; }
    if( x>b ) { *reg=2; return 1.0f; }
    x = (x-a)/(b-a);
    *reg = 1;
    return x*x*(3.0f-2.0f*x);
}

static inline float cubicPulse( float c, float w, float x )
{
    x = fabsf(x - c);
    if( x>w ) return 0.0f;
    x /= w;
    return 1.0f - x*x*(3.0f-2.0f*x);
}

static inline float impulse( float b, float x )
{
    const float h = b*x;
    return h*expf(1.0f-h);
}

static inline float almost( float x, float m, float n )
{
    if( x>m ) return x;

    const float a = 2.0f*n - m;
    const float b = 2.0f*m - 3.0f*n;
    const float t = x/m;

    return (a*t + b)*t*t + n;
}

static inline float hash2f( int n )
{
	n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return (1.0f/2147483647.0f) * (float)n;
}

static inline float hash2sf( int n )
{
	n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return -1.0f + (2.0f/2147483647.0f) * (float)n;
}


} // namespace piLibs