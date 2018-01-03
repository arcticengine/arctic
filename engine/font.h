// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#ifndef ENGINE_FONT_H_
#define ENGINE_FONT_H_

#include <cstring>
#include <iostream>
#include <vector>
#include <list>

#include "engine/arctic_types.h"
#include "engine/easy.h"

namespace arctic {

#pragma pack(push, 1)
enum BmFontBlockType {
  kBlockInfo = 1,
  kBlockCommon = 2,
  kBlockPages = 3,
  kBlockChars = 4,
  kBlockKerningPairs = 5
};

struct BmFontBinHeader {
  Si8 b;
  Si8 m;
  Si8 f;
  Ui8 version;
  void Log() {
    // TODO(Huldra): Use log here
    std::cerr << "header";
    std::cerr << " bmf=" << ((b == 66 && m == 77 && f == 70) ? 1 : 0);
    std::cerr << " version=" << static_cast<Si32>(version);
    std::cerr << std::endl;
  }
};

struct BmFontBinInfo {
  enum Bits {
		kSmooth = 128,
		kUnicode = 64,
		kItalic = 32,
		kBold = 16,
		kFixedHeight = 8
	};
  
	Si16 font_size;
  Ui8 bits;
	Ui8 char_set;
	Ui16 stretch_h;
	Ui8 aa;
	Ui8 padding_up;
	Ui8 padding_right;
	Ui8 padding_down;
	Ui8 padding_left;
	Ui8 spacing_horiz;
	Ui8 spacing_vert;
	Ui8 outline;
	char *font_name;	// n+1	string	14	null terminated string with length n
	// This structure gives the layout of the fields.
  // Remember that there should be no padding between members.
  // Allocate the size of the block using the blockSize,
  // as following the block comes the font name,
  // including the terminating null char.
  // Most of the time this block can simply be ignored.
  void Log() {
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
};

struct BmFontBinCommon {
  enum Bits {
		kPacked = 1
	};

	Ui16 line_height;
	Ui16 base;
	Ui16 scale_w;
	Ui16 scale_h;
	Ui16 pages;
  Ui8 bits;
	Ui8 alpha_chnl;
	Ui8 red_chnl;
	Ui8 green_chnl;
	Ui8 blue_chnl;
  void Log() {
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
};

struct BmFontBinPages {
	char *page_name;	 // p*(n+1)	strings	0	p null terminated strings,
                         // each with length n
    // This block gives the name of each texture file with the image data
    // for the characters. The string pageNames holds the names separated
  // and terminated by null chars. Each filename has the same length,
  // so once you know the size of the first name, you can easily
  // determine the position of each of the names. The id of each page
  // is the zero-based index of the string name.
  void Log(Si32 id) {
    // TODO(Huldra): Use log here
    std::cerr << "page";
    std::cerr << " id=" << id;
    std::cerr << " file=\"" << page_name << "\"";
    std::cerr << std::endl;
  }
};

struct BmFontBinChars {
	Ui32 id;  // These fields are repeated until all characters have been
              // described
	Ui16 x;
	Ui16 y;
	Ui16 width;
	Ui16 height;
	Si16 xoffset;
	Si16 yoffset;
	Si16 xadvance;
	Ui8 page;
	Ui8 chnl;
	// The number of characters in the file can be computed by taking the
  // size of the block and dividing with the size of the charInfo structure,
  // i.e.: numChars = charsBlock.blockSize/20.
  void Log() {
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
};

struct BmFontBinKerningPair {
	// kerning pairs
	Ui32 first;  // These fields are repeated until all kerning pairs have been
               // described
	Ui32 second;
	Si16 amount;
  void Log() {
    // TODO(Huldra): Use log here
    std::cerr << "kerning";
    std::cerr << " first=" << first;
    std::cerr << "\tsecond=" << second;
    std::cerr << "\tamount=" << amount;
    std::cerr << std::endl;
  }
};
#pragma pack(pop)
  
struct Utf32Reader {
  const Ui8 *begin = nullptr;
  const Ui8 *p = nullptr;
  
  void Reset(const Ui8 *data) {
    begin = data;
    p = data;
  }
  
  void Rewind() {
    p = begin;
  }
  
  Ui32 ReadOne() {
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
};

struct Glyph {
  Ui32 codepoint;
  Si32 xadvance;
  easy::Sprite sprite;
  
  Glyph(Ui32 in_codepoint, Si32 in_xadvance, easy::Sprite in_sprite)
    : codepoint(in_codepoint)
    , xadvance(in_xadvance)
    , sprite(in_sprite)
  {
  }
};

struct Font {
  std::vector<Glyph*> codepoint;
  std::list<Glyph> glyph;
  
  void Load(const char *file_name) {
    codepoint.clear();
    glyph.clear();
    
    
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
      
      easy::Sprite sprite;
      sprite.Reference(page_images[chars->page],
          chars->x, page_images[chars->page].Height() - chars->y - chars->height,
          chars->width, chars->height);
      sprite.SetPivot(arctic::Vec2Si32(
          chars->xoffset, chars->height + chars->yoffset - common->base));
      glyph.emplace_back(chars->id, chars->xadvance, sprite);
      
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
    ////////
    Ui32 end_codepoint = 0;
    for (auto it = glyph.begin(); it != glyph.end(); ++it) {
      if (it->codepoint >= end_codepoint) {
        end_codepoint = it->codepoint + 1;
      }
    }
    codepoint.resize(end_codepoint, nullptr);
    for (auto it = glyph.begin(); it != glyph.end(); ++it) {
      codepoint[it->codepoint] = &(*it);
    }
  }
  
  void Draw(const char *text, const Si32 x, const Si32 y) {
    Utf32Reader reader;
    reader.Reset(reinterpret_cast<const Ui8*>(text));
    Si32 next_x = x;
    while (true) {
      Ui32 code = reader.ReadOne();
      if (!code) {
        return;
      }
      if (code < codepoint.size() && codepoint[code]) {
        Glyph &glyph = *codepoint[code];
        glyph.sprite.Draw(next_x, y);
        //next_x += glyph.sprite.Width();
        next_x += glyph.xadvance;
      }
    }
  }
};

}  // namespace arctic

#endif  // ENGINE_FONT_H_
