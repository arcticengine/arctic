// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2015 - 2017 Inigo Quilez
// Copyright (c) 2017 - 2020 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "engine/easy_sprite_instance.h"

#include <cstring>
#include <memory>
#include <sstream>

#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"

namespace arctic {


  SpriteInstance::SpriteInstance(Si32 width, Si32 height)
    : width_(width)
      , height_(height)
      , data_(width * height * sizeof(Rgba)) {
      }

  void SpriteInstance::UpdateOpaqueSpans() {
    if (!height_) {
      opaque_.clear();
      return;
    }
    opaque_.resize(height_);
    for (Si32 y = 0; y < height_; ++y) {
      const Rgba *line = reinterpret_cast<Rgba*>(
          reinterpret_cast<void*>(data_.data())) +
        width_ * y;
      SpanSi32 &span = opaque_[y];
      span.begin = 0;
      span.end = 0;
      Si32 x = 0;
      for (; x < width_; ++x) {
        if (line[x].a != 0) {
          span.begin = x;
          break;
        }
      }
      if (x < width_) {
        for (; x < width_; ++x) {
          if (line[x].a != 0) {
            span.end = x;
          }
        }
        span.end++;
      }
    }
  }

  void SpriteInstance::ClearOpaqueSpans() {
    opaque_.clear();
  }

#pragma pack(1)
struct TgaHeader {
  Ui8 id_field_length;
  Ui8 color_map_type;
  Ui8 image_type;
  Ui16 color_map_origin;  // This much first entries are not included.
  Ui16 color_map_length;  // This much entries are included.
  Ui8 color_map_entry_size;  // Size in bits.
  Ui16 image_x_origin;
  Ui16 image_y_origin;
  Ui16 image_width;
  Ui16 image_height;
  Ui8 pixel_depth;  // Bits per pixel.
  Ui8 image_descriptor;
};
#pragma pack()


