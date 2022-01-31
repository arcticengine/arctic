// this one reverses the output image, to allow for 2 pass bluring
// This could be still optimized with interger SSE2 programming
static void box1D( unsigned char *dst, unsigned char *ori, int xres, int yres, int rad )
{
    int i, j;

    int totalR;
    int totalG;
    int totalB;

    const int sca15b = 32768/(2*rad+1);

    for( j=0; j<yres; j++ )
    {
      const int offOy = xres*j;

      totalR = 0;
      totalG = 0;
      totalB = 0;

      for( i=0; i<rad; i++ )
      {
        totalB += ori[(offOy+i)*3+0];
        totalG += ori[(offOy+i)*3+1];
        totalR += ori[(offOy+i)*3+2];
      }

      for( i=0; i<=rad; i++ )
      {
        totalB += ori[(offOy+i+rad)*3+0];
        totalG += ori[(offOy+i+rad)*3+1];
        totalR += ori[(offOy+i+rad)*3+2];

        const int sca15a = 32768/(rad+1+i);
        const int off = (yres*i+j)*3;
        dst[off+0] = (totalB*sca15a)>>15;
        dst[off+1] = (totalG*sca15a)>>15;
        dst[off+2] = (totalR*sca15a)>>15;
      }

      for( i=rad+1; i<xres-rad; i++ )
      {
          totalB -= ori[(offOy+i-rad-1)*3+0];
          totalG -= ori[(offOy+i-rad-1)*3+1];
          totalR -= ori[(offOy+i-rad-1)*3+2];

          totalB += ori[(offOy+i+rad)*3+0];
          totalG += ori[(offOy+i+rad)*3+1];
          totalR += ori[(offOy+i+rad)*3+2];

          const int off = (yres*i+j)*3;
          dst[off+0] = (totalB*sca15b)>>15;
          dst[off+1] = (totalG*sca15b)>>15;
          dst[off+2] = (totalR*sca15b)>>15;
      }

      for( i=xres-rad; i<xres; i++ )
      {
        totalB -= ori[(offOy+i-rad-1)*3+0];
        totalG -= ori[(offOy+i-rad-1)*3+1];
        totalR -= ori[(offOy+i-rad-1)*3+2];

        const int sca15b = 32768/(2*rad-i+xres-rad);
        const int off = (yres*i+j)*3;
        dst[off+0] = (totalB*sca15b)>>15;
        dst[off+1] = (totalG*sca15b)>>15;
        dst[off+2] = (totalR*sca15b)>>15;
      }
    }
}



void boxblur( unsigned char *buffer, unsigned char *tmp, int xres, int yres, int rad )
{
    box1D( tmp, buffer, xres, yres, rad );
    box1D( buffer, tmp, yres, xres, rad );
}
