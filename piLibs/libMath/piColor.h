#pragma once

#include <math.h>

#include "../libSystem/piTypes.h"
#include "piVecTypes.h"
#include "piShading.h"

namespace piLibs {

//-------------------------------------------------------------------

inline static vec3 desaturate(const vec3 & col, float f)
{
    const float g = col.x*0.299f + col.y*0.587f + col.z*0.114f;

    return vec3( col.x + f*(g - col.x),
                 col.y + f*(g - col.y),
                 col.z + f*(g - col.z) );
}

//-------------------------------------------------------------------

inline static vec3 int2rgb(uint32 x)
{
    return vec3( (float)((x >>  0) & 255) * (1.0f / 255.0f),
                 (float)((x >>  8) & 255) * (1.0f / 255.0f),
                 (float)((x >> 16) & 255) * (1.0f / 255.0f) );
}

inline static uint32 rgb2int(const vec3 & col)
{
    int r = (int)(col.x*255.0f);
    int g = (int)(col.y*255.0f);
    int b = (int)(col.z*255.0f);

    return(0x00000000 | (r << 16) | (g << 8) | b);
}

//-------------------------------------------------------------------

inline static float gamma(float x)
{
    if (x>0.04045f)
        return powf((x + 0.055f) / 1.055f, 2.4f);
    return x * (1.0f / 12.92f);
}

inline static float igamma(float x)
{
    if (x>0.0031308f)
        return 1.055f*powf(x, 1.0f / 2.4f) - 0.055f;
    return 12.92f*x;
}

//-------------------------------------------------------------------

inline static vec3 hsv2rgb(const vec3 & c)
{
    const vec3 rgb = clamp(abs(mod(c.x*6.0f + vec3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f);

    return c.z * mix(vec3(1.0f), rgb, c.y);
}

inline static vec3 rgb2hsv(const vec3 & c)
{
    const vec4 k = vec4(0.0f, -1.0f/3.0f, 2.0f/3.0f, -1.0f);
    const vec4 p = mix(vec4(c.zy(), k.wz()), vec4(c.yz(), k.xy()), (c.z<c.y) ? 1.0f : 0.0f);
    const vec4 q = mix(vec4(p.xyw(), c.x), vec4(c.x, p.yzx()), (p.x<c.x) ? 1.0f : 0.0f);

    const float d = q.x - fminf(q.w, q.y);
    const float e = 1.0e-10f;
    return vec3(fabsf(q.z + (q.w - q.y) / (6.0f*d+e)), d / (q.x + e), q.x);
}

inline static vec3 mix_hsv(const vec3 & a, const vec3 & b, const float x)
{
    float hue = (fmodf(fmodf(b.x - a.x, 1.0f) + 1.5f, 1.0f) - 0.5f)*x + a.x;
    return vec3(hue, mix(a.yz(), b.yz(), x));
}

inline static vec3 mix_hsv_h(const vec3 & a, const vec3 & b, const float x)
{
    float hue = (fmodf(fmodf(b.x - a.x, 1.0f) + 1.5f, 1.0f) - 0.5f)*x + a.x;
    return vec3(hue, a.y, a.z);
}

inline static vec3 mix_hsv_hs(const vec3 & a, const vec3 & b, const float x)
{
    float hue = (fmodf(fmodf(b.x - a.x, 1.0f) + 1.5f, 1.0f) - 0.5f)*x + a.x;
    return vec3(hue, mix(a.y, b.y, x), a.z);
}
//-------------------------------------------------------------------

inline static float rgb2Y(const vec3 & rgb)
{
    return dot(rgb, vec3(0.2126729f, 0.7151522f, 0.0721750f));
}

static const mat3x3 mRGB2XYZ = mat3x3( 0.4124564f, 0.2126729f, 0.0193339f,
                                       0.3575761f, 0.7151522f, 0.1191920f,
                                       0.1804375f, 0.0721750f, 0.9503041f );

static const mat3x3 mXYZ2RGB = mat3x3( 3.2404542f, -0.9692660f,  0.0556434f,
                                      -1.5371385f,  1.8760108f, -0.2040259f,
                                      -0.4985314f,  0.0415560f,  1.0572252f );

// linear rgb 2 Yxy
inline static vec3 rgb2Yxy( const vec3 & rgb )
{
    const vec3 XYZ = mRGB2XYZ * rgb;
    const float r = 1.0f / (XYZ.x + XYZ.y + XYZ.z);
    return vec3( XYZ.y, XYZ.x*r, XYZ.y*r );
}

// Yxy to linear rgb
inline static vec3 Yxy2rgb( const vec3 & Yxy )
{
    const float r = Yxy.x / Yxy.y;


    vec3 XYZ;
    XYZ.x = Yxy.y * r;
    XYZ.y = Yxy.x;
    XYZ.z = r - XYZ.x - XYZ.y;
    return mXYZ2RGB * XYZ;
}

} // namespace piLibs