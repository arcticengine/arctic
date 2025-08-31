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
/// @brief Enumeration of BMFont block types
enum BmFontBlockType {
  kBlockInfo = 1,  ///< Font information block
  kBlockCommon = 2,  ///< Common block
  kBlockPages = 3,  ///< Pages block
  kBlockChars = 4,  ///< Characters block
  kBlockKerningPairs = 5  ///< Kerning pairs block
};

/// @brief Structure representing the BMFont binary header
struct BmFontBinHeader {
  Si8 b;
  Si8 m;
  Si8 f;
  Ui8 version;  ///< Version of the BMFont format
  void Log() const;  ///< Log the header information
};

/// @brief Structure representing the BMFont binary info block
struct BmFontBinInfo {
  /// @brief Enumeration of bit flags for the info block
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
  void Log() const;  ///< Log the info block information
};

/// @brief Structure representing the BMFont binary common block
struct BmFontBinCommon {
  /// @brief Enumeration of bit flags for the common block
  enum Bits {
    kPacked = 1
  };

  Ui16 line_height;  ///< Line height
  Ui16 base;  ///< Base line
  Ui16 scale_w;  ///< Width scale
  Ui16 scale_h;  ///< Height scale
  Ui16 pages;  ///< Number of pages
  Ui8 bits;  ///< Bits per pixel
  Ui8 alpha_chnl;  ///< Alpha channel
  Ui8 red_chnl;  ///< Red channel
  Ui8 green_chnl;  ///< Green channel
  Ui8 blue_chnl;  ///< Blue channel
  void Log() const;
};

/// @brief Structure representing the BMFont binary pages block
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

/// @brief Structure representing the BMFont binary characters block
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

/// @brief Structure representing the BMFont binary kerning pairs block
struct BmFontBinKerningPair {
  // kerning pairs
  Ui32 first;  // These fields are repeated until all kerning pairs have been
               // described
  Ui32 second;
  Si16 amount;
  void Log() const;
};

#pragma pack(pop)

/// @brief Structure representing a letter in the tiny font
struct Letter {
  Ui64 glyph_bitmap;
  const char *utf8_letter;
};

/// @brief Array of letters for the tiny font
extern Letter g_tiny_font_letters[];

/// @brief Structure representing a glyph in the font
struct Glyph {
  Ui32 codepoint;
  Si32 xadvance;
  Sprite sprite;

  /// @brief Constructor for a glyph
  /// @param [in] in_codepoint The codepoint of the glyph
  /// @param [in] in_xadvance The x advance of the glyph
  /// @param [in] in_sprite The sprite containing the glyph
  Glyph(Ui32 in_codepoint, Si32 in_xadvance, Sprite in_sprite)
    : codepoint(in_codepoint)
    , xadvance(in_xadvance)
    , sprite(in_sprite) {
  }
};
/// @}

/// @addtogroup global_drawing
/// @{

/// @brief The origin point used for rendering
enum TextOrigin {
  kTextOriginBottom = 0,  ///< The bottom of the last text line
  kTextOriginFirstBase = 1,  ///< The base of the first text line
  kTextOriginLastBase = 2,  ///< The base of the last text line
  kTextOriginTop = 3,  ///< The top of the first text line
  kTextOriginCenter = 4 ///< The center between the top of the first and the bottom of the last text line
};

/// @brief Enumeration of text alignment options
enum TextAlignment {
  kTextAlignmentLeft = 0,
  kTextAlignmentCenter = 1,
  kTextAlignmentRight = 2
};

