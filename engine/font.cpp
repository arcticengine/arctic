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

Letter g_tiny_font_letters[] = {
  {0x40404040400040, u8"!"}, {0x50505000000000, u8"\""},
  {0x5050f850f85050, u8"#"}, {0x5078d07058f050, u8"$"},
  {0xc0c81020409818, u8"%"}, {0x60906058909068, u8"&"},
  {0x20204000000000, u8"'"}, {0x20408080804020, u8"("},
  {0x80402020204080, u8")"}, {0x20a870f870a820, u8"*"},
  {0x002020f8202000, u8"+"}, {0x00000000002040, u8","},
  {0x000000f0000000, u8"-"}, {0x00000000006060, u8"."},
  {0x00000810204080, u8"/"}, {0x70888888888870, u8"0"},
  {0x2060a0202020f8, u8"1"}, {0x708808708080f8, u8"2"},
  {0x70881030088870, u8"3"}, {0x484888f8080808, u8"4"},
  {0xf880f008088870, u8"5"}, {0x708880f0888870, u8"6"},
  {0xf8081010202020, u8"7"}, {0x70888870888870, u8"8"},
  {0x70888878088870, u8"9"}, {0x00400000004000, u8":"},
  {0x00200000202040, u8";"}, {0x00102040201000, u8"<"},
  {0x0000f000f00000, u8"="}, {0x00402010204000, u8">"},
  {0x70881020200020, u8"?"}, {0x7088a8d8b88870, u8"@"},
  {0x708888f8888888, u8"A"}, {0xf08888f08888f0, u8"B"},
  {0x70888080808870, u8"C"}, {0xe09088888890e0, u8"D"},
  {0xf88080e08080f8, u8"E"}, {0xf88080e0808080, u8"F"},
  {0x708880b8888870, u8"G"}, {0x888888f8888888, u8"H"},
  {0xe04040404040e0, u8"I"}, {0x38101010909060, u8"J"},
  {0x8890a0c0a09088, u8"K"}, {0x808080808080f8, u8"L"},
  {0x88d8a888888888, u8"M"}, {0x88c8a898888888, u8"N"},
  {0x70888888888870, u8"O"}, {0xf08888f0808080, u8"P"},
  {0x60909090b09068, u8"Q"}, {0xf08888f0a09088, u8"R"},
  {0x70888070088870, u8"S"}, {0xf8202020202020, u8"T"},
  {0x88888888888870, u8"U"}, {0x88888850502020, u8"V"},
  {0x888888a8a85050, u8"W"}, {0x88885020508888, u8"X"},
  {0x88888870202020, u8"Y"}, {0xf80810204080f8, u8"Z"},
  {0xe08080808080e0, u8"["}, {0x00804020100800, u8"\\"},
  {0xe02020202020e0, u8"]"}, {0x20508800000000, u8"^"},
  {0x000000000000f8, u8"_"}, {0x00007008788878, u8"a"},
  {0x40a0a0a0f08870, u8"b"}, {0x00007080808070, u8"c"},
  {0x08087888888878, u8"d"}, {0x00007088f08070, u8"e"},
  {0x205040e0404040, u8"f"}, {0x000078887808f0, u8"g"},
  {0x8080e090909090, u8"h"}, {0x80008080808080, u8"i"},
  {0x2000202020a040, u8"j"}, {0x808090a0e09088, u8"k"},
  {0x80808080808080, u8"l"}, {0x0000d0a8a8a8a8, u8"m"},
  {0x0000f088888888, u8"n"}, {0x00007088888870, u8"o"},
  {0x0000f088f08080, u8"p"}, {0x00007888780808, u8"q"},
  {0x00005060404040, u8"r"}, {0x000070807008f0, u8"s"},
  {0x4040e040405020, u8"t"}, {0x00008888889868, u8"u"},
  {0x00008888885020, u8"v"}, {0x000088a8a8a850, u8"w"},
  {0x00008850205088, u8"x"}, {0x000088887808f0, u8"y"},
  {0x0000f8102040f8, u8"z"}, {0x708888f8888888, u8"А"},
  {0xf88080f08888f0, u8"Б"}, {0xf08888f08888f0, u8"В"},
  {0xf8808080808080, u8"Г"}, {0xe09088888890e0, u8"Д"},
  {0xf88080e08080f8, u8"Е"}, {0xa8a8a870a8a8a8, u8"Ж"},
  {0x70880830088870, u8"З"}, {0x888898a8c88888, u8"И"},
  {0x50208898a8c88888, u8"Й"}, {0x8890a0c0a09088, u8"К"},
  {0x38488888888888, u8"Л"}, {0x88d8a888888888, u8"М"},
  {0x888888f8888888, u8"Н"}, {0x70888888888870, u8"О"},
  {0xf8888888888888, u8"П"}, {0xf08888f0808080, u8"Р"},
  {0x70888080808870, u8"С"}, {0xf8202020202020, u8"Т"},
  {0x88888878088870, u8"У"}, {0x70a8a8a8702020, u8"Ф"},
  {0x88885020508888, u8"Х"}, {0x9090909090f808, u8"Ц"},
  {0x88888878080808, u8"Ч"}, {0x888888a8a8a8f8, u8"Ш"},
  {0x8888a8a8a8f808, u8"Щ"}, {0xc0404070484870, u8"Ъ"},
  {0x888888c8a8a8c8, u8"Ы"}, {0x808080f08888f0, u8"Ь"},
  {0xe01008780810e0, u8"Э"}, {0x90a8a8e8a8a890, u8"Ю"},
  {0x78888878284888, u8"Я"}, {0x00007008788878, u8"а"},
  {0x087080f0888870, u8"б"}, {0x40a0a0f0888870, u8"в"},
  {0x00007008708078, u8"г"}, {0x80700878888870, u8"д"},
  {0x00007088f08070, u8"е"}, {0x0000a8a870a8a8, u8"ж"},
  {0x00007088308870, u8"з"}, {0x00009090909068, u8"и"},
  {0x60009090909068, u8"й"}, {0x000090a0e09088, u8"к"},
  {0x00001828488888, u8"л"}, {0x000088d8a88888, u8"м"},
  {0x00008888f88888, u8"н"}, {0x00007088888870, u8"о"},
  {0x0000f088888888, u8"п"}, {0x0000f088f08080, u8"р"},
  {0x00007080808070, u8"с"}, {0x0000f820202020, u8"т"},
  {0x000088887808f0, u8"у"}, {0x000070a8a87020, u8"ф"},
  {0x00008850205088, u8"х"}, {0x00009090906808, u8"ц"},
  {0x00008888780808, u8"ч"}, {0x00008888a8a870, u8"ш"},
  {0x000088a8a87808, u8"щ"}, {0x0000c040704870, u8"ъ"},
  {0x00008888c8a8c8, u8"ы"}, {0x00008080f088f0, u8"ь"},
  {0x0000f0087808f0, u8"э"}, {0x000090a8e8a890, u8"ю"},
  {0x00007888784888, u8"я"}, {0x20404080404020, u8"{"},
  {0x80404020404080, u8"}"}, {0x50f88080e08080f8, u8"Ё"},
  {0x50007088f08070, u8"ё"},
  {0x0, nullptr}
};

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

