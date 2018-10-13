// The MIT License (MIT)
//
// Copyright (c) 2017 - 2018 Huldra
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
#include <algorithm>
#include <iostream>
#include <vector>
#include <list>

#include "engine/font.h"
#include "engine/arctic_types.h"
#include "engine/easy.h"

namespace arctic {

void BmFontBinHeader::Log() {
  // TODO(Huldra): Use log here
  std::cerr << "header";
  std::cerr << " bmf=" << ((b == 66 && m == 77 && f == 70) ? 1 : 0);
  std::cerr << " version=" << static_cast<Si32>(version);
  std::cerr << std::endl;
}


void BmFontBinInfo::Log() {
  // TODO(Huldra): Use log here
  std::cerr << "info";
  std::cerr << " face=\"" << font_name << "\"";
  std::cerr << " size=" << font_size;
  std::cerr << " bold=" << ((bits & kBold) ? 1 : 0);
  std::cerr << " italic=" << ((bits & kItalic) ? 1 : 0);
  std::cerr << " charset=" << static_cast<Si32>(char_set);
  std::cerr << " unicode=" << ((bits & kUnicode) ? 1 : 0);
  std::cerr << " stretchH=" << stretch_h;
  std::cerr << " smooth=" << ((bits & kSmooth) ? 1 : 0);
  std::cerr << " aa=" << static_cast<Si32>(aa);
  std::cerr << " padding=" << static_cast<Si32>(padding_up);
  std::cerr << "," << static_cast<Si32>(padding_right);
  std::cerr << "," << static_cast<Si32>(padding_down);
  std::cerr << "," << static_cast<Si32>(padding_left);
  std::cerr << " spacing=" << static_cast<Si32>(spacing_horiz);
  std::cerr << "," << static_cast<Si32>(spacing_vert);
  std::cerr << " outline=" << static_cast<Si32>(outline);
  std::cerr << std::endl;
}

void BmFontBinCommon::Log() {
  // TODO(Huldra): Use log here
  std::cerr << "common";
  std::cerr << " lineHeight=" << line_height;
  std::cerr << " base=" << base;
  std::cerr << " scaleW=" << scale_w;
  std::cerr << " scaleH=" << scale_h;
  std::cerr << " pages=" << pages;
  std::cerr << " packed=" << ((bits & kPacked) ? 1 : 0);
  std::cerr << " alphaChnl=" << static_cast<Si32>(alpha_chnl);
  std::cerr << " redChnl=" << static_cast<Si32>(red_chnl);
  std::cerr << " greenChnl=" << static_cast<Si32>(green_chnl);
  std::cerr << " blueChnl=" << static_cast<Si32>(blue_chnl);
  std::cerr << std::endl;
}

void BmFontBinPages::Log(Si32 id) {
  // TODO(Huldra): Use log here
  std::cerr << "page";
  std::cerr << " id=" << id;
  std::cerr << " file=\"" << page_name << "\"";
  std::cerr << std::endl;
}

void BmFontBinChars::Log() {
  // TODO(Huldra): Use log here
  std::cerr << "char";
  std::cerr << " id=" << id;
  std::cerr << "\tx=" << x;
  std::cerr << "  \ty=" << y;
  std::cerr << "  \twidth=" << width;
  std::cerr << "  \theight=" << height;
  std::cerr << "  \txoffset=" << xoffset;
  std::cerr << "\tyoffset=" << yoffset;
  std::cerr << "\txadvance=" << xadvance;
  std::cerr << "\tpage=" << static_cast<Si32>(page);
  std::cerr << "\tchnl=" << static_cast<Si32>(chnl);
  std::cerr << std::endl;
}

void BmFontBinKerningPair::Log() {
  // TODO(Huldra): Use log here
  std::cerr << "kerning";
  std::cerr << " first=" << first;
  std::cerr << "\tsecond=" << second;
  std::cerr << "\tamount=" << amount;
  std::cerr << std::endl;
}

void Utf32Reader::Reset(const Ui8 *data) {
  begin = data;
  p = data;
}

void Utf32Reader::Rewind() {
  p = begin;
}

Ui32 Utf32Reader::ReadOne() {
  while (true) {
    Ui32 u = 0;
    if ((p[0] & 0x80) == 0) {
      // 0xxxxxxx
      u = Ui32(p[0]);
      if (p[0] == 0) {
        return 0;
      }
      p++;
      return u;
    } else if ((p[0] & 0xe0) == 0xc0) {
      // 110xxxxx 10xxxxxx
      if ((p[1] & 0xc0) == 0x80) {
        u = (Ui32(p[0] & 0x1f) << 6) | (Ui32(p[1] & 0x3f));
        p += 2;
        return u;
      }
    } else if ((p[0] & 0xf0) == 0xe0) {
      // 1110xxxx 10xxxxxx 10xxxxxx
      if ((p[1] & 0xc0) == 0x80 && (p[2] & 0xc0) == 0x80) {
        u = (Ui32(p[0] & 0x0f) << 12) | (Ui32(p[1] & 0x3f) << 6) |
          (Ui32(p[2] & 0x3f));
        p += 3;
        return u;
      }
    } else if ((p[0] & 0xf8) == 0xf0) {
      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if ((p[1] & 0xc0) == 0x80 && (p[2] & 0xc0) == 0x80 &&
        (p[3] & 0xc0) == 0x80) {
        u = (Ui32(p[0] & 0x07) << 18) | (Ui32(p[1] & 0x3f) << 12) |
          (Ui32(p[2] & 0x3f) << 6) | (Ui32(p[3] & 0x3f));
        p += 4;
        return u;
      }
    }
    p++;
  }
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
  
void Font::AddGlyph(Ui32 codepoint, Si32 xadvance, easy::Sprite sprite) {
  glyph_.emplace_back(codepoint, xadvance, sprite);
  if (codepoint >= codepoint_.size()) {
    codepoint_.resize(codepoint + 1, nullptr);
  }
  codepoint_[codepoint] = &glyph_.back();
}

void Font::Load(const char *file_name) {
  codepoint_.clear();
  glyph_.clear();

  std::vector<Ui8> file = easy::ReadFile(file_name);
  Si32 pos = 0;
  BmFontBinHeader *header = reinterpret_cast<BmFontBinHeader*>(&file[pos]);
  header->Log();
  pos += sizeof(BmFontBinHeader);

  Si8 block_type = file[pos];
  ++pos;
  Si32 block_size = *reinterpret_cast<Si32*>(&file[pos]);
  pos += sizeof(Si32);
  Check(block_type == kBlockInfo, "Unexpected block type 1");

  Check(block_size >=
    sizeof(BmFontBinInfo) - sizeof(BmFontBinInfo::font_name),
    "Info block is too small");
  BmFontBinInfo info;
  memcpy(&info, &file[pos], sizeof(info) - sizeof(info.font_name));
  info.font_name = reinterpret_cast<char*>(
    &file[pos + sizeof(info) - sizeof(info.font_name)]);
  info.Log();
  pos += block_size;

  block_type = file[pos];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[pos]);
  pos += sizeof(Si32);
  Check(block_type == kBlockCommon, "Unexpected block type 2");
  Check(block_size >= sizeof(BmFontBinCommon), "Common block is too small");
  BmFontBinCommon *common = reinterpret_cast<BmFontBinCommon*>(&file[pos]);
  common->Log();

  base_to_top_ = common->base;
  base_to_bottom_ = common->line_height - common->base;
  line_height_ = common->line_height;

  pos += block_size;

  block_type = file[pos];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[pos]);
  pos += sizeof(Si32);
  Check(block_type == kBlockPages, "Unexpected block type 3");
  Check(block_size >= 1, "Pages block is too small");
  Si32 inner_pos = pos;
  std::vector<easy::Sprite> page_images;
  page_images.resize(common->pages);

