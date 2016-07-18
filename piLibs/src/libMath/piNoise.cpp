#include <math.h>
#include "piVecTypes.h"

//=========================================================================================================

namespace piLibs {

#define makeIndex(a,b,c,d) ((a)+(b)*57+(c)*113+(d)*263)

static __forceinline float hash2f( int n )
{
	n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return (1.0f/2147483647.0f) * (float)n;
}

static __forceinline float hash2sf( int n )
{
	n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return -1.0f + (2.0f/2147483647.0f) * (float)n;
}

static __forceinline float hash2vec1( int hash, float x ) 
{
    const int h = hash & 15;
    float grad = 1.0f + (h & 7);
    if (h&8) grad = -grad;
    return grad*x;
}

static __forceinline float hash2vec2( int n, float x, float y ) 
{
    n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589);

    float res = (n & 0x10000000)?x:y;
    if( n & 0x20000000 ) res = -res;

    return res;
}

static __forceinline float hash2vec3( int n, float x, float y, float z ) 
{
    n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589);

	const int h = (n>>25) & 15;
    const float u = h<8 ? x : y;
    const float v = h<4 ? y : h==12||h==14 ? x : z;
    const float res = ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);

    return res;
}

static __forceinline float grad3( int hash, float x, float y, float z ) 
{
    const int h = hash & 15;
    float u = h<8 ? x : y;
    float v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

static __forceinline float grad2( int hash, float x, float y ) 
{
    const int h = hash & 15;
    float u = h<8 ? x : y;
    float v = h<4 ? y : h==12||h==14 ? x : 0.0f;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

static __forceinline int f2i( float x ) { return (int)floorf(x); }

static __forceinline float smoothCub_f( float x ) { return x*x*3.0f - 2.0f*x; }
static __forceinline float smoothQui_f( float x ) { return x*x*x*(10.0f+x*(6.0f*x-15.0f)); }

static __forceinline float smoothQuiDer_f( float x ) { return 30.0f*x*x*(x*(x-2.0f)+1.0f); }


static const unsigned char piNoise_PermLUT[] = {
  151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

  151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

//=========================================================================================================

float piNoise_1f_Grad_Hash( const float x )
{
    const int   i = f2i(x);
    const float u = x-(float)i;
    const float w = smoothQui_f(u);

    const int n = makeIndex(i,0,0,0);

    const float a = hash2vec1( n+makeIndex(0,0,0,0), w      );
    const float b = hash2vec1( n+makeIndex(1,0,0,0), w-1.0f );

    return a + (b-a)*w;
}

float piNoise_1f_Grad_Lut( const float x )
{
    const int ix0 = f2i( x );
    const float fx0 = x - ix0;
    const float fx1 = fx0 - 1.0f;
    const int ix1 = ( ix0+1 );

    float s = smoothQui_f( fx0 );

    const float a = hash2vec1( piNoise_PermLUT[ ix0 ], fx0 );
    const float b = hash2vec1( piNoise_PermLUT[ ix1 ], fx1 );

    return a + s*(b-a);
}

float piNoise_1f_Grad_Hash( const vec2  & x, const int pe )
{
    const int   i[2] = { f2i(x[0]), f2i(x[1]) };
    const float u[2] = { x[0]-(float)i[0], x[1]-(float)i[1] };
    const float w[2] = { smoothQui_f(u[0]), smoothQui_f(u[1]) };

    int A = piNoise_PermLUT[(i[0]  )&pe]+(i[1])&pe, AA = piNoise_PermLUT[A], AB = piNoise_PermLUT[A+1];
    int B = piNoise_PermLUT[(i[0]+1)&pe]+(i[1])&pe, BA = piNoise_PermLUT[B], BB = piNoise_PermLUT[B+1];

    const float a = hash2vec2( AA  , u[0]     , u[1]      );
    const float b = hash2vec2( BA  , u[0]-1.0f, u[1]      );
    const float c = hash2vec2( AB  , u[0]     , u[1]-1.0f );
    const float d = hash2vec2( BB  , u[0]-1.0f, u[1]-1.0f );

    return a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w[0]*w[1];
}

vec3 piNoise_3f_3f_Grad_Hash( const vec3 & x )
{
    const int   i[3] = { f2i(x[0]), f2i(x[1]), f2i(x[2]) };
    const float u[3] = { x[0]-(float)i[0],   x[1]-(float)i[1],   x[2]-(float)i[2] };
    const float w[3] = { smoothQui_f(u[0]),      smoothQui_f(u[1]),      smoothQui_f(u[2]) };

    const int n = makeIndex(i[0],i[1],i[2],0);
    vec3 r;

    {
    const float a = hash2vec3( n+makeIndex(0,0,0,0), w[0]     , w[1]     , w[2]      );
    const float b = hash2vec3( n+makeIndex(1,0,0,0), w[0]-1.0f, w[1]     , w[2]      );
    const float c = hash2vec3( n+makeIndex(0,1,0,0), w[0]     , w[1]-1.0f, w[2]      );
    const float d = hash2vec3( n+makeIndex(1,1,0,0), w[0]-1.0f, w[1]-1.0f, w[2]      );
    const float e = hash2vec3( n+makeIndex(0,0,1,0), w[0]     , w[1]     , w[2]-1.0f );
    const float f = hash2vec3( n+makeIndex(1,0,1,0), w[0]-1.0f, w[1]     , w[2]-1.0f );
    const float g = hash2vec3( n+makeIndex(0,1,1,0), w[0]     , w[1]-1.0f, w[2]-1.0f );
    const float h = hash2vec3( n+makeIndex(1,1,1,0), w[0]-1.0f, w[1]-1.0f, w[2]-1.0f );

    r[0] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w[0]*w[1] + (f-e-b+a)*w[0]*w[2] + (g-e-c+a)*w[1]*w[2]   +   (e-f-g+h-a+b+c-d)*w[0]*w[1]*w[2];
    }

    {
    const float a = hash2vec3( n+makeIndex(0,0,0,1), w[0]     , w[1]     , w[2]      );
    const float b = hash2vec3( n+makeIndex(1,0,0,1), w[0]-1.0f, w[1]     , w[2]      );
    const float c = hash2vec3( n+makeIndex(0,1,0,1), w[0]     , w[1]-1.0f, w[2]      );
    const float d = hash2vec3( n+makeIndex(1,1,0,1), w[0]-1.0f, w[1]-1.0f, w[2]      );
    const float e = hash2vec3( n+makeIndex(0,0,1,1), w[0]     , w[1]     , w[2]-1.0f );
    const float f = hash2vec3( n+makeIndex(1,0,1,1), w[0]-1.0f, w[1]     , w[2]-1.0f );
    const float g = hash2vec3( n+makeIndex(0,1,1,1), w[0]     , w[1]-1.0f, w[2]-1.0f );
    const float h = hash2vec3( n+makeIndex(1,1,1,1), w[0]-1.0f, w[1]-1.0f, w[2]-1.0f );

    r[1] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w[0]*w[1] + (f-e-b+a)*w[0]*w[2] + (g-e-c+a)*w[1]*w[2]   +   (e-f-g+h-a+b+c-d)*w[0]*w[1]*w[2];
    }

    {
    const float a = hash2vec3( n+makeIndex(0,0,0,2), w[0]     , w[1]     , w[2]      );
    const float b = hash2vec3( n+makeIndex(1,0,0,2), w[0]-1.0f, w[1]     , w[2]      );
    const float c = hash2vec3( n+makeIndex(0,1,0,2), w[0]     , w[1]-1.0f, w[2]      );
    const float d = hash2vec3( n+makeIndex(1,1,0,2), w[0]-1.0f, w[1]-1.0f, w[2]      );
    const float e = hash2vec3( n+makeIndex(0,0,1,2), w[0]     , w[1]     , w[2]-1.0f );
    const float f = hash2vec3( n+makeIndex(1,0,1,2), w[0]-1.0f, w[1]     , w[2]-1.0f );
    const float g = hash2vec3( n+makeIndex(0,1,1,2), w[0]     , w[1]-1.0f, w[2]-1.0f );
    const float h = hash2vec3( n+makeIndex(1,1,1,2), w[0]-1.0f, w[1]-1.0f, w[2]-1.0f );

    r[2] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w[0]*w[1] + (f-e-b+a)*w[0]*w[2] + (g-e-c+a)*w[1]*w[2]   +   (e-f-g+h-a+b+c-d)*w[0]*w[1]*w[2];
    }

    return r;
}

float piNoise_1f_Grad_Hash( const vec3 & x )
{
    const int   i[3] = { f2i(x[0]), f2i(x[1]), f2i(x[2]) };
    const float u[3] = { x[0]-(float)i[0], x[1]-(float)i[1], x[2]-(float)i[2] };
    const float w[3] = { smoothQui_f(u[0]), smoothQui_f(u[1]), smoothQui_f(u[2]) };

    int A = piNoise_PermLUT[(i[0]  )&255]+i[1], AA = piNoise_PermLUT[(A)&255]+i[2], AB = piNoise_PermLUT[(A+1)&255]+i[2];
    int B = piNoise_PermLUT[(i[0]+1)&255]+i[1], BA = piNoise_PermLUT[(B)&255]+i[2], BB = piNoise_PermLUT[(B+1)&255]+i[2];

    const float a = hash2vec3( AA  , u[0]     , u[1]     , u[2]      );
    const float b = hash2vec3( BA  , u[0]-1.0f, u[1]     , u[2]      );
    const float c = hash2vec3( AB  , u[0]     , u[1]-1.0f, u[2]      );
    const float d = hash2vec3( BB  , u[0]-1.0f, u[1]-1.0f, u[2]      );
    const float e = hash2vec3( AA+1, u[0]     , u[1]     , u[2]-1.0f );
    const float f = hash2vec3( BA+1, u[0]-1.0f, u[1]     , u[2]-1.0f );
    const float g = hash2vec3( AB+1, u[0]     , u[1]-1.0f, u[2]-1.0f );
    const float h = hash2vec3( BB+1, u[0]-1.0f, u[1]-1.0f, u[2]-1.0f );

    return a + (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2] + (a-b-c+d)*w[0]*w[1] + (f-e-b+a)*w[0]*w[2] + (g-e-c+a)*w[1]*w[2] + (e-f-g+h-a+b+c-d)*w[0]*w[1]*w[2];
}

//=========================================================================================================

float piNoise_1f_Value_Hash( const float x )
{
	const int   i = { f2i(x) };
	const float u = { x-(float)i };
    const float w = { smoothQui_f(u) };

    const float a = hash2sf( i+makeIndex(0,0,0,0) );
    const float b = hash2sf( i+makeIndex(1,0,0,0) );

    return a + (b-a)*w;
}

vec2 piNoise_2f_Value_Hash( const float x )
{
	const int   i = { f2i(x) };
	const float u = { x-(float)i };
    const float w = { smoothQui_f(u) };

    vec2 r;

	{
    const float a = hash2sf( i+makeIndex(0,0,0,0) );
    const float b = hash2sf( i+makeIndex(1,0,0,0) );
    r[0] = a + (b-a)*w;
	}
	{
    const float a = hash2sf( i+makeIndex(0,0,0,1) );
    const float b = hash2sf( i+makeIndex(1,0,0,1) );
    r[1] = a + (b-a)*w;
	}

    return r;
}

vec3 piNoise_1f_3f_Value_Hash( const float x )
{
	const int   i = { f2i(x) };
	const float u = { x-(float)i };
    const float w = { smoothQui_f(u) };

    vec3 r;
	{
    const float a = hash2sf( i+makeIndex(0,0,0,0) );
    const float b = hash2sf( i+makeIndex(1,0,0,0) );
    r[0] = a + (b-a)*w;
	}
	{
    const float a = hash2sf( i+makeIndex(0,0,0,1) );
    const float b = hash2sf( i+makeIndex(1,0,0,1) );
    r[1] = a + (b-a)*w;
	}
	{
    const float a = hash2sf( i+makeIndex(0,0,0,2) );
    const float b = hash2sf( i+makeIndex(1,0,0,2) );
    r[2] = a + (b-a)*w;
	}

    return r;
}

vec2 piNoise_2f_Value_Hash( const vec2 & x )
{
	const int   i[2] = { f2i(x[0]), f2i(x[1]) };
	const float u[2] = { x[0]-(float)i[0],   x[1]-(float)i[1] };
    const float w[2] = { smoothQui_f(u[0]),      smoothQui_f(u[1]) };

    const float w01 = w[0]*w[1];

    const int n = makeIndex(i[0],i[1],0,0);

    vec2 r;

	{
    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );
    r[0] = a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w01;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,1) );
    const float b = hash2sf( n+makeIndex(1,0,0,1) );
    const float c = hash2sf( n+makeIndex(0,1,0,1) );
    const float d = hash2sf( n+makeIndex(1,1,0,1) );
    r[1] = a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w01;
	}

    return r;
}

