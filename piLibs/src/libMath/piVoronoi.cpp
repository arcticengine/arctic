#include <math.h>

namespace piLibs {

static __forceinline int f2i( float x ) { return (int)floorf(x); }

static __forceinline float hash2f( int n )
{
	n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return (1.0f/2147483647.0f) * (float)n;
}

void piVoronoi_2f_1f( const float *x, float *resD1, float *resP1, int *resID1, 
                                      float *resD2, float *resP2, int *resID2, 
                                      unsigned int pe )
{
    const int ix = f2i(x[0]);
    const int iy = f2i(x[1]);
    const float fx = x[0] - (float)ix;
    const float fy = x[1] - (float)iy;

    float di[2] = { 4.0f, 4.0f };

    for( int j=-1; j<=1; j+=1 )
    for( int i=-1; i<=1; i+=1 )
    {
        const int ci = (i + ix) & pe;
        const int cj = (j + iy) & pe;

        const float nxy[2] = { hash2f(ci*1234+cj), hash2f(cj*5432+ci) };

        const float dx = i - fx + nxy[0];
        const float dy = j - fy + nxy[1];
        const float ds = dx*dx + dy*dy;
	
        if( ds<di[0] )
        {
            resID2[0] = resID1[0];
            resID2[1] = resID1[1];
            resP2[0] = resP1[0];
            resP2[1] = resP1[1];

            resID1[0] = ci;
            resID1[1] = cj;
            resP1[0] = dx;
            resP1[1] = dy;

            di[1] = di[0];
            di[0] = ds;
        }
        else if( ds<di[1] )
        {
            resID2[0] = ci;
            resID2[1] = cj;
            resP2[0] = dx;
            resP2[1] = dy;

            di[1] = ds;
        }    
    }

    resD1[0] = di[0];
    resD2[0] = di[1];
}

}