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

#ifndef ENGINE_FONT_H_
#define ENGINE_FONT_H_

#include <vector>
#include <list>

#include "engine/arctic_types.h"
#include "engine/easy_sprite.h"

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
  void Log();
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
  char *font_name;  // n+1 string
  // 14 null terminated string with length n
  // This structure gives the layout of the fields.
  // Remember that there should be no padding between members.
  // Allocate the size of the block using the blockSize,
  // as following the block comes the font name,
  // including the terminating null char.
  // Most of the time this block can simply be ignored.
  void Log();
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
  void Log();
};

struct BmFontBinPages {
  char *page_name;  // p*(n+1) strings 0 p null terminated strings,
                    // each with length n
  // This block gives the name of each texture file with the image data
  // for the characters. The string pageNames holds the names separated
  // and terminated by null chars. Each filename has the same length,
  // so once you know the size of the first name, you can easily
  // determine the position of each of the names. The id of each page
  // is the zero-based index of the string name.
  void Log(Si32 id);
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
  void Log();
};

struct BmFontBinKerningPair {
  // kerning pairs
  Ui32 first;  // These fields are repeated until all kerning pairs have been
               // described
  Ui32 second;
  Si16 amount;
  void Log();
};
#pragma pack(pop)

struct Utf32Reader {
  const Ui8 *begin = nullptr;
  const Ui8 *p = nullptr;

  void Reset(const Ui8 *data);
  void Rewind();
  Ui32 ReadOne();
};

struct Glyph {
  Ui32 codepoint;
  Si32 xadvance;
  easy::Sprite sprite;

  Glyph(Ui32 in_codepoint, Si32 in_xadvance, easy::Sprite in_sprite)
    : codepoint(in_codepoint)
    , xadvance(in_xadvance)
    , sprite(in_sprite) {
  }
};

enum TextOrigin {
  kTextOriginBottom = 0,
  kTextOriginFirstBase = 1,
  kTextOriginLastBase = 2,
  kTextOriginTop = 3
};

struct Font {
  std::vector<Glyph*> codepoint_;
  std::list<Glyph> glyph_;
  Si32 base_to_top_ = 0;
  Si32 base_to_bottom_ = 0;
  Si32 line_height_ = 0;

  void CreateEmpty(Si32 base_to_top, Si32 line_height);
  void AddGlyph(const Glyph &glyph);
  void AddGlyph(Ui32 codepoint, Si32 xadvance, easy::Sprite sprite);
  void Load(const char *file_name);
  void DrawEvaluateSizeImpl(easy::Sprite to_sprite,
    const char *text, bool do_keep_xadvance,
    Si32 x, Si32 y, TextOrigin origin,
    easy::DrawBlendingMode blending_mode,
    easy::DrawFilterMode filter_mode,
    Rgba color, const std::vector<Rgba> &palete, bool do_draw,
    Vec2Si32 *out_size);
  Vec2Si32 EvaluateSize(const char *text, bool do_keep_xadvance);
  void Draw(easy::Sprite to_sprite, const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin = kTextOriginBottom,
    const easy::DrawBlendingMode blending_mode = easy::kAlphaBlend,
    const easy::DrawFilterMode filter_mode = easy::kFilterNearest,
    const Rgba color = Rgba(0xffffffff));
  void Draw(easy::Sprite to_sprite, const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const easy::DrawBlendingMode blending_mode,
    const easy::DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete);
  void Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin = kTextOriginBottom,
    const easy::DrawBlendingMode blending_mode = easy::kAlphaBlend,
    const easy::DrawFilterMode filter_mode = easy::kFilterNearest,
    const Rgba color = Rgba(0xffffffff));
  void Draw(const char *text, const Si32 x, const Si32 y,
    const TextOrigin origin,
    const easy::DrawBlendingMode blending_mode,
    const easy::DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete);
};

}  // namespace arctic

#endif  // ENGINE_FONT_H_