float piNoise_1f_Value_Hash( const vec2 & x )
{
	const int   i[2] = { f2i(x[0]), f2i(x[1]) };
	const float u[2] = { x[0]-(float)i[0],   x[1]-(float)i[1] };
    const float w[2] = { smoothQui_f(u[0]),      smoothQui_f(u[1]) };

    const int n = makeIndex(i[0],i[1],0,0);

    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );

    return a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w[0]*w[1];
}

vec3 piNoise_3f_Value_Hash( const vec2 & x )
{
	const int   i[2] = { f2i(x[0]), f2i(x[1]) };
	const float u[2] = { x[0]-(float)i[0],   x[1]-(float)i[1] };
    const float w[2] = { smoothCub_f(u[0]),      smoothCub_f(u[1]) };

    const float w01 = w[0]*w[1];

    const int n = makeIndex(i[0],i[1],0,0);

    vec3 r;

	{
    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );
    r[0] = a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w01;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,1) );
    const float b = hash2sf( n+makeIndex(1,0,0,1) );
    const float c = hash2sf( n+makeIndex(0,1,0,1) );
    const float d = hash2sf( n+makeIndex(1,1,0,1) );
    r[1] = a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w01;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,2) );
    const float b = hash2sf( n+makeIndex(1,0,0,2) );
    const float c = hash2sf( n+makeIndex(0,1,0,2) );
    const float d = hash2sf( n+makeIndex(1,1,0,2) );
    r[2] = a + (b-a)*w[0] + (c-a)*w[1] + (a-b-c+d)*w01;
	}

    return r;
}