  std::shared_ptr<SpriteInstance> LoadTga(const Ui8 *data, const Si64 size) {
    std::shared_ptr<SpriteInstance> sprite;
    if (size < sizeof(TgaHeader)) {
      *Log() << "Error in LoadTga, size: " << size << " < sizeof(TgaHeader): "
        << sizeof(TgaHeader) << " is too small.";
      return sprite;
    }
    const TgaHeader *tga = static_cast<const TgaHeader*>(
        static_cast<const void*>(data));
#ifdef BIGENDIAN
    tga->image_width = ((tga->image_width & 255) << 8)
      | (tga->image_width >> 8);
    tga->image_height = ((tga->image_height & 255) << 8)
      | (tga->image_height >> 8);
    tga->color_map_origin = ((tga->color_map_origin & 255) << 8)
      | (tga->color_map_origin >> 8);
    tga->color_map_length = ((tga->color_map_length & 255) << 8)
      | (tga->color_map_length >> 8);
#endif  // BIGENDIAN
    if (tga->image_width < 2) {
      *Log() << "Error in LoadTga, tga.xres: " << tga->image_width << " < 2 is too small.";
      return sprite;
    }
    if (tga->image_height < 2) {
      *Log() << "Error in LoadTga, tga.yres: " << tga->image_height << " < 2 is too small.";
      return sprite;
    }
    if ((tga->pixel_depth != 32) && (tga->pixel_depth != 24)
        && (tga->pixel_depth != 16) && (tga->pixel_depth != 15)
        && (tga->pixel_depth != 8)) {
      *Log() << "Error in LoadTga, tga.bpp: " << tga->pixel_depth << " is unsupported.";
      return sprite;
    }
    const Si64 colormap_bytes_per_entry = (((Si64)tga->color_map_entry_size + 7) / 8);
    Si64 colormapSize = tga->color_map_length * colormap_bytes_per_entry;
    if (size < (Si64)sizeof(TgaHeader) + tga->id_field_length + colormapSize) {
      *Log() << "Error in LoadTga, size is too small.";
      return sprite;
    }
    const Ui8 *id_field = data + sizeof(TgaHeader);
    const Ui8 *colormap = id_field + tga->id_field_length;
    const Ui8 *p = colormap + colormapSize;

    bool is_origin_upper_left = !!(tga->image_descriptor & (1u << 5u));

    if ((Ui64)tga->image_width * tga->image_height * sizeof(Rgba) >= (1ull << 30)) {
      *Log() << "Error in LoadTga, sprite image is too large (" << tga->image_width
        << "x" << tga->image_height << "x" << sizeof(Rgba) << ").";
      return sprite;
    }

    const bool is_rle = (tga->image_type == 9 || tga->image_type == 10
        || tga->image_type == 11);
    const bool is_gray = (tga->image_type == 3 || tga->image_type == 11);
    const bool is_palette = (tga->image_type == 1 || tga->image_type == 9);
    const bool is_with_alpha = ((!is_palette && tga->pixel_depth == 32)
        || (!is_palette && tga->pixel_depth == 16)
        || (is_palette && tga->color_map_entry_size == 32)
        || (is_palette && tga->color_map_entry_size == 16));

    const Si32 colormap_origin = tga->color_map_origin;
    const Si32 colormap_size = tga->color_map_length;
    const Si32 src_bytes_per_pixel =
      (is_palette ? 1 : (((Si32)tga->pixel_depth + 7) / 8));
    const Si32 dst_bytes_per_pixel = sizeof(Rgba);
    const Si32 entry_bytes_per_pixel =
      (is_palette ? colormap_bytes_per_entry : src_bytes_per_pixel);
    if (is_palette) {
      if (tga->color_map_type != 1) {
        *Log() << "Error in LoadTga, no palette included.";
        return sprite;
      }
    }
    sprite.reset(new SpriteInstance(tga->image_width, tga->image_height));

    switch (tga->image_type) {
      case 0:  // no image data included
        break;
      case 1:  // uncompressed palette
      case 2:  // uncommpressed rgb
      case 3:  // uncommpressed gray
      case 9:   // run-length encoded palette
      case 10:  // run-length encoded rgb
      case 11: {  // run-length encoded gray
                 const Ui8 *from_line = p;
                 const Ui8 *from_end = data + size;
                 Si64 to_line_size = sprite->width() * dst_bytes_per_pixel;
                 Ui8 *to_line = sprite->RawData() +
                   (is_origin_upper_left ?
                    tga->image_height - 1 : 0) * to_line_size;
                 const Si64 to_line_step =
                   (is_origin_upper_left ? -to_line_size : to_line_size);
                 for (Si64 y = 0; y < tga->image_height; ++y) {
                   Ui8 *to = to_line + y * to_line_step;
                   Ui8 *to_end = to + to_line_size;

                   while (to < to_end) {
                     if (from_line >= from_end) {
                       *Log() << "Error in LoadTga, unexpected end of file.";
                       return std::shared_ptr<SpriteInstance>();
                     }
                     Si32 repetitions = tga->image_width;
                     bool is_rle_packet = false;
                     if (is_rle) {
                       Ui8 repetitionCount = *(from_line + 0);
                       repetitions = (Si32)(repetitionCount & 0x7fu) + 1;
                       is_rle_packet = !!(repetitionCount & 0x80u);
                       from_line++;
                     }
                     if (is_rle_packet) {  // run length packet
                       if (from_line + src_bytes_per_pixel > from_end) {
                         *Log() << "Error in LoadTga, unexpected end of file.";
                         return std::shared_ptr<SpriteInstance>();
                       }
                     } else {
                       if (from_line + src_bytes_per_pixel * repetitions
                           > from_end) {
                         *Log() << "Error in LoadTga, unexpected end of file.";
                         return std::shared_ptr<SpriteInstance>();
                       }
                       if (to + dst_bytes_per_pixel * repetitions > to_end) {
                         *Log() << "Error in LoadTga,"
                           " overflow in data format of file.";
                         return std::shared_ptr<SpriteInstance>();
                       }
                     }
                     for (Si32 idx = 0; idx < repetitions; ++idx) {
                       // <= is intended
                       if (is_rle_packet) {
                         if (to + dst_bytes_per_pixel > to_end) {
                           *Log() << "Error in LoadTga,"
                             " overflow in data format of file.";
                           return std::shared_ptr<SpriteInstance>();
                         }
                       }
                       const Ui8 *entry = from_line;
                       if (is_gray) {
                         to[0] = *entry;
                         to[1] = *entry;
                         to[2] = *entry;
                       } else {
                         if (is_palette) {
                           Si32 entry_idx = static_cast<Si32>(*from_line)
                             - colormap_origin;
                           if (entry_idx < 0) {
                             *Log() << "Error in LoadTga,"
                               " entry_idx underflow in data fromat.";
                             return std::shared_ptr<SpriteInstance>();
                           }
                           if (entry_idx >= colormap_size) {
                             *Log() << "Error in LoadTga,"
                               " entry_idx overflow in data fromat.";
                             return std::shared_ptr<SpriteInstance>();
                           }
                           entry = colormap
                             + entry_idx * colormap_bytes_per_entry;
                         }
                         if (entry_bytes_per_pixel == 2) {
                           Ui8 b = (entry[0] & 0x1fu);
                           Ui8 g = ((entry[1] << 3u) & 0x1cu)
                             | ((entry[0] >> 5u) & 0x07u);
                           Ui8 r = (entry[1] >> 2u) & 0x1fu;
                           to[0] = (r << 3u) | (r >> 2u);
                           to[1] = (g << 3u) | (g >> 2u);
                           to[2] = (b << 3u) | (b >> 2u);
                         } else {
                           to[0] = entry[2];
                           to[1] = entry[1];
                           to[2] = entry[0];
                         }
                         if (is_with_alpha) {
                           if (entry_bytes_per_pixel == 2) {
                             to[3] = ((entry[1] & 0x80u) >> 7u) * 255u;
                           } else {
                             to[3] = entry[3];
                           }
                         } else {
                           to[3] = 255;
                         }
                       }
                       to += dst_bytes_per_pixel;
                       if (!is_rle_packet) {
                         from_line += src_bytes_per_pixel;
                       }
                     }
                     if (is_rle_packet) {
                       from_line += src_bytes_per_pixel;
                     }
                   }
                 }
                 break;
               }
      default: {
                 *Log() << "Error in LoadTga, unexpected file type: "
                   << (Ui32)tga->image_type;
                 break;
               }
    }
    return sprite;
  }

