// Sprites and pixels: Rgba, blending, software and hardware sprites, png/tga
// round trips, the top-left coordinate helpers. The radix sort tests are here
// as well.
#define TEST_NO_MAIN
#include "test_helpers.h"


template <class KeyT>
void radix_sort(std::vector<KeyT> &in_out_data) {
  constexpr Ui64 kBits = sizeof(KeyT) <= 4 ? 2 : 4;
  constexpr KeyT kMask = ((KeyT)1 << kBits) - 1;
  constexpr KeyT kBuckets = (KeyT)1 << kBits;
  const Ui64 kTempSize = in_out_data.size() * (kMask + 1) * 2;
#ifdef alloca
  const bool kIsBig = kTempSize > 50000;
  KeyT *temp = kIsBig ?
    (KeyT*)malloc(kTempSize * sizeof(KeyT)) :
    (KeyT*)alloca(kTempSize * sizeof(KeyT));
#else
  const bool kIsBig = true;
  KeyT *temp = (KeyT*)malloc(kTempSize * sizeof(KeyT));
#endif

  std::array<KeyT*, kBuckets> buf_a;
  std::array<KeyT*, kBuckets> buf_b;
  std::array<KeyT*, kBuckets> buf_a0;
  std::array<KeyT*, kBuckets> buf_b0;

  for (KeyT i = 0; i < kBuckets; ++i) {
    buf_a[i] = &temp[in_out_data.size() * i];
    buf_b[i] = &temp[in_out_data.size() * kBuckets + in_out_data.size() * i];
    buf_a0[i] = &temp[in_out_data.size() * i];
    buf_b0[i] = &temp[in_out_data.size() * kBuckets + in_out_data.size() * i];
  }
  std::array<KeyT*, kBuckets> *pa = &buf_a;
  std::array<KeyT*, kBuckets> *pb = &buf_b;
  std::array<KeyT*, kBuckets> *pa0 = &buf_a0;
  std::array<KeyT*, kBuckets> *pb0 = &buf_b0;

  // input pass
  for (Si64 i = 0; i < (Si64)in_out_data.size(); ++i) {
    KeyT val = in_out_data[static_cast<size_t>(i)];
    KeyT idx = val & kMask;
    *(buf_a[idx]) = val;
    ++buf_a[idx];
  }
  Ui64 shift = kBits;
  while (shift < 8 * sizeof(KeyT)) {
    // a->b pass
    for (Si64 in_bucket_idx = 0; in_bucket_idx < kBuckets; ++in_bucket_idx) {
      KeyT *begin = (*pa0)[static_cast<size_t>(in_bucket_idx)];
      KeyT *end = (*pa)[static_cast<size_t>(in_bucket_idx)];
      for (KeyT *p = begin; p < end; ++p) {
        KeyT val = *p;
        KeyT idx = (val >> shift) & kMask;
        *(*pb)[idx] = val;
        ++(*pb)[idx];
      }
    }
    // swap a and b
    std::swap(pa, pb);
    std::swap(pa0, pb0);
    for (KeyT i = 0; i < kBuckets; ++i) {
      (*pb)[i] = (*pb0)[i];
    }
    shift += kBits;
  }
  // output pass
  KeyT *out_p = &in_out_data[0];
  for (Si64 in_bucket_idx = 0; in_bucket_idx < kBuckets; ++in_bucket_idx) {
    KeyT *begin = (*pa0)[static_cast<size_t>(in_bucket_idx)];
    KeyT *end = (*pa)[static_cast<size_t>(in_bucket_idx)];
    for (KeyT *p = begin; p < end; ++p) {
      *out_p = *p;
      ++out_p;
    }
  }
  if (kIsBig) {
    free(temp);
  }
}

template <class KeyT>
void baseline_radix_sort(std::vector<KeyT> &in_out_data) {
  constexpr Ui64 kBits = sizeof(KeyT) <= 4 ? 2 : 4;
  constexpr KeyT kMask = ((KeyT)1 << kBits) - 1;
  constexpr KeyT kBuckets = (KeyT)1 << kBits;
  const Ui64 kTempSize = in_out_data.size() * ((kMask + 1) * 2 - 1);
#ifdef alloca
  const bool kIsBig = kTempSize > 50000;
  KeyT *temp = kIsBig ?
    (KeyT*)malloc(kTempSize * sizeof(KeyT)) :
    (KeyT*)alloca(kTempSize * sizeof(KeyT));
#else
  const bool kIsBig = true;
  KeyT *temp = (KeyT*)malloc(kTempSize * sizeof(KeyT));
#endif

  std::array<KeyT*, kBuckets> buf_a;
  std::array<KeyT*, kBuckets> buf_b;
  std::array<KeyT*, kBuckets> buf_a0;
  std::array<KeyT*, kBuckets> buf_b0;

  buf_a[0] = &temp[0];
  buf_b[0] = &temp[0];
  buf_a0[0] = &temp[0];
  buf_b0[0] = &temp[0];
  for (KeyT i = 1; i < kBuckets; ++i) {
    buf_a[i] = &temp[in_out_data.size() * (i * 2 - 1)];
    buf_b[i] = &temp[in_out_data.size() * (i * 2)];
    buf_a0[i] = buf_a[i];
    buf_b0[i] = buf_b[i];
  }
  std::array<KeyT*, kBuckets> *pa = &buf_a;
  std::array<KeyT*, kBuckets> *pb = &buf_b;
  std::array<KeyT*, kBuckets> *pa0 = &buf_a0;
  std::array<KeyT*, kBuckets> *pb0 = &buf_b0;

  // input pass
  for (Si64 i = 0; i < (Si64)in_out_data.size(); ++i) {
    KeyT val = in_out_data[static_cast<size_t>(i)];
    KeyT idx = val & kMask;
    *(buf_a[idx]) = val;
    ++buf_a[idx];
  }
  Ui64 shift = kBits;
  while (shift < 8 * sizeof(KeyT)) {
    // a->b pass
    for (Si64 in_bucket_idx = 0; in_bucket_idx < kBuckets; ++in_bucket_idx) {
      KeyT *begin = (*pa0)[static_cast<size_t>(in_bucket_idx)];
      KeyT *end = (*pa)[static_cast<size_t>(in_bucket_idx)];
      for (KeyT *p = begin; p < end; ++p) {
        KeyT val = *p;
        KeyT idx = (val >> shift) & kMask;
        *(*pb)[idx] = val;
        ++(*pb)[idx];
      }
    }
    // swap a and b
    std::swap(pa, pb);
    std::swap(pa0, pb0);
    for (KeyT i = 0; i < kBuckets; ++i) {
      (*pb)[i] = (*pb0)[i];
    }
    shift += kBits;
  }
  // output pass
  KeyT *out_p = &in_out_data[0];
  for (Si64 in_bucket_idx = 0; in_bucket_idx < kBuckets; ++in_bucket_idx) {
    KeyT *begin = (*pa0)[static_cast<size_t>(in_bucket_idx)];
    KeyT *end = (*pa)[static_cast<size_t>(in_bucket_idx)];
    for (KeyT *p = begin; p < end; ++p) {
      *out_p = *p;
      ++out_p;
    }
  }
  if (kIsBig) {
    free(temp);
  }
}

