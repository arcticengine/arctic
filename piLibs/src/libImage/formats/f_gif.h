#pragma once

#include "../piImage.h"

namespace piLibs {

int GIF_Load( piImage *image, const wchar_t *name );
bool GIF_Save(const piImage *image, const wchar_t *name);

} // namespace piLibs