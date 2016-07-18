#include <string.h>         // memset()
#include <math.h>           //sin(), cos()

namespace piLibs {

static signed short coefs[16*2] = {
-1, 2,-1, 0, 3,-5,  0,  2,-3, 4,  1, 0, 1,-1, 0, 0,
 0,-3, 4,-1, 0, 9,-10,  0, 0,-9,  8, 1, 0, 3,-2, 0 };

static int imod( int x, int n )
{
    while( x<0 )
        x += n;

    return( x%n );
}

void piSpline( float *key, int num, int dim, int stride, float t, float *v, int derivada )
{
    int             i, j, size, k, kn;
    signed short    *co;
    float           b;

    size = stride;

    if( t<=0.0f )
        t += key[size*(num-1)];


	t = fmodf( t, key[size*(num-1)] );

    k = 0; while( key[k*size]<t ) k++;
    t = (t-key[ size*imod(k-1,num) ]) / (key[k*size]-key[size*imod(k-1,num)]);
    k--;

    memset( v, 0, dim*sizeof(float) );
    for( i=0; i<4; i++ )
        {
        kn = k+i-1;

        kn = imod( kn, num );

        co = coefs + 4*(derivada*4 + i);
        b  = 0.5f*(((co[0]*t + co[1])*t + co[2])*t + co[3]);

        for( j=0; j<dim; j++ )
            v[j] += b * key[kn*size+j+1];
        }
}


}