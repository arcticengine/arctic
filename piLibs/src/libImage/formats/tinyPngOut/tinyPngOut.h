/* 
 * Tiny PNG Output (C)
 * 
 * Copyright (c) 2016 Project Nayuki
 * https://www.nayuki.io/page/tiny-png-output-c
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING.txt).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// added by iq
#include "../../../libSystem/piTypes.h"
#include "../../../libSystem/piFile.h"

namespace piLibs {


/* 
 * TinyPngOut data structure. Treat this as opaque; do not read or write any fields directly.
 */
struct TinyPngOut
{
    // Configuration
    sint32 width;   // Measured in bytes, not pixels. A row has (width * 3 + 1) bytes.
    sint32 height;  // Measured in pixels
    
    // State
    piFile *outStream;
    sint32 positionX;  // Measured in bytes
    sint32 positionY;  // Measured in pixels
    sint32 deflateRemain;  // Measured in bytes
    sint32 deflateFilled;  // Number of bytes filled in the current block (0 <= n < 65535)
    uint32 crc;    // For IDAT chunk
    uint32 adler;  // For DEFLATE data within IDAT
};


/* 
 * Enumeration of status codes
 */
enum TinyPngOutStatus 
{
	TINYPNGOUT_OK,
	TINYPNGOUT_DONE,
	TINYPNGOUT_INVALID_ARGUMENT,
	TINYPNGOUT_IO_ERROR,
	TINYPNGOUT_IMAGE_TOO_LARGE,
};


enum TinyPngOutStatus TinyPngOut_init(struct TinyPngOut *pngout, piFile *fout, int width, int height);
enum TinyPngOutStatus TinyPngOut_write(struct TinyPngOut *pngout, const uint8 *pixels, int count);

}