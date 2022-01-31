#pragma once

#include "../piImage.h"

namespace piLibs {

// makes box blur with radious "rad" on "buffer". Result is done in the same buffer. "tmp" is a necessary temporal buffer,
// and must be the same size as "buffer"
void boxblur( unsigned char *buffer, unsigned char *tmp, int xres, int yres, int rad );

} // namespace piLibs