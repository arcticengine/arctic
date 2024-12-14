#include "engine/easy.h"
#include "engine/unicode.h"
#pragma warning( disable : 4244 )
using namespace arctic;
using std::string;

Ui32 g_ink = 8;
Ui32 g_paper = 0;
Vec2Si32 g_pos(0, 0);
Vec2Si32 g_text_pos(0, 0);
Font g_font;
Font g_font_8x8;
Vec2Si32 g_text_overlay_size(1, 1);

struct TextCell {
  Ui32 codepoint = ' ';
  Ui32 ink = 8;
  Ui32 paper = 0;
};

std::vector<TextCell> g_text_overlay = {{}};
Vec2Si32 g_cursor(0, 17);


std::vector<Rgba> g_colors = {
  Rgba{0, 0, 0, 255},       // 0=Black
  Rgba{255, 0, 0, 255},     // 1=Red
  Rgba{255, 145, 0, 255},   // 2=Orange
  Rgba{255, 255, 84, 255},  // 3=Yellow
  Rgba{0, 234, 98, 255},    // 4=Green
  Rgba{30, 191, 255, 255},  // 5=Cyan
  Rgba{0, 0, 255, 255},     // 6=Blue
  Rgba{104, 0, 143, 255},   // 7=Violet
  Rgba{255, 255, 255, 255}, // 8=White
  Rgba{128, 128, 128, 255}, // 9=Gray
  Rgba{0, 0, 0, 0},        // 10=Tranparent
  Rgba{191, 0, 0, 255},    // 11=Dark red
  Rgba{191, 108, 0, 255},  // 12=Dark orange
  Rgba{191, 191, 63, 255}, // 13=Dark yellow
  Rgba{0, 175, 73, 255},   // 14=Dark green
  Rgba{22, 93, 191, 255},  // 15=Dark cyan
  Rgba{0, 0, 191, 255},    // 16=Dark blue
  Rgba{78, 0, 107, 255},   // 17=Dark violet
  Rgba{191, 191, 191, 255},// 18=Light gray
  Rgba{96, 96, 96, 255},   // 19=Dark gray
};

std::vector<const char*> g_color_names = {
  "black"
  ,"red"
  ,"orange"
  ,"yellow"
  ,"green"
  ,"cyan"
  ,"blue"
  ,"violet"
  ,"white"
  ,"gray"
  ,"tranparent"
  ,"dark red"
  ,"dark orange"
  ,"dark yellow"
  ,"dark green"
  ,"dark cyan"
  ,"dark blue"
  ,"dark violet"
  ,"light gray"
  ,"dark gray"
};

/// @brief Gets the current ink color as RGBA value
/// @return RGBA color value for current ink
Rgba InkRgba() {
  return (g_ink < g_colors.size() ? g_colors[g_ink] : Rgba(g_ink));
}

/// @brief Gets the current paper color as RGBA value
/// @return RGBA color value for current paper
Rgba PaperRgba() {
  return (g_paper < g_colors.size() ? g_colors[g_paper] : Rgba(g_paper));
}

/// @brief Sets the current ink (foreground) color using a predefined color index
/// @param [in] color_index Index of the color from the predefined color palette
void Ink(Ui32 color_index) {
  g_ink = std::min(color_index, (Ui32)g_colors.size() - 1u);
}

/// @brief Sets the current ink (foreground) color using RGB values
/// @param [in] red Red component (0-255)
/// @param [in] green Green component (0-255)
/// @param [in] blue Blue component (0-255)
void Ink(Si32 red, Si32 green, Si32 blue) {
  g_ink = Rgba(Clamp(red, 0, 255), Clamp(green, 0, 255), Clamp(blue, 0, 255), 255).rgba;
}

/// @brief Sets the current paper (background) color using a predefined color index
/// @param [in] color_index Index of the color from the predefined color palette
void Paper(Ui32 color_index) {
  g_paper = std::min(color_index, (Ui32)g_colors.size() - 1u);
}

/// @brief Sets the current paper (background) color using RGB values
/// @param [in] red Red component (0-255)
/// @param [in] green Green component (0-255)
/// @param [in] blue Blue component (0-255)
void Paper(Si32 red, Si32 green, Si32 blue) {
  g_paper = Rgba(Clamp(red, 0, 255), Clamp(green, 0, 255), Clamp(blue, 0, 255), 255).rgba;
}

