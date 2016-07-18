#include <math.h>

static __inline float cuberoot( float x )
{
    if( x<0.0f )
        return -powf(-x,1.0f/3.0f);
    return powf(x,1.0f/3.0f);
}

void complexSquareRoot( float a, float b, float *res )
{
    float r = sqrtf( a*a+b*b );
    float y = sqrtf(.5f*(r-a));
    float x = .5f*b/y;

    res[0] = x;
    res[1] = y;
}

void complexMult( float *a, float *b, float *res )
{
    res[0] = a[0]*b[0] - a[1]*b[1];
    res[1] = a[0]*b[1] + a[1]*b[0];
}

void complexRealDiv( float a, float *b, float *res )
{
    float d = b[0]*b[0] + b[1]*b[1];
    d = 1.0f/d;

    res[0] = ( a*b[0])*d;
    res[1] = (-a*b[1])*d;
}



void complexSwap( float *a, float *b  )
{
    float tmp[2];
    
    tmp[0] = a[0];
    tmp[1] = a[1];

    a[0] = b[0];
    a[1] = b[1];
    b[0] = tmp[0];
    b[1] = tmp[1];
}


//-----------------------------

int solveCuadratic( float b, float c, float *res )
{
    float d = b*b-4.0f*c;

    if( d<0.0f )
        {
        res[0] = -b*.5f;
        res[1] = .5f*sqrtf( -d );
        res[2] = res[0];
        res[3] = -res[1];
        return 0;
        }

    d = sqrtf(d);

    res[0] = (-b + d)*.5f;
    res[2] = (-b - d)*.5f;

    return 2;
}


// Calculates the 3 roots of the cubic. Returns the number of real roots (1 or 3)
// 1 real root:
//    res[0] = real root 
//    res[2] = Re{root 2}
//    res[3] = Im{root 2}
//    res[4] = Re{root 3}
//    res[5] = Im{root 3}
// 3 real roots
//    res[0] = real root 1
//    res[2] = real root 2
//    res[4] = real root 3
int solveCubic( float b, float c, float d, float *res )
{
    // x^3 + a2·x^2 + a1·x + a0 = 0
    float p = (3.0f*c-b*b)/3.0f;
    float q = (9.0f*b*c - 27.0f*d - 2.0f*b*b*b)/27.0f;
    // x^3 + px + q = 0

    // x = w-(p/3w)
    // (w^3)^2 - q*(w^3) - (p^3)/27 = 0
    float h = q*q*.25f + p*p*p/27.0f;
    if( h>=0.0f )
    {
        // one single real solution!
        h = sqrtf(h);

        float r = (q*.5f) + h;
        float t = (q*.5f) - h;

        float s = cuberoot( r );
        float u = cuberoot( t );

        res[2*0+0] = (s + u) - (b/3.0f);
        res[2*0+1] = 0.0f;

        float re = -(s + u)*.5f - (b/3.0f);
        float im = (s-u)*(sqrtf(3.0f)*.5f);
        res[2*1+0] = re;
        res[2*1+1] = im;
        res[2*2+0] = re;
        res[2*2+1] = -im;

        return 1;
    }
    else
    {
        // three real solutions!

        const float sqrt3 = sqrtf(3.0f);
        const float ovsqrt27 = 1.0f/sqrtf(27.0f);
        const float ov3 = 1.0f/3.0f;

        float i = p*sqrtf( -p )*ovsqrt27;
        float j = cuberoot( i );

        float k = ov3*acosf( (q/(2.0f*i)) );

        float m = cosf(k);
        float n = sinf(k)*sqrt3;
        float s = -b*ov3;

        res[2*0+0] =  2.0f*j*m  + s;
        res[2*1+0] = -j*(m + n) + s;
        res[2*2+0] = -j*(m - n) + s;

        return 3;
    }

    return 1;
}


