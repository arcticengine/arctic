// Fonts: BMFont and TTF loading, system fonts, borders and palettes.
#define TEST_NO_MAIN
#include "test_helpers.h"

// The documented recipe for readable text: bake a black border into the glyphs
// and draw with kDrawBlendingModeColorize. The border has to survive the
// colorize (black times any color is black) while the white body of the glyph
// takes the color asked for. If this ever stops being true, the advice in
// font.h and in the manual is wrong and text has to be drawn several times
// again.
void test_font_border_survives_colorize() {
  Sprite dot;
  dot.Create(1, 1);
  const_cast<Rgba*>(dot.RgbaData())[0] = Rgba(255, 255, 255, 255);
  dot.UpdateOpaqueSpans();

  Font font;
  font.CreateEmpty(4, 5);
  font.AddGlyph(static_cast<Ui32>('a'), 3, dot);
  TEST_CHECK(font.BaseToTop() == 4);
  TEST_CHECK(font.LineHeight() == 5);
  TEST_CHECK(font.BaseToTop() + font.BaseToBottom() == font.LineHeight());
  font.AddBorder(1.0f, Rgba(0, 0, 0, 255));

  Sprite target;
  target.Create(16, 16);
  target.Clear();
  font.Draw(target, "a", 8, 8, kTextOriginFirstBase, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 0, 0));

  Si32 body_count = 0;
  Si32 border_count = 0;
  Vec2Si32 body(0, 0);
  const Rgba *pixels = target.RgbaData();
  const Si32 stride = target.StridePixels();
  for (Si32 y = 0; y < target.Height(); ++y) {
    for (Si32 x = 0; x < target.Width(); ++x) {
      const Rgba p = pixels[x + y * stride];
      if (p.a == 0) {
        continue;
      }
      if (p.r == 255 && p.g == 0 && p.b == 0) {
        ++body_count;
        body = Vec2Si32(x, y);
      } else if (p.r == 0 && p.g == 0 && p.b == 0) {
        ++border_count;
      } else {
        TEST_CHECK_(false, "unexpected pixel %d,%d,%d at (%d, %d)",
            (int)p.r, (int)p.g, (int)p.b, (int)x, (int)y);
      }
    }
  }
  TEST_CHECK_(body_count == 1,
      "the glyph body was drawn as %d colorized pixels, expected 1",
      (int)body_count);
  TEST_CHECK_(border_count > 0,
      "the baked border did not survive the colorize blending");

  // The border has to be around the body, not somewhere else on the sprite.
  Si32 neighbours = 0;
  for (Si32 dy = -1; dy <= 1; ++dy) {
    for (Si32 dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      const Si32 x = body.x + dx;
      const Si32 y = body.y + dy;
      if (x < 0 || y < 0 || x >= target.Width() || y >= target.Height()) {
        continue;
      }
      const Rgba p = pixels[x + y * stride];
      if (p.a != 0 && p.r == 0 && p.g == 0 && p.b == 0) {
        ++neighbours;
      }
    }
  }
  TEST_CHECK_(neighbours > 0, "the border is not adjacent to the glyph body");
}

// The same border, asked for at load time instead of by a separate call. The
// loaders that take a sprite are the ones a test can use without a font file.
void test_font_loads_with_border() {
  Sprite stripe;
  stripe.Create(3, 3);
  stripe.Clear();
  Rgba *data = const_cast<Rgba*>(stripe.RgbaData());
  data[1 + 1 * stripe.StridePixels()] = Rgba(255, 255, 255, 255);
  stripe.UpdateOpaqueSpans();

  Font plain;
  plain.LoadHorizontalStripe(stripe, "a", 3, 4, 2);
  Font bordered;
  bordered.LoadHorizontalStripe(stripe, "a", 3, 4, 2, 1.0f,
      Rgba(0, 0, 0, 255));

  const Vec2Si32 plain_size = plain.EvaluateSize("a", false);
  const Vec2Si32 bordered_size = bordered.EvaluateSize("a", false);
  TEST_CHECK_(bordered_size.x > plain_size.x,
      "the border did not widen the glyphs: %d vs %d",
      (int)bordered_size.x, (int)plain_size.x);
}

// ============================================================================

void test_ttf_font_loading() {
  std::string data_dir = find_test_data_dir();
  std::string ttf_path = data_dir + "/ArcticOne.ttf";

  // Test LoadTtf with explicit parameters
  Font font;
  font.LoadTtf(ttf_path.c_str(), 32.0f);

  TEST_CHECK_(!font.IsEmpty(), "Font should not be empty after LoadTtf");

  // Test that ASCII glyphs were rasterized
  Vec2Si32 size_a = font.EvaluateSize("A", true);
  TEST_CHECK_(size_a.x > 0, "Glyph 'A' should have positive width, got %d", size_a.x);
  TEST_CHECK_(size_a.y > 0, "Glyph 'A' should have positive height, got %d", size_a.y);

  // Test multi-character string size
  Vec2Si32 size_hello = font.EvaluateSize("Hello", true);
  TEST_CHECK_(size_hello.x > size_a.x,
      "String 'Hello' should be wider than 'A': %d vs %d",
      size_hello.x, size_a.x);

  // Test that different pixel heights produce different sizes
  Font font_small;
  font_small.LoadTtf(ttf_path.c_str(), 16.0f);
  Vec2Si32 size_small = font_small.EvaluateSize("A", true);
  TEST_CHECK_(size_small.x < size_a.x || size_small.y < size_a.y,
      "16px font should be smaller than 32px font");

  // Test with custom character set
  Font font_custom;
  font_custom.LoadTtf(ttf_path.c_str(), 24.0f, "ABC123");
  TEST_CHECK_(!font_custom.IsEmpty(), "Font with custom charset should not be empty");
  Vec2Si32 size_b = font_custom.EvaluateSize("B", true);
  TEST_CHECK_(size_b.x > 0, "Glyph 'B' from custom charset should have positive width");
}