/// @brief Class representing a font instance
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

  /// @brief Copy constructor for a font instance
  /// @param [in] font The font instance to copy
  FontInstance(const FontInstance &font) {
    glyph_ = font.glyph_;
    base_to_top_ = font.base_to_top_;
    base_to_bottom_ = font.base_to_bottom_;
    line_height_ = font.line_height_;
    outline_ = font.outline_;
    GenerateCodepointVector();
  }

  /// @brief Creates an empty font instance
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  void CreateEmpty(Si32 base_to_top, Si32 line_height);

  /// @brief Adds a glyph to the font instance
  /// @param [in] glyph The glyph to add
  void AddGlyph(const Glyph &glyph);

  /// @brief Adds a glyph to the font instance
  /// @param [in] codepoint Unicode codepoint of the glyph
  /// @param [in] xadvance Horizontal advance of the glyph
  /// @param [in] sprite Sprite representing the glyph
  void AddGlyph(Ui32 codepoint, Si32 xadvance, Sprite sprite);

  /// @brief Loads a font from a file
  /// @param [in] file_name Path to the font file to load.
  ///
  /// This method loads a font from a file. The file must be in one of the following formats:
  /// - BMFont binary format (.fnt) - Most common format, created by BMFont tool
  /// - BMFont XML format (.xml) - Alternative format for BMFont
  ///
  /// The font file must be accompanied by its corresponding texture file(s) in the same directory.
  /// For example, if you have "font.fnt", you should also have "font_0.png" (and "font_1.png", "font_2.png", etc.
  /// if the font uses multiple texture pages).
  ///
  /// @note TTF (TrueType Font) files are NOT supported. You must convert your TTF fonts to BMFont format
  /// using tools like BMFont (https://www.angelcode.com/products/bmfont/) before using them.
  ///
  /// @throws Fatal error if the file extension is not recognized or the file cannot be loaded
  void Load(const char *file_name);

  /// @brief Loads a font from an XML file
  /// @param [in] file_name Path to the XML font file
  void LoadXml(const char *file_name);

  /// @brief Loads an ASCII square font from a file
  /// @param [in] file_name Path to the font file
  /// @param [in] is_dense Whether the font is densely packed
  void LoadAsciiSquare(const char *file_name, bool is_dense);

  /// @brief Loads a binary BMFont file
  /// @param [in] file_name Path to the binary BMFont file
  void LoadBinaryFnt(const char *file_name);

  /// @brief Loads a font from a horizontal stripe of glyphs
  /// @param [in] sprite Sprite containing the glyph stripe
  /// @param [in] utf8_letters UTF-8 encoded string of letters in the sprite
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  /// @param [in] space_width Width of the space character
  void LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
                            Si32 base_to_top, Si32 line_height, Si32 space_width);

  /// @brief Loads a font from a table of glyphs
  /// @param [in] sprite Sprite containing the glyph table
  /// @param [in] utf8_letters UTF-8 encoded string of letters in the sprite
  /// @param [in] cell_width Width of each cell in the table
  /// @param [in] cell_height Height of each cell in the table
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  /// @param [in] space_width Width of the space character
  /// @param [in] left_offset Left offset of glyphs within cells
  void LoadTable(Sprite sprite, const char* utf8_letters,
                 Si32 cell_width, Si32 cell_height,
                 Si32 base_to_top, Si32 line_height, Si32 space_width,
                 Si32 left_offset);

  /// @brief Loads a font from an array of Letter structures
  /// @param [in] in_letters Array of Letter structures
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  void LoadLetterBits(Letter *in_letters, Si32 base_to_top, Si32 line_height);

  /// @brief Draws text or evaluates its size
  /// @param [in] to_sprite Sprite to draw on (can be empty for size evaluation)
  /// @param [in] text Text to draw or evaluate
  /// @param [in] do_keep_xadvance Whether to keep x-advance
  /// @param [in] x X-coordinate for drawing
  /// @param [in] y Y-coordinate for drawing
  /// @param [in] origin Text origin
  /// @param [in] alignment Text alignment
  /// @param [in] blending_mode Blending mode for drawing
  /// @param [in] filter_mode Filter mode for drawing
  /// @param [in] color Color for drawing
  /// @param [in] palete Color palette for drawing
  /// @param [in] do_draw Whether to actually draw or just evaluate size
  /// @param [out] out_size Output size of the text or character
  /// @param [in] is_one_line Whether the text is single-line
  /// @param [in] character Pointer to the character to get position of
  /// @param [out] out_position Output position of the character
  void DrawEvaluateSizeImpl(Sprite to_sprite,
                            const char *text, bool do_keep_xadvance,
                            Si32 x, Si32 y, TextOrigin origin,
                            TextAlignment alignment,
                            DrawBlendingMode blending_mode,
                            DrawFilterMode filter_mode,
                            Rgba color, const std::vector<Rgba> &palete, bool do_draw,
                            Vec2Si32 *out_size, bool is_one_line, const char *character, Vec2Si32 *out_position);

  /// @brief Evaluates the size of text
  /// @param [in] text Text to evaluate
  /// @param [in] do_keep_xadvance Whether to keep x-advance
  /// @return Size of the text
  Vec2Si32 EvaluateSize(const char *text, bool do_keep_xadvance);

  /// @brief Draws text to a sprite
  /// @param [in] to_sprite Sprite to draw on
  /// @param [in] text Text to draw
  /// @param [in] x X-coordinate for drawing
  /// @param [in] y Y-coordinate for drawing
  /// @param [in] origin Text origin
  /// @param [in] blending_mode Blending mode for drawing
  /// @param [in] filter_mode Filter mode for drawing
  /// @param [in] color Color for drawing
  void Draw(Sprite to_sprite, const char *text,
            const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const Rgba color);

  /// @brief Draws text to a sprite using a color palette
  /// @param [in] to_sprite Sprite to draw on
  /// @param [in] text Text to draw
  /// @param [in] x X-coordinate for drawing
  /// @param [in] y Y-coordinate for drawing
  /// @param [in] origin Text origin
  /// @param [in] blending_mode Blending mode for drawing
  /// @param [in] filter_mode Filter mode for drawing
  /// @param [in] palete Color palette for drawing
  void Draw(Sprite to_sprite, const char *text,
            const Si32 x, const Si32 y,
            const TextOrigin origin,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const std::vector<Rgba> &palete);

  /// @brief Draws text to the screen
  /// @param [in] text Text to draw
  /// @param [in] x X-coordinate for drawing
  /// @param [in] y Y-coordinate for drawing
  /// @param [in] origin Text origin
  /// @param [in] alignment Text alignment
  /// @param [in] blending_mode Blending mode for drawing
  /// @param [in] filter_mode Filter mode for drawing
  /// @param [in] color Color for drawing
  void Draw(const char *text, const Si32 x, const Si32 y,
            const TextOrigin origin,
            const TextAlignment alignment,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const Rgba color);

  /// @brief Draws text to the screen using a color palette
  /// @param [in] text Text to draw
  /// @param [in] x X-coordinate for drawing
  /// @param [in] y Y-coordinate for drawing
  /// @param [in] origin Text origin
  /// @param [in] alignment Text alignment
  /// @param [in] blending_mode Blending mode for drawing
  /// @param [in] filter_mode Filter mode for drawing
  /// @param [in] palete Color palette for drawing
  void Draw(const char *text, const Si32 x, const Si32 y,
            const TextOrigin origin,
            const TextAlignment alignment,
            const DrawBlendingMode blending_mode,
            const DrawFilterMode filter_mode,
            const std::vector<Rgba> &palete);

  /// @brief Generates the codepoint vector for the font instance
  void GenerateCodepointVector();
};