/// @brief Plots a single pixel at the specified coordinates using current ink color
/// @param [in] x X-coordinate of the pixel
/// @param [in] y Y-coordinate of the pixel
void Plot(Si32 x, Si32 y) {
  g_pos.x = x;
  g_pos.y = y;
  SetPixel(g_pos.x, g_pos.y, InkRgba());
}

/// @brief Gets the color index of a pixel at specified coordinates
/// @param [in] x X-coordinate of the pixel
/// @param [in] y Y-coordinate of the pixel
/// @return Color index from the predefined palette, or raw RGBA value if not in palette
Ui32 Point(Si32 x, Si32 y) {
  Rgba color = GetPixel(x, y);
  for (Ui32 i = 0; i < g_colors.size(); ++i) {
    if (color == g_colors[i]) {
      return i;
    }
  }
  return color.rgba;
}

/// @brief Draws a line from current position to specified coordinates using current ink color
/// @param [in] x X-coordinate of the end point
/// @param [in] y Y-coordinate of the end point
void Draw(Si32 x, Si32 y) {
  DrawLine(g_pos, Vec2Si32(x, y), InkRgba());
  g_pos.x = x;
  g_pos.y = y;
}

/// @brief Draws a circle with specified center and radius using current ink color
/// @param [in] x X-coordinate of the center
/// @param [in] y Y-coordinate of the center
/// @param [in] r Radius of the circle
void Circle(Si32 x, Si32 y, float r) {
  for (Si32 i = 0; i < 1024; ++i) {
    float alpha = (float)i * (1.f / 1024.f * (float)M_PI * 2.f);
    float s = std::sin(alpha) * r;
    float c = std::cos(alpha) * r;
    SetPixel(x + Si32(c + 0.5f), y + Si32(s + 0.5f), InkRgba());
  }
}

/// @brief Fills an area starting from specified point with current ink color
/// @param [in] x X-coordinate of the starting point
/// @param [in] y Y-coordinate of the starting point
void Fill(Si32 x, Si32 y) {
  Rgba c = GetPixel(x, y);
  Rgba target = InkRgba();
  if (c == target) {
    return;
  }
  Vec2Si32 screen_size = ScreenSize();
  std::vector<Vec2Si32> next;
  next.reserve(screen_size.x * screen_size.y);
  next.emplace_back(x, y);
  Vec2Si32 add[4] = {Vec2Si32(0,1), Vec2Si32(-1,0), Vec2Si32(1,0), Vec2Si32(0,-1)};
  Sprite from_sprite = GetEngine()->GetBackbuffer();
  Rgba *data = from_sprite.RgbaData();
  Si32 stride = from_sprite.StridePixels();

  while (!next.empty()) {
    Vec2Si32 p = next.back();
    next.pop_back();
    for (Si32 n = 0; n < 4; ++n) {
      Vec2Si32 np(p.x + add[n].x, p.y + add[n].y);
      if (np.x >= 0 && np.x < from_sprite.Width() && np.y >= 0 && np.y < from_sprite.Height()) {
        Rgba color = data[np.x + np.y * stride];
        color.a = 255;
        if (color == c) {
          data[np.x + np.y * stride] = target;
          next.push_back(np);
        }
      }
    }
  }
}

/// @brief Prints text at specified coordinates using pixel font
/// @param [in] x X-coordinate of the text position
/// @param [in] y Y-coordinate of the text position
/// @param [in] text Text to print
void PixelPrint(Si32 x, Si32 y, string text) {
  if (g_font.IsEmpty()) {
    g_font.LoadLetterBits(g_tiny_font_letters, 6, 8);
  }
  g_font.Draw(GetEngine()->GetBackbuffer(), text.c_str(),
              x, y,
              kTextOriginFirstBase,
              kTextAlignmentLeft,
              kDrawBlendingModeColorize,
              kFilterNearest,
              InkRgba());
  g_text_pos.x = x;
  g_text_pos.y = y - g_font.EvaluateSize(text.c_str(), false).y;
}

/// @brief Prints an integer value at specified coordinates
/// @param [in] x X-coordinate of the text position
/// @param [in] y Y-coordinate of the text position
/// @param [in] value Integer value to print
void PixelPrint(Si32 x, Si32 y, Si32 value) {
  char buffer[1024];
  snprintf(buffer, 1024, "%d", value);
  PixelPrint(x, y, buffer);
}