void Font::LoadTable(Sprite sprite, const char* utf8_letters,
    Si32 cell_width, Si32 cell_height,
    Si32 base_to_top, Si32 line_height, Si32 space_width,
    Si32 left_offset) {
  std::string vec(utf8_letters);
  CreateEmpty(base_to_top, line_height);
  Utf32Reader reader;
  reader.Reset(vec.c_str());
  Si32 width = (sprite.Width() + cell_width - 1) / cell_width;
  for (Si32 idx = 0; idx < (Si32)vec.size(); ++idx) {
    Si32 x = idx % width;
    Si32 y = idx / width;
    Ui32 codepoint = reader.ReadOne();
    Sprite tmp;
    tmp.Reference(sprite, x * cell_width, y * cell_height, cell_width, cell_height);
    Sprite cs;
    cs.Clone(tmp);
    cs.UpdateOpaqueSpans();
    cs.SetPivot(Vec2Si32(left_offset, cell_height - base_to_top - 1));
    AddGlyph(codepoint, space_width, cs);
  }
  Sprite space;
  AddGlyph(32, space_width, space);
}

void Font::LoadLetterBits(Letter *in_letters,
    Si32 base_to_top, Si32 line_height) {
  CreateEmpty(base_to_top, line_height);
  if (!in_letters) {
    return;
  }
  Utf32Reader reader;
  while (in_letters->utf8_letter) {
    reader.Reset(in_letters->utf8_letter);
    Ui32 codepoint = reader.ReadOne();
    Sprite sprite;
    Si32 xadvance = 0;
    sprite.Create(8, 8);
    Rgba *data = const_cast<Rgba*>(sprite.RgbaData());
    Si32 stride = sprite.StridePixels();
    for (Si32 y = 0; y < 8; ++y) {
      for (Si32 x = 0; x < 8; ++x) {
        bool is_filled = (in_letters->glyph_bitmap & ((128ull >> x) << (y * 8)));
        Rgba color = is_filled ? Rgba(255,255,255,255) : Rgba(0,0,0,0);
        data[x + y * stride] = color;
        if (is_filled) {
          xadvance = std::max(xadvance, x + 2);
        }
      }
    }
    AddGlyph(codepoint, xadvance, sprite);
    ++in_letters;
  }
  Sprite space;
  AddGlyph(32, 4, space);
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