  for (Si32 id = 0; id < common->pages; ++id) {
    BmFontBinPages page;
    page.page_name = reinterpret_cast<char*>(&file[inner_pos]);
    page.Log(id);

    char path[65536];
    const char *p = file_name;
    Check(strlen(file_name) < sizeof(path) / 2, "File name is too long: ",
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
      memcpy(path, p, end - p);
    }
    strncpy(path + (end - p), page.page_name, sizeof(path) / 2);
    page_images[id].Load(path);

    inner_pos += static_cast<Si32>(std::strlen(page.page_name)) + 1;
  }
  pos += block_size;
  block_type = file[pos];
  ++pos;
  block_size = *reinterpret_cast<Si32*>(&file[pos]);
  pos += sizeof(Si32);
  Check(block_type == kBlockChars, "Unexpected block type 4");
  Check(block_size >= sizeof(BmFontBinChars), "Pages block is too small");
  inner_pos = pos;
  for (Si32 id = 0; id < block_size / 20; ++id) {
    BmFontBinChars *chars = reinterpret_cast<BmFontBinChars*>(
      &file[inner_pos]);
    chars->Log();

    easy::Sprite sprite0;
    sprite0.Reference(page_images[chars->page],
      chars->x, page_images[chars->page].Height() - chars->y - chars->height,
      chars->width, chars->height);
    easy::Sprite sprite;
    sprite.Clone(sprite0);
    sprite.UpdateOpaqueSpans();
    sprite.SetPivot(arctic::Vec2Si32(
      chars->xoffset, chars->height + chars->yoffset - common->base));
    glyph_.emplace_back(chars->id, chars->xadvance, sprite);

    inner_pos += 20;
  }
  pos += block_size;

