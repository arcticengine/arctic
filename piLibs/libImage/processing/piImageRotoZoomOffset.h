#pragma once

#include "../piImage.h"

namespace piLibs {

bool piImage_RotoZoomOffset( piImage *dst, const piImage *src, float rotate, float scale, float ofx, float ofy );

} // namespace piLibs