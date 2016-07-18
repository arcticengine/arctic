/******************************************************************************
 VGLGIF.C

 VGLGif( char* file, char far *outpix, char * palette,
         int* width, int* height );

 Routine to load a 256 color .GIF file into a memory buffer.  *Only* 256
 color images are supported here!  Sorry, no routines to SAVE .GIFs...
 Memory required is allocated on the fly and no checks are in place.  If you
 don't have enough memory it will likely crash.  It's easy to add the checks
 yourself, just put one after each call to malloc().  If you supply a pointer
 to a palette, it will be filled in.  If you supply a pointer to a width
 and/or height variable, it will be filled in as well.

 This code is hereby placed in the public domain.  Have fun!

 Mark Morley
 morley@camosun.bc.ca
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "../piImage.h"

namespace piLibs {


#define MAX_CODES     4096

//static FILE*          fp;

static int            curr_size;
static int            clear;
static int            ending;
static int            newcodes;
static int            top_slot;
static int            slot;
static int            navail_bytes = 0;
static int            nbits_left = 0;
static unsigned char  b1;
static unsigned char  byte_buff[257];
static unsigned char  *pbytes;
static unsigned char  *stack;
static unsigned char  *suffix;
static unsigned short *prefix;

static unsigned long code_mask[13] =
{
   0L,
   0x0001L, 0x0003L,
   0x0007L, 0x000FL,
   0x001FL, 0x003FL,
   0x007FL, 0x00FFL,
   0x01FFL, 0x03FFL,
   0x07FFL, 0x0FFFL
};

static int ggetw( FILE *f )
{
    return( fgetc(f) + (fgetc(f)<<8) );
}

static int get_next_code( FILE *fp )
{
   int                  i;
   static unsigned long ret;

   if( !nbits_left )
   {
      if( navail_bytes <= 0 )
      {
         pbytes = byte_buff;
         navail_bytes = getc( fp );
         if( navail_bytes )
            for( i = 0; i < navail_bytes; ++i )
               byte_buff[i] = (unsigned char)getc( fp );
      }
      b1 = *pbytes++;
      nbits_left = 8;
      --navail_bytes;
   }
   ret = b1 >> (8 - nbits_left);
   while( curr_size > nbits_left )
   {
      if( navail_bytes <= 0 )
      {
         pbytes = byte_buff;
         navail_bytes = getc( fp );
         if( navail_bytes )
            for( i = 0; i < navail_bytes; ++i )
               byte_buff[i] = (unsigned char)getc( fp );
      }
      b1 = *pbytes++;
      ret |= b1 << nbits_left;
      nbits_left += 8;
      --navail_bytes;
   }
   nbits_left -= curr_size;

   return( (int) (ret & code_mask[curr_size]) );
}


int GIF_Load( piImage *image, const wchar_t *filename )
{
    unsigned char   *sp;
    int             code, fc, oc;
    unsigned char   buf[1028];
    unsigned char   pal[3*256];

    FILE * fp = _wfopen(filename, L"rb");
    if( !fp )
        return( 0 );

    // check for signature
    fread( buf, 1, 6, fp );
    if( strncmp( (char *)buf, "GIF", 3 ) )
        {
        fclose( fp );
        return( 0 );
        }

    // ??
    fread( buf, 1, 7, fp );


    //read palette
    memset( pal, 0, 256*3 );

    for( int i=0; i < 3*(2 << (buf[4] & 7)); i++ )
    {
        int c = (unsigned char)getc( fp );
        pal[i] = c;
    }


    // ??
    fread( buf, 1, 5, fp );

    // get resolution
    int xres = 0; fread( &xres, 1, 2, fp );
    int yres = 0; fread(&yres, 1, 2, fp);


    if (!image->Make(piImage::TYPE_2D, xres, yres, 1, piImage::FORMAT_I_INDEXED))
        return( 0 );

    memcpy( image->GetPallette(), pal, 3*256 );
    unsigned char * outpix = (unsigned char*)image->GetData();

    // ??
    fread( buf, 1, 1, fp );

    unsigned char size = (unsigned char)getc( fp );
    if( size < 2 || 9 < size )
    {
        image->Free();
        fclose( fp );
        return( 0 );
    }

   stack  = (unsigned char *)malloc( MAX_CODES + 1 );
   if( !stack ) return( 0 );

   suffix = (unsigned char *)malloc( MAX_CODES + 1 );
   if( !suffix ) return( 0 );

   prefix = (unsigned short*)malloc( 2*(MAX_CODES+1) );
   if( !prefix ) return( 0 );
   

   curr_size = size + 1;
   top_slot = 1 << curr_size;
   clear = 1 << size;
   ending = clear + 1;
   slot = newcodes = ending + 1;
   navail_bytes = nbits_left = 0;
   oc = fc = 0;
   sp = stack;
   int c = 0;
   while( (c = get_next_code(fp)) != ending )
   {
      if( c == clear )
      {
         curr_size = size + 1;
         slot = newcodes;
         top_slot = 1 << curr_size;
         while( (c = get_next_code(fp)) == clear );
         if( c == ending )
            break;
         if( c >= slot )
            c = 0;
         oc = fc = c;
         *outpix++ = (unsigned char)c;
      }
      else
      {
         code = c;
         if( code >= slot )
         {
            code = oc;
            *sp++ = (unsigned char)fc;
         }
         while( code >= newcodes )
         {
            *sp++ = suffix[code];
            code = prefix[code];
         }
         *sp++ = (unsigned char)code;
         if( slot < top_slot )
         {
            fc = code;
            suffix[slot]   = (unsigned char )fc;
            prefix[slot++] = (unsigned short)oc;
            oc = c;
         }
         if( slot >= top_slot && curr_size < 12 )
         {
            top_slot <<= 1;
            ++curr_size;
         }
         while( sp > stack )
         {
            --sp;
            *outpix++ = (unsigned char)*sp;
         }
      }
   }

   free( stack );
   free( suffix );
   free( prefix );

   fclose( fp );

   return( 1 );
}



bool GIF_Save(const piImage *image, const wchar_t *name)
{
	return( 0 );
}


}