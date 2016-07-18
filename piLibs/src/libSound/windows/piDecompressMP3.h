#pragma once

#include "../../libMisc/formats/piWav.h"

namespace piLibs {

    bool  OpenMP3FromFile(piWav *dst, const wchar_t *name);
    bool  OpenMP3FromMemory(piWav *dst, unsigned char * mp3InputBuffer, int mp3InputBufferSize);

} // namesp