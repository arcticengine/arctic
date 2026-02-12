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
#include <cmath>
#include <vector>
#include <sstream>

#include "engine/font.h"
#include "engine/arctic_types.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/easy_advanced.h"
#include "engine/easy_files.h"
#include "engine/log.h"
#include "engine/unicode.h"
#include "engine/pugixml.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "engine/stb_truetype.h"

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
  {0x40404040400040, (const char*)u8"!"}, {0x50505000000000, (const char*)u8"\""},
  {0x5050f850f85050, (const char*)u8"#"}, {0x5078d07058f050, (const char*)u8"$"},
  {0xc0c81020409818, (const char*)u8"%"}, {0x60906058909068, (const char*)u8"&"},
  {0x20204000000000, (const char*)u8"'"}, {0x20408080804020, (const char*)u8"("},
  {0x80402020204080, (const char*)u8")"}, {0x20a870f870a820, (const char*)u8"*"},
  {0x002020f8202000, (const char*)u8"+"}, {0x00000000002040, (const char*)u8","},
  {0x000000f0000000, (const char*)u8"-"}, {0x00000000006060, (const char*)u8"."},
  {0x00000810204080, (const char*)u8"/"}, {0x70888888888870, (const char*)u8"0"},
  {0x2060a0202020f8, (const char*)u8"1"}, {0x708808708080f8, (const char*)u8"2"},
  {0x70881030088870, (const char*)u8"3"}, {0x484888f8080808, (const char*)u8"4"},
  {0xf880f008088870, (const char*)u8"5"}, {0x708880f0888870, (const char*)u8"6"},
  {0xf8081010202020, (const char*)u8"7"}, {0x70888870888870, (const char*)u8"8"},
  {0x70888878088870, (const char*)u8"9"}, {0x00400000004000, (const char*)u8":"},
  {0x00200000202040, (const char*)u8";"}, {0x00102040201000, (const char*)u8"<"},
  {0x0000f000f00000, (const char*)u8"="}, {0x00402010204000, (const char*)u8">"},
  {0x70881020200020, (const char*)u8"?"}, {0x7088a8d8b88870, (const char*)u8"@"},
  {0x708888f8888888, (const char*)u8"A"}, {0xf08888f08888f0, (const char*)u8"B"},
  {0x70888080808870, (const char*)u8"C"}, {0xe09088888890e0, (const char*)u8"D"},
  {0xf88080e08080f8, (const char*)u8"E"}, {0xf88080e0808080, (const char*)u8"F"},
  {0x708880b8888870, (const char*)u8"G"}, {0x888888f8888888, (const char*)u8"H"},
  {0xe04040404040e0, (const char*)u8"I"}, {0x38101010909060, (const char*)u8"J"},
  {0x8890a0c0a09088, (const char*)u8"K"}, {0x808080808080f8, (const char*)u8"L"},
  {0x88d8a888888888, (const char*)u8"M"}, {0x88c8a898888888, (const char*)u8"N"},
  {0x70888888888870, (const char*)u8"O"}, {0xf08888f0808080, (const char*)u8"P"},
  {0x60909090b09068, (const char*)u8"Q"}, {0xf08888f0a09088, (const char*)u8"R"},
  {0x70888070088870, (const char*)u8"S"}, {0xf8202020202020, (const char*)u8"T"},
  {0x88888888888870, (const char*)u8"U"}, {0x88888850502020, (const char*)u8"V"},
  {0x888888a8a85050, (const char*)u8"W"}, {0x88885020508888, (const char*)u8"X"},
  {0x88888870202020, (const char*)u8"Y"}, {0xf80810204080f8, (const char*)u8"Z"},
  {0xe08080808080e0, (const char*)u8"["}, {0x00804020100800, (const char*)u8"\\"},
  {0xe02020202020e0, (const char*)u8"]"}, {0x20508800000000, (const char*)u8"^"},
  {0x000000000000f8, (const char*)u8"_"}, {0x00007008788878, (const char*)u8"a"},
  {0x40a0a0a0f08870, (const char*)u8"b"}, {0x00007080808070, (const char*)u8"c"},
  {0x08087888888878, (const char*)u8"d"}, {0x00007088f08070, (const char*)u8"e"},
  {0x205040e0404040, (const char*)u8"f"}, {0x000078887808f0, (const char*)u8"g"},
  {0x8080e090909090, (const char*)u8"h"}, {0x80008080808080, (const char*)u8"i"},
  {0x2000202020a040, (const char*)u8"j"}, {0x808090a0e09088, (const char*)u8"k"},
  {0x80808080808040, (const char*)u8"l"}, {0x0000d0a8a8a8a8, (const char*)u8"m"},
  {0x0000f088888888, (const char*)u8"n"}, {0x00007088888870, (const char*)u8"o"},
  {0x0000f088f08080, (const char*)u8"p"}, {0x00007888780808, (const char*)u8"q"},
  {0x00005060404040, (const char*)u8"r"}, {0x000070807008f0, (const char*)u8"s"},
  {0x4040e040405020, (const char*)u8"t"}, {0x00008888889868, (const char*)u8"u"},
  {0x00008888885020, (const char*)u8"v"}, {0x000088a8a8a850, (const char*)u8"w"},
  {0x00008850205088, (const char*)u8"x"}, {0x000088887808f0, (const char*)u8"y"},
  {0x0000f8102040f8, (const char*)u8"z"}, {0x708888f8888888, (const char*)u8"А"},
  {0xf88080f08888f0, (const char*)u8"Б"}, {0xf08888f08888f0, (const char*)u8"В"},
  {0xf8808080808080, (const char*)u8"Г"}, {0xe09088888890e0, (const char*)u8"Д"},
  {0xf88080e08080f8, (const char*)u8"Е"}, {0xa8a8a870a8a8a8, (const char*)u8"Ж"},
  {0x70880830088870, (const char*)u8"З"}, {0x888898a8c88888, (const char*)u8"И"},
  {0x50208898a8c88888, (const char*)u8"Й"}, {0x8890a0c0a09088, (const char*)u8"К"},
  {0x38488888888888, (const char*)u8"Л"}, {0x88d8a888888888, (const char*)u8"М"},
  {0x888888f8888888, (const char*)u8"Н"}, {0x70888888888870, (const char*)u8"О"},
  {0xf8888888888888, (const char*)u8"П"}, {0xf08888f0808080, (const char*)u8"Р"},
  {0x70888080808870, (const char*)u8"С"}, {0xf8202020202020, (const char*)u8"Т"},
  {0x88888878088870, (const char*)u8"У"}, {0x70a8a8a8702020, (const char*)u8"Ф"},
  {0x88885020508888, (const char*)u8"Х"}, {0x9090909090f808, (const char*)u8"Ц"},
  {0x88888878080808, (const char*)u8"Ч"}, {0x888888a8a8a8f8, (const char*)u8"Ш"},
  {0x8888a8a8a8f808, (const char*)u8"Щ"}, {0xc0404070484870, (const char*)u8"Ъ"},
  {0x888888c8a8a8c8, (const char*)u8"Ы"}, {0x808080f08888f0, (const char*)u8"Ь"},
  {0xe01008780810e0, (const char*)u8"Э"}, {0x90a8a8e8a8a890, (const char*)u8"Ю"},
  {0x78888878284888, (const char*)u8"Я"}, {0x00007008788878, (const char*)u8"а"},
  {0x087080f0888870, (const char*)u8"б"}, {0x40a0a0f0888870, (const char*)u8"в"},
  {0x00007008708078, (const char*)u8"г"}, {0x80700878888870, (const char*)u8"д"},
  {0x00007088f08070, (const char*)u8"е"}, {0x0000a8a870a8a8, (const char*)u8"ж"},
  {0x00007088308870, (const char*)u8"з"}, {0x00009090909068, (const char*)u8"и"},
  {0x60009090909068, (const char*)u8"й"}, {0x000090a0e09088, (const char*)u8"к"},
  {0x00001828488888, (const char*)u8"л"}, {0x000088d8a88888, (const char*)u8"м"},
  {0x00008888f88888, (const char*)u8"н"}, {0x00007088888870, (const char*)u8"о"},
  {0x0000f088888888, (const char*)u8"п"}, {0x0000f088f08080, (const char*)u8"р"},
  {0x00007080808070, (const char*)u8"с"}, {0x0000f820202020, (const char*)u8"т"},
  {0x000088887808f0, (const char*)u8"у"}, {0x000070a8a87020, (const char*)u8"ф"},
  {0x00008850205088, (const char*)u8"х"}, {0x00009090906808, (const char*)u8"ц"},
  {0x00008888780808, (const char*)u8"ч"}, {0x00008888a8a870, (const char*)u8"ш"},
  {0x000088a8a87808, (const char*)u8"щ"}, {0x0000c040704870, (const char*)u8"ъ"},
  {0x00008888c8a8c8, (const char*)u8"ы"}, {0x00008080f088f0, (const char*)u8"ь"},
  {0x0000f0087808f0, (const char*)u8"э"}, {0x000090a8e8a890, (const char*)u8"ю"},
  {0x00007888784888, (const char*)u8"я"}, {0x20404080404020, (const char*)u8"{"},
  {0x80404020404080, (const char*)u8"}"}, {0x50f88080e08080f8, (const char*)u8"Ё"},
  {0x50007088f08070, (const char*)u8"ё"}, {0x000040a8100000, (const char*)u8"~"},
  {0x40404040404040, (const char*)u8"|"}, {0x70807088700870, (const char*)u8"§"},
  {0x002020f820f800, (const char*)u8"±"}, {0x96d6d0b6909090, (const char*)u8"№"},
  {0x80402000000000, (const char*)u8"`"},
  {0x0, nullptr}
};

