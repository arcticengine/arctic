// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// stb_image_write is public domain, see the header for its license. This file
// exists only to build the implementation once, as the header asks.

#define STB_IMAGE_WRITE_IMPLEMENTATION
// The engine writes to memory and to files of its own, so the stdio path of
// the library, and the locale trouble of its wide-character variant with it,
// is not needed here.
#define STBI_WRITE_NO_STDIO

#include "engine/stb_image_write.h"
