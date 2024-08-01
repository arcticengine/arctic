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

#include <list>
#include <memory>
#include <vector>

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

struct Letter {
  Ui64 glyph_bitmap;
  const char *utf8_letter;
};

extern Letter g_tiny_font_letters[];

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
  kTextOriginTop = 3,  ///< The top of the first text line
  kTextOriginCenter = 4 ///< The center between the top of the first and the bottom of the last text line
};

enum TextAlignment {
  kTextAlignmentLeft = 0,
  kTextAlignmentCenter = 1,
  kTextAlignmentRight = 2
};

class FontInstance {
public:
  std::vector<Glyph*> codepoint_;
  std::list<Glyph> glyph_;
  Si32 base_to_top_ = 0;
  Si32 base_to_bottom_ = 0;
  Si32 line_height_ = 0;
  Si32 outline_ = 0;

  FontInstance() {
  }

  FontInstance(const FontInstance &font) {
    glyph_ = font.glyph_;
    base_to_top_ = font.base_to_top_;
    base_to_bottom_ = font.base_to_bottom_;
    line_height_ = font.line_height_;
    outline_ = font.outline_;
    GenerateCodepointVector();
  }

  void CreateEmpty(Si32 base_to_top, Si32 line_height);
  void AddGlyph(const Glyph &glyph);
  void AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite);
  void Load(const char *file_name);
  void LoadXml(const char *file_name);
  void LoadAsciiSquare(const char *file_name, bool is_dense);
  void LoadBinaryFnt(const char *file_name);
  void LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
                            Si32 base_to_top, Si32 line_height, Si32 space_width);
  void LoadTable(Sprite sprite, const char* utf8_letters,
                 Si32 cell_width, Si32 cell_height,
                 Si32 base_to_top, Si32 line_height, Si32 space_width,
                 Si32 left_offset);
  void LoadLetterBits(Letter *in_letters, Si32 base_to_top, Si32 line_height);
  void DrawEvaluateSizeImpl(Sprite to_sprite,
                            const char *text, bool do_keep_xadvance,
                            Si32 x, Si32 y, TextOrigin origin,
                            TextAlignment alignment,
                            DrawBlendingMode blending_mode,
                            DrawFilterMode filter_mode,
                            Rgba color, const std::vector<Rgba> &palete, bool do_draw,
                            Vec2Si32 *out_size, bool is_one_line);
  Vec2Si32 EvaluateSize(const char *text, bool do_keep_xadvance);
  void Draw(Sprite to_sprite, const char *text,
            const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const Rgba color);
  void Draw(Sprite to_sprite, const char *text,
            const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const std::vector<Rgba> &palete);
  void Draw(const char *text, const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const Rgba color);
  void Draw(const char *text, const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const std::vector<Rgba> &palete);
  void GenerateCodepointVector();
};

class Font {
 private:
  std::shared_ptr<arctic::FontInstance> font_instance_;
 public:

  Font() {
    font_instance_ = std::make_shared<arctic::FontInstance>();
  }

  Font(const Font &font) {
    font_instance_ = font.font_instance_;
  }

  /// @brief Returns true for fonts with no codepoints. False for fonts with codepoints.
  inline bool IsEmpty() const {
    return font_instance_->codepoint_.empty();
  }

  /// @brief Returns outline size in pixels. Outline*2 is counted towards size.
  Si32 GetOutlineSize() {
    return font_instance_->outline_;
  }

  /// @brief Creates an empty font with no glyphs
  /// @param [in] base_to_top Glyph height from base to top.
  /// @param [in] line_height Line height for the font.
  void CreateEmpty(Si32 base_to_top, Si32 line_height) {
    font_instance_->CreateEmpty(base_to_top, line_height);
  }

