// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
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

#include <cstring>
#include <vector>
#include <sstream>

#include "engine/font.h"
#include "engine/arctic_types.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/easy_advanced.h"
#include "engine/easy_files.h"
#include "engine/log.h"
#include "engine/unicode.h"


namespace arctic {

void BmFontBinHeader::Log() const {
  *arctic::Log() << "header"
    << " bmf=" << ((b == 66 && m == 77 && f == 70) ? 1 : 0)
    << " version=" << static_cast<Si32>(version);
}


void BmFontBinInfo::Log() const {
  *arctic::Log() << "info"
    << " face=\"" << font_name << "\""
    << " size=" << font_size
    << " bold=" << ((bits & kBold) ? 1 : 0)
    << " italic=" << ((bits & kItalic) ? 1 : 0)
    << " charset=" << static_cast<Si32>(char_set)
    << " unicode=" << ((bits & kUnicode) ? 1 : 0)
    << " stretchH=" << stretch_h
    << " smooth=" << ((bits & kSmooth) ? 1 : 0)
    << " aa=" << static_cast<Si32>(aa)
    << " padding=" << static_cast<Si32>(padding_up)
    << "," << static_cast<Si32>(padding_right)
    << "," << static_cast<Si32>(padding_down)
    << "," << static_cast<Si32>(padding_left)
    << " spacing=" << static_cast<Si32>(spacing_horiz)
    << "," << static_cast<Si32>(spacing_vert)
    << " outline=" << static_cast<Si32>(outline);
}

void BmFontBinCommon::Log() const {
  *arctic::Log() << "common"
    << " lineHeight=" << line_height
    << " base=" << base
    << " scaleW=" << scale_w
    << " scaleH=" << scale_h
    << " pages=" << pages
    << " packed=" << ((bits & kPacked) ? 1 : 0)
    << " alphaChnl=" << static_cast<Si32>(alpha_chnl)
    << " redChnl=" << static_cast<Si32>(red_chnl)
    << " greenChnl=" << static_cast<Si32>(green_chnl)
    << " blueChnl=" << static_cast<Si32>(blue_chnl);
}

void BmFontBinPages::Log(Si32 id) const {
  *arctic::Log() << "page"
    << " id=" << id
    << " file=\"" << page_name << "\"";
}

void BmFontBinChars::Log() const {
  *arctic::Log() << "char"
    << " id=" << id
    << "\tx=" << x
    << "  \ty=" << y
    << "  \twidth=" << width
    << "  \theight=" << height
    << "  \txoffset=" << xoffset
    << "\tyoffset=" << yoffset
    << "\txadvance=" << xadvance
    << "\tpage=" << static_cast<Si32>(page)
    << "\tchnl=" << static_cast<Si32>(chnl);
}

void BmFontBinKerningPair::Log() const {
  *arctic::Log() << "kerning"
    << " first=" << first
    << "\tsecond=" << second
    << "\tamount=" << amount;
}

void Font::CreateEmpty(Si32 base_to_top, Si32 line_height) {
  codepoint_.clear();
  glyph_.clear();
  base_to_top_ = base_to_top;
  base_to_bottom_ = line_height - base_to_top;
  line_height_ = line_height;
}

void Font::AddGlyph(const Glyph &glyph) {
  AddGlyph(glyph.codepoint, glyph.xadvance, glyph.sprite);
}

void Font::AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite) {
  glyph_.emplace_back(codepoint, xadvance, sprite);
  if (codepoint >= codepoint_.size()) {
    codepoint_.resize(codepoint + 1, nullptr);
  }
  codepoint_[codepoint] = &glyph_.back();
}

