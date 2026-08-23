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

#include <climits>
#include <cstring>
#include <memory>
#include <sstream>

#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/vec2si32.h"

// The implementations of the two libraries are built here, in the one file that
// uses them, the way stb_vorbis is built in easy_sound.cpp: a translation unit of
// its own would have to be listed in every project file of every platform by
// hand, and a file forgotten in one of them shows up as a link error and nothing
// sooner. The engine reads and writes memory and files of its own, so the stdio
// paths of the libraries, and the locale trouble of the wide-character variant
// with them, are left out. Of the formats the reader knows only png is built:
// the rest of them nobody asked for and every one of them costs code size.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "engine/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "engine/stb_image.h"

namespace arctic {


  SpriteInstance::SpriteInstance(Si32 width, Si32 height)
    : width_(width)
      , height_(height)
      , data_(static_cast<size_t>(width) *
          static_cast<size_t>(height) * sizeof(Rgba)) {
      }

  void SpriteInstance::UpdateOpaqueSpans() {
    if (!height_) {
      opaque_.clear();
      return;
    }
    opaque_.resize(static_cast<size_t>(height_));
    for (Si32 y = 0; y < height_; ++y) {
      const Rgba *line = reinterpret_cast<Rgba*>(
          reinterpret_cast<void*>(data_.data())) +
        width_ * y;
      SpanSi32 &span = opaque_[static_cast<size_t>(y)];
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


  std::shared_ptr<SpriteInstance> LoadTga(const Ui8 *data, const Si64 size, Vec2Si32 *out_origin) {
    if (out_origin) {
      *out_origin = Vec2Si32(0, 0);
    }
    std::shared_ptr<SpriteInstance> sprite;
    if (size < sizeof(TgaHeader)) {
      *Log() << "Error in LoadTga, size: " << size << " < sizeof(TgaHeader): "
        << sizeof(TgaHeader) << " is too small.";
      return sprite;
    }
    TgaHeader tga_local;
    memcpy(&tga_local, data, sizeof(TgaHeader));
#ifdef BIGENDIAN
    tga_local.image_width = ((tga_local.image_width & 255) << 8)
      | (tga_local.image_width >> 8);
    tga_local.image_height = ((tga_local.image_height & 255) << 8)
      | (tga_local.image_height >> 8);
    tga_local.color_map_origin = ((tga_local.color_map_origin & 255) << 8)
      | (tga_local.color_map_origin >> 8);
    tga_local.color_map_length = ((tga_local.color_map_length & 255) << 8)
      | (tga_local.color_map_length >> 8);
    tga_local.image_x_origin = ((tga_local.image_x_origin & 255) << 8)
      | (tga_local.image_x_origin >> 8);
    tga_local.image_y_origin = ((tga_local.image_y_origin & 255) << 8)
      | (tga_local.image_y_origin >> 8);
#endif  // BIGENDIAN
    const TgaHeader *tga = &tga_local;
    if (tga->image_width < 2) {
      *Log() << "Error in LoadTga, tga.xres: " << tga->image_width
        << " < 2 is too small.";
      return sprite;
    }
    if (tga->image_height < 2) {
      *Log() << "Error in LoadTga, tga.yres: " << tga->image_height
        << " < 2 is too small.";
      return sprite;
    }
    if ((tga->pixel_depth != 32) && (tga->pixel_depth != 24)
        && (tga->pixel_depth != 16) && (tga->pixel_depth != 15)
        && (tga->pixel_depth != 8)) {
      *Log() << "Error in LoadTga, tga.bpp: " << tga->pixel_depth
        << " is unsupported.";
      return sprite;
    }
    const Si64 colormap_bytes_per_entry =
      (((Si64)tga->color_map_entry_size + 7) / 8);
    Si64 colormap_size = tga->color_map_length * colormap_bytes_per_entry;
    if (size < (Si64)sizeof(TgaHeader) + tga->id_field_length + colormap_size) {
      *Log() << "Error in LoadTga, size is too small.";
      return sprite;
    }
    const Ui8 *id_field = data + sizeof(TgaHeader);
    const Ui8 *colormap = id_field + tga->id_field_length;
    const Ui8 *p = colormap + colormap_size;

    bool is_origin_upper_left = !!(tga->image_descriptor & (1u << 5u));

    if ((Ui64)tga->image_width * tga->image_height * sizeof(Rgba) >= (1ull << 30)) {
      *Log() << "Error in LoadTga, sprite image is too large (" << tga->image_width
        << "x" << tga->image_height << "x" << sizeof(Rgba) << ").";
      return sprite;
    }

    if (out_origin) {
      Si16 x = static_cast<Si16>(tga->image_x_origin);
      Si16 y = static_cast<Si16>(tga->image_y_origin);
      *out_origin = Vec2Si32(x, y);
    }

    const bool is_rle = (tga->image_type == 9 || tga->image_type == 10
        || tga->image_type == 11);
    const bool is_gray = (tga->image_type == 3 || tga->image_type == 11);
    const bool is_palette = (tga->image_type == 1 || tga->image_type == 9);
    const bool is_with_alpha = ((!is_palette && tga->pixel_depth == 32)
        || (!is_palette && tga->pixel_depth == 16)
        || (is_palette && tga->color_map_entry_size == 32)
        || (is_palette && tga->color_map_entry_size == 16));

    const Si64 colormap_origin = tga->color_map_origin;
    const Si64 colormap_length = tga->color_map_length;
    const Si64 src_bytes_per_pixel =
      (is_palette ? 1 : (((Si64)tga->pixel_depth + 7) / 8));
    const Si64 dst_bytes_per_pixel = sizeof(Rgba);
    const Si64 entry_bytes_per_pixel =
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
                     Si64 repetitions = tga->image_width;
                     bool is_rle_packet = false;
                     if (is_rle) {
                       Ui8 repetitionCount = *(from_line + 0);
                       repetitions = (Si64)(repetitionCount & 0x7fu) + 1;
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
                     for (Si64 idx = 0; idx < repetitions; ++idx) {
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
                         to[3] = 255;
                       } else {
                         if (is_palette) {
                           Si64 entry_idx = static_cast<Si64>(*from_line)
                             - colormap_origin;
                           if (entry_idx < 0) {
                             *Log() << "Error in LoadTga,"
                               " entry_idx underflow in data fromat.";
                             return std::shared_ptr<SpriteInstance>();
                           }
                           if (entry_idx >= colormap_length) {
                             *Log() << "Error in LoadTga,"
                               " entry_idx overflow in data fromat.";
                             return std::shared_ptr<SpriteInstance>();
                           }
                           entry = colormap
                             + entry_idx * colormap_bytes_per_entry;
                         }
                         if (entry_bytes_per_pixel == 2) {
                           Ui8 b = (entry[0] & 0x1fu);
                           Ui8 g = (((entry[1] << 3u) & static_cast<Ui8>(0x1cu))
                             | ((entry[0] >> 5u) & 0x07u));
                           Ui8 r = ((entry[1] >> 2u) & static_cast<Ui8>(0x1fu));
                           to[0] = static_cast<Ui8>((r << 3u) | (r >> 2u));
                           to[1] = static_cast<Ui8>((g << 3u) | (g >> 2u));
                           to[2] = static_cast<Ui8>((b << 3u) | (b >> 2u));
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

  std::shared_ptr<SpriteInstance> LoadPng(const Ui8 *data, const Si64 size,
      Vec2Si32 *out_origin) {
    // A tga carries an origin of its own and the caller takes it for the pivot of
    // the sprite; a png has no such field, so the answer here is always zero.
    if (out_origin) {
      *out_origin = Vec2Si32(0, 0);
    }
    std::shared_ptr<SpriteInstance> sprite;
    if (data == nullptr || size <= 0) {
      *Log() << "Error in LoadPng, there is no data to read, size: " << size;
      return sprite;
    }
    if (size > static_cast<Si64>(INT_MAX)) {
      *Log() << "Error in LoadPng, size: " << size << " does not fit in an int.";
      return sprite;
    }
    int width = 0;
    int height = 0;
    int channels_in_file = 0;
    // Four channels are asked for, so a grayscale, a palette, a 16 bit and an
    // interlaced png all arrive as rgba and the engine sees one kind of pixel.
    Ui8 *pixels = stbi_load_from_memory(data, static_cast<int>(size),
        &width, &height, &channels_in_file, 4);
    if (pixels == nullptr) {
      *Log() << "Error in LoadPng, " << stbi_failure_reason() << ".";
      return sprite;
    }
    if ((Ui64)width * (Ui64)height * sizeof(Rgba) >= (1ull << 30)) {
      *Log() << "Error in LoadPng, sprite image is too large (" << width
        << "x" << height << "x" << sizeof(Rgba) << ").";
      stbi_image_free(pixels);
      return sprite;
    }
    sprite.reset(new SpriteInstance(width, height));
    // The rows of a png run from the top down, a sprite keeps them from the
    // bottom up, so they are copied in reverse order.
    const size_t row_size = static_cast<size_t>(width) * sizeof(Rgba);
    Ui8 *to = sprite->RawData();
    for (Si32 y = 0; y < height; ++y) {
      memcpy(to + static_cast<size_t>(y) * row_size,
          pixels + static_cast<size_t>(height - 1 - y) * row_size, row_size);
    }
    stbi_image_free(pixels);
    return sprite;
  }

  void SaveTga(std::shared_ptr<SpriteInstance> sprite, std::vector<Ui8> *data) {
    TgaHeader tga;
    memset(&tga, 0, sizeof(tga));
    const Si64 w = sprite->width();
    const Si64 h = sprite->height();
    tga.image_width = static_cast<Ui16>(w);
    tga.image_height = static_cast<Ui16>(h);
#ifdef BIGENDIAN
    tga.image_width = ((tga.image_width & 255) << 8)
      | (tga.image_width >> 8);
    tga.image_height = ((tga.image_height & 255) << 8)
      | (tga.image_height >> 8);
#endif  // BIGENDIAN
    tga.pixel_depth = 32;
    tga.image_type = 2;  // uncommpressed rgb

    Si64 length = sizeof(tga) + 4 * w * h;
    data->resize(static_cast<size_t>(length));
    memcpy(data->data(), &tga, sizeof(tga));

    const Ui8 *from = sprite->RawData();
    Ui8 *to = data->data() + sizeof(tga);

    for (Si64 y = 0; y < h; ++y) {
      for (Si64 x = 0; x < w; ++x) {
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

  // Collects what stb hands out into the vector the caller gave us.
  static void AppendToVector(void *context, void *data, int size) {
    std::vector<Ui8> *out = static_cast<std::vector<Ui8>*>(context);
    const Ui8 *bytes = static_cast<const Ui8*>(data);
    out->insert(out->end(), bytes, bytes + size);
  }

  void SavePng(std::shared_ptr<SpriteInstance> sprite, std::vector<Ui8> *data) {
    data->clear();
    if (!sprite) {
      *Log() << "Error in SavePng, the sprite is empty, nothing is saved.";
      return;
    }
    const Si32 w = sprite->width();
    const Si32 h = sprite->height();
    if (w <= 0 || h <= 0) {
      *Log() << "Error in SavePng, the sprite is " << w << " by " << h
        << " pixels, nothing is saved.";
      return;
    }

    // A sprite keeps its rows bottom-up, the way TGA does, while PNG counts
    // rows from the top, so the copy given to stb is turned over.
    const size_t stride = static_cast<size_t>(w) * sizeof(Rgba);
    std::vector<Ui8> top_down(stride * static_cast<size_t>(h));
    const Ui8 *from = sprite->RawData();
    for (Si32 y = 0; y < h; ++y) {
      memcpy(top_down.data() + stride * static_cast<size_t>(h - 1 - y),
          from + stride * static_cast<size_t>(y), stride);
    }

    const int written = stbi_write_png_to_func(AppendToVector, data, w, h, 4,
        top_down.data(), static_cast<int>(stride));
    if (written == 0) {
      data->clear();
      *Log() << "Error in SavePng, the encoder failed on a " << w << " by " << h
        << " pixel image, nothing is saved.";
    }
  }

}  // namespace arctic