void test_find_system_font() {
  // Test FindSystemFont with a font that should exist on this platform
#ifdef ARCTIC_PLATFORM_MACOSX
  std::string path = arctic::FindSystemFont("Helvetica");
  TEST_CHECK_(!path.empty(), "Helvetica should be found on macOS, got empty path");
  TEST_CHECK_(path.find(".ttc") != std::string::npos ||
              path.find(".ttf") != std::string::npos,
              "Path should end with .ttc or .ttf, got '%s'", path.c_str());
#endif

  // Test with a non-existent font name
  std::string bad_path = arctic::FindSystemFont("NonExistentFont12345XYZ");
  TEST_CHECK_(bad_path.empty(),
      "Non-existent font should return empty path, got '%s'",
      bad_path.c_str());

  // Test with nullptr
  std::string null_path = arctic::FindSystemFont(nullptr);
  TEST_CHECK_(null_path.empty(), "nullptr font_name should return empty path");
}

void test_load_system_font() {
#ifdef ARCTIC_PLATFORM_MACOSX
  Font font;
  font.LoadSystemFont("Helvetica", 24.0f);
  TEST_CHECK_(!font.IsEmpty(), "System font Helvetica should load successfully");

  Vec2Si32 size = font.EvaluateSize("Hello", true);
  TEST_CHECK_(size.x > 0, "System font should render text with positive width");
  TEST_CHECK_(size.y > 0, "System font should render text with positive height");
#endif
}

// The palette overloads of Font::Draw take the plain color from palete[0].
// An empty palette used to be read anyway, which is a crash; now it means
// white, and the text still has to appear.
void test_font_draw_with_empty_palette() {
  Sprite dot;
  dot.Create(1, 1);
  const_cast<Rgba*>(dot.RgbaData())[0] = Rgba(255, 255, 255, 255);
  dot.UpdateOpaqueSpans();
  Font font;
  font.CreateEmpty(4, 5);
  font.AddGlyph(static_cast<Ui32>('a'), 3, dot);

  const std::vector<Rgba> empty_palette;
  const std::vector<Rgba> red_palette(1, Rgba(255, 0, 0, 255));
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kRed(255, 0, 0, 255);
  const Rgba kBlack(0, 0, 0, 255);

  // Into a sprite.
  Sprite target;
  target.Create(16, 16);
  target.Clear(kBlack);
  font.Draw(target, "a", 8, 8, kTextOriginFirstBase, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest, empty_palette);
  Si32 white_count = 0;
  Si32 other_count = 0;
  const Rgba *pixels = target.RgbaData();
  const Si32 stride = target.StridePixels();
  for (Si32 y = 0; y < target.Height(); ++y) {
    for (Si32 x = 0; x < target.Width(); ++x) {
      const Rgba p = pixels[x + y * stride];
      if (p.rgba == kBlack.rgba) {
        continue;
      }
      if (p.rgba == kWhite.rgba) {
        ++white_count;
      } else {
        ++other_count;
      }
    }
  }
  TEST_CHECK_(white_count == 1 && other_count == 0,
      "an empty palette drew %d white and %d other pixels instead of one "
      "white glyph", white_count, other_count);

  // Control: a palette with one entry paints with that entry, so the white
  // above came from the empty palette and not from the blending mode.
  target.Clear(kBlack);
  font.Draw(target, "a", 8, 8, kTextOriginFirstBase, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest, red_palette);
  Si32 red_count = 0;
  for (Si32 y = 0; y < target.Height(); ++y) {
    for (Si32 x = 0; x < target.Width(); ++x) {
      if (pixels[x + y * stride].rgba == kRed.rgba) {
        ++red_count;
      }
    }
  }
  TEST_CHECK_(red_count == 1, "a one-entry palette drew %d red pixels",
      red_count);

  // Onto the backbuffer, which is the other overload with the same history.
  ResizeScreen(320, 200);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  font.Draw("a", 8, 8, kTextOriginFirstBase, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest, empty_palette);
  Sprite backbuffer = GetEngine()->GetBackbuffer();
  white_count = 0;
  other_count = 0;
  for (Si32 y = 0; y < backbuffer.Height(); ++y) {
    for (Si32 x = 0; x < backbuffer.Width(); ++x) {
      const Rgba p = BackbufferPixel(Vec2Si32(x, y));
      if (p.rgba == kBlack.rgba) {
        continue;
      }
      if (p.rgba == kWhite.rgba) {
        ++white_count;
      } else {
        ++other_count;
      }
    }
  }
  TEST_CHECK_(white_count == 1 && other_count == 0,
      "on the backbuffer an empty palette drew %d white and %d other pixels "
      "instead of one white glyph", white_count, other_count);
}