  void SaveTga(std::shared_ptr<SpriteInstance> sprite, std::vector<Ui8> *data) {
    TgaHeader tga;
    memset(&tga, 0, sizeof(tga));
    tga.image_width = sprite->width();
    tga.image_height = sprite->height();
#ifdef BIGENDIAN
    tga.image_width = ((tga.image_width & 255) << 8)
      | (tga.image_width >> 8);
    tga.image_height = ((tga.image_height & 255) << 8)
      | (tga.image_height >> 8);
#endif  // BIGENDIAN
    tga.pixel_depth = 32;
    tga.image_type = 2;  // uncommpressed rgb

    Si64 length = sizeof(tga) + 4 * tga.image_width * tga.image_height;
    data->resize(static_cast<size_t>(length));
    memcpy(data->data(), &tga, sizeof(tga));

    const Ui8 *from = sprite->RawData();
    Ui8 *to = data->data() + sizeof(tga);

    for (Si64 y = 0; y < tga.image_height; ++y) {
      for (Si64 x = 0; x < tga.image_width; ++x) {
        to[0] = from[2];
        to[1] = from[1];
        to[2] = from[0];
        to[3] = from[3];
        from += sizeof(Rgba);
        to += sizeof(Rgba);
      }
    }
    return;
  }

}  // namespace arctic

template class std::shared_ptr<arctic::SpriteInstance>;
