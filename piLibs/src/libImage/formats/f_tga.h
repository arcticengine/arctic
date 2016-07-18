#pragma once

#include "../piImage.h"

namespace piLibs {

int TGA_Load( piImage *bmp, const wchar_t *name );
bool TGA_Save( const piImage *bmp, const wchar_t *name );

} // namespace piLibs