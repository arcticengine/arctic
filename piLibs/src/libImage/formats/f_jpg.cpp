#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <setjmp.h>


#include "../piImage.h"


#include "jpeglib/jpeglib.h"
#include "jpeglib/jerror.h"

namespace piLibs {

struct my_error_mgr 
{
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


int JPG_Load( piImage *image, const wchar_t *name )
{

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    FILE *infile = _wfopen(name, L"rb");
    if( !infile  )
    {
        return 0;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) 
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 0;
    }

    jpeg_create_decompress( &cinfo );
    jpeg_stdio_src(&cinfo, infile);

    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    int row_stride = cinfo.output_width * cinfo.output_components;
    
    if (!image->Make(piImage::TYPE_2D, cinfo.output_width, cinfo.output_height, 1, piImage::FORMAT_I_RGB))
        return 0;

    // Make a one-row-high sample array that will go away when done with image
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    unsigned char *dst = (unsigned char *)image->GetData();

    while (cinfo.output_scanline < cinfo.output_height) 
    {
#if 0
        unsigned char *buffer[1] = { dst };
        jpeg_read_scanlines(&cinfo, buffer, 1);
        dst += row_stride;
#else
        jpeg_read_scanlines(&cinfo, buffer, 1);
        unsigned char *src = buffer[0];
        for( unsigned int i=0; i<cinfo.output_width; i++ )
        {
            dst[3*i+0] = src[3*i+2];
            dst[3*i+1] = src[3*i+1];
            dst[3*i+2] = src[3*i+0];
        }
        dst += row_stride;
#endif
    }

    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);

    fclose(infile);

    return 1;
}


}