void test_radix_sort() {
  std::vector<Ui32> input;
  input.resize(10000000);
  std::independent_bits_engine<std::mt19937_64, 64, Ui64> rnd;
  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = (Ui32)rnd() % 10;
  }
  std::chrono::high_resolution_clock clock;

  std::cerr << std::fixed << std::endl;
  std::cerr.precision(8);
  std::cerr << "testing " << input.size() << " items" << std::endl;
  
  {
    std::vector<Ui32> input1(input);
    auto t0 = clock.now();
    std::sort(input1.begin(), input1.end());
    auto t1 = clock.now();
    std::cerr << "std::sort  duration: " << std::chrono::duration<double>(t1 - t0).count() << std::endl;
  }

  {
    std::vector<Ui32> input2(input);
    auto t2 = clock.now();
    radix_sort(input2);
    auto t3 = clock.now();
    std::cerr << "radix sort duration: " << std::chrono::duration<double>(t3 - t2).count() << std::endl;
  }

  {
    std::vector<Ui32> input3(input);
    auto t4 = clock.now();
    baseline_radix_sort(input3);
    auto t5 = clock.now();
    std::cerr << "base radix duration: " << std::chrono::duration<double>(t5 - t4).count() << std::endl;
  }

  {
    std::vector<Ui32> input1(input);
    auto t0 = clock.now();
    std::sort(input1.begin(), input1.end());
    auto t1 = clock.now();
    std::cerr << "std::sort  duration: " << std::chrono::duration<double>(t1 - t0).count() << std::endl;
  }

  {
    std::vector<Ui32> input2(input);
    auto t2 = clock.now();
    radix_sort(input2);
    auto t3 = clock.now();
    std::cerr << "radix sort duration: " << std::chrono::duration<double>(t3 - t2).count() << std::endl;
  }

  {
    std::vector<Ui32> input3(input);
    auto t4 = clock.now();
    baseline_radix_sort(input3);
    auto t5 = clock.now();
    std::cerr << "base radix duration: " << std::chrono::duration<double>(t5 - t4).count() << std::endl;
  }
  {
    std::vector<Ui32> input2(input);
    auto t2 = clock.now();
    radix_sort(input2);
    auto t3 = clock.now();
    std::cerr << "radix sort duration: " << std::chrono::duration<double>(t3 - t2).count() << std::endl;
  }

  {
    std::vector<Ui32> input1(input);
    auto t0 = clock.now();
    std::sort(input1.begin(), input1.end());
    auto t1 = clock.now();
    std::cerr << "std::sort  duration: " << std::chrono::duration<double>(t1 - t0).count() << std::endl;
  }
  {
    std::vector<Ui32> input3(input);
    auto t4 = clock.now();
    baseline_radix_sort(input3);
    auto t5 = clock.now();
    std::cerr << "base radix duration: " << std::chrono::duration<double>(t5 - t4).count() << std::endl;
  }

}

void test_radix_sort_correctness() {
  std::vector<Ui32> input1;
  input1.resize(5000);
  std::independent_bits_engine<std::mt19937_64, 64, Ui64> rnd;
  for (size_t i = 0; i < input1.size(); ++i) {
    input1[i] = (Ui32)rnd();
  }
  std::vector<Ui32> input2(input1);

  std::sort(input1.begin(), input1.end());
  radix_sort(input2);

  for (size_t i = 0; i < input1.size(); ++i) {
    if (input1[i] != input2[i]) {
      TEST_CHECK(false && "lines do not match");
      break;
    }
  }
}

void test_rgb() {
  Rgb x(1, 2, 3);
  Rgb y(Ui8(4));
  Rgb z(Ui32(0xaabbcc));
  Rgb u = z;
  Ui8 rx = x[0];
  Ui8 gx = x[1];
  Ui8 bx = x[2];
  y.r = 5;
  Rgb yt(5, 4, 4);
  TEST_CHECK(yt == y);
  TEST_CHECK(yt != u);
  TEST_CHECK(rx == 1);
  TEST_CHECK(gx == 2);
  TEST_CHECK(bx == 3);
  TEST_CHECK(u.r == 0xcc);
  TEST_CHECK(u.g == 0xbb);
  TEST_CHECK(u.b == 0xaa);
}

void test_tga_oom() {
  Sprite sp;
  sp.Load("data/oom.tga");
}

Rgba ExactBilerp(Rgba a, Rgba b, Rgba c, Rgba d, float fx, float fy) {
  TEST_CHECK(fx >= 0.f && fx <= 1.f && fy >= 0.f && fy <= 1.f);
  return Rgba(
      static_cast<Ui8>(
        float(a.r) * (1.f - fx) * (1.f - fy)
        + float(b.r) * fx * (1.f - fy)
        + float(c.r) * (1.f - fx) * fy
        + float(d.r) * fx * fy),
      static_cast<Ui8>(
        float(a.g) * (1.f - fx) * (1.f - fy)
        + float(b.g) * fx * (1.f - fy)
        + float(c.g) * (1.f - fx) * fy
        + float(d.g) * fx * fy),
      static_cast<Ui8>(
        float(a.b) * (1.f - fx) * (1.f - fy)
        + float(b.b) * fx * (1.f - fy)
        + float(c.b) * (1.f - fx) * fy
        + float(d.b) * fx * fy),
      static_cast<Ui8>(
        float(a.a) * (1.f - fx) * (1.f - fy)
        + float(b.a) * fx * (1.f - fy)
        + float(c.a) * (1.f - fx) * fy
        + float(d.a) * fx * fy));
}