float piNoise_1f_Value_Hash( const vec3 & x )
{
	const int   i[3] = { f2i(x[0]), f2i(x[1]), f2i(x[2]) };
	const float u[3] = { x[0]-(float)i[0],   x[1]-(float)i[1],   x[2]-(float)i[2] };
    const float w[3] = { smoothQui_f(u[0]),      smoothQui_f(u[1]),      smoothQui_f(u[2]) };

    const int n = makeIndex(i[0],i[1],i[2],0);

    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );
    const float e = hash2sf( n+makeIndex(0,0,1,0) );
    const float f = hash2sf( n+makeIndex(1,0,1,0) );
    const float g = hash2sf( n+makeIndex(0,1,1,0) );
    const float h = hash2sf( n+makeIndex(1,1,1,0) );

    return a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w[0]*w[1] + (f-e-b+a)*w[0]*w[2] + (g-e-c+a)*w[1]*w[2]   +   (e-f-g+h-a+b+c-d)*w[0]*w[1]*w[2];
}

vec2 piNoise_3f_2f_Value_Hash( const vec3 & x )
{
	const int   i[3] = { f2i(x[0]), f2i(x[1]), f2i(x[2]) };
	const float u[3] = { x[0]-(float)i[0],   x[1]-(float)i[1],   x[2]-(float)i[2] };
    const float w[3] = { smoothQui_f(u[0]),      smoothQui_f(u[1]),      smoothQui_f(u[2]) };

	const float w01 = w[0]*w[1];
	const float w12 = w[1]*w[2];
	const float w20 = w[2]*w[0];
	const float w012 = w[0]*w[1]*w[2];

    const int n = makeIndex(i[0],i[1],i[2],0);

    vec2 r;

	{
    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );
    const float e = hash2sf( n+makeIndex(0,0,1,0) );
    const float f = hash2sf( n+makeIndex(1,0,1,0) );
    const float g = hash2sf( n+makeIndex(0,1,1,0) );
    const float h = hash2sf( n+makeIndex(1,1,1,0) );
    r[0] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w01 + (f-e-b+a)*w20 + (g-e-c+a)*w12   +   (e-f-g+h-a+b+c-d)*w012;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,1) );
    const float b = hash2sf( n+makeIndex(1,0,0,1) );
    const float c = hash2sf( n+makeIndex(0,1,0,1) );
    const float d = hash2sf( n+makeIndex(1,1,0,1) );
    const float e = hash2sf( n+makeIndex(0,0,1,1) );
    const float f = hash2sf( n+makeIndex(1,0,1,1) );
    const float g = hash2sf( n+makeIndex(0,1,1,1) );
    const float h = hash2sf( n+makeIndex(1,1,1,1) );
    r[1] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w01 + (f-e-b+a)*w20 + (g-e-c+a)*w12   +   (e-f-g+h-a+b+c-d)*w012;
	}

    return r;
}