void FontInstance::CreateEmpty(Si32 base_to_top, Si32 line_height) {
  codepoint_.clear();
  glyph_.clear();
  base_to_top_ = base_to_top;
  base_to_bottom_ = line_height - base_to_top;
  line_height_ = line_height;
}

void FontInstance::AddGlyph(const Glyph &glyph) {
  AddGlyph(glyph.codepoint, glyph.xadvance, glyph.sprite);
}

void FontInstance::AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite) {
  glyph_.emplace_back(codepoint, xadvance, sprite);
  if (codepoint >= codepoint_.size()) {
    codepoint_.resize(codepoint + 1, nullptr);
  }
  codepoint_[codepoint] = &glyph_.back();
}

void FontInstance::GenerateCodepointVector() {
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

void FontInstance::Load(const char *file_name) {
  Check(!!file_name, "Error in FontInstance::Load, file_name is nullptr.");
  const char *last_dot = strrchr(file_name, '.');
  if (!last_dot || StrCaseCmp(last_dot, ".fnt") == 0) {
    LoadBinaryFnt(file_name);
    return;
  }
  if (StrCaseCmp(last_dot, ".xml") == 0) {
    LoadXml(file_name);
    return;
  }
  Fatal("Error in FontInstance::Load, file_name has an unknown extension: ", file_name);
}

void FontInstance::LoadXml(const char *file_name) {
  pugi::XmlDocument doc;
  pugi::XmlParseResult parse_result = doc.load_file(file_name);
  if (parse_result.status != pugi::status_ok) {
    std::stringstream str;
    str << "Error loading " << file_name << " FontInstance, at offset " << parse_result.offset << " (line " << parse_result.line;
    str << " column " << parse_result.column << ": " << parse_result.description();
    Fatal(str.str().c_str());
  }
  std::string parent_path = ParentPath(file_name);

  const char* font_type_str = doc.child("font").attribute("type").as_string(nullptr);
  if (!font_type_str) {
    std::stringstream str;
    str << "Error loading " << file_name << " FontInstance, font type is missing";
    Fatal(str.str().c_str());
  }
  if (strcmp(font_type_str, "ascii_square") == 0) {
    // 16x16 square of ASCII letters
    const char* font_sprite_path = doc.child("font").attribute("path").as_string(nullptr);
    bool is_dense = doc.child("font").attribute("is_dense").as_bool(false);
    LoadAsciiSquare(GluePath(parent_path.c_str(), font_sprite_path).c_str(), is_dense);
  } else {
    std::stringstream str;
    str << "Error loading " << file_name << " FontInstance, font type " << font_type_str << " is unknown";;
    Fatal(str.str().c_str());
  }
}

void FontInstance::LoadAsciiSquare(const char *file_name, bool is_dense) {
  Sprite sprites;
  sprites.Load(file_name);

  Si32 width = sprites.Width() / 16 - 1;
  Si32 height = sprites.Height() / 16 - 1;

  CreateEmpty(height, height);
  for (Ui32 y = 0; y < 16; ++y) {
    for (Ui32 x = 0; x < 16; ++x) {
      Ui32 codepoint = x + y * 16;
      Sprite letter;
      letter.Reference(sprites,
                       x * (width + 1), (16 - 1 - y) * (height + 1) + 1,
                       width, height);
      Si32 dense_width = width;
      if (is_dense && codepoint != 32) {
        bool is_empty = true;
        for (Si32 xx = width - 1; xx > 0; --xx) {
          for (Si32 yy = 0; yy < height; ++yy) {
            Si32 offset = xx + yy * letter.StridePixels();
            Rgba *rgba = letter.RgbaData() + offset;
            if (rgba->a != 0) {
              is_empty = false;
              break;
            }
          }
          if (!is_empty) {
            break;
          }
          dense_width = xx;
        }
        letter.Reference(sprites,
                         x * (width + 1), (16 - 1 - y) * (height + 1) + 1,
                         dense_width, height);
      }
      Sprite optimized;
      optimized.Clone(letter);
      optimized.UpdateOpaqueSpans();
      AddGlyph(codepoint, dense_width + 1, optimized);
    }
  }
  codepoint_[32]->sprite.Reference(
    codepoint_[32]->sprite, 0, 0,
    codepoint_['I']->sprite.Width(),
    codepoint_['I']->sprite.Height());
  codepoint_[32]->xadvance = codepoint_['I']->xadvance;
}

void FontInstance::LoadBinaryFnt(const char *file_name) {
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

  GenerateCodepointVector();
}

void FontInstance::LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
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

void FontInstance::LoadTable(Sprite sprite, const char* utf8_letters,
    Si32 cell_width, Si32 cell_height,
    Si32 base_to_top, Si32 line_height, Si32 space_width,
    Si32 left_offset) {
  std::string vec(utf8_letters);
  CreateEmpty(base_to_top, line_height);
  Sprite space;
  AddGlyph(32, space_width, space);
  Utf32Reader reader;
  reader.Reset(vec.c_str());
  Si32 width = (sprite.Width() + cell_width - 1) / cell_width;
  Si32 idx = 0;
  while (true) {
    Ui32 codepoint = reader.ReadOne();
    if (codepoint == 0) {
      break;
    }
    Si32 x = idx % width;
    Si32 y = idx / width;
    Sprite tmp;
    tmp.Reference(sprite, x * cell_width, y * cell_height, cell_width, cell_height);
    Sprite cs;
    cs.Clone(tmp);
    cs.UpdateOpaqueSpans();
    cs.SetPivot(Vec2Si32(left_offset, cell_height - base_to_top - 1));
    AddGlyph(codepoint, space_width, cs);
    ++idx;
  }

}

void FontInstance::LoadLetterBits(Letter *in_letters,
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

void FontInstance::LoadTtf(const char *file_name, float pixel_height,
                           const char *utf8_chars, Si32 font_index) {
  Check(!!file_name,
    "Error in FontInstance::LoadTtf, file_name is nullptr.");
  Check(pixel_height > 0.0f,
    "Error in FontInstance::LoadTtf, pixel_height must be positive.");
  Check(font_index >= 0,
    "Error in FontInstance::LoadTtf, font_index must be non-negative.");

  // Default character set: ASCII printable + Cyrillic
  const char *default_chars =
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~"
    "\xd0\x81"  // Ё
    "\xd0\x90\xd0\x91\xd0\x92\xd0\x93\xd0\x94\xd0\x95\xd0\x96\xd0\x97"
    "\xd0\x98\xd0\x99\xd0\x9a\xd0\x9b\xd0\x9c\xd0\x9d\xd0\x9e\xd0\x9f"
    "\xd0\xa0\xd0\xa1\xd0\xa2\xd0\xa3\xd0\xa4\xd0\xa5\xd0\xa6\xd0\xa7"
    "\xd0\xa8\xd0\xa9\xd0\xaa\xd0\xab\xd0\xac\xd0\xad\xd0\xae\xd0\xaf"
    "\xd0\xb0\xd0\xb1\xd0\xb2\xd0\xb3\xd0\xb4\xd0\xb5\xd0\xb6\xd0\xb7"
    "\xd0\xb8\xd0\xb9\xd0\xba\xd0\xbb\xd0\xbc\xd0\xbd\xd0\xbe\xd0\xbf"
    "\xd1\x80\xd1\x81\xd1\x82\xd1\x83\xd1\x84\xd1\x85\xd1\x86\xd1\x87"
    "\xd1\x88\xd1\x89\xd1\x8a\xd1\x8b\xd1\x8c\xd1\x8d\xd1\x8e\xd1\x8f"
    "\xd1\x91";  // ё

  if (!utf8_chars) {
    utf8_chars = default_chars;
  }

  std::vector<Ui8> ttf_data = ReadFile(file_name);
  Check(!ttf_data.empty(),
    "Error in FontInstance::LoadTtf, could not read file: ", file_name);

  stbtt_fontinfo font_info;
  int font_offset = stbtt_GetFontOffsetForIndex(ttf_data.data(), font_index);
  Check(font_offset >= 0,
    "Error in FontInstance::LoadTtf, font_index out of range: ", file_name);
  int init_result = stbtt_InitFont(&font_info, ttf_data.data(), font_offset);
  Check(init_result != 0,
    "Error in FontInstance::LoadTtf, failed to parse font: ", file_name);

  float scale = stbtt_ScaleForPixelHeight(&font_info, pixel_height);

  int ascent_unscaled, descent_unscaled, line_gap_unscaled;
  stbtt_GetFontVMetrics(&font_info,
    &ascent_unscaled, &descent_unscaled, &line_gap_unscaled);

  Si32 ascent = static_cast<Si32>(
    std::floor(ascent_unscaled * scale + 0.5f));
  Si32 descent = static_cast<Si32>(
    std::floor(-descent_unscaled * scale + 0.5f));
  Si32 line_gap = static_cast<Si32>(
    std::floor(line_gap_unscaled * scale + 0.5f));

  CreateEmpty(ascent, ascent + descent + line_gap);

  Utf32Reader reader;
  reader.Reset(utf8_chars);
  while (true) {
    Ui32 cp = reader.ReadOne();
    if (cp == 0) {
      break;
    }

    int advance_unscaled, lsb_unscaled;
    stbtt_GetCodepointHMetrics(&font_info,
      static_cast<int>(cp), &advance_unscaled, &lsb_unscaled);

    int w = 0, h = 0, xoff = 0, yoff = 0;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(
      &font_info, scale, scale,
      static_cast<int>(cp), &w, &h, &xoff, &yoff);

    Si32 xadvance = static_cast<Si32>(
      std::floor(advance_unscaled * scale + 0.5f));

    if (!bitmap || w <= 0 || h <= 0) {
      // Invisible glyph (e.g. space) - add with empty sprite
      if (bitmap) {
        stbtt_FreeBitmap(bitmap, nullptr);
      }
      Sprite empty;
      AddGlyph(cp, xadvance, empty);
      continue;
    }

    Sprite glyph_sprite;
    glyph_sprite.Create(w, h);
    Rgba *pixels = const_cast<Rgba*>(glyph_sprite.RgbaData());
    Si32 stride = glyph_sprite.StridePixels();

    // stb_truetype produces top-to-bottom bitmaps,
    // arctic Sprite stores bottom-to-top
    for (Si32 row = 0; row < h; ++row) {
      for (Si32 col = 0; col < w; ++col) {
        Ui8 alpha = bitmap[row * w + col];
        pixels[col + (h - 1 - row) * stride] =
          Rgba(255, 255, 255, alpha);
      }
    }
    stbtt_FreeBitmap(bitmap, nullptr);

    glyph_sprite.UpdateOpaqueSpans();

    // Pivot positions the glyph relative to the baseline cursor.
    // In stb_truetype, xoff/yoff are offsets from the baseline cursor
    // to the top-left of the bitmap (in screen coords where y is down).
    // In arctic (y-up), the sprite bottom-left should be at
    // (cursor_x + xoff, cursor_y - yoff - h), so:
    //   pivot.x = -xoff
    //   pivot.y = h + yoff
    // This matches the BMFont formula: pivot.y = height + yoffset - base,
    // since stb_truetype's yoff = BMFont's (yoffset - base).
    glyph_sprite.SetPivot(Vec2Si32(-xoff, h + yoff));

    AddGlyph(cp, xadvance, glyph_sprite);
  }
}

void FontInstance::LoadSystemFont(const char *font_name, float pixel_height,
                                  const char *utf8_chars, Si32 font_index) {
  Check(!!font_name,
    "Error in FontInstance::LoadSystemFont, font_name is nullptr.");
  std::string path = FindSystemFont(font_name);
  Check(!path.empty(),
    "Error in FontInstance::LoadSystemFont, system font not found: ",
    font_name);
  LoadTtf(path.c_str(), pixel_height, utf8_chars, font_index);
}

void FontInstance::DrawEvaluateSizeImpl(Sprite to_sprite,
    const char *text, bool do_keep_xadvance,
    Si32 x, Si32 y, TextOrigin origin, TextAlignment alignment,
    DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode,
    Rgba color, const std::vector<Rgba> &palete, bool do_draw,
    Vec2Si32 *out_size, bool is_one_line, const char *character, Vec2Si32 *out_position) {

  Si32 next_y = y;
  Vec2Si32 total_size(0, 0);
  if (do_draw || character) {
    DrawEvaluateSizeImpl(to_sprite, text, do_keep_xadvance,
      x, y, origin, alignment, blending_mode, filter_mode, color, palete, false,
      &total_size, false, nullptr, nullptr);
    if (origin == kTextOriginTop) {
      next_y = y - base_to_top_ + line_height_ - outline_;
    } else if (origin == kTextOriginFirstBase) {
      next_y = y + line_height_;
    } else {
      if (origin == kTextOriginBottom) {
        next_y = y + total_size.y - base_to_top_ + line_height_ - outline_;
      } else if (origin == kTextOriginLastBase) {
        next_y = y + total_size.y;
      } else if (origin == kTextOriginCenter) {
        next_y = y + total_size.y / 2;
      }
    }
  }

  Si32 width = 0;
  Si32 max_width = 0;
  Si32 lines = 0;
  Ui32 prev_code = 0;
  bool is_newline = false;
  bool is_first_line = true;
  Si32 newline_count = 1;
  Ui32 color_idx = 0;
  Utf32Reader reader;
  reader.Reset(reinterpret_cast<const Ui8*>(text));
  Glyph *glyph = nullptr;
  Si32 next_x = x;
  bool do_store_character = false;

  while (true) {
    const char *p = (const char*)reader.p;
    if (p == character) {
      do_store_character = true;
    }
    Ui32 code = reader.ReadOne();
    if (code && (code == '\r' || code == '\n')) {
      is_first_line = false;
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
      if (!code || (newline_count && is_one_line && !is_first_line)) {
        if (glyph && !do_keep_xadvance) {
          width += glyph->sprite.Width() - glyph->xadvance;
        }
        max_width = std::max(max_width, width);
        if (out_size && !character) {
          *out_size = Vec2Si32(max_width + outline_*2,
                               lines * line_height_ + outline_*2);
        }
        return;
      }
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

          next_x = x;
          if (do_draw || character) {
            if (alignment == kTextAlignmentLeft) {
              ;
            } else {
              Vec2Si32 size;
              DrawEvaluateSizeImpl(to_sprite, p, do_keep_xadvance,
                x, y, origin, alignment, blending_mode, filter_mode, color, palete, false,
                &size, true, nullptr, nullptr);
              if (alignment == kTextAlignmentRight) {
                next_x -= size.x;
              } else if (alignment == kTextAlignmentCenter) {
                next_x -= size.x / 2;
              }
            }
          }
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
        } else if (character) {
          if (do_store_character) {
            if (out_position) {
              out_position->x = next_x;
              out_position->y = next_y;
            }
            if (out_size) {
              *out_size = glyph->sprite.Size() - glyph->sprite.Pivot();
            }
            return;
          }
          next_x += glyph->xadvance;
        }
      }
    }
  }
}

void Font::Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const TextAlignment alignment,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const Rgba color) {
  font_instance_->DrawEvaluateSizeImpl(GetEngine()->GetBackbuffer(),
      text, false, x, y, origin, alignment,
      blending_mode, filter_mode, color,
      std::vector<Rgba>(), true, nullptr, false, nullptr, nullptr);
}

void Font::Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const TextAlignment alignment,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete) {
  Rgba color = palete.empty() ? Rgba(0xffffffff) : palete[0];
  font_instance_->DrawEvaluateSizeImpl(GetEngine()->GetBackbuffer(),
      text, false, x, y, origin, alignment,
      blending_mode, filter_mode, color,
      palete, true, nullptr, false, nullptr, nullptr);
}



}  // namespace arctic