void TestBilerp(Rgba a, Rgba b, Rgba c, Rgba d, float fx, float fy) {
  Rgba ref_p = ExactBilerp(a, b, c, d, fx, fy);
  Si32 ax = static_cast<Ui8>(256.f * fx);
  Si32 ay = static_cast<Ui8>(256.f * fy);
  Rgba p = Bilerp(a, b, c, d, ax, ay);
  TEST_CHECK_(abs(Si32(p.r) - Si32(ref_p.r)) <= 2,
      "fx=%f fy=%f p.r=%i ref_p.r=%i", fx, fy, int(p.r), int(ref_p.r));
  TEST_CHECK_(abs(Si32(p.g) - Si32(ref_p.g)) <= 2,
      "fx=%f fy=%f p.r=%i ref_p.r=%i", fx, fy, int(p.g), int(ref_p.g));
  TEST_CHECK_(abs(Si32(p.b) - Si32(ref_p.b)) <= 2,
      "fx=%f fy=%f p.r=%i ref_p.r=%i", fx, fy, int(p.b), int(ref_p.b));
  TEST_CHECK_(abs(Si32(p.a) - Si32(ref_p.a)) <= 2,
      "fx=%f fy=%f p.r=%i ref_p.r=%i", fx, fy, int(p.a), int(ref_p.a));
}

void test_rgba() {
  {
    Rgba a(0, 255, 0, 255);
    Rgba b(0, 0, 0, 255);
    Rgba c(0, 0, 0, 255);
    Rgba d(0, 0, 0, 255);
    TestBilerp(a, b, c, d, 0.5f, 0.5f);
    for (Si32 y = 0; y < 256; ++y) {
      for (Si32 x = 0; x < 256; ++x) {
        TestBilerp(a, b, c, d, 1.f * x / 256.f, 1.f * y / 256.f);
      }
    }
  }
  {
    Rgba a(0, 0, 0, 255);
    Rgba b(0, 0, 0, 255);
    Rgba c(0, 0, 255, 255);
    Rgba d(0, 0, 255, 255);
    for (Si32 y = 0; y < 256; ++y) {
      for (Si32 x = 0; x < 256; ++x) {
        TestBilerp(a, b, c, d, 1.f * x / 256.f, 1.f * y / 256.f);
      }
    }
  }
  {
    Rgba a(0, 0, 0, 255);
    Rgba b(255, 0, 255, 255);
    Rgba c(64, 0, 0, 255);
    Rgba d(128, 128, 128, 255);
    // Sprite s;
    // s.Create(256, 256);
    for (Si32 y = 0; y < 256; ++y) {
      for (Si32 x = 0; x < 256; ++x) {
        TestBilerp(a, b, c, d, 1.f * x / 256.f, 1.f * y / 256.f);
        // SetPixel(s, x, y, Bilerp(a, b, c, d, x, y));
      }
    }
    // s.Save("bilerp1.tga");
  }
  {
    Rgba a(0, 255, 0, 0);
    Rgba b(255, 0, 255, 255);
    Rgba c(64, 0, 0, 32);
    Rgba d(128, 128, 128, 100);
    for (Si32 y = 0; y < 256; ++y) {
      for (Si32 x = 0; x < 256; ++x) {
        TestBilerp(a, b, c, d, 1.f * x / 256.f, 1.f * y / 256.f);
      }
    }
  }
  for (Si32 i = 0; i < 10; ++i) {
    Rgba a(static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)),
        static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)));
    Rgba b(static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)),
        static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)));
    Rgba c(static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)),
        static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)));
    Rgba d(static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)),
        static_cast<Ui8>(Random(0, 255)), static_cast<Ui8>(Random(0, 255)));
    for (Si32 y = 0; y < 256; ++y) {
      for (Si32 x = 0; x < 256; ++x) {
        TestBilerp(a, b, c, d, 1.f * x / 256.f, 1.f * y / 256.f);
      }
    }
  }
}

void test_sprite_save_png(void) {
  Sprite sprite;
  sprite.Create(7, 3);
  sprite.RgbaData()[0] = Rgba(255, 0, 0);
  sprite.RgbaData()[sprite.StridePixels() * 2 + 6] = Rgba(0, 255, 0);

  std::vector<Ui8> png = sprite.SaveToData("picture.png");
  TEST_CHECK_(png.size() > 8, "the png is %zu bytes long", png.size());
  if (png.size() < 33) {
    return;
  }

  const Ui8 signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  TEST_CHECK_(memcmp(png.data(), signature, 8) == 0,
      "the data does not start with the png signature");
  TEST_CHECK_(memcmp(png.data() + 12, "IHDR", 4) == 0,
      "the first chunk is not IHDR");
  const Ui32 width = ((Ui32)png[16] << 24) | ((Ui32)png[17] << 16)
    | ((Ui32)png[18] << 8) | (Ui32)png[19];
  const Ui32 height = ((Ui32)png[20] << 24) | ((Ui32)png[21] << 16)
    | ((Ui32)png[22] << 8) | (Ui32)png[23];
  TEST_CHECK_(width == 7 && height == 3,
      "IHDR says %u by %u instead of 7 by 3", width, height);
  TEST_CHECK_(png[24] == 8 && png[25] == 6,
      "IHDR says %u bits and color type %u instead of 8 and 6",
      (unsigned int)png[24], (unsigned int)png[25]);

  // The tga path has to keep working next to the new one.
  std::vector<Ui8> tga = sprite.SaveToData("picture.tga");
  TEST_CHECK_(tga.size() >= 18 + 7 * 3 * 4,
      "the tga is %zu bytes long, too short for a 7 by 3 image", tga.size());
}

