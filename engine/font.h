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
    std::cerr << "\ty=" << y;
    std::cerr << "\twidth=" << width;
    std::cerr << "\theight=" << height;
    std::cerr << "\txoffset=" << xoffset;
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

// This block is only in the file if there are any kerning pairs with amount
// differing from 0.
struct Font {
  void Load(const char *file_name) {
    std::vector<Ui8> file = easy::ReadFile(file_name);
    Si32 pos = 0;
    BmFontBinHeader *header = reinterpret_cast<BmFontBinHeader*>(&file[pos]);
    header->Log();
    pos += sizeof(BmFontBinHeader);
    
    Si8 block_type = file[pos];
    ++pos;
    Si32 block_size = *reinterpret_cast<Si32*>(&file[pos]);
    pos += sizeof(Si32);
    Check(block_type == kBlockInfo, "Unexpected block type");
  
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
    Check(block_type == kBlockCommon, "Unexpected block type");
    Check(block_size >= sizeof(BmFontBinCommon), "Common block is too small");
    BmFontBinCommon *common = reinterpret_cast<BmFontBinCommon*>(&file[pos]);
    common->Log();
    pos += block_size;

    block_type = file[pos];
    ++pos;
    block_size = *reinterpret_cast<Si32*>(&file[pos]);
    pos += sizeof(Si32);
    Check(block_type == kBlockPages, "Unexpected block type");
    Check(block_size >= 1, "Pages block is too small");
    Si32 inner_pos = pos;
    for (Si32 id = 0; id < common->pages; ++id) {
      BmFontBinPages page;
      page.page_name = reinterpret_cast<char*>(&file[inner_pos]);
      page.Log(id);
      inner_pos += std::strlen(page.page_name) + 1;
    }
    pos += block_size;
    
    block_type = file[pos];
    ++pos;
    block_size = *reinterpret_cast<Si32*>(&file[pos]);
    pos += sizeof(Si32);
    Check(block_type == kBlockChars, "Unexpected block type");
    Check(block_size >= sizeof(BmFontBinChars), "Pages block is too small");
    inner_pos = pos;
    for (Si32 id = 0; id < block_size / 20; ++id) {
      BmFontBinChars *chars = reinterpret_cast<BmFontBinChars*>(&file[inner_pos]);
      chars->Log();
      inner_pos += 20;
    }
    pos += block_size;
    
    block_type = file[pos];
    ++pos;
    block_size = *reinterpret_cast<Si32*>(&file[pos]);
    pos += sizeof(Si32);
    Check(block_type == kBlockKerningPairs, "Unexpected block type");
    Check(block_size >= sizeof(BmFontBinKerningPair),
          "KerningPair block is too small");
    inner_pos = pos;
    for (Si32 id = 0; id < block_size / 10; ++id) {
      BmFontBinKerningPair *kerning_pair = reinterpret_cast<BmFontBinKerningPair*>(&file[inner_pos]);
      kerning_pair->Log();
      inner_pos += 10;
    }
    pos += block_size;

  }
};

}  // namespace arctic

#endif  // ENGINE_FONT_H_