  if (static_cast<Si32>(file.size()) > pos) {
    block_type = file[pos];
    ++pos;
    block_size = *reinterpret_cast<Si32*>(&file[pos]);
    pos += sizeof(Si32);
    Check(block_type == kBlockKerningPairs, "Unexpected block type 5");
    Check(block_size >= sizeof(BmFontBinKerningPair),
      "KerningPair block is too small");
    inner_pos = pos;
    for (Si32 id = 0; id < block_size / 10; ++id) {
      BmFontBinKerningPair *kerning_pair =
        reinterpret_cast<BmFontBinKerningPair*>(&file[inner_pos]);
      kerning_pair->Log();
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

void Font::DrawEvaluateSizeImpl(easy::Sprite to_sprite,
    const char *text, bool do_keep_xadvance,
    Si32 x, Si32 y, TextOrigin origin,
    easy::DrawBlendingMode blending_mode,
    easy::DrawFilterMode filter_mode,
    Rgba color, const std::vector<Rgba> &palete, bool do_draw,
    Vec2Si32 *out_size) {
  Si32 next_x = x;
  Si32 next_y = y;
  if (do_draw) {
    if (origin == kTextOriginTop) {
      next_y = y - base_to_top_ + line_height_;
    } else if (origin == kTextOriginFirstBase) {
      next_y = y + line_height_;
    } else {
      Vec2Si32 size;
      DrawEvaluateSizeImpl(to_sprite, text, do_keep_xadvance,
        x, y, origin, blending_mode, filter_mode, color, palete, false,
        &size);
      if (origin == kTextOriginBottom) {
        next_y = y + size.y - base_to_top_ + line_height_;
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
        *out_size = Vec2Si32(max_width, lines * line_height_);
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
      if (code >= 1 && code <= 8) {
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
          next_x = x;
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
  easy::Sprite empty;
  DrawEvaluateSizeImpl(empty, text, do_keep_xadvance,
    0, 0, kTextOriginFirstBase, easy::kCopyRgba, easy::kFilterNearest,
    Rgba(255, 255, 255), std::vector<Rgba>(), false,
    &size);
  return size;
}

void Font::Draw(easy::Sprite to_sprite, const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const easy::DrawBlendingMode blending_mode,
    const Rgba color) {
  DrawEvaluateSizeImpl(to_sprite,
    text, false, x, y, origin, blending_mode, color,
    std::vector<Rgba>(), true, nullptr);
}

void Font::Draw(easy::Sprite to_sprite, const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const easy::DrawBlendingMode blending_mode,
    const std::vector<Rgba> &palete) {
  DrawEvaluateSizeImpl(to_sprite,
    text, false, x, y, origin, blending_mode, palete[0],
    palete, true, nullptr);
}
  
void Font::Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin,
      const easy::DrawBlendingMode blending_mode,
      const easy::DrawFilterMode filter_mode,
      const Rgba color) {
  DrawEvaluateSizeImpl(easy::GetEngine()->GetBackbuffer(),
      text, false, x, y, origin,
      blending_mode, filter_mode, color,
      std::vector<Rgba>(), true, nullptr);
}

void Font::Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const easy::DrawBlendingMode blending_mode,
    const easy::DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete) {
  DrawEvaluateSizeImpl(easy::GetEngine()->GetBackbuffer(),
      text, false, x, y, origin,
      blending_mode, filter_mode, palete[0],
      palete, true, nullptr);
}

}  // namespace arctic