void test_sprite_load_png_round_trip(void) {
  Sprite written;
  written.Create(5, 3);
  // Corners and one middle pixel differ from each other, so a flipped or
  // transposed image can not pass the comparison below by accident.
  written.RgbaData()[0] = Rgba(255, 0, 0, 255);
  written.RgbaData()[4] = Rgba(0, 255, 0, 255);
  written.RgbaData()[written.StridePixels() * 2] = Rgba(0, 0, 255, 255);
  written.RgbaData()[written.StridePixels() * 2 + 4] = Rgba(255, 255, 0, 128);
  written.RgbaData()[written.StridePixels() + 2] = Rgba(1, 2, 3, 4);

  std::vector<Ui8> png = written.SaveToData("picture.png");
  Sprite read;
  read.LoadFromData(png.data(), png.size(), "picture.png");
  TEST_CHECK_(read.Width() == 5 && read.Height() == 3,
      "the png came back as %d by %d instead of 5 by 3",
      read.Width(), read.Height());
  if (read.Width() != 5 || read.Height() != 3) {
    return;
  }
  for (Si32 y = 0; y < 3; ++y) {
    for (Si32 x = 0; x < 5; ++x) {
      Rgba from = written.RgbaData()[y * written.StridePixels() + x];
      Rgba to = read.RgbaData()[y * read.StridePixels() + x];
      TEST_CHECK_(from.rgba == to.rgba,
          "pixel %d,%d came back as %u,%u,%u,%u instead of %u,%u,%u,%u",
          x, y, to.r, to.g, to.b, to.a, from.r, from.g, from.b, from.a);
    }
  }
}

void test_sprite_load_png_palette(void) {
  // A four by two palette png with a transparent entry, written by hand rather
  // than by the engine, so that the reader is checked against a file it did not
  // make itself. The rows are red, green, blue, clear from the top, and the same
  // four backwards below; the palette and the transparency both have to be
  // resolved to arrive at rgba pixels.
  const Ui8 kPng[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13,
    73, 72, 68, 82, 0, 0, 0, 4, 0, 0, 0, 2,
    8, 3, 0, 0, 0, 72, 118, 141, 81, 0, 0, 0,
    12, 80, 76, 84, 69, 255, 0, 0, 0, 255, 0, 0,
    0, 255, 0, 0, 0, 251, 190, 70, 228, 0, 0, 0,
    4, 116, 82, 78, 83, 255, 255, 255, 0, 64, 42, 169,
    244, 0, 0, 0, 18, 73, 68, 65, 84, 120, 218, 99,
    96, 96, 100, 98, 102, 96, 102, 98, 100, 0, 0, 0,
    70, 0, 13, 164, 0, 89, 123, 0, 0, 0, 0, 73,
    69, 78, 68, 174, 66, 96, 130};

  Sprite sprite;
  sprite.LoadFromData(kPng, sizeof(kPng), "palette.png");
  TEST_CHECK_(sprite.Width() == 4 && sprite.Height() == 2,
      "the palette png came back as %d by %d instead of 4 by 2",
      sprite.Width(), sprite.Height());
  if (sprite.Width() != 4 || sprite.Height() != 2) {
    return;
  }
  // A sprite keeps its rows from the bottom up, so row 0 is the lower row of the
  // file, the one that runs clear, blue, green, red.
  const Rgba kExpected[2][4] = {
    {Rgba(0, 0, 0, 0), Rgba(0, 0, 255, 255),
     Rgba(0, 255, 0, 255), Rgba(255, 0, 0, 255)},
    {Rgba(255, 0, 0, 255), Rgba(0, 255, 0, 255),
     Rgba(0, 0, 255, 255), Rgba(0, 0, 0, 0)}};
  for (Si32 y = 0; y < 2; ++y) {
    for (Si32 x = 0; x < 4; ++x) {
      Rgba to = sprite.RgbaData()[y * sprite.StridePixels() + x];
      Rgba from = kExpected[y][x];
      TEST_CHECK_(from.rgba == to.rgba,
          "pixel %d,%d came back as %u,%u,%u,%u instead of %u,%u,%u,%u",
          x, y, to.r, to.g, to.b, to.a, from.r, from.g, from.b, from.a);
    }
  }
}

void test_sprite_load_png_refuses_garbage(void) {
  const Ui8 kGarbage[] = {137, 80, 78, 71, 13, 10, 26, 10, 1, 2, 3, 4, 5};
  Sprite sprite;
  sprite.LoadFromData(kGarbage, sizeof(kGarbage), "broken.png");
  TEST_CHECK_(sprite.Width() == 0 && sprite.Height() == 0,
      "a broken png produced a %d by %d sprite instead of an empty one",
      sprite.Width(), sprite.Height());
}

void test_colorize_blend_rb_vs_g_inconsistency() {
  // The partial-alpha colorize blend uses different formulas for R/B vs G
  // channels. R/B are packed and processed via integer bit tricks, while G
  // is computed with a single multiply chain. For a perfectly gray source
  // pixel (R=G=B) drawn with a perfectly gray tint, all three output
  // channels must be equal. The bug causes R and B to differ from G.

  // Create a 4x4 source sprite filled with white at partial alpha
  Sprite src;
  src.Create(4, 4);
  {
    Rgba *p = src.RgbaData();
    for (int i = 0; i < 4 * 4; ++i) {
      p[i] = Rgba(255, 255, 255, 200);  // Semi-transparent white
    }
  }

  // Create a 4x4 destination sprite filled with black
  Sprite dst;
  dst.Create(4, 4);
  dst.Clear(Rgba(0, 0, 0, 255));

  // Draw source onto destination with colorize blending and a gray tint.
  // Since src R=G=B and tint R=G=B, the output R, G, B must all be equal.
  Rgba tint(128, 128, 128, 255);
  src.Draw(dst, 0, 0, kDrawBlendingModeColorize, kFilterNearest, tint);

  // Read back a pixel
  Rgba result = dst.RgbaData()[0];

  // Compute what the correct reference value should be for all channels.
  // With the full-alpha colorize formula (known correct):
  //   colorized = (src_ch * (tint_ch + 1)) >> 8 = (255 * 129) >> 8 = 100
  // Then alpha-blended onto black:
  //   ca = (200 * 256) >> 8 = 200
  //   result = (colorized * ca) / 256 = (100 * 200) / 256 = 78
  //
  // The G channel computes this correctly via:
  //   g2 = (src_G * m2 * (tint_G + 1)) >> 8
  // The R/B channels use a packed approach that first truncates src*alpha
  // to 8 bits, then multiplies by tint, losing precision:
  //   rb2_scaled = floor(src_R * ca / 256) = floor(255 * 200 / 256) = 199
  //   rb2_R = rb2_scaled * tint_R = 199 * 128 = 25472
  //   result_R = 25472 / 256 = 99
  //
  // So G = 100 but R = B = 99 -- a visible inconsistency.

  TEST_MSG("Colorize partial-alpha result: R=%d, G=%d, B=%d, A=%d",
      (int)result.r, (int)result.g, (int)result.b, (int)result.a);

  TEST_CHECK_(result.r == result.g,
      "R and G should be equal for gray-on-gray colorize, "
      "but R=%d, G=%d (R/B formula loses precision vs G formula)",
      (int)result.r, (int)result.g);
  TEST_CHECK_(result.g == result.b,
      "G and B should be equal for gray-on-gray colorize, "
      "but G=%d, B=%d",
      (int)result.g, (int)result.b);
}

