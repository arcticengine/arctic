#pragma once

namespace piLibs {

typedef void * piAviReader;
typedef void * piAviWriter;

piAviReader     piAviReader_Init( const wchar_t *name );
void            piAviReader_End( piAviReader vme );
unsigned char * piAviReader_GetFrame( piAviReader vme, int index );
void            piAviReader_GetInfo( piAviReader vme, int *xres, int *yres, int *numframes, int *fps );

//-----------------------

piAviWriter piAviWriter_Init( const wchar_t *name, int xres, int yres, int fps );
void	    piAviWriter_End( piAviWriter vme );
int         piAviWriter_DoFrame( piAviWriter vme, const unsigned char *data );

} // namespace piLibs