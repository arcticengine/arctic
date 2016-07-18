#include "piDraw.h"
#include "../../libImage/color/piIColor.h"

namespace piLibs {


void piDraw_BarFill24(   unsigned char *buffer, int xres, int yres, int xo, int yo, int w, int h, const unsigned char *color, int mode )
{
    int     i;

    // early skip
    if( xo+w<0 ) return;
    if( yo+h<0 ) return;
    if( xo>=xres ) return;
    if( yo>=yres ) return;


    // left clip
    if( xo<0 )
        {
        w += xo;
        xo = 0;
        }
    // right clip
    if( xo+w>=xres )
        {
        w -= (xo+w) - (xres-1);
        }
    // up clip
    if( yo<0 )
        {
        h += yo;
        yo = 0;
        }
    // bottom clip
    if( yo+h>=yres )
        {
        h -= (yo+h) - (yres-1);
        }

    buffer += (xres*yo+xo)*3;

    switch( mode )
        {
        case DM_SOLID:
            while( h-- )
    	        {
    	        for( i=0; i<w; i++ )
                {
                    buffer[3*i+0] = color[0];
                    buffer[3*i+1] = color[1];
                    buffer[3*i+2] = color[2];
                }
    	        buffer+=xres*3;
    	        }
            break;

        case DM_BLEND:
            break;
        }
}

void piDraw_BarFill32( unsigned int *buffer, int xres, int yres, int xo, int yo, int w, int h, unsigned int color, int mode )
{
    int     i;


    // early skip
    if( xo+w<0 ) return;
    if( yo+h<0 ) return;
    if( xo>=xres ) return;
    if( yo>=yres ) return;


    // left clip
    if( xo<0 )
        {
        w += xo;
        xo = 0;
        }
    // right clip
    if( xo+w>=xres )
        {
        w -= (xo+w) - (xres-1);
        }
    // up clip
    if( yo<0 )
        {
        h += yo;
        yo = 0;
        }
    // bottom clip
    if( yo+h>=yres )
        {
        h -= (yo+h) - (yres-1);
        }

    buffer +=xres*yo+xo;

    switch( mode )
        {
        case DM_SOLID:
            while( h-- )
    	        {
    	        for( i=0; i<w; i++ )
                    buffer[i] = color;
    	        buffer+=xres;
    	        }
            break;

        case DM_BLEND:
            while( h-- )
    	        {
    	        for( i=0; i<w; i++ )
                    buffer[i] = piIColor_FastBlend( buffer[i], color );
    	        buffer+=xres;
    	        }
            break;
        }
}

void piDraw_BarBorder( unsigned int *buffer, int xres, int yres, int xo, int yo, int w, int h, unsigned int color, int mode )
{
    int     i, j;

    int  dl = 1;
    int  dr = 1;
    int  du = 1;
    int  db = 1;

    // early skip
    if( xo+w<0 ) return;
    if( yo+h<0 ) return;
    if( xo>=xres ) return;
    if( yo>=yres ) return;

    // left clip
    if( xo<0 )
        {
        w += xo;
        xo = 0;
        dl = 0;
        }
    // right clip
    if( xo+w>=xres )
        {
        w -= (xo+w) - (xres-1);
        dr = 0;
        }
    // up clip
    if( yo<0 )
        {
        h += yo;
        yo = 0;
        du = 0;
        }
    // bottom clip
    if( yo+h>=yres )
        {
        h -= (yo+h) - (yres-1);
        db = 0;
        }


    buffer += xres*yo+xo;

    switch( mode )
        {
        case DM_SOLID:

            if( du )
                for( i=0; i<w; i++ )
                    buffer[i] = color;

            if( dl )
                for( j=0; j<h; j++ )
                    buffer[xres*j+0] = color;
            if( dr )
                for( j=0; j<h; j++ )
                    buffer[xres*j+w-1] = color;

            if( db )
                for( i=0; i<w; i++ )
                    buffer[xres*h+i] = color;

            break;

        case DM_BLEND:

            if( du )
                for( i=0; i<w; i++ )
                    buffer[i] = piIColor_FastBlend( color, buffer[i] );

            if( dl )
                for( j=0; j<h; j++ )
                    buffer[xres*j+0] = piIColor_FastBlend( color, buffer[xres*j+0] );
            if( dr )
                for( j=0; j<h; j++ )
                    buffer[xres*j+w-1] = piIColor_FastBlend( color, buffer[xres*j+w-1] );

            if( db )
                for( i=0; i<w; i++ )
                    buffer[xres*h+i] = piIColor_FastBlend( color, buffer[xres*h+i] );

            break;
        }
}



void piDraw_Line( unsigned int *buffer, int xres, int yres, int x1, int y1, int x2, int y2, unsigned int color, int width )
{

    signed int px, py, dx, dy, d;
    signed int ox, oy, sx, sy;
    signed int     i;

    dx = x2-x1; if( dx<0 ) dx=-dx;
    dy = y2-y1; if( dy<0 ) dy=-dy;

    if( dx>dy ) d=dx; else d=dy;
    if( !d ) return;


    px = (int)(65535.0f*(x2-x1)/(float)d);
    py = (int)(65535.0f*(y2-y1)/(float)d);
    ox = x1*65535;
    oy = y1*65535;

    for( i=0; i<d; i++ )
        {
        sx = ox>>16;
        sy = oy>>16;

        if( sx>=0 && sx<xres && sy>=0 && sy<yres )
            buffer[xres*sy+sx] = color;
            //buffer[xres*sy+sx] = ~buffer[xres*sy+sx];

        ox += px;
        oy += py;
        }
}




}