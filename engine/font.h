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

#ifndef ENGINE_FONT_H_
#define ENGINE_FONT_H_

#include <vector>
#include <list>

#include "engine/arctic_types.h"
#include "engine/easy_sprite.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

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
  void Log() const;
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
  void Log() const;
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
  void Log() const;
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
  void Log(Si32 id) const;
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
  void Log() const;
};

struct BmFontBinKerningPair {
  // kerning pairs
  Ui32 first;  // These fields are repeated until all kerning pairs have been
               // described
  Ui32 second;
  Si16 amount;
  void Log() const;
};
#pragma pack(pop)

struct Glyph {
  Ui32 codepoint;
  Si32 xadvance;
  Sprite sprite;

  Glyph(Ui32 in_codepoint, Si32 in_xadvance, Sprite in_sprite)
    : codepoint(in_codepoint)
    , xadvance(in_xadvance)
    , sprite(in_sprite) {
  }
};
/// @}

/// @addtogroup global_drawing
/// @{

/// The origin point used for rendering
enum TextOrigin {
  kTextOriginBottom = 0,  ///< The bottom of the last text line
  kTextOriginFirstBase = 1,  ///< The base of the first text line
  kTextOriginLastBase = 2,  ///< The base of the last text line
  kTextOriginTop = 3  ///< The top of the first text line
};

struct Font {
  std::vector<Glyph*> codepoint_;
  std::list<Glyph> glyph_;
  Si32 base_to_top_ = 0;
  Si32 base_to_bottom_ = 0;
  Si32 line_height_ = 0;
  Si32 outline_ = 0;

  /// @brief Returns outline size in pixels. Outline*2 is counted towards size.
  Si32 GetOutlineSize() {
    return outline_;
  }

  /// @brief Creates an empty font with no glyphs
  /// @param [in] base_to_top Glyph height from base to top.
  void CreateEmpty(Si32 base_to_top, Si32 line_height);

  /// @brief Adds a glyph to the font
  /// @param [in] glyph Glyph to add.
  void AddGlyph(const Glyph &glyph);
  /// @brief Adds a glyph to the font
  /// @param [in] codepoint UTF32 codepoint the glyph represents.
  /// @param [in] xadvance The increment of the 'cursor' x position used
  ///   for font rendering.
  ///   \code
  /// |XXXXXXX  |XXXXXXX
  /// X       X X       X
  /// X       X X       X
  /// |XXXXXXX  |XXXXXXX
  /// |         |
  /// |<------->|
  ///  xadvance
  ///   \endcode
  /// @param [in] sprite The Sprite containing the graphical representation of
  ///   the glyph.
  void AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite);

  /// @brief Loads the font from file
  /// @param [in] file_name Path to the font file to load.
  void Load(const char *file_name);

  void DrawEvaluateSizeImpl(Sprite to_sprite,
      const char *text, bool do_keep_xadvance,
      Si32 x, Si32 y, TextOrigin origin,
      DrawBlendingMode blending_mode,
      DrawFilterMode filter_mode,
      Rgba color, const std::vector<Rgba> &palete, bool do_draw,
      Vec2Si32 *out_size);
  Vec2Si32 EvaluateSize(const char *text, bool do_keep_xadvance);

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   destination sprite
  /// @param [in] to_sprite Destination sprite.
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X destination sprite coordinate to draw text at.
  /// @param [in] y Y destination sprite coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified
  ///   coordinates.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  void Draw(Sprite to_sprite, const char *text,
      const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      const DrawFilterMode filter_mode = kFilterNearest,
      const Rgba color = Rgba(0xffffffff));

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   destination sprite
  /// @param [in] to_sprite Destination sprite.
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X destination sprite coordinate to draw text at.
  /// @param [in] y Y destination sprite coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified
  ///   coordinates.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] palete The vector of Rgba colors used to colorize the text
  ///   by some blending modes (for example, the kDrawBlendingModeColorize
  ///   blending mode). Characters with codepoints <= 8 are considered the
  ///   color-control characters and select the color from the palatte for the
  ///   following text.
  void Draw(Sprite to_sprite, const char *text,
    const Si32 x, const Si32 y,
    const TextOrigin origin,
    const DrawBlendingMode blending_mode,
    const DrawFilterMode filter_mode,
    const std::vector<Rgba> &palete);

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   backbuffer
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X screen coordinate to draw text at.
  /// @param [in] y Y screen coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified screen
  ///   coordinates.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      const DrawFilterMode filter_mode = kFilterNearest,
      const Rgba color = Rgba(0xffffffff));

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   backbuffer
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X screen coordinate to draw text at.
  /// @param [in] y Y screen coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified screen
  ///   coordinates.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] palete The vector of Rgba colors used to colorize the text
  ///   by some blending modes (for example, the kDrawBlendingModeColorize
  ///   blending mode). Characters with codepoints <= 8 are considered the
  ///   color-control characters and select the color from the palatte for the
  ///   following text.
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin,
      const DrawBlendingMode blending_mode,
      const DrawFilterMode filter_mode,
      const std::vector<Rgba> &palete);
};
/// @}

}  // namespace arctic

#endif  // ENGINE_FONT_H_