/// @brief Prints a floating-point value at specified coordinates
/// @param [in] x X-coordinate of the text position
/// @param [in] y Y-coordinate of the text position
/// @param [in] value Floating-point value to print
void PixelPrint(Si32 x, Si32 y, double value) {
  char buffer[1024];
  snprintf(buffer, 1024, "%f", value);
  PixelPrint(x, y, buffer);
}

/// @brief Prints text at current text position
/// @param [in] text Text to print
void PixelPrint(string text) {
  PixelPrint(g_text_pos.x, g_text_pos.y, text);
}

/// @brief Prints an integer value at current text position
/// @param [in] value Integer value to print
void PixelPrint(Si32 value) {
  PixelPrint(g_text_pos.x, g_text_pos.y, value);
}

/// @brief Prints a floating-point value at current text position
/// @param [in] value Floating-point value to print
void PixelPrint(double value) {
  PixelPrint(g_text_pos.x, g_text_pos.y, value);
}

/// @brief Gets text input from user at specified coordinates
/// @param [in] x X-coordinate of the input position
/// @param [in] y Y-coordinate of the input position
/// @param [in] text Prompt text to display
/// @return User input as string
string PixelInput(Si32 x, Si32 y, string text) {
  Sprite prev_backbuffer;
  prev_backbuffer.Clone(GetEngine()->GetBackbuffer());
  prev_backbuffer.ClearOpaqueSpans();
  string input = "";
  Vec2Si32 prev_text_pos = g_text_pos;
  while(true) {
    ShowFrame();
    for (Si32 i = 0; i < InputMessageCount(); ++i) {
      const InputMessage &msg = GetInputMessage(i);
      if (msg.kind == arctic::InputMessage::kKeyboard) {
        if (msg.keyboard.key_state == 1) {
          Ui32 key = msg.keyboard.key;
          if (key == kKeyBackspace) {
            if (!input.empty()) {
              input.pop_back();
              while (!input.empty() && (input.back() & 128) && (input.back() & 64)) {
                input.pop_back();
              }
            }
          } else if (key == kKeyEnter) {
            Clear();
            g_text_pos = prev_text_pos;
            prev_backbuffer.Draw(0, 0, kDrawBlendingModeCopyRgba);
            PixelPrint(x, y, text + input);
            return input;
          } else {
            if (msg.keyboard.characters[0]) {
              input.append(msg.keyboard.characters);
            }
          }
        }
      }
    }
    Clear();
    g_text_pos = prev_text_pos;
    prev_backbuffer.Draw(0, 0, kDrawBlendingModeCopyRgba);
    PixelPrint(x, y, text + input + (std::fmod(Time(), 0.66) > 0.33 ? "_" : "") );
  }
}

/// @brief Gets text input from user at specified coordinates
/// @param [in] text Prompt text to display
/// @return User input as string
string PixelInput(string text) {
  return PixelInput(g_text_pos.x, g_text_pos.y, text);
}

/// @brief Converts a string to a number
/// @param [in] text String to convert
/// @return Converted number value
double Number(string text) {
  double result = 0.0;
  double sign = 1.0;
  bool is_fract = false;
  double fract_mul = 0.1;
  size_t size = text.size();
  size_t i = 0;
  while (true) {
    if (i == size) {
      return 0.0;
    }
    if (text[i] != ' ' && text[i] != '\t') {
      break;
    }
    ++i;
  }
  if (text[i] == '-') {
    sign = -1.0;
    ++i;
  }
  while (i < size) {
    char ch = text[i];
    if (ch == '.' || ch == ',') {
      is_fract = true;
    } else if (ch >= '0' && ch <= '9') {
      if (is_fract) {
        result = result + fract_mul * double(ch - '0');
        fract_mul *= 0.1;
      } else {
        result = result * 10.0 + double(ch - '0');
      }
    }
    ++i;
  }
  return result * sign;
}

/// @brief Calculates squared distance between two RGBA colors
/// @param [in] a First RGBA color
/// @param [in] b Second RGBA color
/// @return Squared distance between the colors
Si32 DistanceSq(Rgba a, Rgba b) {
  Si32 deltaR = Si32(a.r) - Si32(b.r);
  Si32 deltaG = Si32(a.g) - Si32(b.g);
  Si32 deltaB = Si32(a.b) - Si32(b.b);
  Si32 deltaA = Si32(a.a) - Si32(b.a);
  return deltaR * deltaR + deltaG * deltaG + deltaB * deltaB + deltaA * deltaA;
}