  /// @brief Adds a glyph to the font
  /// @param [in] glyph Glyph to add.
  void AddGlyph(const Glyph &glyph) {
    font_instance_->AddGlyph(glyph);
  }
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
  void AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite) {
    font_instance_->AddGlyph(codepoint, xadvance, sprite);
  }

  /// @brief Loads the font from file
  /// @param [in] file_name Path to the font file to load.
  void Load(const char *file_name) {
    font_instance_->Load(file_name);
  }

  /// @brief Loads the font from a sprite containing a stripe of letters
  /// @param [in] sprite Sprite containing 1 pixel divided glyphs in a horizontal stripe.
  /// @param [in] utf8_letters A zero-terminated UTF8 string containing codepoints that
  ///   correspond to the glyphs of the sprite.
  /// @param [in] base_to_top Glyph height from base to top.
  /// @param [in] line_height Line height for the font.
  /// @param [in] space_width Space glyph widht.
  void LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
      Si32 base_to_top, Si32 line_height, Si32 space_width) {
    font_instance_->LoadHorizontalStripe(sprite, utf8_letters, base_to_top, line_height, space_width);
  }

  /// @brief Loads the font from a sprite containing a table of letters
  /// @param [in] sprite Sprite containing glyphs in a table.
  /// @param [in] utf8_letters A zero-terminated UTF8 string containing codepoints that
  ///   correspond to the glyphs of the sprite (in left to right bottom to top order).
  /// @param [in] cell_width Table cell width.
  /// @param [in] cell_height Table cell height.
  /// @param [in] base_to_top Glyph height from base to top.
  /// @param [in] line_height Line height for the font.
  /// @param [in] space_width Space glyph widht.
  /// @param [in] left_offset Glyph offset from the left edge of the cell.
  void LoadTable(Sprite sprite, const char* utf8_letters,
      Si32 cell_width, Si32 cell_height,
      Si32 base_to_top, Si32 line_height, Si32 space_width,
      Si32 left_offset) {
    font_instance_->LoadTable(sprite, utf8_letters, cell_width, cell_height,
                              base_to_top, line_height, space_width, left_offset);
  }

  void LoadLetterBits(Letter *in_letters, Si32 base_to_top, Si32 line_height) {
    font_instance_->LoadLetterBits(in_letters, base_to_top, line_height);
  }

  Vec2Si32 EvaluateSize(const char *text, bool do_keep_xadvance) {
    Vec2Si32 size;
    Sprite empty;
    font_instance_->DrawEvaluateSizeImpl(empty, text, do_keep_xadvance,
                                         0, 0, kTextOriginFirstBase, kTextAlignmentLeft, kDrawBlendingModeCopyRgba, kFilterNearest,
                                         Rgba(255, 255, 255), std::vector<Rgba>(), false,
                                         &size, false);
    return size;
  }

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   destination sprite
  /// @param [in] to_sprite Destination sprite.
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X destination sprite coordinate to draw text at.
  /// @param [in] y Y destination sprite coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified
  ///   coordinates.
  /// @param [in] alignment The alignment that will be used for the text.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  void Draw(Sprite to_sprite, const char *text,
      const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const TextAlignment alignment = kTextAlignmentLeft,
      const DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      const DrawFilterMode filter_mode = kFilterNearest,
      const Rgba color = Rgba(0xffffffff)) {
    font_instance_->DrawEvaluateSizeImpl(to_sprite,
                                         text, false, x, y, origin, alignment, blending_mode, filter_mode, color,
                                         std::vector<Rgba>(), true, nullptr, false);
  }

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   destination sprite
  /// @param [in] to_sprite Destination sprite.
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X destination sprite coordinate to draw text at.
  /// @param [in] y Y destination sprite coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified
  ///   coordinates.
  /// @param [in] alignment The alignment that will be used for the text.
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
      const TextAlignment alignment,
      const DrawBlendingMode blending_mode,
      const DrawFilterMode filter_mode,
      const std::vector<Rgba> &palete) {
    font_instance_->DrawEvaluateSizeImpl(to_sprite,
                                         text, false, x, y, origin, alignment, blending_mode, filter_mode, palete[0],
                                         palete, true, nullptr, false);
  }

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   backbuffer
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X screen coordinate to draw text at.
  /// @param [in] y Y screen coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified screen
  ///   coordinates.
  /// @param [in] alignment The alignment that will be used for the text.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const TextAlignment alignment = kTextAlignmentLeft,
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
  /// @param [in] alignment The alignment that will be used for the text.
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  /// @param [in] palete The vector of Rgba colors used to colorize the text
  ///   by some blending modes (for example, the kDrawBlendingModeColorize
  ///   blending mode). Characters with codepoints <= 8 are considered the
  ///   color-control characters and select the color from the palatte for the
  ///   following text.
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin,
      const TextAlignment alignment,
      const DrawBlendingMode blending_mode,
      const DrawFilterMode filter_mode,
      const std::vector<Rgba> &palete);

  const std::shared_ptr<FontInstance> &FontInstance() const {
    return font_instance_;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_FONT_H_