/// @brief Class representing a font for text rendering
///
/// The Font class provides functionality for loading and rendering text in your game.
/// It supports various text origins, alignments, blending modes, and colors.
///
/// @section font_usage Basic Usage
/// 
/// 1. Create a Font instance:
///    @code
///    Font gameFont;
///    @endcode
///
/// 2. Load a font file (BMFont format):
///    @code
///    gameFont.Load("data/arctic_one_bmf.fnt");
///    @endcode
///
/// 3. Draw text to the screen:
///    @code
///    // Simple white text
///    gameFont.Draw("Hello World", 100, 100, kTextOriginTop, kTextAlignmentLeft);
///    
///    // Colored text
///    gameFont.Draw("Score: 100", 20, 20, kTextOriginTop, kTextAlignmentLeft, 
///                  kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 0));
///    @endcode
///
/// @section font_format Font Format Requirements
///
/// The Arctic Engine font system ONLY supports bitmap fonts in the following formats:
/// - BMFont binary format (.fnt)
/// - BMFont XML format (.xml)
/// - ASCII square font format (16x16 grid)
/// - Horizontal stripe font format
/// - Table font format
///
/// TTF (TrueType Font) files are NOT supported. You must convert your TTF fonts to one of the supported bitmap formats
/// using tools like BMFont (https://www.angelcode.com/products/bmfont/) before using them with the Arctic Engine.
///
/// @section font_blending Blending Modes
/// 
/// The Font class supports several blending modes for text rendering:
///
/// - kDrawBlendingModeColorize: Applies the specified color to the text while preserving
///   its shape and transparency. Works best with black and white fonts.
///   This is the recommended mode for text output.
/// - kDrawBlendingModeSolidColor: Replaces all colors of the font with the specified color.
///   This may work with a pre-colored font file.
///
/// @section font_origins Text Origins
///
/// The origin determines the reference point for positioning text:
///
/// - kTextOriginTop: The top of the text is at the specified y-coordinate
/// - kTextOriginFirstBase: The baseline of the first line is at the specified y-coordinate
/// - kTextOriginBottom: The bottom of the text is at the specified y-coordinate
/// - kTextOriginCenter: The center of the text is at the specified y-coordinate
///
/// @section font_alignment Text Alignment
///
/// The alignment determines how text is positioned horizontally:
///
/// - kTextAlignmentLeft: Text is aligned to the left of the specified x-coordinate
/// - kTextAlignmentCenter: Text is centered at the specified x-coordinate
/// - kTextAlignmentRight: Text is aligned to the right of the specified x-coordinate
class Font {
 private:
  std::shared_ptr<arctic::FontInstance> font_instance_;
 public:

  /// @brief Default constructor
  /// Creates an empty font instance
  Font() {
    font_instance_ = std::make_shared<arctic::FontInstance>();
  }

  /// @brief Copy constructor
  /// @param font The font to copy
  Font(const Font &font) {
    font_instance_ = font.font_instance_;
  }

  /// @brief Checks if the font is empty (has no codepoints)
  /// @return True if the font has no codepoints, false otherwise
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
  /// @param [in] file_name Path to the *.fnt or *.xml font file to load.
  ///
  /// This method loads a font from a file. The file must be in one of the following formats:
  /// - BMFont binary format (.fnt) - Most common format, created by BMFont tool
  /// - BMFont XML format (.xml) - Alternative format for BMFont
  ///
  /// The font file must be accompanied by its corresponding texture file(s) in the same directory.
  /// For example, if you have "font.fnt", you should also have "font_0.tga"
  ///
  /// @note TTF (TrueType Font) files are NOT supported. You must convert your TTF fonts to BMFont format
  /// using tools like BMFont (https://www.angelcode.com/products/bmfont/) before using them.
  ///
  /// @throws Fatal error if the file extension is not recognized or the file cannot be loaded
  void Load(const char *file_name) {
    font_instance_->Load(file_name);
  }

  /// @brief Loads a font from an XML file
  /// @param [in] file_name Path to the XML font file
  void LoadXml(const char *file_name) {
    font_instance_->LoadXml(file_name);
  }

  /// @brief Loads an ASCII square font from a file
  /// @param [in] file_name Path to the font file
  /// @param [in] is_dense Whether the font is densely packed
  void LoadAsciiSquare(const char *file_name, bool is_dense) {
    font_instance_->LoadAsciiSquare(file_name, is_dense);
  }