void Font::Load(const char *file_name) {
  codepoint_.clear();
  glyph_.clear();

  std::vector<Ui8> file = ReadFile(file_name);
  Si32 pos = 0;
  // BmFontBinHeader *header = reinterpret_cast<BmFontBinHeader*>(&file[pos]);
  // header->Log();
  pos += sizeof(BmFontBinHeader);

  Ui8 block_type = file[static_cast<size_t>(pos)];
  ++pos;
  Si32 block_size = *reinterpret_cast<Si32*>(&file[static_cast<size_t>(pos)]);
  pos += sizeof(Si32);
  Check(block_type == kBlockInfo, "Unexpected block type 1");

  Check(block_size >=
    sizeof(BmFontBinInfo) - sizeof(BmFontBinInfo::font_name),
    "Info block is too small");
  BmFontBinInfo info;
  memcpy(&info, &file[static_cast<size_t>(pos)],
    sizeof(info) - sizeof(info.font_name));
  info.font_name = reinterpret_cast<char*>(
    &file[static_cast<size_t>(pos) + sizeof(info) - sizeof(info.font_name)]);
  outline_ = info.outline;
  // info.Log();
  pos += block_size;

  block_type = file[static_cast<size_t>(pos)];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[static_cast<size_t>(pos)]);
  pos += sizeof(Si32);
  Check(block_type == kBlockCommon, "Unexpected block type 2");
  Check(block_size >= sizeof(BmFontBinCommon), "Common block is too small");
  BmFontBinCommon *common =
    reinterpret_cast<BmFontBinCommon*>(&file[static_cast<size_t>(pos)]);
  // common->Log();

  base_to_top_ = common->base;
  base_to_bottom_ = common->line_height - common->base;
  line_height_ = common->line_height;

  pos += block_size;

  block_type = file[static_cast<size_t>(pos)];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[static_cast<size_t>(pos)]);
  pos += sizeof(Si32);
  Check(block_type == kBlockPages, "Unexpected block type 3");
  Check(block_size >= 1, "Pages block is too small");
  Si32 inner_pos = pos;
  std::vector<Sprite> page_images;
  page_images.resize(common->pages);

  size_t file_name_len = strlen(file_name);

  for (Si32 id = 0; id < common->pages; ++id) {
    BmFontBinPages page;
    page.page_name =
      reinterpret_cast<char*>(&file[static_cast<size_t>(inner_pos)]);
    // page.Log(id);

    char path[8 << 10];
    const char *p = file_name;
    Check(file_name_len < sizeof(path) / 2, "File name is too long: ",
      file_name);
    Check(strlen(page.page_name) < sizeof(path) / 2,
      "File name is too long: ", page.page_name);
    const char *p2 = p;
    const char *end = p;
    while (*p2) {
      if (*p2 == '\\' || *p2 == '/') {
        end = p2 + 1;
      }
      ++p2;
    }
    if (end != p) {
      memcpy(path, p, static_cast<size_t>(end - p));
    }
    strncpy(path + (end - p), page.page_name, sizeof(path) / 2);
    page_images[static_cast<size_t>(id)].Load(path);

    inner_pos += static_cast<Si32>(std::strlen(page.page_name)) + 1;
  }
  pos += block_size;
  block_type = file[static_cast<size_t>(pos)];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[static_cast<size_t>(pos)]);
  pos += sizeof(Si32);
  Check(block_type == kBlockChars, "Unexpected block type 4");
  Check(block_size >= sizeof(BmFontBinChars), "Pages block is too small");
  inner_pos = pos;
  for (Si32 id = 0; id < block_size / 20; ++id) {
    BmFontBinChars *chars = reinterpret_cast<BmFontBinChars*>(
      &file[static_cast<size_t>(inner_pos)]);
    // chars->Log();

    Sprite sprite0;
    sprite0.Reference(page_images[chars->page],
      chars->x, page_images[chars->page].Height() - chars->y - chars->height,
      chars->width, chars->height);
    Sprite sprite;
    sprite.Clone(sprite0);
    sprite.UpdateOpaqueSpans();
    sprite.SetPivot(arctic::Vec2Si32(
      -chars->xoffset, chars->height + chars->yoffset - common->base));
    glyph_.emplace_back(chars->id, chars->xadvance, sprite);

    inner_pos += 20;
  }
  pos += block_size;

  if (static_cast<Si32>(file.size()) > pos) {
    block_type = static_cast<Ui8>(file[static_cast<size_t>(pos)]);
    ++pos;
    block_size = *reinterpret_cast<Si32*>(&file[static_cast<size_t>(pos)]);
    pos += sizeof(Si32);
    Check(block_type == kBlockKerningPairs, "Unexpected block type 5");
    Check(block_size >= sizeof(BmFontBinKerningPair),
      "KerningPair block is too small");
    inner_pos = pos;
    for (Si32 id = 0; id < block_size / 10; ++id) {
      // BmFontBinKerningPair *kerning_pair =
      //  reinterpret_cast<BmFontBinKerningPair*>(&file[inner_pos]);
      // kerning_pair->Log();
      inner_pos += 10;
    }
    pos += block_size;
  }

  Ui32 end_codepoint = 0;
  for (auto it = glyph_.begin(); it != glyph_.end(); ++it) {
    if (it->codepoint >= end_codepoint) {
      end_codepoint = it->codepoint + 1;
    }
  }
  codepoint_.resize(end_codepoint, nullptr);
  for (auto it = glyph_.begin(); it != glyph_.end(); ++it) {
    codepoint_[it->codepoint] = &(*it);
  }
}

void Font::LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
    Si32 base_to_top, Si32 line_height, Si32 space_width) {
  CreateEmpty(base_to_top, line_height);
  Si32 begin_x = 0;
  Utf32Reader reader;
  reader.Reset(utf8_letters);
  for (Si32 x = 0; x < sprite.Width(); ++x) {
    bool is_empty = true;
    for (Si32 y = 0; y < sprite.Height(); ++y) {
      if (sprite.RgbaData()[x + y * sprite.StridePixels()].a != 0) {
        is_empty = false;
      }
    }
    if (is_empty) {
      if (x != begin_x) {
        Sprite letter;
        letter.Reference(sprite, begin_x, 0, x - begin_x, sprite.Height());
        Sprite ls;
        ls.Clone(letter);
        Ui32 codepoint = reader.ReadOne();
        AddGlyph(codepoint, x - begin_x + 1, ls);
      }
      begin_x = x + 1;
    }
  }
  Sprite space;
  AddGlyph(32, space_width, space);
}