void test_colorize_blend_full_vs_partial_alpha_discontinuity() {
  // When source alpha changes from 254 to 255, the colorize blend
  // switches between two code paths with different formulas. This
  // causes a color discontinuity: the output can jump by 2+ values
  // for a single alpha step.

  Rgba tint(128, 128, 128, 255);

  // --- Full alpha (ca=255): uses per-channel formula ---
  Sprite src_full;
  src_full.Create(4, 4);
  {
    Rgba *p = src_full.RgbaData();
    for (int i = 0; i < 16; ++i) {
      p[i] = Rgba(255, 255, 255, 255);  // Fully opaque
    }
  }

  Sprite dst_full;
  dst_full.Create(4, 4);
  dst_full.Clear(Rgba(0, 0, 0, 255));

  src_full.Draw(dst_full, 0, 0, kDrawBlendingModeColorize, kFilterNearest, tint);
  Rgba result_full = dst_full.RgbaData()[0];

  // --- Partial alpha (ca=254): uses packed formula ---
  Sprite src_partial;
  src_partial.Create(4, 4);
  {
    Rgba *p = src_partial.RgbaData();
    for (int i = 0; i < 16; ++i) {
      p[i] = Rgba(255, 255, 255, 254);  // Nearly opaque
    }
  }

  Sprite dst_partial;
  dst_partial.Create(4, 4);
  dst_partial.Clear(Rgba(0, 0, 0, 255));

  src_partial.Draw(dst_partial, 0, 0, kDrawBlendingModeColorize, kFilterNearest, tint);
  Rgba result_partial = dst_partial.RgbaData()[0];

  // The full-alpha result for R: (255 * 129) >> 8 = 128
  // The partial-alpha result for R with ca=254, dest=black:
  //   rb2_R_scaled = floor(255 * 254 / 256) = 252
  //   rb2_R = 252 * 128 = 32256
  //   result_R = 32256 / 256 = 126
  // Stepping from alpha=254 to alpha=255 jumps R from 126 to 128.
  // The expected continuous value at alpha=254 is ~127.5.
  // A well-behaved blend should differ by at most 1 per alpha step.

  int r_jump = abs((int)result_full.r - (int)result_partial.r);
  int g_jump = abs((int)result_full.g - (int)result_partial.g);

  TEST_MSG("Full-alpha (a=255): R=%d, G=%d, B=%d",
      (int)result_full.r, (int)result_full.g, (int)result_full.b);
  TEST_MSG("Partial-alpha (a=254): R=%d, G=%d, B=%d",
      (int)result_partial.r, (int)result_partial.g, (int)result_partial.b);
  TEST_MSG("Jump when crossing alpha boundary: R=%d, G=%d", r_jump, g_jump);

  TEST_CHECK_(r_jump <= 1,
      "R channel jumps by %d when alpha goes from 254 to 255, "
      "expected at most 1 (discontinuity between full-alpha and "
      "partial-alpha colorize formulas)",
      r_jump);
}

// Coordinates grow upward here while a mock-up counts downward from the top, so
// the engine converts instead of leaving it to subtraction at every call site.
// A point and a box are two different conversions, and the box is the one that
// gets it wrong.
void test_top_left_coordinate_helpers() {
  ResizeScreen(320, 200);
  TEST_CHECK_(ScreenSize() == Vec2Si32(320, 200),
      "the backbuffer is %d x %d, the rest of this test assumes 320 x 200",
      ScreenSize().x, ScreenSize().y);

  // Row 0 from the top is the topmost row of pixels.
  TEST_CHECK_(FromTopLeft(Vec2Si32(7, 0)) == Vec2Si32(7, 199),
      "the top row went to y = %d instead of 199",
      FromTopLeft(Vec2Si32(7, 0)).y);
  TEST_CHECK_(FromTopLeft(Vec2Si32(7, 199)) == Vec2Si32(7, 0),
      "the bottom row went to y = %d instead of 0",
      FromTopLeft(Vec2Si32(7, 199)).y);
  for (Si32 y = 0; y < 200; y += 37) {
    const Vec2Si32 point(11, y);
    TEST_CHECK_(FromTopLeft(FromTopLeft(point)) == point,
        "converting (11, %d) twice moved it to (%d, %d)", y,
        FromTopLeft(FromTopLeft(point)).x, FromTopLeft(FromTopLeft(point)).y);
    TEST_CHECK_(ToTopLeft(FromTopLeft(point)) == point,
        "ToTopLeft did not undo FromTopLeft at y = %d", y);
  }

  // "twenty from the left, forty below the top, three hundred by a hundred and
  // twenty": the measured edge is the top one, so the position a Draw call
  // wants is 200 - 40 - 120 = 40.
  const Vec2Si32 kSize(300, 120);
  const Vec2Si32 kFromTop(20, 40);
  const Vec2Si32 box = FromTopLeft(kFromTop, kSize);
  TEST_CHECK_(box == Vec2Si32(20, 40),
      "a box measured 40 below the top landed at (%d, %d)", box.x, box.y);
  TEST_CHECK_(box.y + kSize.y == ScreenSize().y - kFromTop.y,
      "the top edge of the box is at %d, and 40 below the top is %d",
      box.y + kSize.y, ScreenSize().y - kFromTop.y);
  TEST_CHECK_(ToTopLeft(box, kSize) == kFromTop,
      "the box did not survive the trip back: (%d, %d)",
      ToTopLeft(box, kSize).x, ToTopLeft(box, kSize).y);

  // Negative control. The point conversion applied to a box puts its top edge
  // a whole height away from the measured one, which is exactly the mistake
  // the two overloads exist to prevent; if this ever agreed, the box overload
  // would be testing nothing.
  const Vec2Si32 as_a_point = FromTopLeft(kFromTop);
  TEST_CHECK_(as_a_point.y + kSize.y != ScreenSize().y - kFromTop.y,
      "the point conversion placed a box correctly, so this test proves "
      "nothing about the box overload");

  // Inside a sprite the same conversion turns around the height of the sprite.
  Sprite target;
  target.Create(64, 32);
  TEST_CHECK_(FromTopLeft(target, Vec2Si32(3, 0)) == Vec2Si32(3, 31),
      "the top row of a 32 pixel sprite went to y = %d",
      FromTopLeft(target, Vec2Si32(3, 0)).y);
  TEST_CHECK(FromTopLeft(target, Vec2Si32(3, 31)) == Vec2Si32(3, 0));
  TEST_CHECK_(FromTopLeft(target, Vec2Si32(3, 4), Vec2Si32(10, 8))
          == Vec2Si32(3, 20),
      "a box 4 below the top of a 32 pixel sprite landed at y = %d",
      FromTopLeft(target, Vec2Si32(3, 4), Vec2Si32(10, 8)).y);

  Sprite empty;
  TEST_CHECK_(FromTopLeft(empty, Vec2Si32(5, 6)) == Vec2Si32(5, 6),
      "an empty sprite has no height to turn a point around");
}

