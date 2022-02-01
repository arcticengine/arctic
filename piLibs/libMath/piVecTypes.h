#ifndef _VEC_TYPES_H_
#define _VEC_TYPES_H_

#define _USE_MATH_DEFINES
#include <cmath>

namespace piLibs {

//--------------------------------------------------------------------------------
// int
//--------------------------------------------------------------------------------

inline int clamp( const int v, int mi, int ma )
{
    return (v<mi) ? mi : ((v>ma)?ma:v);
}


//--------------------------------------------------------------------------------
// ivec2 
//--------------------------------------------------------------------------------

struct ivec2
{
    int x, y;

	ivec2() {}

	explicit ivec2( int a, int b )
    {
        x = a;
        y = b;
    }

          int & operator [](int i) { return ((int*)this)[i]; }
    const int & operator [](int i) const { return ((int*)this)[i]; }
	
	ivec2 & operator =( ivec2  const & v ) { x = v.x;  y = v.y;  return *this; }
	ivec2 & operator+=( int const & s ) { x += s;   y += s;   return *this; }
	ivec2 & operator+=( ivec2  const & v ) { x += v.x; y += v.y; return *this; }
	ivec2 & operator-=( int const & s ) { x -= s;   y -= s;   return *this; }
	ivec2 & operator-=( ivec2  const & v ) { x -= v.x; y -= v.y; return *this; }
	ivec2 & operator*=( int const & s ) { x *= s;   y *= s;   return *this; }
	ivec2 & operator*=( ivec2  const & v ) { x *= v.x; y *= v.y; return *this; }
	ivec2 & operator/=( int const & s ) { x /= s;   y /= s;   return *this; }
	ivec2 & operator/=( ivec2  const & v ) { x /= v.x; y /= v.y; return *this; }

    ivec2 xx() const { return ivec2(x,x); }
    ivec2 xy() const { return ivec2(x,y); }
    ivec2 yx() const { return ivec2(y,x); }
    ivec2 yy() const { return ivec2(y,y); }

};

inline ivec2 operator+( ivec2  const & v, int const & s ) { return ivec2( v.x + s, v.y + s ); }
inline ivec2 operator+( int const & s, ivec2  const & v ) { return ivec2( s + v.x, s + v.y ); }
inline ivec2 operator+( ivec2  const & a, ivec2  const & b ) { return ivec2( a.x + b.x, a.y + b.y ); }
inline ivec2 operator-( ivec2  const & v, int const & s ) { return ivec2( v.x - s, v.y - s ); }
inline ivec2 operator-( int const & s, ivec2  const & v ) { return ivec2( s - v.x, s - v.y ); }
inline ivec2 operator-( ivec2  const & a, ivec2  const & b ) { return ivec2( a.x - b.x, a.y - b.y ); }
inline ivec2 operator*( ivec2  const & v, int const & s ) { return ivec2( v.x * s, v.y * s ); }
inline ivec2 operator*( int const & s, ivec2  const & v ) { return ivec2( s * v.x, s * v.y ); }
inline ivec2 operator*( ivec2  const & a, ivec2  const & b ) { return ivec2( a.x * b.x, a.y * b.y ); }
inline ivec2 operator/( ivec2  const & v, int const & s ) { return ivec2( v.x / s, v.y / s ); }
inline ivec2 operator/( int const & s, ivec2  const & v ) { return ivec2( s / v.x, s / v.y ); }
inline ivec2 operator/( ivec2  const & a, ivec2  const & b ) { return ivec2( a.x / b.x, a.y / b.y ); }


inline ivec2 vmax( const ivec2 & a, const ivec2 & b )
{
    return ivec2( (a.x>b.x)?a.x:b.x, (a.y>b.y)?a.y:b.y );
}

inline ivec2 vmin(const ivec2 & a, ivec2 & b )
{
    return ivec2((a.x<b.x) ? a.x : b.x, (a.y<b.y) ? a.y : b.y);
}

inline ivec2 min(const ivec2 & v, int mi)
{
    return ivec2((v.x>mi) ? mi : v.x, (v.y>mi) ? mi : v.y);
}

inline ivec2 clamp(const ivec2 & v, int mi, int ma)
{
    return ivec2((v.x<mi) ? mi : ((v.x>ma) ? ma : v.x),
        (v.y<mi) ? mi : ((v.y>ma) ? ma : v.y));
}

//--------------------------------------------------------------------------------
// ivec3 
//--------------------------------------------------------------------------------

struct ivec3
{
    int x, y, z;

	ivec3() {}

	explicit ivec3( int a, int b, int c )
    {
        x = a;
        y = b;
        z = c;
    }
    explicit ivec3( int s )
    {
        x = s;
        y = s;
        z = s;
    }

/*
    explicit ivec3( const vec3 & v )
    {
        x = (int)v.x;
        y = (int)v.y;
        z = (int)v.z;
    }*/

          int & operator [](int i) { return ((int*)this)[i]; }
    const int & operator [](int i) const { return ((int*)this)[i]; }

	ivec3 & operator =(ivec3 const & v) { x = v.x;  y = v.y;  z = v.z; return *this; }
	ivec3 & operator+=(int const & s) { x += s;   y += s;   return *this; }
	ivec3 & operator+=(ivec3 const & v) { x += v.x; y += v.y; return *this; }
	ivec3 & operator-=(int const & s) { x -= s;   y -= s;   return *this; }
	ivec3 & operator-=(ivec3 const & v) { x -= v.x; y -= v.y; return *this; }
	ivec3 & operator*=(int const & s) { x *= s;   y *= s;   return *this; }
	ivec3 & operator*=(ivec3 const & v) { x *= v.x; y *= v.y; return *this; }
	ivec3 & operator/=(int const & s) { x /= s;   y /= s;   return *this; }
	ivec3 & operator/=(ivec3 const & v) { x /= v.x; y /= v.y; return *this; }