void Font::DrawEvaluateSizeImpl(Sprite to_sprite,
    const char *text, bool do_keep_xadvance,
    Si32 x, Si32 y, TextOrigin origin,
    DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode,
    Rgba color, const std::vector<Rgba> &palete, bool do_draw,
    Vec2Si32 *out_size) {
  Si32 next_x = x + outline_;
  Si32 next_y = y;
  if (do_draw) {
    if (origin == kTextOriginTop) {
      next_y = y - base_to_top_ + line_height_ - outline_;
    } else if (origin == kTextOriginFirstBase) {
      next_y = y + line_height_;
    } else {
      Vec2Si32 size;
      DrawEvaluateSizeImpl(to_sprite, text, do_keep_xadvance,
        x, y, origin, blending_mode, filter_mode, color, palete, false,
        &size);
      if (origin == kTextOriginBottom) {
        next_y = y + size.y - base_to_top_ + line_height_ - outline_;
      } else if (origin == kTextOriginLastBase) {
        next_y = y + size.y;
      }
    }
  }

  Si32 width = 0;
  Si32 max_width = 0;
  Si32 lines = 0;
  Ui32 prev_code = 0;
  bool is_newline = false;
  Si32 newline_count = 1;
  Ui32 color_idx = 0;
  Utf32Reader reader;
  reader.Reset(reinterpret_cast<const Ui8*>(text));
  Glyph *glyph = nullptr;
  while (true) {
    Ui32 code = reader.ReadOne();
    if (!code) {
      if (glyph && !do_keep_xadvance) {
        width += glyph->sprite.Width() - glyph->xadvance;
      }
      max_width = std::max(max_width, width);
      if (out_size) {
        *out_size = Vec2Si32(max_width + outline_*2,
          lines * line_height_+outline_*2);
      }
      return;
    }
    if (code == '\r' || code == '\n') {
      if (is_newline) {
        if (code == prev_code) {
          newline_count++;
        } else {
          is_newline = false;
        }
      } else {
        is_newline = true;
        prev_code = code;
        newline_count++;
      }
    } else {
      is_newline = false;
      if (code <= 8) {
        color_idx = code;
        if (color_idx >= palete.size()) {
          color_idx = 0;
          // TODO(Huldra): Log error here
        }
      } else if (code < codepoint_.size() && codepoint_[code]) {
        if (newline_count) {
          if (glyph && !do_keep_xadvance) {
            width += glyph->sprite.Width() - glyph->xadvance;
          }
          max_width = std::max(max_width, width);
          width = 0;
          next_x = x + outline_;
          lines += newline_count;
          next_y -= newline_count * line_height_;
          newline_count = 0;
        }

        glyph = codepoint_[code];
        width += glyph->xadvance;
        if (do_draw) {
          if (palete.size()) {
            glyph->sprite.Draw(to_sprite, next_x, next_y,
               blending_mode, filter_mode, palete[color_idx]);
          } else {
            glyph->sprite.Draw(to_sprite, next_x, next_y,
               blending_mode, filter_mode, color);
          }
          next_x += glyph->xadvance;
        }
      }
    }
  }
}

Vec2Si32 Font::EvaluateSize(const char *text, bool do_keep_xadvance) {
  Vec2Si32 size;
  Sprite empty;
  DrawEvaluateSizeImpl(empty, text, do_keep_xadvance,
    0, 0, kTextOriginFirstBase, kDrawBlendingModeCopyRgba, kFilterNearest,
    Rgba(255, 255, 255), std::vector<Rgba>(), false,
    &size);
  return size;
}

void Font::Draw(Sprite to_sprite, const char *text,
    const Si32 x, const Si32 y,
    const TextOrigin origin,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const Rgba color) {  //-V801
  DrawEvaluateSizeImpl(to_sprite,
    text, false, x, y, origin, blending_mode, filter_mode, color,
    std::vector<Rgba>(), true, nullptr);
}

void Font::Draw(Sprite to_sprite, const char *text,
    const Si32 x, const Si32 y,
    const TextOrigin origin,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete) {
  DrawEvaluateSizeImpl(to_sprite,
    text, false, x, y, origin, blending_mode, filter_mode, palete[0],
    palete, true, nullptr);
}

void Font::Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin,
      const DrawBlendingMode blending_mode,
      const DrawFilterMode filter_mode,
      const Rgba color) {  //-V801
  DrawEvaluateSizeImpl(GetEngine()->GetBackbuffer(),
      text, false, x, y, origin,
      blending_mode, filter_mode, color,
      std::vector<Rgba>(), true, nullptr);
}

void Font::Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete) {
  DrawEvaluateSizeImpl(GetEngine()->GetBackbuffer(),
      text, false, x, y, origin,
      blending_mode, filter_mode, palete[0],
      palete, true, nullptr);
}

}  // namespace arctic