// A sprite is placed by its pivot, so a click test written as a box at the draw
// position misses by the pivot. The check below does not repeat the arithmetic
// of the engine: it draws the sprite and compares the pixels that came out with
// what IsPointInSprite claims, pixel by pixel.
void test_is_point_in_sprite_follows_the_drawing() {
  const Rgba kBackground(0, 0, 0, 255);
  const Rgba kBody(255, 0, 0, 255);
  const Vec2Si32 kDrawnAt(100, 50);

  Sprite hero;
  hero.Create(16, 24);
  hero.Clear(kBody);
  // What a tga brings by itself through its origin field: feet in the middle
  // of the bottom edge.
  hero.SetPivot(Vec2Si32(8, 0));

  Sprite canvas;
  canvas.Create(200, 120);
  canvas.Clear(kBackground);
  hero.Draw(canvas, kDrawnAt);

  Si32 painted_pixels = 0;
  Si32 disagreements = 0;
  Vec2Si32 first_disagreement(-1, -1);
  const Rgba *pixels = canvas.RgbaData();
  const Si32 stride = canvas.StridePixels();
  for (Si32 y = 0; y < canvas.Height(); ++y) {
    for (Si32 x = 0; x < canvas.Width(); ++x) {
      const bool is_painted = pixels[y * stride + x] == kBody;
      if (is_painted) {
        ++painted_pixels;
      }
      const bool is_claimed =
          IsPointInSprite(hero, kDrawnAt, Vec2Si32(x, y));
      if (is_painted != is_claimed) {
        ++disagreements;
        if (first_disagreement.x < 0) {
          first_disagreement = Vec2Si32(x, y);
        }
      }
    }
  }
  TEST_CHECK_(painted_pixels == 16 * 24,
      "the sprite painted %d pixels instead of %d, so the comparison below "
      "is meaningless", painted_pixels, 16 * 24);
  TEST_CHECK_(disagreements == 0,
      "IsPointInSprite disagrees with the drawing at %d pixels, first at "
      "(%d, %d)", disagreements, first_disagreement.x, first_disagreement.y);

  // Negative control: the same test written the way it gets written by hand,
  // as a box at the draw position, must fail on the pixels the pivot moved.
  Si32 naive_disagreements = 0;
  for (Si32 y = 0; y < canvas.Height(); ++y) {
    for (Si32 x = 0; x < canvas.Width(); ++x) {
      const bool is_painted = pixels[y * stride + x] == kBody;
      const bool is_claimed = x >= kDrawnAt.x
        && x < kDrawnAt.x + hero.Width()
        && y >= kDrawnAt.y && y < kDrawnAt.y + hero.Height();
      if (is_painted != is_claimed) {
        ++naive_disagreements;
      }
    }
  }
  TEST_CHECK_(naive_disagreements > 0,
      "a hit test that ignores the pivot agreed with the drawing, so this "
      "test proves nothing about the pivot");

  // A pivot at zero is the case where the two agree, and it must still be
  // right rather than merely different.
  Sprite plain;
  plain.Create(16, 24);
  plain.Clear(kBody);
  canvas.Clear(kBackground);
  plain.Draw(canvas, kDrawnAt);
  disagreements = 0;
  for (Si32 y = 0; y < canvas.Height(); ++y) {
    for (Si32 x = 0; x < canvas.Width(); ++x) {
      const bool is_painted = pixels[y * stride + x] == kBody;
      if (is_painted != IsPointInSprite(plain, kDrawnAt, Vec2Si32(x, y))) {
        ++disagreements;
      }
    }
  }
  TEST_CHECK_(disagreements == 0,
      "with the pivot at zero IsPointInSprite still disagrees with the "
      "drawing at %d pixels", disagreements);

  Sprite nothing;
  TEST_CHECK_(!IsPointInSprite(nothing, kDrawnAt, kDrawnAt),
      "an empty sprite covers no point at all");

  // The hardware overload answers about the same rectangle. A run without a
  // GL context has no hardware sprites to ask.
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped the HwSprite half: this run has no OpenGL context");
    return;
  }
  HwSprite hw_hero;
  hw_hero.LoadFromSoftwareSprite(hero);
  TEST_CHECK_(hw_hero.Pivot() == hero.Pivot(),
      "LoadFromSoftwareSprite lost the pivot: (%d, %d)",
      hw_hero.Pivot().x, hw_hero.Pivot().y);
  for (Si32 y = 0; y < canvas.Height(); ++y) {
    for (Si32 x = 0; x < canvas.Width(); ++x) {
      const Vec2Si32 point(x, y);
      if (IsPointInSprite(hw_hero, kDrawnAt, point)
          != IsPointInSprite(hero, kDrawnAt, point)) {
        TEST_CHECK_(false,
            "the hardware and the software answers differ at (%d, %d)", x, y);
        return;
      }
    }
  }
}