  /// @brief Loads a binary BMFont file
  /// @param [in] file_name Path to the binary BMFont file
  void LoadBinaryFnt(const char *file_name) {
    font_instance_->LoadBinaryFnt(file_name);
  }

  /// @brief Loads a font from a horizontal stripe of glyphs
  /// @param [in] sprite Sprite containing the glyph stripe
  /// @param [in] utf8_letters UTF-8 encoded string of letters in the sprite
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  /// @param [in] space_width Width of the space character
  void LoadHorizontalStripe(Sprite sprite, const char* utf8_letters,
      Si32 base_to_top, Si32 line_height, Si32 space_width) {
    font_instance_->LoadHorizontalStripe(sprite, utf8_letters, base_to_top, line_height, space_width);
  }

  /// @brief Loads a font from a table of glyphs
  /// @param [in] sprite Sprite containing the glyph table
  /// @param [in] utf8_letters UTF-8 encoded string of letters in the sprite
  /// @param [in] cell_width Width of each cell in the table
  /// @param [in] cell_height Height of each cell in the table
  /// @param [in] base_to_top Distance from baseline to top of the font
  /// @param [in] line_height Height of a line of text
  /// @param [in] space_width Width of the space character
  /// @param [in] left_offset Left offset of glyphs within cells
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
                                         &size, false, nullptr, nullptr);
    return size;
  }


  /// @brief Evaluates the relative position of a character in UTF-8 string
  ///   containing one or more lines of text
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] character a pointer to a character of the UTF-8 c-string.
  /// @param [in] origin The origin that will be used for the evaluation.
  /// @param [in] alignment The alignment that will be used for the evaluation.
  /// @param [out] out_size Pointer to the variable to store the size of the character.
  Vec2Si32 EvaluateCharacterPos(const char *text,
      const char *character,
      const TextOrigin origin = kTextOriginBottom,
      const TextAlignment alignment = kTextAlignmentLeft,
      Vec2Si32 *out_size = nullptr) {
    Vec2Si32 size;
    Vec2Si32 character_position;
    Sprite empty;
    font_instance_->DrawEvaluateSizeImpl(empty,
                                         text, false, 0, 0, origin, alignment, kDrawBlendingModeCopyRgba, kFilterNearest,
                                         Rgba(255, 255, 255), std::vector<Rgba>(), false,
                                         &size, false, character, &character_position);
    if (out_size) {
      *out_size = size;
    }
    return character_position;
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
  ///   - kDrawBlendingModeColorize: Best for colored text (applies the color parameter)
  ///   - kDrawBlendingModeSolidColor: Replaces all colors of the font with the specified color.
  ///   - kDrawBlendingModeAlphaBlend: Standard alpha blending
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  ///   - kFilterNearest: Pixel-perfect rendering (recommended for pixel art)
  ///   - kFilterBilinear: Smooth rendering (better for high-resolution text)
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  ///
  /// Example:
  /// @code
  /// // Draw red text on a sprite
  /// Sprite mySprite;
  /// mySprite.Create(Vec2Si32(200, 100));
  /// gameFont.Draw(mySprite, "Hello World", 10, 50, kTextOriginCenter, 
  ///               kTextAlignmentLeft, kDrawBlendingModeColorize, kFilterNearest, 
  ///               Rgba(255, 0, 0));
  /// @endcode
  void Draw(Sprite to_sprite, const char *text,
      const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const TextAlignment alignment = kTextAlignmentLeft,
      const DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      const DrawFilterMode filter_mode = kFilterNearest,
      const Rgba color = Rgba(0xffffffff)) {
    font_instance_->DrawEvaluateSizeImpl(to_sprite,
                                         text, false, x, y, origin, alignment, blending_mode, filter_mode, color,
                                         std::vector<Rgba>(), true, nullptr, false, nullptr, nullptr);
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
  ///   color-control characters and select the color from the palette for the
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
                                         palete, true, nullptr, false, nullptr, nullptr);
  }

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   backbuffer (directly to the screen)
  /// @param [in] text UTF-8 c-string with one or more lines of text
  ///   (separated either with `/n`, `/r` or both)
  /// @param [in] x X screen coordinate to draw text at.
  /// @param [in] y Y screen coordinate to draw text at.
  /// @param [in] origin The origin that will be located at the specified screen
  ///   coordinates.
  ///   - kTextOriginTop: The top of the text is at the specified y-coordinate
  ///   - kTextOriginFirstBase: The baseline of the first line is at the specified y-coordinate
  ///   - kTextOriginBottom: The bottom of the text is at the specified y-coordinate
  ///   - kTextOriginCenter: The center of the text is at the specified y-coordinate
  /// @param [in] alignment The alignment that will be used for the text.
  ///   - kTextAlignmentLeft: Text is aligned to the left of the specified x-coordinate
  ///   - kTextAlignmentCenter: Text is centered at the specified x-coordinate
  ///   - kTextAlignmentRight: Text is aligned to the right of the specified x-coordinate
  /// @param [in] blending_mode The blending mode to use when drawing the text.
  ///   - kDrawBlendingModeColorize: Best for colored text (applies the color parameter)
  ///   - kDrawBlendingModeSolidColor: Replaces all colors of the font with the specified color.
  ///   - kDrawBlendingModeAlphaBlend: Standard alpha blending
  /// @param [in] filter_mode The filtering mode to use when drawing the text.
  ///   - kFilterNearest: Pixel-perfect rendering (recommended for pixel art)
  ///   - kFilterBilinear: Smooth rendering (better for high-resolution text)
  /// @param [in] color The color used by some blending modes (for example,
  ///   the kDrawBlendingModeColorize blending mode).
  ///
  /// Example:
  /// @code
  /// // Draw score text at the top of the screen in white
  /// gameFont.Draw("Score: 100", 20, screenHeight - 20, kTextOriginTop, 
  ///               kTextAlignmentLeft, kDrawBlendingModeColorize, kFilterNearest, 
  ///               Rgba(255, 255, 255));
  ///
  /// // Draw centered game over text in yellow
  /// gameFont.Draw("Game Over", screenWidth/2, screenHeight/2, kTextOriginCenter, 
  ///               kTextAlignmentCenter, kDrawBlendingModeColorize, kFilterNearest, 
  ///               Rgba(255, 255, 0));
  /// @endcode
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin = kTextOriginBottom,
      const TextAlignment alignment = kTextAlignmentLeft,
      const DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      const DrawFilterMode filter_mode = kFilterNearest,
      const Rgba color = Rgba(0xffffffff));

  /// @brief Draws a UTF-8 string containing one or more lines of text to the
  ///   backbuffer (directly to the screen) using a color palette
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
  ///   color-control characters and select the color from the palette for the
  ///   following text.
  ///
  /// Example:
  /// @code
  /// // Create a color palette with 3 colors
  /// std::vector<Rgba> palette;
  /// palette.push_back(Rgba(255, 255, 255)); // White (index 0)
  /// palette.push_back(Rgba(255, 0, 0));     // Red (index 1)
  /// palette.push_back(Rgba(0, 255, 0));     // Green (index 2)
  ///
  /// // Draw text with color control characters
  /// // \1 switches to red, \2 switches to green
  /// gameFont.Draw("Normal \1Red \2Green", 20, 100, kTextOriginTop, 
  ///               kTextAlignmentLeft, kDrawBlendingModeColorize, kFilterNearest, 
  ///               palette);
  /// @endcode
  void Draw(const char *text, const Si32 x, const Si32 y,
      const TextOrigin origin,
      const TextAlignment alignment,
      const DrawBlendingMode blending_mode,
      const DrawFilterMode filter_mode,
      const std::vector<Rgba> &palete);

  /// @brief Gets the internal font instance
  /// @return Pointer to the internal font instance
  inline const std::shared_ptr<FontInstance> &FontInstance() const {
    return font_instance_;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_FONT_H_