/// @brief Gets the name of a color closest to the specified RGBA value
/// @param [in] color RGBA color to find name for
/// @return Name of the closest matching color from predefined palette
const char* ColorName(Rgba color) {
  const char* name = g_color_names[0];
  Si32 min_dist_sq = DistanceSq(color, g_colors[0]);
  for (size_t i = 0; i < g_colors.size(); ++i) {
    Si32 dist = DistanceSq(color, g_colors[i]);
    if (dist < min_dist_sq) {
      min_dist_sq = dist;
      name = g_color_names[i];
    }
  }
  return name;
}

/// @brief Gets character at specified screen coordinates
/// @param [in] x X-coordinate on text screen
/// @param [in] y Y-coordinate on text screen
/// @return Character at specified position as string
string Screen(Si32 x, Si32 y) {
  if (x < 0 || x >= g_text_overlay_size.x || y < 0 || y >= g_text_overlay_size.y) {
    return string("");
  }
  Ui32 data[2];
  data[0] = g_text_overlay[y*g_text_overlay_size.x+x].codepoint;
  data[1] = 0;
  return Utf32ToUtf8(data);
};

/// @brief Gets the ink color index at specified screen coordinates
/// @param [in] x X-coordinate on text screen
/// @param [in] y Y-coordinate on text screen
/// @return Ink color index at the specified position
Si32 ScreenInk(Si32 x, Si32 y) {
  if (x < 0 || x >= g_text_overlay_size.x || y < 0 || y >= g_text_overlay_size.y) {
    return 0;
  }
  return g_text_overlay[y*g_text_overlay_size.x+x].ink;
}

/// @brief Gets the paper color index at specified screen coordinates
/// @param [in] x X-coordinate on text screen
/// @param [in] y Y-coordinate on text screen
/// @return Paper color index at the specified position
Si32 ScreenPaper(Si32 x, Si32 y) {
  if (x < 0 || x >= g_text_overlay_size.x || y < 0 || y >= g_text_overlay_size.y) {
    return 0;
  }
  return g_text_overlay[y*g_text_overlay_size.x+x].paper;
}

/// @brief Sets the cursor position on the text screen
/// @param [in] x X-coordinate for cursor
/// @param [in] y Y-coordinate for cursor
void At(Si32 x, Si32 y) {
  Si32 sx = x % g_text_overlay_size.x;
  Si32 ya = x / g_text_overlay_size.x;
  if (sx < 0) {
    sx += g_text_overlay_size.x;
    ya -= 1;
  }
  Si32 yy = y + ya;
  Si32 sy = yy % g_text_overlay_size.y;
  if (sy < 0) {
    sy += g_text_overlay_size.y;
  }
  g_cursor = Vec2Si32(sx, sy);
}

/// @brief Prints text at current cursor position
/// @param [in] text Text to print
void Print(string text) {
  Utf32Reader r;
  r.Reset(text.c_str());
  while (true) {
    Ui32 codepoint = r.ReadOne();
    if (codepoint == 0) {
      return;
    }
    TextCell &tc = g_text_overlay[g_cursor.y*g_text_overlay_size.x+g_cursor.x];
    tc.ink = g_ink;
    tc.paper = g_paper;
    tc.codepoint = codepoint;
    if (codepoint < g_font_8x8.FontInstance()->codepoint_.size()) {
      if (codepoint == 10) {
        g_cursor.x = g_text_overlay_size.x - 1;
      } else if (g_font_8x8.FontInstance()->codepoint_[codepoint]) {
        DrawRectangle(g_cursor * 8, g_cursor * 8 + Vec2Si32(7, 7), PaperRgba());
        g_font_8x8.FontInstance()->codepoint_[codepoint]->sprite.Draw(g_cursor.x * 8, g_cursor.y * 8,
          kDrawBlendingModeColorize, kFilterNearest, InkRgba());
      }
    }
    g_cursor.x++;
    if (g_cursor.x >= g_text_overlay_size.x) {
      g_cursor.x = 0;
      g_cursor.y--;
      if (g_cursor.y < 0) {
        g_cursor.y = g_text_overlay_size.y - 1;
      }
    }
  }
}