int solveQuartic( float b, float c, float d, float e, float *res )
{
    // x^4 + b·x^3 + c·x^2 + d·x + e = 0

    // x = w - 1/4;
    float f = c - (3.0f*b*b/8.0f);
    float g = d + (b*b*b/8.0f) - (.5f*b*c);
    float h = e - (3.0f*b*b*b*b/256.0f) + (b*b*c/16.0f) - (b*d*.25f);

    // x^4 + f·x^2 + g·x + h = 0
#if 0
    if( h==0.0f )
        {
        res[0] = -0.25f;
        int nqsols = solveCubic( 1.0, f, g, res+2 );
        res[2] -= .25f;
        res[4] -= .25f;
        return 1+nqsols;
        }
    else if( g==0 )
        {
        int ncsols = solveCuadratic( 1.0f, f, res );
        if( ncsols==2 )
            {
            res[0] = sqrtf( res[0] );
            res[2] = sqrtf( res[2] );
            res[4] = -res[0];
            res[6] = -res[2];
            return 4;
            }
        else
            {
            float p[2];
            float q[2];
            complexSquareRoot( res[0], res[1], p );
            complexSquareRoot( res[2], res[3], q );

            res[0] =  p[0];
            res[1] =  p[1];
            res[2] =  p[0];
            res[3] = -p[1];
            res[4] =  q[0];
            res[5] =  q[1];
            res[6] =  q[0];
            res[7] = -q[1];

            return 0;
            }
        }
#endif
    // general method

    int   nqsols;
    float qres[6];
    float qb = .5f*f;
    float qc = (f*f-4.0f*h)/16.0f;
    float qd = -g*g/64.0f;

    nqsols = solveCubic( qb, qc, qd, qres );

    if( nqsols==3 )
        {
        float p, q;
        p = sqrtf( qres[2] );
        q = sqrtf( qres[4] );

        float r = -g/(8.0f*p*q);
        float s = b*.25f;

        res[0] =  p + q + r -s;
        res[2] =  p - q - r -s;
        res[4] = -p + q - r -s;
        res[6] = -p - q + r -s;

        return 4;
        }
    else
        {
        float p[2];
        float q[2];
        float r[2];

        complexSquareRoot( qres[2], qres[3], p );
        complexSquareRoot( qres[4], qres[5], q );

        float pq[2];
        complexMult( p, q, pq );

        complexRealDiv( -g/8.0f, pq, r );

        float s = b*.25f;

        res[0] =  p[0] + q[0] + r[0] -s;
        res[1] =  p[1] + q[1] + r[1];
        res[2] =  p[0] - q[0] - r[0] -s;
        res[3] =  p[1] - q[1] - r[1];
        res[4] = -p[0] + q[0] - r[0] -s;
        res[5] = -p[1] + q[1] - r[1];
        res[6] = -p[0] - q[0] + r[0] -s;
        res[7] = -p[1] - q[1] + r[1];
        

        int i, j;
        for( i=0; i<4; i++ )
        for( j=i; j<4; j++ )
            {
            if( res[2*j+1]==0.0f )
                {
                complexSwap( res+2*i, res+2*j  );
                }
            }

        return 2;

        return 0;
        }
    
}

/*
void test( void )
{
    int res;
    float fres[4];
    
    LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "cubic" );
    //res = solveCubic( 1.0f, 1.0f, 1.0f, fres );
    res = solveCubic( -2.0f, -11.0f, 12.0f, fres );
    if( res==1 )
        LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "%f ", fres[0] ); 
    else
        LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "%f  %f   %f", fres[0], fres[2], fres[4] ); 
    
    LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "quartic" );
    //res = solveQuartic( 2.0f, -42.0f, -42.0f, 360.0f, fres ); // 4 reals :    2.932812124940269,    5.121864525939025,    -3.883473049098499,    -6.171203601780795
    res = solveQuartic( 2.0f, 1.0f, 4.0f, 5.0f, fres );         // 2 reals :    -1.2829418866006472,    -1.8331472365574277, (   0.5580445615790375   +i* 1.347070180188941), (   0.5580445615790375   -i* 1.347070180188941)
    //res = solveQuartic( 2.0f, 1.0f, 1.0f, 5.0f, fres );       // 0 reals : (    0.5950268009596706   &   i*1.0850753933098518) (    -1.5950268009596706   &   i*0.8489869141947539) (    -1.5950268009596706   &   i*-0.8489869141947539) (    0.5950268009596706   &   i*-1.0850753933098518)
    if( res==2 )
        LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "%f   %f", fres[0], fres[2] ); 
    else if( res==4 )
        LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "%f  %f   %f    %f", fres[0], fres[2], fres[4], fres[6] ); 
    else 
        LOG_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, "no real solutions" );
}
*/