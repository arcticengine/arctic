#ifndef _PIDRAW_H_
#define _PIDRAW_H_


// draw modes
#define DM_SOLID    0       // solid
#define DM_BLEND    1       // 50% blend
#define DM_MIX      2       // a% blend (alpha channel)
#define DM_SADD     3       // saturated addition
#define DM_SSUB     4       // saturated substraction


void piDraw_BarFill32(   unsigned int *buffer, int xres, int yres, int xo, int yo, int w, int h, unsigned int color, int mode );
void piDraw_BarFill24(   unsigned char *buffer, int xres, int yres, int xo, int yo, int w, int h, const unsigned char *color, int mode );
void piDraw_BarBorder( unsigned int *buffer, int xres, int yres, int xo, int yo, int w, int h, unsigned int color, int mode );
void piDraw_Line( unsigned int *buffer, int xres, int yres, int x1, int y1, int x2, int y2, unsigned int color, int width=1 );

#endif
