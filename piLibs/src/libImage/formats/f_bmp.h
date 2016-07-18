#pragma once

#include "../piImage.h"

namespace piLibs {

//long BMP_Carga8( char *nombre, unsigned char *buffer, long xres, long yres );

int BMP_Load( piImage *bmp, const wchar_t *name );
bool BMP_Save( const piImage *bmp, const wchar_t *name );

} // namespace piLibs