    const bool operator== (const ivec3 & v) const { return x == v.x && y == v.y && z == v.z; }
    const bool operator== (ivec3 & v) const { return x == v.x && y == v.y && z == v.z; }
    const bool operator!= (const ivec3 & v) const { return x != v.x || y != v.y || z != v.z; }
    const bool operator!= (ivec3 & v) const { return x != v.x || y != v.y || z != v.z; }

};

inline ivec3 operator+(ivec3  const & v, int const & s) { return ivec3(v.x + s, v.y + s, v.z + s); }
inline ivec3 operator+(int const & s, ivec3  const & v) { return ivec3(s + v.x, s + v.y, s + v.z); }
inline ivec3 operator+(ivec3  const & a, ivec3  const & b) { return ivec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline ivec3 operator-(ivec3  const & v, int const & s) { return ivec3(v.x - s, v.y - s, v.z - s); }
inline ivec3 operator-(int const & s, ivec3  const & v) { return ivec3(s - v.x, s - v.y, s - v.z); }
inline ivec3 operator-(ivec3  const & a, ivec3  const & b) { return ivec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline ivec3 operator*(ivec3  const & v, int const & s) { return ivec3(v.x * s, v.y * s, v.z * s); }
inline ivec3 operator*(int const & s, ivec3  const & v) { return ivec3(s * v.x, s * v.y, s * v.z); }
inline ivec3 operator*(ivec3  const & a, ivec3  const & b) { return ivec3(a.x * b.x, a.y * b.y, a.z * b.z); }
inline ivec3 operator/(ivec3  const & v, int const & s) { return ivec3(v.x / s, v.y / s, v.z / s); }
inline ivec3 operator/(int const & s, ivec3  const & v) { return ivec3(s / v.x, s / v.y, s / v.z); }
inline ivec3 operator/(ivec3  const & a, ivec3  const & b) { return ivec3(a.x / b.x, a.y / b.y, a.z / b.z); }


inline ivec3 min( const ivec3 & v, int mi )
{
    return ivec3( (v.x>mi) ? mi : v.x,
                  (v.y>mi) ? mi : v.y,
                  (v.z>mi) ? mi : v.z );
}

inline ivec3 max( const ivec3 & v, int ma )
{
    return ivec3( (v.x<ma) ? ma : v.x,
                  (v.y<ma) ? ma : v.y,
                  (v.z<ma) ? ma : v.z );
}

inline ivec3 clamp( const ivec3 & v, int mi, int ma )
{
    return ivec3( (v.x<mi) ? mi : ((v.x>ma)?ma:v.x),
                  (v.y<mi) ? mi : ((v.y>ma)?ma:v.y),
                  (v.z<mi) ? mi : ((v.z>ma)?ma:v.z));
}


//--------------------------------------------------------------------------------
// ivec4
//--------------------------------------------------------------------------------

struct ivec4
{
    int x, y, z, w;

    ivec4() {}

    explicit ivec4( int a, int b, int c, int d )
    {
        x = a;
        y = b;
        z = c;
        w = d;
    }
    explicit ivec4( int s )
    {
        x = s;
        y = s;
        z = s;
        w = s;
    }

    int & operator [](int i) { return ((int*)this)[i]; }
    const int & operator [](int i) const { return ((int*)this)[i]; }
};


//--------------------------------------------------------------------------------
// vec2 
//--------------------------------------------------------------------------------

struct vec2
{
    float x, y;

	vec2() {}

	explicit vec2( float a )
    {
        x = a;
        y = a;
    }

	explicit vec2( float a, float b )
    {
        x = a;
        y = b;
    }

	explicit vec2( float const * const v )
    {
        x = v[0];
        y = v[1];
    }

	explicit vec2( const ivec2 & v )
    {
        x = (float)v.x;
        y = (float)v.y;
    }

          float & operator [](int i) { return ((float*)this)[i]; }
    const float & operator [](int i) const { return ((float*)this)[i]; }
	
	vec2 & operator =( vec2  const & v ) { x = v.x;  y = v.y;  return *this; }
	vec2 & operator+=( float const & s ) { x += s;   y += s;   return *this; }
	vec2 & operator+=( vec2  const & v ) { x += v.x; y += v.y; return *this; }
	vec2 & operator-=( float const & s ) { x -= s;   y -= s;   return *this; }
	vec2 & operator-=( vec2  const & v ) { x -= v.x; y -= v.y; return *this; }
	vec2 & operator*=( float const & s ) { x *= s;   y *= s;   return *this; }
	vec2 & operator*=( vec2  const & v ) { x *= v.x; y *= v.y; return *this; }
	vec2 & operator/=( float const & s ) { x /= s;   y /= s;   return *this; }
	vec2 & operator/=( vec2  const & v ) { x /= v.x; y /= v.y; return *this; }

    vec2 xx() const { return vec2(x,x); }
    vec2 xy() const { return vec2(x,y); }
    vec2 yx() const { return vec2(y,x); }
    vec2 yy() const { return vec2(y,y); }
};

inline vec2 operator+( vec2  const & v, float const & s ) { return vec2( v.x + s, v.y + s ); }
inline vec2 operator+( float const & s, vec2  const & v ) { return vec2( s + v.x, s + v.y ); }
inline vec2 operator+( vec2  const & a, vec2  const & b ) { return vec2( a.x + b.x, a.y + b.y ); }
inline vec2 operator-( vec2  const & v, float const & s ) { return vec2( v.x - s, v.y - s ); }
inline vec2 operator-( float const & s, vec2  const & v ) { return vec2( s - v.x, s - v.y ); }
inline vec2 operator-( vec2  const & a, vec2  const & b ) { return vec2( a.x - b.x, a.y - b.y ); }
inline vec2 operator*( vec2  const & v, float const & s ) { return vec2( v.x * s, v.y * s ); }
inline vec2 operator*( float const & s, vec2  const & v ) { return vec2( s * v.x, s * v.y ); }
inline vec2 operator*( vec2  const & a, vec2  const & b ) { return vec2( a.x * b.x, a.y * b.y ); }
inline vec2 operator/( vec2  const & v, float const & s ) { return vec2( v.x / s, v.y / s ); }
inline vec2 operator/( float const & s, vec2  const & v ) { return vec2( s / v.x, s / v.y ); }
inline vec2 operator/( vec2  const & a, vec2  const & b ) { return vec2( a.x / b.x, a.y / b.y ); }

inline vec2 floor( vec2 const & v )
{
    return vec2( floorf(v.x), floorf(v.y) );
}

inline vec2 normalize( vec2 const & v )
{
    float const m2 = v.x*v.x + v.y*v.y;
    float const im = 1.0f/sqrtf( m2 );
    return vec2( v.x*im, v.y*im );
}

inline vec2 mix( vec2 const & a, vec2 const & b, float const f )
{
    return vec2( a.x*(1.0f-f) + f*b.x, 
                 a.y*(1.0f-f) + f*b.y ); 
}

inline float length( vec2 const & v )
{
    return sqrtf( v.x*v.x + v.y*v.y );
}

inline float dot( vec2 const & a, vec2 const & b )
{
    return a.x*b.x + a.y*b.y;
}

inline float distance( vec2 const & a, vec2 const & b )
{
    return length( a - b );
}

inline vec2 perpendicular( vec2 const & v )
{
    return vec2( v.y, -v.x );
}

inline vec2 fromPolar( const float a )
{
    return vec2( cosf(a), sinf(a) );
}

inline float inverseLength( vec2 const & v )
{
    return 1.0f/sqrtf( v.x*v.x + v.y*v.y );
}

inline vec2 sin( const vec2 & v )
{
    return vec2( sinf(v.x), sinf(v.y) );
}

inline vec2 cos( const vec2 & v )
{
    return vec2( cosf(v.x), cosf(v.y) );
}

inline vec2 sqrt( const vec2 & v )
{
    return vec2( sqrtf(v.x), sqrtf(v.y) );
}

//--------------------------------------------------------------------------------
// vec3 
//--------------------------------------------------------------------------------

struct vec3
{
    float x, y, z;

	vec3() {}

	explicit vec3( float a )
    {
        x = a;
        y = a;
        z = a;
    }

	explicit vec3( float a, float b, float c )
    {
        x = a;
        y = b;
        z = c;
    }

	explicit vec3( const float * v )
    {
        x = v[0];
        y = v[1];
        z = v[2];
    }

	explicit vec3( vec2 const & v, float s )
    {
        x = v.x;
        y = v.y;
        z = s;
    }

	explicit vec3( float s, vec2 const & v )
    {
        x = s;
        y = v.x;
        z = v.y;
    }

    explicit vec3( const ivec3 & v )
    {
        x = float(v.x);
        y = float(v.y);
        z = float(v.z);
    }


          float & operator [](int i) { return ((float*)this)[i]; }
    const float & operator [](int i) const { return ((float*)this)[i]; }

	vec3 & operator =( vec3  const & v ) { x = v.x;  y = v.y;  z = v.z;  return *this; }
	vec3 & operator+=( float const & s ) { x += s;   y += s;   z += s;   return *this; }
	vec3 & operator+=( vec3  const & v ) { x += v.x; y += v.y; z += v.z; return *this; }
	vec3 & operator-=( float const & s ) { x -= s;   y -= s;   z -= s;   return *this; }
	vec3 & operator-=( vec3  const & v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	vec3 & operator*=( float const & s ) { x *= s;   y *= s;   z *= s;   return *this; }
	vec3 & operator*=( vec3  const & v ) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	vec3 & operator/=( float const & s ) { x /= s;   y /= s;   z /= s;   return *this; }
	vec3 & operator/=( vec3  const & v ) { x /= v.x; y /= v.y; z /= v.z; return *this; }

    vec2 xx() const { return vec2(x,x); }
    vec2 xy() const { return vec2(x,y); }
    vec2 xz() const { return vec2(x,z); }
    vec2 yx() const { return vec2(y,x); }
    vec2 yy() const { return vec2(y,y); }
    vec2 yz() const { return vec2(y,z); }
    vec2 zx() const { return vec2(z,x); }
    vec2 zy() const { return vec2(z,y); }
    vec2 zz() const { return vec2(z,z); }

    vec3 xxx() const { return vec3(x,x,x); }
    vec3 xxy() const { return vec3(x,x,y); }
    vec3 xxz() const { return vec3(x,x,z); }
    vec3 xyx() const { return vec3(x,y,x); }
    vec3 xyy() const { return vec3(x,y,y); }
    vec3 xyz() const { return vec3(x,y,z); }
    vec3 xzx() const { return vec3(x,z,x); }
    vec3 xzy() const { return vec3(x,z,y); }
    vec3 xzz() const { return vec3(x,z,z); }
    vec3 yxx() const { return vec3(y,x,x); }
    vec3 yxy() const { return vec3(y,x,y); }
    vec3 yxz() const { return vec3(y,x,z); }
    vec3 yyx() const { return vec3(y,y,x); }
    vec3 yyy() const { return vec3(y,y,y); }
    vec3 yyz() const { return vec3(y,y,z); }
    vec3 yzx() const { return vec3(y,z,x); }
    vec3 yzy() const { return vec3(y,z,y); }
    vec3 yzz() const { return vec3(y,z,z); }
    vec3 zxx() const { return vec3(z,x,x); }
    vec3 zxy() const { return vec3(z,x,y); }
    vec3 zxz() const { return vec3(z,x,z); }
    vec3 zyx() const { return vec3(z,y,x); }
    vec3 zyy() const { return vec3(z,y,y); }
    vec3 zyz() const { return vec3(z,y,z); }
    vec3 zzx() const { return vec3(z,z,x); }
    vec3 zzy() const { return vec3(z,z,y); }
    vec3 zzz() const { return vec3(z,z,z); }
};

inline vec3 operator+( vec3  const & v, float const & s ) { return vec3( v.x + s, v.y + s, v.z + s ); }
inline vec3 operator+( float const & s, vec3  const & v ) { return vec3( s + v.x, s + v.y, s + v.z ); }
inline vec3 operator+( vec3  const & a, vec3  const & b ) { return vec3( a.x + b.x, a.y + b.y, a.z + b.z ); }
inline vec3 operator-( vec3  const & v, float const & s ) { return vec3( v.x - s, v.y - s, v.z - s); }
inline vec3 operator-( float const & s, vec3  const & v ) { return vec3( s - v.x, s - v.y, s - v.z ); }
inline vec3 operator-( vec3  const & a, vec3  const & b ) { return vec3( a.x - b.x, a.y - b.y, a.z - b.z ); }
inline vec3 operator*( vec3  const & v, float const & s ) { return vec3( v.x * s, v.y * s, v.z * s ); }
inline vec3 operator*( float const & s, vec3  const & v ) { return vec3( s * v.x, s * v.y, s * v.z ); }
inline vec3 operator*( vec3  const & a, vec3  const & b ) { return vec3( a.x * b.x, a.y * b.y, a.z * b.z ); }
inline vec3 operator/( vec3  const & v, float const & s ) { return vec3( v.x / s, v.y / s, v.z / s ); }
inline vec3 operator/( float const & s, vec3  const & v ) { return vec3( s / v.x, s / v.y, s / v.z); }
inline vec3 operator/( vec3  const & a, vec3  const & b ) { return vec3( a.x / b.x, a.y / b.y, a.z / b.z ); }

inline vec3 floor( vec3 const & v )
{
    return vec3( floorf(v.x), floorf(v.y), floorf(v.z) );
}

inline vec3 normalize( vec3 const & v )
{
    float const m2 = v.x*v.x + v.y*v.y + v.z*v.z;
    float const im = 1.0f/sqrtf( m2 );
    return vec3( v.x*im, v.y*im, v.z*im );
}

inline vec3 normalizeSafe( vec3 const & v )
{
    float const m2 = v.x*v.x + v.y*v.y + v.z*v.z;
    if( m2<=0.000000001f ) return vec3(0.0f);
    float const im = 1.0f/sqrtf( m2 );
    return vec3( v.x*im, v.y*im, v.z*im );
}

inline vec3 mix( vec3 const & a, vec3 const & b, float const f )
{
    return vec3( a.x*(1.0f-f) + f*b.x, 
                 a.y*(1.0f-f) + f*b.y, 
                 a.z*(1.0f-f) + f*b.z );
}

inline vec3 cross( vec3 const & a, vec3 const & b )
{
    return vec3( a.y*b.z - a.z*b.y,
                 a.z*b.x - a.x*b.z,
                 a.x*b.y - a.y*b.x );
}

inline float length( vec3 const & v )
{
    return sqrtf( v.x*v.x + v.y*v.y + v.z*v.z );
}

inline float lengthSquared( vec3 const & v )
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline float inverseLength( vec3 const & v )
{
    return 1.0f/sqrtf( v.x*v.x + v.y*v.y + v.z*v.z );
}

inline float dot( vec3 const & a, vec3 const & b )
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float distance( vec3 const & a, vec3 const & b )
{
    return length( a - b );
}

inline void buildBase( const vec3 & n, vec3 & uu, vec3 & vv )
{
    vec3 up;
    if( fabsf(n.z)<0.9f ) { up.x=0.0f; up.y=0.0f; up.z=1.0f; }
    else                  { up.x=1.0f; up.y=0.0f; up.z=0.0f; }
    vv = normalize( cross( n, up ) );
    uu = normalize( cross( vv, n ) );
}


inline vec3 orientate( const vec3 & v, const vec3 & dir )
{
    vec3 res = v;
    const float kk = dot( dir, v );
    if( kk<0.0f ) res -= 2.0f*dir*kk;
    return res;

}

inline vec3 pow(const vec3 & v, const float f)
{
    return vec3(powf(v.x, f), powf(v.y, f), powf(v.z, f));
}

inline vec3 pow(const vec3 & v, const vec3 & f)
{
    return vec3(powf(v.x, f.x), powf(v.y, f.y), powf(v.z, f.z));
}


inline vec3 sin( const vec3 & v )
{
    return vec3( sinf(v.x), sinf(v.y), sinf(v.z) );
}

inline vec3 cos( const vec3 & v )
{
    return vec3( cosf(v.x), cosf(v.y), cosf(v.z) );
}


inline vec3 mod( const vec3 & v, float s )
{
    return vec3( fmodf(v.x,s), fmodf(v.y,s), fmodf(v.z,s) );
}

inline vec3 sqrt( const vec3 & v )
{
    return vec3( sqrtf(v.x), sqrtf(v.y), sqrtf(v.z) );
}

inline vec3 vmin( const vec3 & v, float mi )
{
    return vec3( (v.x<mi)?v.x:mi, (v.y<mi)?v.y:mi, (v.z<mi)?v.z:mi );
}
inline vec3 vmax( const vec3 & v, float ma )
{
    return vec3( (v.x>ma)?v.x:ma, (v.y>ma)?v.y:ma, (v.z>ma)?v.z:ma );
}
inline vec3 clamp( const vec3 & v, float mi, float ma )
{
    return vmax( vmin( v, ma ), mi );
}
inline vec3 clamp01( const vec3 & v )
{
    return vmax( vmin( v, 1.0f ), 0.0f );
}
inline vec3 clamp1( const vec3 & v )
{
    return vmax( vmin( v, 1.0f ), -1.0f );
}
inline vec3 abs( const vec3 & v )
{
    return vec3( fabsf(v.x), fabsf(v.y), fabsf(v.z) );
}

inline vec3 smoothstep( float a, float b, const vec3 & v )
{
    vec3 x = clamp01( (v-vec3(a))/(b-a) );

    return x*x*(3.0f - 2.0f*x);
}

inline float maxcomp( const vec3 & v )
{
    return (v.x>v.y) ? ( (v.x>v.z) ? v.x : v.z) : (  (v.y>v.z) ? v.y : v.z );
}

//--------------------------------------------------------------------------------
// vec4 
//--------------------------------------------------------------------------------

struct vec4
{
    float x, y, z, w;

	vec4() {}

	explicit vec4( float a, float b, float c, float d )
    {
        x = a;
        y = b;
        z = c;
        w = d;
    }

	explicit vec4( float const * const v )
    {
        x = v[0];
        y = v[1];
        z = v[2];
        w = v[3];
    }

	explicit vec4( vec3 const & v, const float s )
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = s;
    }

    explicit vec4(vec2 const & a, vec2 const & b)
    {
        x = a.x;
        y = a.y;
        z = b.x;
        w = b.y;
    }

    explicit vec4(const float s, vec3 const & v)
    {
        x = s;
        y = v.x;
        z = v.y;
        w = v.z;
    }

    explicit vec4( float s )
    {
        x = s;
        y = s;
        z = s;
        w = s;
    }

          float & operator [](int i) { return ((float*)this)[i]; }
    const float & operator [](int i) const { return ((float*)this)[i]; }

	vec4 & operator =( vec4  const & v ) { x = v.x;  y = v.y;  z = v.z;  w = v.w;  return *this; }
	vec4 & operator+=( float const & s ) { x += s;   y += s;   z += s;   w += s;   return *this; }
	vec4 & operator+=( vec4  const & v ) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	vec4 & operator-=( float const & s ) { x -= s;   y -= s;   z -= s;   w -= s;   return *this; }
	vec4 & operator-=( vec4  const & v ) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	vec4 & operator*=( float const & s ) { x *= s;   y *= s;   z *= s;   w *= s;   return *this; }
	vec4 & operator*=( vec4  const & v ) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	vec4 & operator/=( float const & s ) { x /= s;   y /= s;   z /= s;   w /= s;   return *this; }
	vec4 & operator/=( vec4  const & v ) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

    vec2 xx() const { return vec2(x,x); }
    vec2 xy() const { return vec2(x,y); }
    vec2 xz() const { return vec2(x,z); }
    vec2 yx() const { return vec2(y,x); }
    vec2 yy() const { return vec2(y,y); }
    vec2 yz() const { return vec2(y,z); }
    vec2 zx() const { return vec2(z,x); }
    vec2 zy() const { return vec2(z,y); }
    vec2 zz() const { return vec2(z,z); }
    vec2 wz() const { return vec2(w,z); }

    vec3 xxx() const { return vec3(x,x,x); }
    vec3 xxy() const { return vec3(x,x,y); }
    vec3 xxz() const { return vec3(x,x,z); }
    vec3 xxw() const { return vec3(x,x,w); }
    vec3 xyx() const { return vec3(x,y,x); }
    vec3 xyy() const { return vec3(x,y,y); }
    vec3 xyz() const { return vec3(x,y,z); }
    vec3 xyw() const { return vec3(x,y,w); }
    vec3 xzx() const { return vec3(x,z,x); }
    vec3 xzy() const { return vec3(x,z,y); }
    vec3 xzz() const { return vec3(x,z,z); }
    vec3 xzw() const { return vec3(x,z,w); }
    vec3 yxx() const { return vec3(y,x,x); }
    vec3 yxy() const { return vec3(y,x,y); }
    vec3 yxz() const { return vec3(y,x,z); }
    vec3 yxw() const { return vec3(y,x,w); }
    vec3 yyx() const { return vec3(y,y,x); }
    vec3 yyy() const { return vec3(y,y,y); }
    vec3 yyz() const { return vec3(y,y,z); }
    vec3 yyw() const { return vec3(y,y,w); }
    vec3 yzx() const { return vec3(y,z,x); }
    vec3 yzy() const { return vec3(y,z,y); }
    vec3 yzz() const { return vec3(y,z,z); }
    vec3 yzw() const { return vec3(y,z,w); }
    vec3 zxx() const { return vec3(z,x,x); }
    vec3 zxy() const { return vec3(z,x,y); }
    vec3 zxz() const { return vec3(z,x,z); }
    vec3 zxw() const { return vec3(z,x,w); }
    vec3 zyx() const { return vec3(z,y,x); }
    vec3 zyy() const { return vec3(z,y,y); }
    vec3 zyz() const { return vec3(z,y,z); }
    vec3 zzx() const { return vec3(z,z,x); }
    vec3 zzy() const { return vec3(z,z,y); }
    vec3 zzz() const { return vec3(z,z,z); }
    vec3 zzw() const { return vec3(z,z,w); }
    vec3 www() const { return vec3(w,w,w); }


};

inline vec4 operator+( vec4  const & v, float const & s ) { return vec4( v.x + s, v.y + s, v.z + s, v.w + s ); }
inline vec4 operator+( float const & s, vec4  const & v ) { return vec4( s + v.x, s + v.y, s + v.z, s + v.w ); }
inline vec4 operator+( vec4  const & a, vec4  const & b ) { return vec4( a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w ); }
inline vec4 operator-( vec4  const & v, float const & s ) { return vec4( v.x - s, v.y - s, v.z - s, v.w - s); }
inline vec4 operator-( float const & s, vec4  const & v ) { return vec4( s - v.x, s - v.y, s - v.z, s - v.w ); }
inline vec4 operator-( vec4  const & a, vec4  const & b ) { return vec4( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w ); }
inline vec4 operator*( vec4  const & v, float const & s ) { return vec4( v.x * s, v.y * s, v.z * s, v.w * s ); }
inline vec4 operator*( float const & s, vec4  const & v ) { return vec4( s * v.x, s * v.y, s * v.z, s * v.w ); }
inline vec4 operator*( vec4  const & a, vec4  const & b ) { return vec4( a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w ); }
inline vec4 operator/( vec4  const & v, float const & s ) { return vec4( v.x / s, v.y / s, v.z / s, v.w / s ); }
inline vec4 operator/( float const & s, vec4  const & v ) { return vec4( s / v.x, s / v.y, s / v.z, s / v.w); }
inline vec4 operator/( vec4  const & a, vec4  const & b ) { return vec4( a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w ); }

inline vec4 normalize( vec4 const & v )
{
    float const m2 = v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
    float const im = 1.0f/sqrtf( m2 );
    return vec4( v.x*im, v.y*im, v.z*im, v.w*im );
}

inline vec4 mix( vec4 const & a, vec4 const & b, float const f )
{
    return vec4( a.x*(1.0f-f) + f*b.x, 
                 a.y*(1.0f-f) + f*b.y, 
                 a.z*(1.0f-f) + f*b.z, 
                 a.w*(1.0f-f) + f*b.w );
}

inline float length( vec4 const & v )
{
    return sqrtf( v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w );
}

inline float dot( vec4 const & a, vec4 const & b )
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

inline float distance( vec4 const & a, vec4 const & b )
{
    return length( a - b );
}

// make a plane that goes through pointa a, b, c
inline vec4 makePlane( vec3 const & a, vec3 const & b, vec3 const & c ) 
{
//    const vec3 n = normalize( cross( c-a, b-a ) );
    const vec3 n = normalize( cross( b-a, c-a ) );
    return vec4( n, -dot(a,n) );
}

inline vec3 intersectPlanes( vec4 const & p1, vec4 const & p2, vec4 const & p3 )
{
    const float den = dot( p1.xyz(), cross( p2.xyz(), p3.xyz() ) );

    if( den==0.0f )
    	return vec3(0.0f);

    vec3 res = p1.w * cross( p2.xyz(), p3.xyz() ) +
               p2.w * cross( p3.xyz(), p1.xyz() ) +
               p3.w * cross( p1.xyz(), p2.xyz() );

    return res * (-1.0f/den);
}


//--------------------------------------------------------------------------------
// mat2x2
//--------------------------------------------------------------------------------

struct mat2x2
{
    float m[4];

	mat2x2() {}

	explicit mat2x2( float a0, float a1,
	                 float a2, float a3 )
    {
        m[0] = a0; m[1] = a1;
        m[2] = a2; m[3] = a3;
    }

          float & operator [](int i) { return m[i]; }
    const float & operator [](int i) const { return m[i]; }
};

inline vec2 operator*( mat2x2 const & m, vec2 const & v ) 
{ 
    return vec2( v.x*m[0] + v.y*m[1],
                 v.x*m[2] + v.y*m[3] );
}

inline float determinant( mat2x2 const & m )
{
    return m.m[0]*m.m[3] - m.m[1]*m.m[2];
}

inline mat2x2 rotation( const float t )
{
    const float co = cosf( t );
    const float si = sinf( t );

    return mat2x2( co, -si, 
                   si,  co );
}

inline mat2x2 rotation( vec2 const & v )
{
    return mat2x2( v.x, -v.y, 
                   v.y,  v.x );
}

//--------------------------------------------------------------------------------
// mat3x3 
//--------------------------------------------------------------------------------

struct mat3x3
{
    float m[9];

	mat3x3() {}

	explicit mat3x3( float a0, float a1, float a2,
	                 float a3, float a4, float a5,
	                 float a6, float a7, float a8 )
    {
        m[0] = a0; m[1] = a1; m[2] = a2;
        m[3] = a3; m[4] = a4; m[5] = a5;
        m[6] = a6; m[7] = a7; m[8] = a8;
    }

	explicit mat3x3( const vec3 & a,const vec3 & b, const vec3 & c )
    {
        m[0] = a.x; m[1] = b.x; m[2] = c.x;
        m[3] = a.y; m[4] = b.y; m[5] = c.y;
        m[6] = a.z; m[7] = b.z; m[8] = c.z;
    }

          float & operator [](int i) { return m[i]; }
    const float & operator [](int i) const { return m[i]; }

    vec3 x() const { return vec3(m[0],m[3],m[6]); }
    vec3 y() const { return vec3(m[1],m[4],m[7]); }
    vec3 z() const { return vec3(m[2],m[5],m[8]); }
};

inline mat3x3 operator*( mat3x3 const & a, mat3x3 const & b ) 
{ 

    return mat3x3( a[0]*b[0] + a[1]*b[3] + a[2]*b[6],
                   a[0]*b[1] + a[1]*b[4] + a[2]*b[7],
                   a[0]*b[2] + a[1]*b[5] + a[2]*b[8],

                   a[3]*b[0] + a[4]*b[3] + a[5]*b[6],
                   a[3]*b[1] + a[4]*b[4] + a[5]*b[7],
                   a[3]*b[2] + a[4]*b[5] + a[5]*b[8],

                   a[6]*b[0] + a[7]*b[3] + a[8]*b[6],
                   a[6]*b[1] + a[7]*b[4] + a[8]*b[7],
                   a[6]*b[2] + a[7]*b[5] + a[8]*b[8] ); 
}

inline vec3 operator*( mat3x3 const & m, vec3 const & v ) 
{ 
    return vec3( v.x*m[0] + v.y*m[1] + v.z*m[2],
                 v.x*m[3] + v.y*m[4] + v.z*m[5],
                 v.x*m[6] + v.y*m[7] + v.z*m[8] );
}

inline float determinant( mat3x3 const & m )
{
    return m.m[0]*m.m[4]*m.m[8] + m.m[3]*m.m[7]*m.m[2] + m.m[1]*m.m[5]*m.m[6] - m.m[2]*m.m[4]*m.m[6] - m.m[1]*m.m[3]*m.m[8] - m.m[5]*m.m[7]*m.m[0];
}

inline mat3x3 setScale3( const vec3 & s )
{
    return mat3x3( s.x,  0.0f, 0.0f,
                   0.0f, s.y,  0.0f,
                   0.0f, 0.0f, s.z );
}


inline mat3x3 transpose( mat3x3 const & m )
{
    return mat3x3( m.m[0], m.m[3], m.m[6], 
                   m.m[1], m.m[4], m.m[7], 
                   m.m[2], m.m[5], m.m[8] );
}

inline vec3 rotate( const vec3 & v, float t, const vec3 & a )
{
    const float sint = sinf(t);
    const float cost = cosf(t);
    const float icost = 1.0f - cost;

    const mat3x3 m = mat3x3( a.x*a.x*icost + cost,
                             a.y*a.x*icost - sint*a.z,
                             a.z*a.x*icost + sint*a.y,

                             a.x*a.y*icost + sint*a.z,
                             a.y*a.y*icost + cost,
                             a.z*a.y*icost - sint*a.x,

                             a.x*a.z*icost - sint*a.y,
                             a.y*a.z*icost + sint*a.x,
                             a.z*a.z*icost + cost );
    return m * v;
}

inline vec3 rotate( const vec3 & v, float cost, float sint, const vec3 & a )
{
    const float icost = 1.0f - cost;

    const mat3x3 m = mat3x3( a.x*a.x*icost + cost,
                             a.y*a.x*icost - sint*a.z,
                             a.z*a.x*icost + sint*a.y,

                             a.x*a.y*icost + sint*a.z,
                             a.y*a.y*icost + cost,
                             a.z*a.y*icost - sint*a.x,

                             a.x*a.z*icost - sint*a.y,
                             a.y*a.z*icost + sint*a.x,
                             a.z*a.z*icost + cost );
    return m * v;
}

inline vec3 rotateX(const vec3 &v, float a)
{
    const float si = sinf(a);
    const float co = cosf(a);

    return vec3(v[0], v[1] * co - v[2] * si, v[1] * si + v[2] * co);
}

inline vec3 rotateY(const vec3 & v, float a)
{
    const float si = sinf(a);
    const float co = cosf(a);

    return vec3(v[0] * co + v[2] * si, v[1], -v[0] * si + v[2] * co);
}

inline vec3 rotateZ(const vec3 & v, float a)
{
    const float si = sinf(a);
    const float co = cosf(a);

    return vec3(v[0] * co + v[1] * si, -v[0] * si + v[1] * co, v[2] );
}

inline mat3x3 setRotationAxisAngle3( const vec3 & a, const float t )
{
    const float sint = sinf(t);
    const float cost = cosf(t);
    const float icost = 1.0f - cost;

    return mat3x3( a.x*a.x*icost + cost,
                   a.y*a.x*icost - sint*a.z,
                   a.z*a.x*icost + sint*a.y,

                   a.x*a.y*icost + sint*a.z,
                   a.y*a.y*icost + cost,
                   a.z*a.y*icost - sint*a.x,

                   a.x*a.z*icost - sint*a.y,
                   a.y*a.z*icost + sint*a.x,
                   a.z*a.z*icost + cost );
}


inline mat3x3 setRotationEuler3( float x, float y, float z )
{
    const float a = sinf(x); 
    const float b = cosf(x); 
    const float c = sinf(y); 
    const float d = cosf(y); 
    const float e = sinf(z); 
    const float f = cosf(z); 
    const float ac = a*c;
    const float bc = b*c;

    return mat3x3( d*f, d*e, -c,
                   ac*f-b*e, ac*e+b*f, a*d,
                   bc*f+a*e, bc*e-a*f, b*d );

}


inline mat3x3 setRotationEuler3( const vec3 & xyz )
{
    return setRotationEuler3( xyz.x, xyz.y, xyz.z );
}

inline mat3x3 setIdentity3( void )
{
    return mat3x3( 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f );
}


inline mat3x3 buildBase( const vec3 & nn )
{
    vec3 up;
    if( fabsf(nn.z)<0.9f ) { up.x=0.0f; up.y=0.0f; up.z=1.0f; }
    else                   { up.x=1.0f; up.y=0.0f; up.z=0.0f; }
    const vec3 vv = normalize( cross( nn, up ) );
    const vec3 uu = normalize( cross( vv, nn ) );

    return mat3x3( uu.x, vv.x, nn.x,
                   uu.y, vv.y, nn.y,
                   uu.z, vv.z, nn.z );
}

//--------------------------------------------------------------------------------
// mat4x4 
//--------------------------------------------------------------------------------

struct mat4x4
{
    float m[16];

	mat4x4() {}

	explicit mat4x4( float a00, float a01, float a02, float a03,
	                 float a04, float a05, float a06, float a07,
	                 float a08, float a09, float a10, float a11,
	                 float a12, float a13, float a14, float a15 )
    {
        m[ 0] = a00; m[ 1] = a01; m[ 2] = a02; m[ 3] = a03;
        m[ 4] = a04; m[ 5] = a05; m[ 6] = a06; m[ 7] = a07;
        m[ 8] = a08; m[ 9] = a09; m[10] = a10; m[11] = a11;
        m[12] = a12; m[13] = a13; m[14] = a14; m[15] = a15;
    }

	explicit mat4x4( const float *v )
    {
        m[ 0] = v[ 0]; m[ 1] = v[ 1]; m[ 2] = v[ 2]; m[ 3] = v[ 3];
        m[ 4] = v[ 4]; m[ 5] = v[ 5]; m[ 6] = v[ 6]; m[ 7] = v[ 7];
        m[ 8] = v[ 8]; m[ 9] = v[ 9]; m[10] = v[10]; m[11] = v[11];
        m[12] = v[12]; m[13] = v[13]; m[14] = v[14]; m[15] = v[15];
    }

          float & operator [](int i) { return m[i]; }
    const float & operator [](int i) const { return m[i]; }
};

inline mat4x4 operator*( mat4x4 const & a, mat4x4 const & b ) 
{
    mat4x4 res;
    for( int i=0; i<4; i++ )
    {
        const float x = a.m[4*i+0];
        const float y = a.m[4*i+1];
        const float z = a.m[4*i+2];
        const float w = a.m[4*i+3];

        res.m[4*i+0] = x * b[ 0] + y * b[ 4] + z * b[ 8] + w * b[12];
        res.m[4*i+1] = x * b[ 1] + y * b[ 5] + z * b[ 9] + w * b[13];
        res.m[4*i+2] = x * b[ 2] + y * b[ 6] + z * b[10] + w * b[14];
        res.m[4*i+3] = x * b[ 3] + y * b[ 7] + z * b[11] + w * b[15];
    }

    return res;
}


inline vec4 operator*( mat4x4 const & m, vec4 const & v ) 
{ 
    return vec4( v.x*m[ 0] + v.y*m[ 1] + v.z*m[ 2] + v.w*m[ 3],
                 v.x*m[ 4] + v.y*m[ 5] + v.z*m[ 6] + v.w*m[ 7],
                 v.x*m[ 8] + v.y*m[ 9] + v.z*m[10] + v.w*m[11],
                 v.x*m[12] + v.y*m[13] + v.z*m[14] + v.w*m[15] );
}

inline vec4 vtransform( const mat4x4 & m, const vec4 & v ) 
{ 
    return vec4( v.x*m[ 0] + v.y*m[ 1] + v.z*m[ 2] + v.w*m[ 3],
                 v.x*m[ 4] + v.y*m[ 5] + v.z*m[ 6] + v.w*m[ 7],
                 v.x*m[ 8] + v.y*m[ 9] + v.z*m[10] + v.w*m[11],
                 v.x*m[12] + v.y*m[13] + v.z*m[14] + v.w*m[15] );
}

inline vec3 vtransform( const mat4x4 & m, const vec3 & v ) 
{ 
    return vec3( v.x*m[ 0] + v.y*m[ 1] + v.z*m[ 2] + m[ 3],
                 v.x*m[ 4] + v.y*m[ 5] + v.z*m[ 6] + m[ 7],
                 v.x*m[ 8] + v.y*m[ 9] + v.z*m[10] + m[11] );
}

inline vec3 ntransform( mat4x4 const & m, vec3 const & v ) 
{ 
    return vec3( v.x*m[ 0] + v.y*m[ 1] + v.z*m[ 2],
                 v.x*m[ 4] + v.y*m[ 5] + v.z*m[ 6],
                 v.x*m[ 8] + v.y*m[ 9] + v.z*m[10] );
}

inline mat4x4 transpose( mat4x4 const & m )
{
    return mat4x4( m.m[0], m.m[4], m.m[ 8], m.m[12],
                   m.m[1], m.m[5], m.m[ 9], m.m[13], 
                   m.m[2], m.m[6], m.m[10], m.m[14], 
                   m.m[3], m.m[7], m.m[11], m.m[15] );
}

inline mat4x4 invertFast( mat4x4 const & m )
{
    mat4x4 inv = mat4x4( 
   
             m.m[5]  * m.m[10] * m.m[15] - 
             m.m[5]  * m.m[11] * m.m[14] - 
             m.m[9]  * m.m[6]  * m.m[15] + 
             m.m[9]  * m.m[7]  * m.m[14] +
             m.m[13] * m.m[6]  * m.m[11] - 
             m.m[13] * m.m[7]  * m.m[10],

             -m.m[1]  * m.m[10] * m.m[15] + 
              m.m[1]  * m.m[11] * m.m[14] + 
              m.m[9]  * m.m[2] * m.m[15] - 
              m.m[9]  * m.m[3] * m.m[14] - 
              m.m[13] * m.m[2] * m.m[11] + 
              m.m[13] * m.m[3] * m.m[10],

             m.m[1]  * m.m[6] * m.m[15] - 
             m.m[1]  * m.m[7] * m.m[14] - 
             m.m[5]  * m.m[2] * m.m[15] + 
             m.m[5]  * m.m[3] * m.m[14] + 
             m.m[13] * m.m[2] * m.m[7] - 
             m.m[13] * m.m[3] * m.m[6],

             -m.m[1] * m.m[6] * m.m[11] + 
              m.m[1] * m.m[7] * m.m[10] + 
              m.m[5] * m.m[2] * m.m[11] - 
              m.m[5] * m.m[3] * m.m[10] - 
              m.m[9] * m.m[2] * m.m[7] + 
              m.m[9] * m.m[3] * m.m[6],

             -m.m[4]  * m.m[10] * m.m[15] + 
              m.m[4]  * m.m[11] * m.m[14] + 
              m.m[8]  * m.m[6]  * m.m[15] - 
              m.m[8]  * m.m[7]  * m.m[14] - 
              m.m[12] * m.m[6]  * m.m[11] + 
              m.m[12] * m.m[7]  * m.m[10],

             m.m[0]  * m.m[10] * m.m[15] - 
             m.m[0]  * m.m[11] * m.m[14] - 
             m.m[8]  * m.m[2] * m.m[15] + 
             m.m[8]  * m.m[3] * m.m[14] + 
             m.m[12] * m.m[2] * m.m[11] - 
             m.m[12] * m.m[3] * m.m[10],

             -m.m[0]  * m.m[6] * m.m[15] + 
              m.m[0]  * m.m[7] * m.m[14] + 
              m.m[4]  * m.m[2] * m.m[15] - 
              m.m[4]  * m.m[3] * m.m[14] - 
              m.m[12] * m.m[2] * m.m[7] + 
              m.m[12] * m.m[3] * m.m[6],


             m.m[0] * m.m[6] * m.m[11] - 
             m.m[0] * m.m[7] * m.m[10] - 
             m.m[4] * m.m[2] * m.m[11] + 
             m.m[4] * m.m[3] * m.m[10] + 
             m.m[8] * m.m[2] * m.m[7] - 
             m.m[8] * m.m[3] * m.m[6],


             m.m[4]  * m.m[9] * m.m[15] - 
             m.m[4]  * m.m[11] * m.m[13] - 
             m.m[8]  * m.m[5] * m.m[15] + 
             m.m[8]  * m.m[7] * m.m[13] + 
             m.m[12] * m.m[5] * m.m[11] - 
             m.m[12] * m.m[7] * m.m[9],



             -m.m[0]  * m.m[9] * m.m[15] + 
              m.m[0]  * m.m[11] * m.m[13] + 
              m.m[8]  * m.m[1] * m.m[15] - 
              m.m[8]  * m.m[3] * m.m[13] - 
              m.m[12] * m.m[1] * m.m[11] + 
              m.m[12] * m.m[3] * m.m[9],

              m.m[0]  * m.m[5] * m.m[15] - 
              m.m[0]  * m.m[7] * m.m[13] - 
              m.m[4]  * m.m[1] * m.m[15] + 
              m.m[4]  * m.m[3] * m.m[13] + 
              m.m[12] * m.m[1] * m.m[7] - 
              m.m[12] * m.m[3] * m.m[5],

              -m.m[0] * m.m[5] * m.m[11] + 
               m.m[0] * m.m[7] * m.m[9] + 
               m.m[4] * m.m[1] * m.m[11] - 
               m.m[4] * m.m[3] * m.m[9] - 
               m.m[8] * m.m[1] * m.m[7] + 
               m.m[8] * m.m[3] * m.m[5],

              -m.m[4]  * m.m[9] * m.m[14] + 
               m.m[4]  * m.m[10] * m.m[13] +
               m.m[8]  * m.m[5] * m.m[14] - 
               m.m[8]  * m.m[6] * m.m[13] - 
               m.m[12] * m.m[5] * m.m[10] + 
               m.m[12] * m.m[6] * m.m[9],

              m.m[0]  * m.m[9] * m.m[14] - 
              m.m[0]  * m.m[10] * m.m[13] - 
              m.m[8]  * m.m[1] * m.m[14] + 
              m.m[8]  * m.m[2] * m.m[13] + 
              m.m[12] * m.m[1] * m.m[10] - 
              m.m[12] * m.m[2] * m.m[9],

              -m.m[0]  * m.m[5] * m.m[14] + 
               m.m[0]  * m.m[6] * m.m[13] + 
               m.m[4]  * m.m[1] * m.m[14] - 
               m.m[4]  * m.m[2] * m.m[13] - 
               m.m[12] * m.m[1] * m.m[6] + 
               m.m[12] * m.m[2] * m.m[5],

              m.m[0] * m.m[5] * m.m[10] - 
              m.m[0] * m.m[6] * m.m[9] - 
              m.m[4] * m.m[1] * m.m[10] + 
              m.m[4] * m.m[2] * m.m[9] + 
              m.m[8] * m.m[1] * m.m[6] - 
              m.m[8] * m.m[2] * m.m[5] );

    float det = m.m[0] * inv.m[0] + m.m[1] * inv.m[4] + m.m[2] * inv.m[8] + m.m[3] * inv.m[12];
    det = 1.0f/det;
    for(int i = 0; i < 16; i++ ) inv.m[i] = inv.m[i] * det;

    return inv;
}

inline mat4x4 invert( mat4x4 const & src, int *status=0 )
{
    int   i, j, k, swap;
    float t, temp[4][4];


    for (i=0; i<4; i++)
	for (j=0; j<4; j++)
	    temp[i][j] = src[i*4+j];

    float inv[16];
    for( i=0; i<16; i++ ) inv[i] = 0.0f;
    inv[ 0] = 1.0f;
    inv[ 5] = 1.0f;
    inv[10] = 1.0f;
    inv[15] = 1.0f;

    for( i=0; i<4; i++ ) 
    	{
		// Look for largest element in column
		swap = i;
		for (j = i + 1; j < 4; j++) 
			if (fabsf(temp[j][i]) > fabsf(temp[i][i])) 
				swap = j;

		if (swap != i)
    	    {
			// Swap rows.
			for( k=0; k<4; k++ )
    	    	{
				t = temp[i][k];
				temp[i][k] = temp[swap][k];
				temp[swap][k] = t;

				t = inv[i*4+k];
				inv[i*4+k] = inv[swap*4+k];
				inv[swap*4+k] = t;
	    		}
			}

    	// pivot==0 -> singular matrix!
		if (temp[i][i] == 0)
        {
		    if( status ) status[0] = 0;
            return mat4x4(0.0f,0.0f,0.0f,0.0f,
                          0.0f,0.0f,0.0f,0.0f,
                          0.0f,0.0f,0.0f,0.0f,
                          0.0f,0.0f,0.0f,0.0f);
        }

		t = temp[i][i];
		t = 1.0f/t;
		for (k=0; k<4; k++ )
    			{
				temp[i][k] *= t;
				inv[i*4+k] *= t;
				}

		for( j=0; j<4; j++ )
			{
			if( j != i )
    	   		{
				t = temp[j][i];
				for( k=0; k<4; k++ )
    	    		{
					temp[j][k] -= temp[i][k]*t;
					inv[j*4+k] -= inv[i*4+k]*t;
					}
	    		}
			}
		}

    if( status ) status[0] = 1;

    return mat4x4( inv[ 0], inv[ 1], inv[ 2], inv[ 3],
                   inv[ 4], inv[ 5], inv[ 6], inv[ 7],
                   inv[ 8], inv[ 9], inv[10], inv[11],
                   inv[12], inv[13], inv[14], inv[15] );

}

inline mat4x4 extractRotation( mat4x4 const & m )
{
    return mat4x4( m.m[0], m.m[4], m.m[ 8], 0.0f,
                   m.m[1], m.m[5], m.m[ 9], 0.0f, 
                   m.m[2], m.m[6], m.m[10], 0.0f, 
                   0.0f, 0.0f, 0.0f, 1.0f );
}


inline vec3 extractTranslation( mat4x4 const & m )
{
    return vec3( m.m[3], m.m[7], m.m[11] );
}

inline vec3 extractScale( mat4x4 const & m )
{
    const vec3 v1a = vec3(1.0f,0.0f,0.0f);
    const vec3 v2a = vec3(0.0f,1.0f,0.0f);
    const vec3 v3a = vec3(0.0f,0.0f,1.0f);
    const vec3 v1b = (m*vec4(v1a,0.0f)).xyz();
    const vec3 v2b = (m*vec4(v2a,0.0f)).xyz();
    const vec3 v3b = (m*vec4(v3a,0.0f)).xyz();

    return vec3( length(v1b), length(v2b), length(v3b) );
}

inline vec3 extractRotationEuler( const mat4x4 & m )
{
    vec3 res = vec3(0.0f);
    if( m.m[0] == 1.0f )
    {
        res.x = atan2f( m.m[2], m.m[11] );
        res.y = 0.0f;
        res.z = 0.0f;

    }
    else if( m.m[0] == -1.0f)
    {
        res.x = atan2f(m.m[2], m.m[11] );
        res.y = 0.0f;
        res.z = 0.0f;
    }
    else 
    {

        res.x = atan2f(-m.m[9],m.m[10]);
        res.y = atan2f(m.m[8],sqrtf(m.m[9]*m.m[9]+m.m[10]*m.m[10]));
        res.z = atan2f(m.m[4],m.m[0]);
    }
    return res;
}

inline mat4x4 setIdentity( void )
{
    return mat4x4( 1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}


inline mat4x4 setRotationQuaternion( const vec4 & q )
{
    float ww = q[3]*q[3];
    float xx = q[0]*q[0];
    float yy = q[1]*q[1];
    float zz = q[2]*q[2];

    return mat4x4(  ww + xx - yy - zz,           2.0f*(q[0]*q[1] - q[3]*q[2]),  2.0f*(q[0]*q[2] + q[3]*q[1]),  0.0f,
                    2.0f*(q[0]*q[1]+q[3]*q[2]),  ww - xx + yy - zz,             2.0f*(q[1]*q[2] - q[3]*q[0]),  0.0f,
                    2.0f*(q[0]*q[2]-q[3]*q[1]),  2.0f*(q[1]*q[2] + q[3]*q[0]),  ww - xx - yy + zz,             0.0f,
                    0.0f,                        0.0f,                          0.0f,                          1.0f );
}

inline mat4x4 setRotationAxisAngle4( const vec3 & a, const float t )
{
    const float sint = sinf(t);
    const float cost = cosf(t);
    const float icost = 1.0f - cost;

    return mat4x4( a.x*a.x*icost + cost,
                   a.y*a.x*icost - sint*a.z,
                   a.z*a.x*icost + sint*a.y,  
                   0.0f,
                   a.x*a.y*icost + sint*a.z,
                   a.y*a.y*icost + cost,
                   a.z*a.y*icost - sint*a.x,
                   0.0f,
                   a.x*a.z*icost - sint*a.y,
                   a.y*a.z*icost + sint*a.x,
                   a.z*a.z*icost + cost,
                   0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline mat4x4 setRotationX( const float t )
{
    const float sint = sinf(t);
    const float cost = cosf(t);

    return mat4x4( 1.0f,  0.0f,  0.0f, 0.0f,
                   0.0f,  cost,  sint, 0.0f, 
                   0.0f, -sint,  cost, 0.0f,
                   0.0f,  0.0f,  0.0f, 1.0f );
}

inline mat4x4 setRotationY( const float t )
{
    const float sint = sinf(t);
    const float cost = cosf(t);

    return mat4x4( cost, 0.0f, -sint, 0.0f,
                   0.0f, 1.0f,  0.0f, 0.0f, 
                   sint, 0.0f,  cost, 0.0f,
                   0.0f, 0.0f,  0.0f, 1.0f );
}

inline mat4x4 setRotationZ( const float t )
{
    const float sint = sinf(t);
    const float cost = cosf(t);

    return mat4x4( cost, -sint, 0.0f, 0.0f, 
                   sint,  cost, 0.0f, 0.0f,
                   0.0f,  0.0f, 1.0f, 0.0f,
                   0.0f,  0.0f, 0.0f, 1.0f );
}

inline mat4x4 setRotationEuler4( const vec3 & r )
{
    const float a = sinf(r.x); 
    const float b = cosf(r.x); 
    const float c = sinf(r.y); 
    const float d = cosf(r.y); 
    const float e = sinf(r.z); 
    const float f = cosf(r.z); 
    const float ac = a*c;
    const float bc = b*c;

    return mat4x4( d*f,      d*e,       -c,  0.0f, 
                   ac*f-b*e, ac*e+b*f, a*d,  0.0f,
                   bc*f+a*e, bc*e-a*f, b*d,  0.0f,
                   0.0f,     0.0f,     0.0f, 1.0f );
}


inline mat4x4 setTranslation( const vec3 & p )
{
    return mat4x4( 1.0f, 0.0f, 0.0f, p.x,
                   0.0f, 1.0f, 0.0f, p.y,
                   0.0f, 0.0f, 1.0f, p.z,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline mat4x4 setScale4( const vec3 & s )
{
    return mat4x4( s.x,  0.0f, 0.0f, 0.0f,
                   0.0f, s.y,  0.0f, 0.0f,
                   0.0f, 0.0f, s.z,  0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline mat4x4 setIdentity4( void )
{
    return mat4x4( 1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline mat4x4 setRotationEuler4( float x, float y, float z )
{
    return setRotationEuler4( vec3(x,y,z) );
}


inline mat4x4 setSwapYZ( void )
{
    return mat4x4( 1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, -1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}


inline mat4x4 setTranslation( float x, float y, float z )
{
    return setTranslation( vec3(x,y,z) );
}

inline mat4x4 setScale4( float x, float y, float z )
{
    return setScale4( vec3(x,y,z) );
}

inline mat4x4 buildBase4( const vec3 & nn )
{
    vec3 up;
    if( fabsf(nn.z)<0.9f ) { up.x=0.0f; up.y=0.0f; up.z=1.0f; }
    else                   { up.x=1.0f; up.y=0.0f; up.z=0.0f; }
    const vec3 vv = normalize( cross( nn, up ) );
    const vec3 uu = normalize( cross( vv, nn ) );

    return mat4x4( uu.x, vv.x, nn.x, 0.0f, 
                   uu.y, vv.y, nn.y, 0.0f, 
                   uu.z, vv.z, nn.z, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline mat4x4 buildBase4( const vec3 & u, const vec3 & v, const vec3 & n  )
{
    return mat4x4( u.x,  v.x,  n.x,  0.0f,
                   u.y,  v.y,  n.y,  0.0f, 
                   u.z,  v.z,  n.z,  0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f );
}

inline float determinant( mat4x4 const & m )
{
    float inv0 = m[ 5]*m[10]*m[15] - 
                 m[ 5]*m[11]*m[14] - 
                 m[ 9]*m[ 6]*m[15] + 
                 m[ 9]*m[ 7]*m[14] +
                 m[13]*m[ 6]*m[11] - 
                 m[13]*m[ 7]*m[10];

    float inv4 = -m[ 4]*m[10]*m[15] + 
                  m[ 4]*m[11]*m[14] + 
                  m[ 8]*m[ 6]*m[15] - 
                  m[ 8]*m[ 7]*m[14] - 
                  m[12]*m[ 6]*m[11] + 
                  m[12]*m[ 7]*m[10];

    float inv8 = m[ 4]*m[ 9]*m[15] - 
                 m[ 4]*m[11]*m[13] - 
                 m[ 8]*m[ 5]*m[15] + 
                 m[ 8]*m[ 7]*m[13] + 
                 m[12]*m[ 5]*m[11] - 
                 m[12]*m[ 7]*m[ 9];

    float inv12 = -m[4 ]*m[ 9]*m[14] + 
                   m[4 ]*m[10]*m[13] +
                   m[8 ]*m[ 5]*m[14] - 
                   m[8 ]*m[ 6]*m[13] - 
                   m[12]*m[ 5]*m[10] + 
                   m[12]*m[ 6]*m[ 9];

  
    return m[0]*inv0 + m[1]*inv4 + m[2]*inv8 + m[3]*inv12;
}

inline mat4x4 setFrustumMat( float left, float right, float bottom, float top, float znear, float zfar )
{
   const float x = (2.0f * znear) / (right - left);
   const float y = (2.0f * znear) / (top - bottom);
   const float a = (right + left) / (right - left);
   const float b = (top + bottom) / (top - bottom);
   const float c = -(zfar + znear) / ( zfar - znear);
   const float d = -(2.0f * zfar * znear) / (zfar - znear);

   return mat4x4( x,     0.0f,  a,      0.0f,
                  0.0f,  y,     b,      0.0f,
                  0.0f,  0.0f,  c,      d,
                  0.0f,  0.0f,  -1.0f,  0.0f );
   // inverse is:
   //return mat4x4( 1.0/x, 0.0f,  0.0f,   a/x,
   //               0.0f,  1.0/y, 0.0f,   b/x,
   //               0.0f,  0.0f,  0.0f,   -1.0,
   //               0.0f,  0.0f,  1.0f/d, c/d );
}

inline vec2 getNearFar_FromPerspectiveMatrix( const mat4x4 & m )
{
    const float c = m.m[10];
    const float d = m.m[11];
    return vec2( d/(c-1.0f), d/(c+1.0f) );
}
// Zeye = a / (Zbuffer[0..1] + b);
inline vec2 getZBufferToZetaEye_FromPerspectiveMatrix( const mat4x4 & m )
{
    return vec2( m.m[11]/2.0f, (m.m[10]-1.0f)/2.0f );
}
// Zeye = a / (Zclip[-1..1] + b);
inline vec2 getZClipToZetaEye_FromPerspectiveMatrix( const mat4x4 & m )
{
    return vec2(0.0f,0.0f);//vec2( -m.m[11], m.m[10] );
}


inline mat4x4 setOrtho( float left, float right, float bottom, float top, float znear, float zfar )
{
   mat4x4 me;

   const float x = 2.0f / (right - left);
   const float y = 2.0f / (top - bottom);
   const float a = (right + left) / (right - left);
   const float b = (top + bottom) / (top - bottom);
   const float c = -2.0f / (zfar - znear);
   const float d = -(zfar + znear) / ( zfar - znear);

   me[ 0] = x;     me[ 1] = 0.0f;  me[ 2] = 0.0f;   me[ 3] = a;
   me[ 4] = 0.0f;  me[ 5] = y;     me[ 6] = 0.0f;   me[ 7] = b;
   me[ 8] = 0.0f;  me[ 9] = 0.0f;  me[10] = c;      me[11] = d;
   me[12] = 0.0f;  me[13] = 0.0f;  me[14] = 0.0f;   me[15] = 1.0f;

   return me;
}

inline mat4x4 setPerspective( float fovy, float aspect, float znear, float zfar )
{
#if 0
	const float ymax = znear * tanf(fovy * 3.141592653589f/180.0f );
	const float ymin = -ymax;
	const float xmin = ymin * aspect;
	const float xmax = ymax * aspect;
        
	return setFrustumMat( xmin, xmax, ymin, ymax, znear, zfar );
#else

    const float tan = tanf(fovy * 3.141592653589f/180.0f );
    const float x = 1.0f / (tan*aspect);
    const float y = 1.0f / (tan);
    const float c = -(zfar + znear) / ( zfar - znear);
    const float d = -(2.0f * zfar * znear) / (zfar - znear);

    return mat4x4( x,     0.0f,  0.0f,   0.0f,
                   0.0f,  y,     0.0f,   0.0f,
                   0.0f,  0.0f,  c,      d,
                   0.0f,  0.0f,  -1.0f,  0.0f );
   // inverse is:
   //return mat4x4( tan*aspect, 0.0f,  0.0f,   0.0f,
   //               0.0f,       tan,   0.0f,   0.0f,
   //               0.0f,       0.0f,  0.0f,  -1.0f,
   //               0.0f,       0.0f,  -(zfar-znear)/(2.0f*zfar*znear), (zfar+znear)/(2.0f*zfar*znear) );
#endif
}

inline mat4x4 setProjection( const vec4 & fov, float znear, float zfar )
{
#if 0
	const float ymax =  znear * fov.x;
	const float ymin = -znear * fov.y;
	const float xmin = -znear * fov.z;
	const float xmax =  znear * fov.w;

	return setFrustumMat( xmin, xmax, ymin, ymax, znear, zfar );
#else
    float x = 2.0f / (fov.w+fov.z);
    float y = 2.0f / (fov.x+fov.y);
    float a = (fov.w-fov.z) / (fov.w+fov.z);
    float b = (fov.x-fov.y) / (fov.x+fov.y);
    float c = -(zfar + znear) / ( zfar - znear);
    float d = -(2.0f*zfar*znear) / (zfar - znear);
    return mat4x4( x,     0.0f,  a,      0.0f,
                   0.0f,  y,     b,      0.0f,
                   0.0f,  0.0f,  c,      d,
                   0.0f,  0.0f,  -1.0f,  0.0f );
   // inverse is:
   //return mat4x4( 1.0/x, 0.0f,  0.0f,   a/x,
   //               0.0f,  1.0/y, 0.0f,   b/x,
   //               0.0f,  0.0f,  0.0f,   -1.0,
   //               0.0f,  0.0f,  1.0f/d, c/d );
#endif
}

inline mat4x4 setPerspectiveTiled(float fovy, float aspect, float znear, float zfar, const vec2 & off, const vec2 & wid)
{
    float ym = znear * tanf(fovy * 3.141592653589f / 180.0f);
    float xm = ym * aspect;

    const float xmin = -xm + 2.0f*xm*off.x;
    const float ymin = -ym + 2.0f*ym*off.y;
    const float xmax = xmin + 2.0f*xm*wid.x;
    const float ymax = ymin + 2.0f*ym*wid.y;

    return setFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);
}



//
// faceid:
//
//        -----
//        | 1 |
//   ------------------
//   | 0  | 2 | 4 | 5 |
//   ------------------
//        | 3 | 
//        -----
//
inline mat4x4 setCubeFaceTiled(int faceid, float znear, float zfar, const vec2 & off, const vec2 & wid)
{
    const float w = znear;

    float xmin = -w + 2.0f*w*off.x;
    float ymin = -w + 2.0f*w*off.y;
    float xmax = xmin + 2.0f*w*wid.x;
    float ymax = ymin + 2.0f*w*wid.y;


    mat4x4 m = setFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);

    if (faceid == 0) m = m * mat4x4(0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    if (faceid == 1) m = m * mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    if (faceid == 3) m = m * mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    if (faceid == 4) m = m * mat4x4(0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    if (faceid == 5) m = m * mat4x4(-1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    return m;
}


inline mat4x4 setLookat( const vec3 & eye, const vec3 & tar, const vec3 & up  )
{
    mat4x4 mat;

    float im;

    const float dir[3] = { -tar[0]+eye[0], -tar[1]+eye[1], -tar[2]+eye[2] };

    // right vector
	mat[0] = dir[2]*up[1] - dir[1]*up[2]; 
    mat[1] = dir[0]*up[2] - dir[2]*up[0];
    mat[2] = dir[1]*up[0] - dir[0]*up[1];
    im = 1.0f/sqrtf( mat[0]*mat[0] + mat[1]*mat[1] + mat[2]*mat[2] );
    mat[0] *= im;
    mat[1] *= im;
    mat[2] *= im;
    // up vector
	mat[4] = mat[2]*dir[1] - mat[1]*dir[2]; 
    mat[5] = mat[0]*dir[2] - mat[2]*dir[0];
    mat[6] = mat[1]*dir[0] - mat[0]*dir[1];
    im = 1.0f/sqrtf( mat[4]*mat[4] + mat[5]*mat[5] + mat[6]*mat[6] );
    mat[4] *= im;
    mat[5] *= im;
    mat[6] *= im;
    // view vector
	mat[ 8] = dir[0];
	mat[ 9] = dir[1];
	mat[10] = dir[2];
    im = 1.0f/sqrtf( mat[8]*mat[8] + mat[9]*mat[9] + mat[10]*mat[10] );
    mat[ 8] *= im;
    mat[ 9] *= im;
    mat[10] *= im;

	mat[ 3] = -(mat[0]*eye[0] + mat[1]*eye[1] + mat[ 2]*eye[2] );
	mat[ 7] = -(mat[4]*eye[0] + mat[5]*eye[1] + mat[ 6]*eye[2] );
	mat[11] = -(mat[8]*eye[0] + mat[9]*eye[1] + mat[10]*eye[2] );

    mat[12] = 0.0f;
    mat[13] = 0.0f;
    mat[14] = 0.0f;
	mat[15] = 1.0f;

    return mat;
}


//--------------------------------------------------------------------------------
// bound2
//--------------------------------------------------------------------------------

struct bound2
{
    float mMinX;
    float mMaxX;
    float mMinY;
    float mMaxY;

    bound2() {}

    explicit bound2(float mix, float max, float miy, float may)
    {
        mMinX = mix;
        mMaxX = max;
        mMinY = miy;
        mMaxY = may;
    }

    explicit bound2(float infi)
    {
        mMinX = infi;
        mMaxX = -infi;
        mMinY = infi;
        mMaxY = -infi;
    }


    explicit bound2(float const * const v)
    {
        mMinX = v[0];
        mMaxX = v[1];
        mMinY = v[2];
        mMaxY = v[3];
    }

    explicit bound2(float const * const vmin, float const * const vmax)
    {
        mMinX = vmin[0];
        mMaxX = vmax[0];
        mMinY = vmin[1];
        mMaxY = vmax[1];
    }

    explicit bound2(const vec2 & mi, const vec2 & ma)
    {
        mMinX = mi.x;
        mMaxX = ma.x;
        mMinY = mi.y;
        mMaxY = ma.y;
    }

    float & operator [](int i) { return ((float*)this)[i]; }
    const float & operator [](int i) const { return ((float*)this)[i]; }

};

inline bool contains(bound2 const & b, vec2 const & p)
{
    if (p.x < b.mMinX) return false;
    if (p.y < b.mMinY) return false;
    if (p.x > b.mMaxX) return false;
    if (p.y > b.mMaxY) return false;
    return true;
}
//--------------------------------------------------------------------------------
// bound3 
//--------------------------------------------------------------------------------

struct bound3
{
    float mMinX;
    float mMaxX;
    float mMinY;
    float mMaxY;
    float mMinZ;
    float mMaxZ;

	bound3() {}

	explicit bound3( float mix, float max, float miy, float may, float miz, float maz )
    {
        mMinX = mix;
        mMaxX = max;
        mMinY = miy;
        mMaxY = may;
        mMinZ = miz;
        mMaxZ = maz;
    }

	explicit bound3( float infi )
    {
        mMinX =  infi;
        mMaxX = -infi;
        mMinY =  infi;
        mMaxY = -infi;
        mMinZ =  infi;
        mMaxZ = -infi;
    }


	explicit bound3( float const * const v )
    {
        mMinX = v[0];
        mMaxX = v[1];
        mMinY = v[2];
        mMaxY = v[3];
        mMinZ = v[4];
        mMaxZ = v[5];
    }

	explicit bound3( float const * const vmin, float const * const vmax )
    {
        mMinX = vmin[0];
        mMaxX = vmax[0];
        mMinY = vmin[1];
        mMaxY = vmax[1];
        mMinZ = vmin[2];
        mMaxZ = vmax[2];
    }

	explicit bound3( const vec3 & mi, const vec3 & ma )
    {
        mMinX = mi.x;
        mMaxX = ma.x;
        mMinY = mi.y;
        mMaxY = ma.y;
        mMinZ = mi.z;
        mMaxZ = ma.z;
    }

          float & operator [](int i) { return ((float*)this)[i]; }
    const float & operator [](int i) const { return ((float*)this)[i]; }

};

inline bound3 expand( const bound3 & a, const bound3 & b )
{
    return bound3( a.mMinX+b.mMinX, a.mMaxX+b.mMaxX, 
                   a.mMinY+b.mMinY, a.mMaxY+b.mMaxY, 
                   a.mMinZ+b.mMinZ, a.mMaxZ+b.mMaxZ );
}

inline bound3 expand( const bound3 & a, float const b )
{
    return bound3( a.mMinX-b, a.mMaxX+b, 
                   a.mMinY-b, a.mMaxY+b, 
                   a.mMinZ-b, a.mMaxZ+b );
}

inline bound3 include( const bound3 & a, const vec3 & p )
{
    bound3 res = bound3(
        (p.x<a.mMinX) ? p.x : a.mMinX,
        (p.x>a.mMaxX) ? p.x : a.mMaxX,
        (p.y<a.mMinY) ? p.y : a.mMinY,
        (p.y>a.mMaxY) ? p.y : a.mMaxY,
        (p.z<a.mMinZ) ? p.z : a.mMinZ,
        (p.z>a.mMaxZ) ? p.z : a.mMaxZ );

    return res;
}

inline bound3 include( const bound3 & a, const bound3 & b )
{
    return bound3( fminf(a.mMinX,b.mMinX),
        fmaxf(a.mMaxX,b.mMaxX),
        fminf(a.mMinY,b.mMinY),
        fmaxf(a.mMaxY,b.mMaxY),
        fminf(a.mMinZ,b.mMinZ),
        fmaxf(a.mMaxZ,b.mMaxZ) );

}

inline float distance( const bound3 & b, const vec3 & p )
{
    const vec3 bc = 0.5f*vec3( b.mMaxX + b.mMinX, b.mMaxY + b.mMinY, b.mMaxZ + b.mMinZ );
    const vec3 br = 0.5f*vec3( b.mMaxX - b.mMinX, b.mMaxY - b.mMinY, b.mMaxZ - b.mMinZ );

    const vec3 d = abs(p-bc) - br;
    return fminf(fmaxf(d.x,fmaxf(d.y,d.z)),0.0f) + length(vmax(d,0.0f));
}

inline bool contains( bound3 const & b, vec3 const & p )
{
    if( p.x < b.mMinX ) return false;
    if( p.y < b.mMinY ) return false;
    if( p.z < b.mMinZ ) return false;
    if( p.x > b.mMaxX ) return false;
    if( p.y > b.mMaxY ) return false;
    if( p.z > b.mMaxZ ) return false;
    return true;
}

#if 0

// 0 = a and b are disjoint
// 1 = a and b intersect
// 2 = b is fully contained in a
// 3 = a is fully contained in b
inline int contains( bound3 const & a, bound3 const & b )
{
    int nBinA = containsp( a, vec3(b.mMinX,b.mMinY,b.mMinZ) ) +
                containsp( a, vec3(b.mMaxX,b.mMinY,b.mMinZ) ) +
                containsp( a, vec3(b.mMinX,b.mMaxY,b.mMinZ) ) +
                containsp( a, vec3(b.mMaxX,b.mMaxY,b.mMinZ) ) +
                containsp( a, vec3(b.mMinX,b.mMinY,b.mMaxZ) ) +
                containsp( a, vec3(b.mMaxX,b.mMinY,b.mMaxZ) ) +
                containsp( a, vec3(b.mMinX,b.mMaxY,b.mMaxZ) ) +
                containsp( a, vec3(b.mMaxX,b.mMaxY,b.mMaxZ) );

    int nAinB = containsp( b, vec3(a.mMinX,a.mMinY,a.mMinZ) ) +
                containsp( b, vec3(a.mMaxX,a.mMinY,a.mMinZ) ) +
                containsp( b, vec3(a.mMinX,a.mMaxY,a.mMinZ) ) +
                containsp( b, vec3(a.mMaxX,a.mMaxY,a.mMinZ) ) +
                containsp( b, vec3(a.mMinX,a.mMinY,a.mMaxZ) ) +
                containsp( b, vec3(a.mMaxX,a.mMinY,a.mMaxZ) ) +
                containsp( b, vec3(a.mMinX,a.mMaxY,a.mMaxZ) ) +
                containsp( b, vec3(a.mMaxX,a.mMaxY,a.mMaxZ) );

    if( nAinB==0 && nBinA==0 )   return 0;
    if( nAinB!=0 && nBinA!=0 )   return 1;
    if( nAinB==0 && nBinA!=0 )   return 2;
  /*if( nAinB!=0 && nBinA==0 )*/ return 3;
}
#endif

// 0 if they are disjoint
// 1 if they intersect 
inline int overlap( bound3 const & a, bound3 const & b )
{
    if( a.mMaxX < b.mMinX ) return 0;
    if( a.mMinX > b.mMaxX ) return 0;
    if( a.mMaxY < b.mMinY ) return 0;
    if( a.mMinY > b.mMaxY ) return 0;
    if( a.mMaxZ < b.mMinZ ) return 0;
    if( a.mMinZ > b.mMaxZ ) return 0;
    return 1;
}

inline bound3 compute( const vec3 * const p, const int num )
{
    bound3 res = bound3( p[0].x, p[0].x, 
                         p[0].y, p[0].y,
                         p[0].z, p[0].z );

    for( int k=1; k<num; k++ ) 
    {
        res.mMinX = (p[k].x<res.mMinX) ? p[k].x : res.mMinX;
        res.mMaxX = (p[k].x>res.mMaxX) ? p[k].x : res.mMaxX;
        res.mMinY = (p[k].y<res.mMinY) ? p[k].y : res.mMinY;
        res.mMaxY = (p[k].y>res.mMaxY) ? p[k].y : res.mMaxY;
        res.mMinZ = (p[k].z<res.mMinZ) ? p[k].z : res.mMinZ;
        res.mMaxZ = (p[k].z>res.mMaxZ) ? p[k].z : res.mMaxZ;
    }

    return res;
}

inline float diagonal( bound3 const & bbox )
{
    const float dx = bbox.mMaxX - bbox.mMinX;
    const float dy = bbox.mMaxY - bbox.mMinY;
    const float dz = bbox.mMaxZ - bbox.mMinZ;
    return sqrtf( dx*dx + dy*dy + dz*dz );
}

inline float volume( bound3 const & bbox )
{
    const float dx = bbox.mMaxX - bbox.mMinX;
    const float dy = bbox.mMaxY - bbox.mMinY;
    const float dz = bbox.mMaxZ - bbox.mMinZ;
    return dx*dy*dz;
}

inline vec3 getcenter( bound3 const & bbox )
{
    return vec3( 0.5f*(bbox.mMinX+bbox.mMaxX), 0.5f*(bbox.mMinY+bbox.mMaxY), 0.5f*(bbox.mMinZ+bbox.mMaxZ) );
}

inline vec3 getradiius( bound3 const & bbox )
{
    return vec3( 0.5f*(bbox.mMaxX-bbox.mMinX), 0.5f*(bbox.mMaxY-bbox.mMinY), 0.5f*(bbox.mMaxZ-bbox.mMinZ) );
}

inline bound3 btransform( bound3 const & bbox, const mat4x4 & m )
{
    vec3 p0 = vtransform( m, vec3( bbox.mMinX, bbox.mMinY, bbox.mMinZ ) );
    vec3 p1 = vtransform( m, vec3( bbox.mMaxX, bbox.mMinY, bbox.mMinZ ) );
    vec3 p2 = vtransform( m, vec3( bbox.mMinX, bbox.mMaxY, bbox.mMinZ ) );
    vec3 p3 = vtransform( m, vec3( bbox.mMaxX, bbox.mMaxY, bbox.mMinZ ) );
    vec3 p4 = vtransform( m, vec3( bbox.mMinX, bbox.mMinY, bbox.mMaxZ ) );
    vec3 p5 = vtransform( m, vec3( bbox.mMaxX, bbox.mMinY, bbox.mMaxZ ) );
    vec3 p6 = vtransform( m, vec3( bbox.mMinX, bbox.mMaxY, bbox.mMaxZ ) );
    vec3 p7 = vtransform( m, vec3( bbox.mMaxX, bbox.mMaxY, bbox.mMaxZ ) );

    bound3 res = bound3( p0, p0 );
    res = include( res, p1 );
    res = include( res, p2 );
    res = include( res, p3 );
    res = include( res, p4 );
    res = include( res, p5 );
    res = include( res, p6 );
    res = include( res, p7 );
    return res;
}

//--------------------------------------------------------------------------------
// frustum3 (6 mPlanes[anes defining a convex volume)
//--------------------------------------------------------------------------------

struct frustum3
{
    vec4   mPlanes[6];
    vec3   mPoints[8];
    mat4x4 mMatrix;

	frustum3() {}
/*
    // build a frustum from 6 mPlanes[anes
	explicit frustum3( vec4 const & a, vec4 const & b, 
                       vec4 const & c, vec4 const & d,
                       vec4 const & e, vec4 const & f ) 
    {
        mPlanes[0] = a;
        mPlanes[1] = b;
        mPlanes[2] = c;
        mPlanes[3] = d;
        mPlanes[4] = e;
        mPlanes[5] = f;
    }

    // build a frustum from its 8 corner points
	explicit frustum3( vec3 const & v0, vec3 const & v1, 
                       vec3 const & v2, vec3 const & v3,
                       vec3 const & v4, vec3 const & v5,
                       vec3 const & v6, vec3 const & v7 ) 
    {
        mPoints[0] = v0;
        mPoints[1] = v1;
        mPoints[2] = v2;
        mPoints[3] = v3;
        mPoints[4] = v4;
        mPoints[5] = v5;
        mPoints[6] = v6;
        mPoints[7] = v7;

        mPlanes[0] = makemPlanes[ane( v0, v1, v2 );
        mPlanes[1] = makemPlanes[ane( v7, v6, v5 );
        mPlanes[2] = makemPlanes[ane( v1, v5, v6 );
        mPlanes[3] = makemPlanes[ane( v0, v3, v7 );
        mPlanes[4] = makemPlanes[ane( v3, v2, v6 );
        mPlanes[5] = makemPlanes[ane( v0, v4, v5 );
    }
*/

    explicit frustum3( mat4x4 const & m )
    {
        mMatrix = m;

		mPlanes[0].x = mMatrix[12] - mMatrix[ 0];
		mPlanes[0].y = mMatrix[13] - mMatrix[ 1];
		mPlanes[0].z = mMatrix[14] - mMatrix[ 2];
		mPlanes[0].w = mMatrix[15] - mMatrix[ 3];
		mPlanes[1].x = mMatrix[12] + mMatrix[ 0];
		mPlanes[1].y = mMatrix[13] + mMatrix[ 1];
		mPlanes[1].z = mMatrix[14] + mMatrix[ 2];
		mPlanes[1].w = mMatrix[15] + mMatrix[ 3];
		mPlanes[2].x = mMatrix[12] + mMatrix[ 4];
		mPlanes[2].y = mMatrix[13] + mMatrix[ 5];
		mPlanes[2].z = mMatrix[14] + mMatrix[ 6];
		mPlanes[2].w = mMatrix[15] + mMatrix[ 7];
		mPlanes[3].x = mMatrix[12] - mMatrix[ 4];
		mPlanes[3].y = mMatrix[13] - mMatrix[ 5];
		mPlanes[3].z = mMatrix[14] - mMatrix[ 6];
		mPlanes[3].w = mMatrix[15] - mMatrix[ 7];
		mPlanes[4].x = mMatrix[12] - mMatrix[ 8];
		mPlanes[4].y = mMatrix[13] - mMatrix[ 9];
		mPlanes[4].z = mMatrix[14] - mMatrix[10];
		mPlanes[4].w = mMatrix[15] - mMatrix[11];
		mPlanes[5].x = mMatrix[12] + mMatrix[ 8];
		mPlanes[5].y = mMatrix[13] + mMatrix[ 9];
		mPlanes[5].z = mMatrix[14] + mMatrix[10];
		mPlanes[5].w = mMatrix[15] + mMatrix[11];

		for( int i=0; i<6; i++ )
		{
			mPlanes[i] *= inverseLength( mPlanes[i].xyz() );
		}

		mPoints[0] = intersectPlanes( mPlanes[1], mPlanes[2], mPlanes[4] ); // same as bellow, just that *zar/near
		mPoints[1] = intersectPlanes( mPlanes[0], mPlanes[2], mPlanes[4] );
		mPoints[2] = intersectPlanes( mPlanes[0], mPlanes[3], mPlanes[4] );
		mPoints[3] = intersectPlanes( mPlanes[1], mPlanes[3], mPlanes[4] );
		mPoints[4] = intersectPlanes( mPlanes[1], mPlanes[2], mPlanes[5] ); //  left, bottom, near = -right, -top, near
		mPoints[5] = intersectPlanes( mPlanes[0], mPlanes[2], mPlanes[5] ); // right, bottom, near =  right, -top, near
		mPoints[6] = intersectPlanes( mPlanes[0], mPlanes[3], mPlanes[5] ); // right,    top, near
		mPoints[7] = intersectPlanes( mPlanes[1], mPlanes[3], mPlanes[5] ); //  left,    top, near = -right, top, near
    }

};

inline vec3 getNearPoint( frustum3 const &fru, const vec2 & uv )
{
    return mix( mix(fru.mPoints[4],fru.mPoints[5],uv.x),
                mix(fru.mPoints[7],fru.mPoints[6],uv.x), uv.y );
}

inline frustum3 setFrustum( float left, float right, float bottom, float top, float znear, float zfar )
{
    const float x = (2.0f * znear) / (right - left);
    const float y = (2.0f * znear) / (top - bottom);
    const float a = (right + left) / (right - left);
    const float b = (top + bottom) / (top - bottom);
    const float c = -(zfar + znear) / ( zfar - znear);
    const float d = -(2.0f * zfar * znear) / (zfar - znear);

    return frustum3( mat4x4(   x, 0.0f,     a, 0.0f,
                            0.0f,    y,     b, 0.0f,
                            0.0f, 0.0f,     c,    d,
                            0.0f, 0.0f, -1.0f, 0.0f ) );
}

inline frustum3 setFrustumPerspective( float fovy, float aspect, float znear, float zfar )
{
	const float ymax = znear * tanf(fovy * 3.141592653589f/180.0f );
	const float ymin = -ymax;
	const float xmin = ymin * aspect;
	const float xmax = ymax * aspect;

	return setFrustum( xmin, xmax, ymin, ymax, znear, zfar );
}

inline frustum3 setFrustumProjection( const vec4 & fov, float znear, float zfar )
{
	const float ymax =  znear * fov.x;
	const float ymin = -znear * fov.y;
	const float xmin = -znear * fov.z;
	const float xmax =  znear * fov.w;

	return setFrustum( xmin, xmax, ymin, ymax, znear, zfar );
}

/*
inline frustum3 setPersectiveCheap( float fovy, float aspect, float znear, float zfar, float expansion=0.0f )
{
    frustum3 res;

        const float an = fovy * 0.5f * (3.141592653589f/180.0f);
        const float si = sinf(an);
        const float co = cosf(an);
        #if 0
        mPlanes[0] = vec4(  -co, 0.0f, si*aspect, 0.0f );
        mPlanes[1] = vec4(   co, 0.0f, si*aspect, 0.0f );
        mPlanes[2] = vec4( 0.0f,   co, si,        0.0f );
        mPlanes[3] = vec4( 0.0f,  -co, si,        0.0f );
        #else
        mPlanes[0] = vec4(  -co, 0.0f, si, 0.0f );
        mPlanes[1] = vec4(   co, 0.0f, si, 0.0f );
        mPlanes[2] = vec4( 0.0f,   co, si/aspect,        0.0f );
        mPlanes[3] = vec4( 0.0f,  -co, si/aspect,        0.0f );
        #endif
        mPlanes[4] = vec4( 0.0f, 0.0f, -1.0f,     zfar );
        mPlanes[5] = vec4( 0.0f, 0.0f,  1.0f,   -znear );

		//mPoints[0] = intersectPlanes( mPlanes[1], mPlanes[2], mPlanes[4] ); // same as bellow, just that *zar/near
		//mPoints[1] = intersectPlanes( mPlanes[0], mPlanes[2], mPlanes[4] );
		//mPoints[2] = intersectPlanes( mPlanes[0], mPlanes[3], mPlanes[4] );
		//mPoints[3] = intersectPlanes( mPlanes[1], mPlanes[3], mPlanes[4] );
		//mPoints[4] = vec3( -right, -top, near );
		//mPoints[5] = vec3(  right, -top, near );
		//mPoints[6] = vec3(  right,  top, near );
		//mPoints[7] = vec3( -right,  top, near );

        for( int i=0; i<6; i++ )
        {
            // normalize plane
            const float im = inverseLength( mPlanes[i].xyz() );
            mPlanes[i] *= im;
            // move plane outwards 
            //mPlane[i].w += expansion;
        }
}
*/

// 0: outside  1: inside/intersect
inline int boxInFrustum( frustum3 const &fru, bound3 const & box )
{
    float band = 0.0f;

    // check box outside/inside of frustum
	for( int i=0; i<6; i++ )
    {
        int out = 0;
    	out += ((dot( fru.mPlanes[i], vec4(box.mMinX, box.mMinY, box.mMinZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMaxX, box.mMinY, box.mMinZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMinX, box.mMaxY, box.mMinZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMaxX, box.mMaxY, box.mMinZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMinX, box.mMinY, box.mMaxZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMaxX, box.mMinY, box.mMaxZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMinX, box.mMaxY, box.mMaxZ, 1.0f) ) < -band )?1:0);
    	out += ((dot( fru.mPlanes[i], vec4(box.mMaxX, box.mMaxY, box.mMaxZ, 1.0f) ) < -band )?1:0);
		if( out==8 ) return 0;
	}

    // check frustum outside/inside box
    int out;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].x>(box.mMaxX+band))?1:0); if( out==8 ) return 0;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].x<(box.mMinX-band))?1:0); if( out==8 ) return 0;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].y>(box.mMaxY+band))?1:0); if( out==8 ) return 0;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].y<(box.mMinY-band))?1:0); if( out==8 ) return 0;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].z>(box.mMaxZ+band))?1:0); if( out==8 ) return 0;
    out=0; for( int i=0; i<8; i++ ) out+=((fru.mPoints[i].z<(box.mMinZ-band))?1:0); if( out==8 ) return 0;

    return 1;
}

} // namespace piLibs

#endif