// A filled rectangle in the hardware path, which used to require a texture per
// wall. The check reads the frame back, so it also pins down the composition
// order: the software backbuffer covers the hardware fill, never the other way
// round.
void test_hw_rectangle_fill_and_composition_order() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  // A backbuffer of window size makes the frame a pixel to pixel copy of it,
  // so a screenshot can be read by backbuffer coordinates.
  const Vec2Si32 window = WindowSize();
  TEST_CHECK_(window.x > 32 && window.y > 32,
      "the window is %d x %d, too small for this test", window.x, window.y);
  ResizeScreen(window);

  const Rgba kFill(0, 0, 255, 255);
  const Rgba kSoftware(255, 255, 0, 255);
  // The fill covers the left half of a band across the middle of the screen,
  // and a software rectangle covers the middle third of that same band, so the
  // three cases are one screenshot apart.
  const Si32 band_bottom = window.y / 4;
  const Si32 band_top = window.y / 2;
  const Vec2Si32 fill_ll(0, band_bottom);
  const Vec2Si32 fill_ur(window.x / 2, band_top);
  const Vec2Si32 sw_ll(window.x / 6, band_bottom);
  const Vec2Si32 sw_ur(window.x / 3, band_top);

  Clear();
  DrawRectangleHw(fill_ll, fill_ur, kFill);
  DrawRectangle(sw_ll, sw_ur, kSoftware);
  Sprite frame = Screenshot();
  ShowFrame();

  TEST_CHECK_(frame.Size() == window,
      "the screenshot is %d x %d while the window is %d x %d",
      frame.Width(), frame.Height(), window.x, window.y);
  if (frame.Size() != window) {
    return;
  }

  const Rgba *pixels = frame.RgbaData();
  const Si32 stride = frame.StridePixels();
  auto pixel_at = [&](Vec2Si32 pos) {
    return pixels[pos.y * stride + pos.x];
  };
  auto is_color = [](Rgba got, Rgba want) {
    // The frame goes through a texture, so allow a rounding step per channel.
    return std::abs(static_cast<int>(got.r) - static_cast<int>(want.r)) <= 1
      && std::abs(static_cast<int>(got.g) - static_cast<int>(want.g)) <= 1
      && std::abs(static_cast<int>(got.b) - static_cast<int>(want.b)) <= 1;
  };

  // Inside the fill and outside the software rectangle: the fill is visible,
  // which is the whole point of the function.
  const Vec2Si32 fill_only((fill_ll.x + sw_ll.x) / 2,
      (band_bottom + band_top) / 2);
  TEST_CHECK_(is_color(pixel_at(fill_only), kFill),
      "the hardware fill at (%d, %d) came out as %d %d %d",
      fill_only.x, fill_only.y, pixel_at(fill_only).r, pixel_at(fill_only).g,
      pixel_at(fill_only).b);

  // The corners of the fill belong to it, since the corners are inclusive.
  TEST_CHECK_(is_color(pixel_at(Vec2Si32(fill_ur.x, band_top)), kFill),
      "the upper right corner of the fill is %d %d %d, and the corners are "
      "inclusive", pixel_at(Vec2Si32(fill_ur.x, band_top)).r,
      pixel_at(Vec2Si32(fill_ur.x, band_top)).g,
      pixel_at(Vec2Si32(fill_ur.x, band_top)).b);
  TEST_CHECK_(!is_color(pixel_at(Vec2Si32(fill_ur.x + 1, band_top)), kFill),
      "the fill leaked one pixel past its upper right corner");
  TEST_CHECK_(!is_color(pixel_at(Vec2Si32(fill_only.x, band_bottom - 1)),
          kFill),
      "the fill leaked one pixel below its lower edge");

  // Where the two overlap, the software rectangle wins whatever the call order
  // was, because the backbuffer is composed above the hardware sprites.
  const Vec2Si32 both((sw_ll.x + sw_ur.x) / 2, (band_bottom + band_top) / 2);
  TEST_CHECK_(is_color(pixel_at(both), kSoftware),
      "in the overlap the frame shows %d %d %d, and the software backbuffer "
      "is supposed to be composed above the hardware fill",
      pixel_at(both).r, pixel_at(both).g, pixel_at(both).b);

  // Negative control: a screenshot of a frame with neither rectangle must not
  // show the colors, otherwise the checks above would pass on an empty frame.
  Clear();
  Sprite empty_frame = Screenshot();
  ShowFrame();
  const Rgba *empty_pixels = empty_frame.RgbaData();
  const Si32 empty_stride = empty_frame.StridePixels();
  TEST_CHECK_(!is_color(empty_pixels[fill_only.y * empty_stride + fill_only.x],
          kFill),
      "a frame with no rectangles in it already showed the fill color");
  TEST_CHECK_(!is_color(empty_pixels[both.y * empty_stride + both.x],
          kSoftware),
      "a frame with no rectangles in it already showed the software color");

  // The hardware sprite overload writes into a texture instead of the frame,
  // and a texture is read by drawing it: fill a sprite, put it on the screen
  // and look at the frame.
  HwSprite canvas;
  canvas.Create(16, 16);
  canvas.Clear(Rgba(0, 0, 0, 255));
  const Rgba kPatch(0, 255, 0, 255);
  DrawRectangleHw(canvas, Vec2Si32(4, 4), Vec2Si32(11, 11), kPatch);
  Clear();
  canvas.Draw(Vec2Si32(0, 0), kDrawBlendingModeCopyRgba);
  Sprite patched = Screenshot();
  ShowFrame();
  const Rgba *patch_pixels = patched.RgbaData();
  const Si32 patch_stride = patched.StridePixels();
  TEST_CHECK_(is_color(patch_pixels[8 * patch_stride + 8], kPatch),
      "the middle of the filled area of the sprite came out as %d %d %d",
      patch_pixels[8 * patch_stride + 8].r,
      patch_pixels[8 * patch_stride + 8].g,
      patch_pixels[8 * patch_stride + 8].b);
  TEST_CHECK_(!is_color(patch_pixels[1 * patch_stride + 1], kPatch),
      "the fill spread outside the rectangle asked for");
  TEST_CHECK_(is_color(patch_pixels[11 * patch_stride + 11], kPatch),
      "the upper right corner of the rectangle is outside the fill, and the "
      "corners are inclusive");
  TEST_CHECK_(!is_color(patch_pixels[12 * patch_stride + 12], kPatch),
      "the fill went one pixel past the corner it was given");
}