/// @brief Prints an integer value at current cursor position
/// @param [in] value Integer value to print
void Print(Si32 value) {
  char buffer[1024];
  snprintf(buffer, 1024, "%d", value);
  Print(buffer);
}

/// @brief Prints a floating-point value at current cursor position
/// @param [in] value Floating-point value to print
void Print(double value) {
  char buffer[1024];
  snprintf(buffer, 1024, "%f", value);
  Print(buffer);
}

/// @brief Gets text input from user at specified coordinates
/// @return User input as string
string Input() {
  Sprite prev_backbuffer;
  prev_backbuffer.Clone(GetEngine()->GetBackbuffer());
  prev_backbuffer.ClearOpaqueSpans();
  string input = "";
  Vec2Si32 prev_cursor_pos = g_cursor;
  while(true) {
    ShowFrame();
    for (Si32 i = 0; i < InputMessageCount(); ++i) {
      const InputMessage &msg = GetInputMessage(i);
      if (msg.kind == arctic::InputMessage::kKeyboard) {
        if (msg.keyboard.key_state == 1) {
          Ui32 key = msg.keyboard.key;
          if (key == kKeyBackspace) {
            if (!input.empty()) {
              input.pop_back();
              while (!input.empty() && (input.back() & 128) && (input.back() & 64)) {
                input.pop_back();
              }
            }
          } else if (key == kKeyEnter) {
            Clear();
            g_cursor = prev_cursor_pos;
            prev_backbuffer.Draw(0, 0, kDrawBlendingModeCopyRgba);
            Print(input);
            ShowFrame();
            return input;
          } else {
            if (msg.keyboard.characters[0]) {
              input.append(msg.keyboard.characters);
            }
          }
        }
      }
    }
    Clear();
    g_cursor = prev_cursor_pos;
    prev_backbuffer.Draw(0, 0, kDrawBlendingModeCopyRgba);
    Print(input + (std::fmod(Time(), 0.66) > 0.33 ? "_" : "") );
  }
}

/// @brief Clears the screen with current paper color
void Cls() {
  for (size_t i = 0; i < g_text_overlay.size(); ++i) {
    g_text_overlay[i].ink = g_ink;
    g_text_overlay[i].paper = g_paper;
    g_text_overlay[i].codepoint = ' ';
  }
  Clear(PaperRgba());
}

/// @brief Updates the screen display
void Show() {
  Vec2Si32 size = ScreenSize() + Vec2Si32(7, 7);
  g_text_overlay_size = Vec2Si32(size.x/8, size.y/8);
  size_t prev_size = g_text_overlay.size();
  g_text_overlay.resize(g_text_overlay_size.x * g_text_overlay_size.y);
  for (size_t i = prev_size; i < g_text_overlay.size(); ++i) {
    g_text_overlay[i].codepoint = ' ';
    g_text_overlay[i].paper = 0;
    g_text_overlay[i].ink = 8;
  }

  ShowFrame();
}

/// @brief Sets a custom character in the 8x8 font
/// @param [in] codepoint Unicode codepoint of the character
/// @param [in] data_bits Bitmap data for the character (8x8 pixels)
void SetCharacter(string codepoint, Ui64 data_bits) {
  Sprite sprite;
  sprite.Create(Vec2Si32(8, 8));
  for (Si32 y = 0; y < 8; ++y) {
    Ui64 row = (data_bits >> (8*y));
    for (Si32 x = 0; x < 8; ++x) {
      if ((row << x) & 0x80) {
        SetPixel(sprite, x, y, Rgba(255, 255, 255));
      } else {
        SetPixel(sprite, x, y, Rgba(0, 0, 0, 0));
      }
    }
  }
  sprite.UpdateOpaqueSpans();
  Utf32Reader reader;
  reader.Reset(codepoint.c_str());
  g_font_8x8.AddGlyph(reader.ReadOne(), 0, sprite);
}

/// @brief Main entry point for the application
void EasyMain() {
  ResizeScreen(256, 144);
  std::vector<Ui8> letters = ReadFile("data/font_8x8.txt");
  letters.emplace_back(0);
  Sprite table;
  table.Load("data/font_8x8.tga");
  g_font_8x8.LoadTable(table, reinterpret_cast<char*>(letters.data()), 8, 8, 8, 8, 8, 0);
  Show();

  #include "code.inc.h"

  while (!IsAnyKeyDownward()) {
    Show();
  }
}
