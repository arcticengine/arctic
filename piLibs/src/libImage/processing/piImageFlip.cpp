#include <malloc.h>
#include <string.h>

#include "../piImage.h"

namespace piLibs {

bool piImage_Flip( piImage *spr )
{
    const int xres = spr->GetXRes();
    const int yres = spr->GetYRes();
    const int bpp = spr->GetBpp();
    const int n = xres*bpp;
    
    unsigned char *aux = (unsigned char *)malloc( n );
    if( !aux )
        return false;

    unsigned char *b1 = (unsigned char *)spr->GetData();
    unsigned char *b2 = (unsigned char *)spr->GetData() + (yres - 1)*n;
    const int numl = yres>>1;
    for( int i=0; i<numl; i++ )
    {
        memcpy( aux, b1, n );
        memcpy( b1, b2, n );
        memcpy( b2, aux, n );
        b1 += n;
        b2 -= n;
    }

    free( aux );

    return true;
}

}