vec3 piNoise_3f_Value_Hash( const vec3 & x )
{
	const int   i[3] = { f2i(x[0]), f2i(x[1]), f2i(x[2]) };
	const float u[3] = { x[0]-(float)i[0],   x[1]-(float)i[1],   x[2]-(float)i[2] };
    const float w[3] = { smoothQui_f(u[0]),      smoothQui_f(u[1]),      smoothQui_f(u[2]) };

	const float w01 = w[0]*w[1];
	const float w12 = w[1]*w[2];
	const float w20 = w[2]*w[0];
	const float w012 = w[0]*w[1]*w[2];

    const int n = makeIndex(i[0],i[1],i[2],0);

    vec3 r;

	{
    const float a = hash2sf( n+makeIndex(0,0,0,0) );
    const float b = hash2sf( n+makeIndex(1,0,0,0) );
    const float c = hash2sf( n+makeIndex(0,1,0,0) );
    const float d = hash2sf( n+makeIndex(1,1,0,0) );
    const float e = hash2sf( n+makeIndex(0,0,1,0) );
    const float f = hash2sf( n+makeIndex(1,0,1,0) );
    const float g = hash2sf( n+makeIndex(0,1,1,0) );
    const float h = hash2sf( n+makeIndex(1,1,1,0) );
    r[0] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w01 + (f-e-b+a)*w20 + (g-e-c+a)*w12   +   (e-f-g+h-a+b+c-d)*w012;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,1) );
    const float b = hash2sf( n+makeIndex(1,0,0,1) );
    const float c = hash2sf( n+makeIndex(0,1,0,1) );
    const float d = hash2sf( n+makeIndex(1,1,0,1) );
    const float e = hash2sf( n+makeIndex(0,0,1,1) );
    const float f = hash2sf( n+makeIndex(1,0,1,1) );
    const float g = hash2sf( n+makeIndex(0,1,1,1) );
    const float h = hash2sf( n+makeIndex(1,1,1,1) );
    r[1] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w01 + (f-e-b+a)*w20 + (g-e-c+a)*w12   +   (e-f-g+h-a+b+c-d)*w012;
	}
	{
    const float a = hash2sf( n+makeIndex(0,0,0,2) );
    const float b = hash2sf( n+makeIndex(1,0,0,2) );
    const float c = hash2sf( n+makeIndex(0,1,0,2) );
    const float d = hash2sf( n+makeIndex(1,1,0,2) );
    const float e = hash2sf( n+makeIndex(0,0,1,2) );
    const float f = hash2sf( n+makeIndex(1,0,1,2) );
    const float g = hash2sf( n+makeIndex(0,1,1,2) );
    const float h = hash2sf( n+makeIndex(1,1,1,2) );
    r[2] = a   +   (b-a)*w[0] + (c-a)*w[1] + (e-a)*w[2]   +   (a-b-c+d)*w01 + (f-e-b+a)*w20 + (g-e-c+a)*w12   +   (e-f-g+h-a+b+c-d)*w012;
	}

    return r;
}

}