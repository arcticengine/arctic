#include <string.h>

namespace piLibs {

static void radix( unsigned int byte, unsigned int N, unsigned int *source, unsigned int *dest )
{
    int count[256];
    int index[256];

    memset(count, 0, 256 * sizeof(int));

    for( unsigned int i=0; i<N; i++ )
    {
        unsigned int b = source[i*2+1];
        unsigned int d = (b>>byte)&0xff;
        count[d]++;
    }

    index[0]=0;
    for( unsigned int i=1; i<256; i++ )
        index[i]=index[i-1]+count[i-1];

    for( unsigned int i=0; i<N; i++ )
    {
        unsigned int b  = source[i*2+1];
        unsigned int e  = (b>>byte)&0xff;
        unsigned int d  = index[e]++;

        dest[2*d+0] = source[2*i+0];
        dest[2*d+1] = source[2*i+1];
    }
}

void piRadixSort4( unsigned int *source, unsigned int *temp, unsigned int n )
{
    radix( 0*8, n, source, temp );
    radix( 1*8, n, temp, source );
    radix( 2*8, n, source, temp );
    radix( 3*8, n, temp, source );
}

void piRadixSort2( unsigned int *source, unsigned int *temp, unsigned int n )
{
    radix( 0*8, n, source, temp );
    radix( 1*8, n, temp, source );
}


}