// Bug 35: Sprite::Reference on a zero-sized sprite must not produce
// negative ref_pos_ (from.ref_size_.x - 1 == -1 when ref_size_ is 0).
void test_sprite_reference_zero_size() {
  Sprite empty;
  TEST_CHECK_(empty.Width() == 0 && empty.Height() == 0,
      "Default Sprite must be 0x0, got %dx%d",
      empty.Width(), empty.Height());

  Sprite ref;
  ref.Reference(empty, 0, 0, 0, 0);
  TEST_CHECK_(ref.RefPos().x >= 0 && ref.RefPos().y >= 0,
      "Reference from zero-size sprite: RefPos must be >= (0,0), "
      "got (%d,%d)", ref.RefPos().x, ref.RefPos().y);
  TEST_CHECK_(ref.Width() >= 0 && ref.Height() >= 0,
      "Reference from zero-size sprite: size must be >= 0, "
      "got %dx%d", ref.Width(), ref.Height());
}

void test_hw_sprite_subregion_draws_correctly() {
  // Hardware sprites live in a GL context, and a run without a window has none,
  // so on a build machine with no display there is nothing here to check.
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  const Si32 TEX_W = 16;
  const Si32 TEX_H = 16;
  const Rgba GREEN(0, 200, 0, 255);
  const Rgba MAGENTA(200, 0, 200, 255);

  // Sub-region deliberately away from all edges and off-center:
  //   x=3..8  (width 5),  y=2..6  (height 4)
  // within a 16x16 texture. Margins: left=3, right=8, top=10, bottom=2.
  const Si32 REF_X = 3;
  const Si32 REF_Y = 2;
  const Si32 REF_W = 5;
  const Si32 REF_H = 4;

  // -- 1. Build a 16x16 SW sprite: fill everything MAGENTA,
  //        then paint the sub-region GREEN. --
  Sprite sw_source;
  sw_source.Create(TEX_W, TEX_H);
  {
    Rgba *px = sw_source.RgbaData();
    Si32 stride = sw_source.StridePixels();
    for (Si32 y = 0; y < TEX_H; ++y) {
      for (Si32 x = 0; x < TEX_W; ++x) {
        bool inside = (x >= REF_X && x < REF_X + REF_W &&
                       y >= REF_Y && y < REF_Y + REF_H);
        px[y * stride + x] = inside ? GREEN : MAGENTA;
      }
    }
  }

  // -- 2. Upload to GPU --
  HwSprite hw_source;
  hw_source.LoadFromSoftwareSprite(sw_source);

  // -- 3. Reference ONLY the green rectangle --
  HwSprite hw_ref;
  hw_ref.Reference(hw_source, REF_X, REF_Y, REF_W, REF_H);

  // -- 4. Target framebuffer same size as the sub-region --
  HwSprite hw_target;
  hw_target.Create(REF_W, REF_H);

  // -- 5. Shader (same as Engine::Draw2d) --
  const char *vs = R"(
    attribute vec3 vPosition;
    attribute vec2 vTex;
    varying vec2 v_texCoord;
    void main() {
      gl_Position = vec4(vPosition, 1.0);
      v_texCoord = vTex;
    }
  )";
  const char *fs = R"(
    #ifdef GL_ES
    precision mediump float;
    #endif
    varying vec2 v_texCoord;
    uniform sampler2D s_texture;
    void main() {
      gl_FragColor = texture2D(s_texture, v_texCoord);
    }
  )";
  GlProgram program;
  program.Create(vs, fs);

  // -- 6. Compute sub-region UVs (same formula as fixed Draw2d) --
  float tex_w = static_cast<float>(hw_ref.sprite_instance()->width());
  float tex_h = static_cast<float>(hw_ref.sprite_instance()->height());
  Vec2Si32 rp = hw_ref.RefPos();
  Vec2Si32 rs = hw_ref.Size();
  float u0 = static_cast<float>(rp.x) / tex_w;
  float v0 = static_cast<float>(rp.y) / tex_h;
  float u1 = static_cast<float>(rp.x + rs.x) / tex_w;
  float v1 = static_cast<float>(rp.y + rs.y) / tex_h;

  struct V {
    float px, py, pz;
    float u, v;
  };
  V quad[6] = {
    {-1, -1, 0,  u0, v0}, { 1, -1, 0,  u1, v0}, { 1,  1, 0,  u1, v1},
    {-1, -1, 0,  u0, v0}, { 1,  1, 0,  u1, v1}, {-1,  1, 0,  u0, v1},
  };
  GlBuffer vbo;
  vbo.Create();
  vbo.Bind(GL_ARRAY_BUFFER);
  vbo.SetData(quad, sizeof(quad));

  // -- 7. Render --
  hw_target.sprite_instance()->framebuffer().Bind();
  glViewport(0, 0, REF_W, REF_H);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  program.Bind();
  program.SetUniform("s_texture", 0);
  hw_ref.sprite_instance()->texture().Bind(0);

  vbo.Bind(GL_ARRAY_BUFFER);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
      sizeof(V), reinterpret_cast<void *>(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
      sizeof(V), reinterpret_cast<void *>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glFinish();

  // -- 8. Read back --
  std::vector<Rgba> pixels(REF_W * REF_H);
  glReadPixels(0, 0, REF_W, REF_H, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  GlFramebuffer::BindDefault();

  // -- 9. Every pixel must be GREEN. Any MAGENTA means
  //        the UV sampled outside the sub-region. --
  Si32 green_count = 0;
  Si32 magenta_count = 0;
  Si32 other_count = 0;
  for (size_t i = 0; i < pixels.size(); ++i) {
    Rgba c = pixels[i];
    if (c.g > 128 && c.r < 64 && c.b < 64) {
      ++green_count;
    } else if (c.r > 128 && c.b > 128 && c.g < 64) {
      ++magenta_count;
    } else {
      ++other_count;
    }
  }

  Si32 total = REF_W * REF_H;
  TEST_CHECK_(magenta_count == 0,
    "GPU rendered %d GREEN, %d MAGENTA, %d other out of %d pixels. "
    "Sub-region ref at (%d,%d) size %dx%d inside %dx%d texture "
    "must produce only green.",
    green_count, magenta_count, other_count, total,
    REF_X, REF_Y, REF_W, REF_H, TEX_W, TEX_H);
}
