#include "engine/test_main.h"

#include <algorithm>
#include <array>
#include <chrono>  // NOLINT
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <type_traits>
#include <vector>

#include "engine/arctic_pi.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"
#include "engine/arctic_platform_tcpip.h"

#include "engine/arctic_types.h"
#include "engine/easy.h"
#include "engine/easy_hw_sprite.h"
#include "engine/opengl.h"
#include "engine/gl_state.h"
#include "engine/localization.h"
#include "engine/json.h"
#include "engine/rgb.h"
#include "engine/data_writer.h"
#include "engine/data_reader.h"
#include "engine/easy_sound_instance.h"
#include "engine/quaternion.h"
#include "engine/transform3f.h"
#include "engine/skeleton.h"
#include "engine/unicode.h"
#include "engine/frustum3f.h"
#include "engine/mesh.h"
#include "engine/mesh_gen_mod_complex.h"
#include "engine/gui.h"
#include "engine/csv.h"
#include "engine/ofbx.h"
#include "engine/gl_texture_cache.h"
#include "engine/mesh_fbx.h"
#include "engine/model.h"
#include "engine/sphere_vs_triangle.h"
#include "engine/collide_soup.h"
#include "engine/physics_contact.h"
#include "engine/physics_debug.h"
#include "engine/physics_solver.h"
#include "engine/physics_sphere_body.h"
#include "engine/physics_world.h"


using namespace arctic;

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

void test_file_operations() {
  std::vector<DirectoryEntry> list;
  std::string canonic = arctic::CanonicalizePath("./..");
  
  std::string arctic_engine_dir = "../engine";
  bool is_engine_dir_found = false;
  for (Si32 i = 0; i < 10; ++i) {
    if (arctic::DoesDirectoryExist(arctic_engine_dir.c_str())
        == kTrivalentTrue) {
      is_engine_dir_found = true;
      break;
    }
    arctic_engine_dir = std::string("../") + arctic_engine_dir;
  }
  if (!is_engine_dir_found) {
    TEST_MSG("skipped: no engine directory above the binary");
    return;
  }

  std::string arctic_root_dir = arctic_engine_dir + std::string("/..");

  std::string relative = RelativePathFromTo(arctic_root_dir.c_str(),
      arctic_engine_dir.c_str());
  TEST_CHECK_(relative == std::string("./engine"), "relative: %s", relative.c_str());

  std::string relative2 = RelativePathFromTo(arctic_engine_dir.c_str(),
                                             arctic_root_dir.c_str());
  TEST_CHECK_(relative2 == std::string("../"), "relative2: %s", relative2.c_str());
  
  bool isok = GetDirectoryEntries(arctic_engine_dir.c_str(), &list);
  TEST_CHECK(isok);
  TEST_CHECK(list.size() > 0);
}

// A path from argv means "relative to where the user was standing", and the
// current directory is not that place: on macOS the engine makes the resources
// folder of the bundle current before EasyMain. So the helper must resolve
// against the startup directory and must keep doing that after the current
// directory moves again.
void test_canonicalize_argv_path() {
  const std::string startup = arctic::GetStartupDirectory();
  TEST_CHECK_(!startup.empty(),
      "the startup directory was not remembered");
  TEST_CHECK_(startup == arctic::CanonicalizePath(startup.c_str()),
      "the startup directory is not canonical: '%s'", startup.c_str());

  const std::string relative = arctic::CanonicalizeArgvPath("snaps/world.dcs");
  const std::string expected = arctic::CanonicalizePath(
      arctic::GluePath(startup.c_str(), "snaps/world.dcs").c_str());
  TEST_CHECK_(relative == expected,
      "relative argv path resolved to '%s', expected '%s'",
      relative.c_str(), expected.c_str());

  // An absolute path is nobody's business but its own.
  const std::string absolute = arctic::CanonicalizeArgvPath(
      arctic::GluePath(startup.c_str(), "snaps/world.dcs").c_str());
  TEST_CHECK_(absolute == expected,
      "absolute argv path changed to '%s', expected '%s'",
      absolute.c_str(), expected.c_str());

  // Empty in, empty out: the caller can tell "no path given" from a path.
  TEST_CHECK(arctic::CanonicalizeArgvPath("").empty());
  TEST_CHECK(arctic::CanonicalizeArgvPath(nullptr).empty());

  // The whole point: the answer does not follow the current directory. This is
  // the situation a bundled application is in from the very first line of
  // EasyMain, and the reason CanonicalizePath alone is not enough.
  std::string current_before;
  TEST_CHECK(arctic::GetCurrentPath(&current_before));
  const std::string parent = arctic::CanonicalizePath(
      arctic::GluePath(current_before.c_str(), "..").c_str());
  if (parent != current_before && parent != startup) {
    TEST_CHECK(arctic::ChangeCurrentDirectory(parent.c_str()));
    const std::string moved = arctic::CanonicalizeArgvPath("snaps/world.dcs");
    const std::string plain = arctic::CanonicalizePath("snaps/world.dcs");
    TEST_CHECK(arctic::ChangeCurrentDirectory(current_before.c_str()));
    TEST_CHECK_(moved == expected,
        "after a chdir the same argv path resolved to '%s', expected '%s'",
        moved.c_str(), expected.c_str());
    TEST_CHECK_(plain != expected,
        "CanonicalizePath happened to give the same answer, so this test "
        "proves nothing: '%s'", plain.c_str());
  }
}

// "The file does not exist" is not a diagnosis. Whatever else the description
// says, it has to name the path as given and the absolute path behind it, and it
// has to point at the startup directory when the file is sitting right there.
void test_describe_file_path() {
  const std::string missing =
      arctic::DescribeFilePath("data/no_such_file_41287.tga");
  TEST_CHECK_(missing.find("data/no_such_file_41287.tga") != std::string::npos,
      "the description lost the path as given: '%s'", missing.c_str());
  const std::string absolute =
      arctic::CanonicalizePath("data/no_such_file_41287.tga");
  TEST_CHECK_(missing.find(absolute) != std::string::npos,
      "the description lost the absolute path '%s': '%s'",
      absolute.c_str(), missing.c_str());

  std::string current;
  TEST_CHECK(arctic::GetCurrentPath(&current));
  TEST_CHECK_(missing.find(current) != std::string::npos,
      "the description does not say what a relative path was resolved "
      "against: '%s'", missing.c_str());

  // An absolute path needs no directory to be resolved against, so the current
  // directory would only be noise there.
  const std::string absolute_missing = arctic::DescribeFilePath(
      arctic::GluePath(current.c_str(), "no_such_file_41287.tga").c_str());
  TEST_CHECK_(absolute_missing.find("current directory") == std::string::npos,
      "an absolute path was described through the current directory: '%s'",
      absolute_missing.c_str());

  const std::string deep =
      arctic::DescribeFilePath("no_such_dir_41287/file.txt");
  TEST_CHECK_(deep.find("does not exist") != std::string::npos,
      "a missing parent directory was not reported: '%s'", deep.c_str());

  // The trap the description exists for: the file is where the user typed it,
  // and the engine looked for it next to the assets.
  const std::string startup = arctic::GetStartupDirectory();
  if (!startup.empty() && startup != current) {
    const char *name = "test_startup_hint_41287.txt";
    const std::string in_startup =
        arctic::GluePath(startup.c_str(), name);
    {
      std::ofstream ofs(in_startup.c_str());
      TEST_CHECK_(ofs.good(), "failed to create '%s'", in_startup.c_str());
      ofs << "test";
    }
    const std::string hinted = arctic::DescribeFilePath(name);
    std::remove(in_startup.c_str());
    TEST_CHECK_(hinted.find(in_startup) != std::string::npos,
        "the description does not mention the file in the startup directory: "
        "'%s'", hinted.c_str());
    TEST_CHECK_(hinted.find("CanonicalizeArgvPath") != std::string::npos,
        "the description does not name the way out: '%s'", hinted.c_str());
  }
}

// DoesFileExist has to tell a file from a directory and from nothing at all,
// because "it exists" and "it is a file I can read" are different answers.
// ChangeCurrentDirectory is checked together with it: a test that moves the
// current directory has to be able to move it back.
void test_file_existence_and_current_directory() {
  std::string current;
  TEST_CHECK(arctic::GetCurrentPath(&current));

  TEST_CHECK(arctic::DoesFileExist("data/no_such_file_41287.tga")
      == kTrivalentFalse);
  // A directory is not a file, and saying "false" here would hide the reason a
  // read of such a path fails.
  TEST_CHECK(arctic::DoesFileExist(current.c_str()) == kTrivalentUnknown);

  const std::string temp = arctic::GluePath(current.c_str(),
      "test_file_exists_41287.txt");
  {
    std::ofstream ofs(temp.c_str());
    TEST_CHECK_(ofs.good(), "failed to create '%s'", temp.c_str());
    ofs << "test";
  }
  const Trivalent created = arctic::DoesFileExist(temp.c_str());
  std::remove(temp.c_str());
  TEST_CHECK_(created == kTrivalentTrue,
      "a file that was just created is not seen at '%s'", temp.c_str());
  TEST_CHECK(arctic::DoesFileExist(temp.c_str()) == kTrivalentFalse);

  TEST_CHECK(!arctic::ChangeCurrentDirectory(""));
  TEST_CHECK(!arctic::ChangeCurrentDirectory(nullptr));
  TEST_CHECK(!arctic::ChangeCurrentDirectory("no_such_dir_41287"));

  const std::string parent = arctic::CanonicalizePath(
      arctic::GluePath(current.c_str(), "..").c_str());
  if (parent != current) {
    TEST_CHECK(arctic::ChangeCurrentDirectory(parent.c_str()));
    std::string moved;
    TEST_CHECK(arctic::GetCurrentPath(&moved));
    TEST_CHECK(arctic::ChangeCurrentDirectory(current.c_str()));
    TEST_CHECK_(moved == parent,
        "the current directory moved to '%s', expected '%s'",
        moved.c_str(), parent.c_str());
    std::string back;
    TEST_CHECK(arctic::GetCurrentPath(&back));
    TEST_CHECK_(back == current,
        "the current directory was left at '%s', expected '%s'",
        back.c_str(), current.c_str());
  }
}

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

void test_random() {
  {
    float mi = 1.f;
    float ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      float f = GetEngine()->GetRandomF();
      TEST_CHECK_(f >= 0.0f && f < 1.0f, "f=%.16f", f);
      mi = std::min(mi, f);
      ma = std::max(ma, f);
    }
    TEST_CHECK_(ma - mi > 0.9f, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    double mi = 1.f;
    double ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      double d = GetEngine()->GetRandomD();
      TEST_CHECK_(d >= 0.0 && d < 1.0, "d=%.16f", d);
      mi = std::min(mi, d);
      ma = std::max(ma, d);
    }
    TEST_CHECK_(ma - mi > 0.9, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    float mi = 1.f;
    float ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      float sf = GetEngine()->GetRandomSF();
      TEST_CHECK_(sf >= -1.0f && sf < 1.0f, "sf=%.16f", sf);
      mi = std::min(mi, sf);
      ma = std::max(ma, sf);
    }
    TEST_CHECK_(ma - mi > 1.9f, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    double mi = 1.f;
    double ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      double sd = GetEngine()->GetRandomSD();
      TEST_CHECK_(sd >= -1.0 && sd < 1.0, "sd=%.16f", sd);
      mi = std::min(mi, sd);
      ma = std::max(ma, sd);
    }
    TEST_CHECK_(ma - mi > 1.9, "mi=%.16f ma=%.16f", mi, ma);
  }
}


// ============================================================================
// Localization tests
// ============================================================================

std::string find_test_data_dir() {
  std::string data_dir = "data";
  for (int i = 0; i < 10; ++i) {
    if (arctic::DoesDirectoryExist(data_dir.c_str())) {
      return data_dir;
    }
    data_dir = "../" + data_dir;
  }
  return "data";
}

void test_localization_basic_load() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  bool loaded = loc.Load(data_dir + "/test_locale_en.csv");
  TEST_CHECK_(loaded, "Failed to load test_locale_en.csv");
  
  loc.SetLocale("en");
  
  // Test basic string retrieval
  const std::string& hello = Loc("hello");
  TEST_CHECK_(hello == "Hello World", "Expected 'Hello World', got '%s'", hello.c_str());
  
  // Test missing key
  const std::string& missing = Loc("nonexistent_key");
  TEST_CHECK_(missing.find("nonexistent_key") != std::string::npos, 
      "Missing key should contain key name, got '%s'", missing.c_str());
  
  // Test HasKey
  TEST_CHECK(loc.HasKey("hello"));
  TEST_CHECK(!loc.HasKey("nonexistent_key"));
  
  // Test Count
  TEST_CHECK_(loc.Count() >= 6, "Expected at least 6 strings, got %llu", 
      static_cast<unsigned long long>(loc.Count()));
  
  loc.Clear();
}

void test_localization_simple_substitution() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  // Test simple variable substitution
  std::string result = Loc("greeting", {{"name", "Alice"}});
  TEST_CHECK_(result == "Hello Alice!", "Expected 'Hello Alice!', got '%s'", result.c_str());
  
  // Test with missing variable (should keep placeholder)
  std::string result2 = Loc("greeting", {});
  TEST_CHECK_(result2.find("name") != std::string::npos, 
      "Missing var should be preserved, got '%s'", result2.c_str());
  
  loc.Clear();
}

void test_localization_plural_english() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  // Test plural forms
  std::string one = Loc("items", {{"count", 1}});
  TEST_CHECK_(one == "1 item", "Expected '1 item', got '%s'", one.c_str());
  
  std::string two = Loc("items", {{"count", 2}});
  TEST_CHECK_(two == "2 items", "Expected '2 items', got '%s'", two.c_str());
  
  std::string zero = Loc("items", {{"count", 0}});
  TEST_CHECK_(zero == "0 items", "Expected '0 items', got '%s'", zero.c_str());
  
  std::string many = Loc("items", {{"count", 100}});
  TEST_CHECK_(many == "100 items", "Expected '100 items', got '%s'", many.c_str());
  
  loc.Clear();
}

void test_localization_plural_russian() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_ru.csv");
  loc.SetLocale("ru");
  
  // Russian has complex plural rules: one, few, many
  // 1 -> one (предмет)
  // 2,3,4 -> few (предмета)
  // 5-20 -> many (предметов)
  // 21 -> one, 22-24 -> few, 25-30 -> many
  
  std::string one = Loc("items", {{"count", 1}});
  TEST_CHECK_(one == "1 предмет", "Expected '1 предмет', got '%s'", one.c_str());
  
  std::string two = Loc("items", {{"count", 2}});
  TEST_CHECK_(two == "2 предмета", "Expected '2 предмета', got '%s'", two.c_str());
  
  std::string five = Loc("items", {{"count", 5}});
  TEST_CHECK_(five == "5 предметов", "Expected '5 предметов', got '%s'", five.c_str());
  
  std::string eleven = Loc("items", {{"count", 11}});
  TEST_CHECK_(eleven == "11 предметов", "Expected '11 предметов', got '%s'", eleven.c_str());
  
  std::string twentyone = Loc("items", {{"count", 21}});
  TEST_CHECK_(twentyone == "21 предмет", "Expected '21 предмет', got '%s'", twentyone.c_str());
  
  std::string twentytwo = Loc("items", {{"count", 22}});
  TEST_CHECK_(twentytwo == "22 предмета", "Expected '22 предмета', got '%s'", twentytwo.c_str());
  
  loc.Clear();
}

void test_localization_select() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  std::string male = Loc("gender", {{"g", "male"}});
  TEST_CHECK_(male == "He", "Expected 'He', got '%s'", male.c_str());
  
  std::string female = Loc("gender", {{"g", "female"}});
  TEST_CHECK_(female == "She", "Expected 'She', got '%s'", female.c_str());
  
  std::string other = Loc("gender", {{"g", "unknown"}});
  TEST_CHECK_(other == "They", "Expected 'They', got '%s'", other.c_str());
  
  loc.Clear();
}

void test_localization_complex_pattern() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  // Test pattern with multiple substitutions
  std::string result = Loc("complex", {{"name", "Bob"}, {"count", 1}});
  TEST_CHECK_(result == "Bob has 1 cat", "Expected 'Bob has 1 cat', got '%s'", result.c_str());
  
  std::string result2 = Loc("complex", {{"name", "Alice"}, {"count", 3}});
  TEST_CHECK_(result2 == "Alice has 3 cats", "Expected 'Alice has 3 cats', got '%s'", result2.c_str());
  
  loc.Clear();
}

void test_localization_nested_plural_select() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  // Test nested plural inside select
  std::string he_one = Loc("nested", {{"g", "male"}, {"n", 1}});
  TEST_CHECK_(he_one == "He has 1 apple", "Expected 'He has 1 apple', got '%s'", he_one.c_str());
  
  std::string he_many = Loc("nested", {{"g", "male"}, {"n", 5}});
  TEST_CHECK_(he_many == "He has 5 apples", "Expected 'He has 5 apples', got '%s'", he_many.c_str());
  
  std::string she_one = Loc("nested", {{"g", "female"}, {"n", 1}});
  TEST_CHECK_(she_one == "She has 1 apple", "Expected 'She has 1 apple', got '%s'", she_one.c_str());
  
  std::string she_many = Loc("nested", {{"g", "female"}, {"n", 3}});
  TEST_CHECK_(she_many == "She has 3 apples", "Expected 'She has 3 apples', got '%s'", she_many.c_str());
  
  std::string they_one = Loc("nested", {{"g", "unknown"}, {"n", 1}});
  TEST_CHECK_(they_one == "They have 1 apple", "Expected 'They have 1 apple', got '%s'", they_one.c_str());
  
  std::string they_many = Loc("nested", {{"g", "other"}, {"n", 10}});
  TEST_CHECK_(they_many == "They have 10 apples", "Expected 'They have 10 apples', got '%s'", they_many.c_str());
  
  loc.Clear();
}

void test_localization_multi_locale_csv() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  bool loaded = loc.Load(data_dir + "/test_multi_locale.csv");
  TEST_CHECK_(loaded, "Failed to load test_multi_locale.csv");
  
  // Check that all locales are available
  std::vector<std::string> locales = loc.GetAvailableLocales();
  TEST_CHECK_(locales.size() >= 3, "Expected at least 3 locales, got %zu", locales.size());
  
  // Test English
  loc.SetLocale("en");
  TEST_CHECK_(Loc("simple") == "Simple", "en: Expected 'Simple', got '%s'", Loc("simple").c_str());
  
  // Test Russian  
  loc.SetLocale("ru");
  TEST_CHECK_(Loc("simple") == "Простой", "ru: Expected 'Простой', got '%s'", Loc("simple").c_str());
  
  // Test German
  loc.SetLocale("de");
  TEST_CHECK_(Loc("simple") == "Einfach", "de: Expected 'Einfach', got '%s'", Loc("simple").c_str());
  
  // Test plural with multi-locale
  loc.SetLocale("ru");
  std::string ru_plural = Loc("count", {{"n", 5}});
  TEST_CHECK_(ru_plural == "5 вещей", "ru plural: Expected '5 вещей', got '%s'", ru_plural.c_str());
  
  loc.Clear();
}

void test_localization_fallback() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.Load(data_dir + "/test_locale_ru.csv");
  
  loc.SetFallbackLocale("en");
  loc.SetLocale("ru");
  
  // "hello" exists in Russian
  TEST_CHECK_(Loc("hello") == "Привет Мир", "Should get Russian hello");
  
  // "gender" only exists in English, should fall back
  const std::string& gender = Loc("gender");
  TEST_CHECK_(gender.find("select") != std::string::npos || gender.find("male") != std::string::npos,
      "Should fall back to English for 'gender', got '%s'", gender.c_str());
  
  loc.Clear();
}

void test_localization_merge_files() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  
  // Load first file
  loc.Load(data_dir + "/test_locale_en.csv");
  Ui64 count1 = loc.Count("en");
  
  // Load multi-locale file (should merge)
  loc.Load(data_dir + "/test_multi_locale.csv");
  Ui64 count2 = loc.Count("en");
  
  TEST_CHECK_(count2 > count1, "Merging should increase count: %llu -> %llu",
      static_cast<unsigned long long>(count1), static_cast<unsigned long long>(count2));
  
  // Both old and new keys should exist
  loc.SetLocale("en");
  TEST_CHECK(loc.HasKey("hello"));  // From first file
  TEST_CHECK(loc.HasKey("simple")); // From multi-locale file
  
  loc.Clear();
}

void test_localization_loc_function() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  // Test global Loc() function
  const std::string& hello = Loc("hello");
  TEST_CHECK_(hello == "Hello World", "Loc() should return 'Hello World', got '%s'", hello.c_str());
  
  // Test Loc() with args
  std::string greeting = Loc("greeting", {{"name", "World"}});
  TEST_CHECK_(greeting == "Hello World!", "Loc() with args should return 'Hello World!', got '%s'", greeting.c_str());
  
  loc.Clear();
}

void test_localization_format_pattern_direct() {
  Localization& loc = Localization::Instance();
  loc.SetLocale("en");
  
  // Test FormatPattern directly without loading any files
  std::string result = loc.FormatPattern("Hello {name}!", {{"name", "Test"}});
  TEST_CHECK_(result == "Hello Test!", "Direct format: Expected 'Hello Test!', got '%s'", result.c_str());
  
  std::string plural = loc.FormatPattern("{n, plural, one {# apple} other {# apples}}", {{"n", 5}});
  TEST_CHECK_(plural == "5 apples", "Direct plural: Expected '5 apples', got '%s'", plural.c_str());
}

void test_localization_ordinal_english() {
  Localization& loc = Localization::Instance();
  loc.Clear();
  
  std::string data_dir = find_test_data_dir();
  loc.Load(data_dir + "/test_locale_en.csv");
  loc.SetLocale("en");
  
  std::string first = Loc("ordinal", {{"n", 1}});
  TEST_CHECK_(first == "1st", "Expected '1st', got '%s'", first.c_str());
  
  std::string second = Loc("ordinal", {{"n", 2}});
  TEST_CHECK_(second == "2nd", "Expected '2nd', got '%s'", second.c_str());
  
  std::string third = Loc("ordinal", {{"n", 3}});
  TEST_CHECK_(third == "3rd", "Expected '3rd', got '%s'", third.c_str());
  
  std::string fourth = Loc("ordinal", {{"n", 4}});
  TEST_CHECK_(fourth == "4th", "Expected '4th', got '%s'", fourth.c_str());
  
  std::string eleventh = Loc("ordinal", {{"n", 11}});
  TEST_CHECK_(eleventh == "11th", "Expected '11th', got '%s'", eleventh.c_str());
  
  std::string twentyfirst = Loc("ordinal", {{"n", 21}});
  TEST_CHECK_(twentyfirst == "21st", "Expected '21st', got '%s'", twentyfirst.c_str());
  
  loc.Clear();
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

// ============================================================================
// JSON tests (nlohmann/json)
// ============================================================================

using json = nlohmann::json;

void test_json_parse_string() {
  // Parse from string
  json j = json::parse(R"({"name": "Arctic", "version": 1})");
  TEST_CHECK(j.is_object());
  TEST_CHECK_(j["name"] == "Arctic", "Expected 'Arctic', got '%s'",
      j["name"].get<std::string>().c_str());
  TEST_CHECK_(j["version"] == 1, "Expected 1, got %d",
      j["version"].get<int>());

  // Parse array
  json arr = json::parse("[1, 2, 3]");
  TEST_CHECK(arr.is_array());
  TEST_CHECK_(arr.size() == 3, "Expected size 3, got %zu", arr.size());
  TEST_CHECK(arr[0] == 1);
  TEST_CHECK(arr[1] == 2);
  TEST_CHECK(arr[2] == 3);

  // Parse scalar types
  TEST_CHECK(json::parse("true").get<bool>() == true);
  TEST_CHECK(json::parse("false").get<bool>() == false);
  TEST_CHECK(json::parse("null").is_null());
  TEST_CHECK(json::parse("42").get<int>() == 42);
  TEST_CHECK(json::parse("3.14").get<double>() > 3.13);
  TEST_CHECK(json::parse("\"hello\"").get<std::string>() == "hello");
}

void test_json_parse_file() {
  std::string data_dir = find_test_data_dir();
  std::string path = data_dir + "/test_config.json";

  std::ifstream file(path);
  if (!TEST_CHECK_(file.is_open(), "Failed to open %s",
      arctic::DescribeFilePath(path.c_str()).c_str())) {
    return;
  }

  json j = json::parse(file);

  // Window section
  TEST_CHECK(j.contains("window"));
  TEST_CHECK_(j["window"]["title"] == "Arctic Test",
      "Expected 'Arctic Test', got '%s'",
      j["window"]["title"].get<std::string>().c_str());
  TEST_CHECK(j["window"]["width"] == 1280);
  TEST_CHECK(j["window"]["height"] == 720);
  TEST_CHECK(j["window"]["fullscreen"] == false);

  // Audio section
  TEST_CHECK_(j["audio"]["master_volume"].get<double>() > 0.79,
      "Expected ~0.8, got %f", j["audio"]["master_volume"].get<double>());

  // Nested object
  TEST_CHECK_(j["player"]["name"] == "Arctic Fox",
      "Expected 'Arctic Fox'");
  TEST_CHECK(j["player"]["level"] == 42);

  // Nested array
  const auto &inv = j["player"]["inventory"];
  TEST_CHECK(inv.is_array());
  TEST_CHECK_(inv.size() == 3, "Expected 3 items, got %zu", inv.size());
  TEST_CHECK(inv[0] == "sword");
  TEST_CHECK(inv[1] == "shield");
  TEST_CHECK(inv[2] == "potion");

  // Deep nesting
  TEST_CHECK_(j["player"]["position"]["x"].get<double>() > 10.4,
      "Expected x ~10.5");

  // Array of objects
  const auto &enemies = j["enemies"];
  TEST_CHECK_(enemies.size() == 2, "Expected 2 enemies");
  TEST_CHECK(enemies[0]["type"] == "goblin");
  TEST_CHECK(enemies[0]["hp"] == 30);
  TEST_CHECK(enemies[0]["aggressive"] == true);
  TEST_CHECK(enemies[1]["type"] == "dragon");
  TEST_CHECK(enemies[1]["hp"] == 500);

  // Empty containers
  TEST_CHECK(j["empty_object"].is_object());
  TEST_CHECK(j["empty_object"].empty());
  TEST_CHECK(j["empty_array"].is_array());
  TEST_CHECK(j["empty_array"].empty());

  // Null
  TEST_CHECK(j["null_value"].is_null());
}

void test_json_build_and_serialize() {
  // Build JSON programmatically
  json j;
  j["name"] = "test";
  j["count"] = 42;
  j["pi"] = 3.14159;
  j["active"] = true;
  j["tags"] = {"alpha", "beta", "gamma"};
  j["nested"]["x"] = 1;
  j["nested"]["y"] = 2;

  // Serialize and re-parse (round-trip)
  std::string serialized = j.dump();
  json j2 = json::parse(serialized);

  TEST_CHECK(j2["name"] == "test");
  TEST_CHECK(j2["count"] == 42);
  TEST_CHECK(j2["active"] == true);
  TEST_CHECK(j2["tags"].size() == 3);
  TEST_CHECK(j2["tags"][0] == "alpha");
  TEST_CHECK(j2["nested"]["x"] == 1);
  TEST_CHECK(j2["nested"]["y"] == 2);

  // Pretty print round-trip
  std::string pretty = j.dump(2);
  json j3 = json::parse(pretty);
  TEST_CHECK(j3 == j);
}

void test_json_type_conversions() {
  json j = json::parse(R"({"i": 42, "f": 3.14, "s": "hello", "b": true, "n": null})");

  // value() with defaults (like IniSection::GetInt / GetString style)
  TEST_CHECK(j.value("i", 0) == 42);
  TEST_CHECK(j.value("missing", 99) == 99);
  TEST_CHECK(j.value("s", std::string("default")) == "hello");
  TEST_CHECK(j.value("missing_str", std::string("fallback")) == "fallback");
  TEST_CHECK(j.value("b", false) == true);
  TEST_CHECK(j.value("missing_bool", true) == true);
  TEST_CHECK(j.value("f", 0.0) > 3.13);
  TEST_CHECK(j.value("missing_f", 1.5) > 1.49);

  // Null checks
  TEST_CHECK(j["n"].is_null());
  TEST_CHECK(!j["i"].is_null());
}

void test_json_iteration() {
  json j = json::parse(R"({"a": 1, "b": 2, "c": 3})");

  // Iterate object
  int sum = 0;
  int count = 0;
  for (auto it = j.items().begin(); it != j.items().end(); ++it) {
    sum += it.value().get<int>();
    count++;
  }
  TEST_CHECK_(sum == 6, "Expected sum 6, got %d", sum);
  TEST_CHECK_(count == 3, "Expected 3 items, got %d", count);

  // Iterate array
  json arr = json::parse("[10, 20, 30]");
  int arr_sum = 0;
  for (const auto &elem : arr) {
    arr_sum += elem.get<int>();
  }
  TEST_CHECK_(arr_sum == 60, "Expected sum 60, got %d", arr_sum);
}

void test_json_error_handling() {
  // Invalid JSON should throw
  bool caught = false;
  try {
    (void)json::parse("{invalid json}");
  } catch (const json::parse_error &) {
    caught = true;
  }
  TEST_CHECK_(caught, "Expected parse_error for invalid JSON");

  // parse with default value on error (accept policy)
  json j = json::parse("{bad}", nullptr, false);
  TEST_CHECK_(j.is_discarded(), "Expected discarded value for invalid JSON");
}

void test_json_modification() {
  json j = json::parse(R"({"items": [1, 2, 3], "meta": {"version": 1}})");

  // Modify values
  j["meta"]["version"] = 2;
  TEST_CHECK(j["meta"]["version"] == 2);

  // Add new keys
  j["meta"]["author"] = "tester";
  TEST_CHECK(j["meta"]["author"] == "tester");

  // Modify array
  j["items"].push_back(4);
  TEST_CHECK_(j["items"].size() == 4, "Expected 4 items after push_back");
  TEST_CHECK(j["items"][3] == 4);

  // Erase
  j["items"].erase(j["items"].begin());
  TEST_CHECK_(j["items"].size() == 3, "Expected 3 items after erase");
  TEST_CHECK(j["items"][0] == 2);

  // Remove key from object
  j["meta"].erase("author");
  TEST_CHECK(!j["meta"].contains("author"));
}

void test_json_comparison() {
  json a = json::parse(R"({"x": 1, "y": 2})");
  json b = json::parse(R"({"y": 2, "x": 1})");
  json c = json::parse(R"({"x": 1, "y": 3})");

  // Object equality is independent of key order
  TEST_CHECK(a == b);
  TEST_CHECK(a != c);

  // Array equality is order-dependent
  json arr1 = json::parse("[1, 2, 3]");
  json arr2 = json::parse("[1, 2, 3]");
  json arr3 = json::parse("[3, 2, 1]");
  TEST_CHECK(arr1 == arr2);
  TEST_CHECK(arr1 != arr3);
}

// ============================================================================
// DataWriter / DataReader tests
// ============================================================================

void test_data_writer_empty_initial_write() {
  DataWriter w;
  TEST_CHECK(w.data.empty());

  Ui8 val = 0xAB;
  w.WriteUInt8(val);
  TEST_CHECK_(w.data.size() == 1, "Expected size 1, got %zu", w.data.size());
  TEST_CHECK_(w.data[0] == 0xAB, "Expected 0xAB, got 0x%02X", w.data[0]);
}

void test_data_writer_multiple_writes_no_overlap() {
  DataWriter w;
  w.WriteUInt8(0x11);
  w.WriteUInt8(0x22);
  w.WriteUInt8(0x33);
  TEST_CHECK_(w.data.size() == 3, "Expected size 3, got %zu", w.data.size());
  TEST_CHECK_(w.data[0] == 0x11, "Byte 0: expected 0x11, got 0x%02X", w.data[0]);
  TEST_CHECK_(w.data[1] == 0x22, "Byte 1: expected 0x22, got 0x%02X", w.data[1]);
  TEST_CHECK_(w.data[2] == 0x33, "Byte 2: expected 0x33, got 0x%02X", w.data[2]);
}

void test_data_writer_uint16() {
  DataWriter w;
  w.WriteUInt16(0x1234);
  TEST_CHECK_(w.data.size() == 2, "Expected size 2, got %zu", w.data.size());
  Ui16 result;
  memcpy(&result, &w.data[0], 2);
  TEST_CHECK_(result == 0x1234, "Expected 0x1234, got 0x%04X", result);
}

void test_data_writer_uint32() {
  DataWriter w;
  w.WriteUInt32(0xDEADBEEF);
  TEST_CHECK_(w.data.size() == 4, "Expected size 4, got %zu", w.data.size());
  Ui32 result;
  memcpy(&result, &w.data[0], 4);
  TEST_CHECK_(result == 0xDEADBEEF, "Expected 0xDEADBEEF, got 0x%08X", result);
}

void test_data_writer_uint64() {
  DataWriter w;
  w.WriteUInt64(0x0102030405060708ULL);
  TEST_CHECK_(w.data.size() == 8, "Expected size 8, got %zu", w.data.size());
  Ui64 result;
  memcpy(&result, &w.data[0], 8);
  TEST_CHECK_(result == 0x0102030405060708ULL, "Expected 0x0102030405060708");
}

void test_data_writer_float() {
  DataWriter w;
  float val = 3.14f;
  w.WriteFloat(val);
  TEST_CHECK_(w.data.size() == 4, "Expected size 4, got %zu", w.data.size());
  float result;
  memcpy(&result, &w.data[0], 4);
  TEST_CHECK_(result == val, "Expected 3.14, got %f", result);
}

void test_data_writer_mixed_sequence() {
  DataWriter w;
  w.WriteUInt8(0xAA);
  w.WriteUInt16(0xBBCC);
  w.WriteUInt32(0xDDEEFF00);
  w.WriteFloat(1.5f);
  TEST_CHECK_(w.data.size() == 1 + 2 + 4 + 4,
      "Expected size 11, got %zu", w.data.size());

  // Verify no overlap: first byte should still be 0xAA
  TEST_CHECK_(w.data[0] == 0xAA, "First byte corrupted: 0x%02X", w.data[0]);
  Ui16 u16;
  memcpy(&u16, &w.data[1], 2);
  TEST_CHECK_(u16 == 0xBBCC, "Ui16 corrupted: 0x%04X", u16);
  Ui32 u32;
  memcpy(&u32, &w.data[3], 4);
  TEST_CHECK_(u32 == 0xDDEEFF00, "Ui32 corrupted: 0x%08X", u32);
}

void test_data_writer_uint16array() {
  DataWriter w;
  Ui16 arr[] = {0x1111, 0x2222, 0x3333};
  w.WriteUInt16array(arr, 3);
  TEST_CHECK_(w.data.size() == 6, "Expected size 6, got %zu", w.data.size());
  Ui16 out[3];
  memcpy(out, &w.data[0], 6);
  TEST_CHECK_(out[0] == 0x1111, "arr[0] expected 0x1111, got 0x%04X", out[0]);
  TEST_CHECK_(out[1] == 0x2222, "arr[1] expected 0x2222, got 0x%04X", out[1]);
  TEST_CHECK_(out[2] == 0x3333, "arr[2] expected 0x3333, got 0x%04X", out[2]);
}

void test_data_reader_advances_pointer() {
  DataWriter w;
  w.WriteUInt8(0x11);
  w.WriteUInt8(0x22);
  w.WriteUInt8(0x33);

  DataReader r;
  r.Reset(std::move(w.data));

  Ui8 a = r.ReadUInt8();
  Ui8 b = r.ReadUInt8();
  Ui8 c = r.ReadUInt8();
  TEST_CHECK_(a == 0x11, "First read: expected 0x11, got 0x%02X", a);
  TEST_CHECK_(b == 0x22, "Second read: expected 0x22, got 0x%02X", b);
  TEST_CHECK_(c == 0x33, "Third read: expected 0x33, got 0x%02X", c);
}

void test_data_roundtrip_all_types() {
  DataWriter w;
  w.WriteUInt8(42);
  w.WriteUInt16(1234);
  w.WriteUInt32(0xCAFEBABE);
  w.WriteUInt64(0x0123456789ABCDEFULL);
  w.WriteFloat(2.718f);

  DataReader r;
  r.Reset(std::move(w.data));

  Ui8 v8 = r.ReadUInt8();
  TEST_CHECK_(v8 == 42, "Ui8: expected 42, got %u", v8);

  Ui16 v16 = r.ReadUInt16();
  TEST_CHECK_(v16 == 1234, "Ui16: expected 1234, got %u", v16);

  Ui32 v32 = r.ReadUInt32();
  TEST_CHECK_(v32 == 0xCAFEBABE, "Ui32: expected 0xCAFEBABE, got 0x%08X", v32);

  Ui64 v64 = r.ReadUInt64();
  TEST_CHECK_(v64 == 0x0123456789ABCDEFULL, "Ui64 mismatch");

  float vf = r.ReadFloat();
  TEST_CHECK_(vf == 2.718f, "Float: expected 2.718, got %f", vf);
}

void test_data_roundtrip_arrays() {
  DataWriter w;
  Ui16 src16[] = {100, 200, 300, 400};
  w.WriteUInt16array(src16, 4);
  Ui32 src32[] = {0xAAAA, 0xBBBB};
  w.WriteUInt32array(src32, 2);

  DataReader r;
  r.Reset(std::move(w.data));

  Ui16 dst16[4] = {};
  r.ReadUInt16array(dst16, 4);
  for (int i = 0; i < 4; ++i) {
    TEST_CHECK_(dst16[i] == src16[i], "Ui16 arr[%d]: expected %u, got %u",
        i, src16[i], dst16[i]);
  }

  Ui32 dst32[2] = {};
  r.ReadUInt32array(dst32, 2);
  for (int i = 0; i < 2; ++i) {
    TEST_CHECK_(dst32[i] == src32[i], "Ui32 arr[%d]: expected 0x%X, got 0x%X",
        i, src32[i], dst32[i]);
  }
}

void test_data_reader_past_end() {
  DataWriter w;
  w.WriteUInt8(0xFF);

  DataReader r;
  r.Reset(std::move(w.data));

  // Read the one available byte
  Ui8 v = r.ReadUInt8();
  TEST_CHECK_(v == 0xFF, "Expected 0xFF, got 0x%02X", v);

  // Reading past end should return 0 bytes
  Ui8 buf[4] = {0xCC, 0xCC, 0xCC, 0xCC};
  Ui64 read = r.Read(buf, 4);
  TEST_CHECK_(read == 0, "Expected 0 bytes read past end, got %llu",
      static_cast<unsigned long long>(read));
}

void test_data_writer_large_sequence() {
  DataWriter w;
  for (Ui32 i = 0; i < 1000; ++i) {
    w.WriteUInt32(i);
  }
  TEST_CHECK_(w.data.size() == 4000, "Expected 4000 bytes, got %zu", w.data.size());

  DataReader r;
  r.Reset(std::move(w.data));
  for (Ui32 i = 0; i < 1000; ++i) {
    Ui32 v = r.ReadUInt32();
    if (!TEST_CHECK_(v == i, "At index %u: expected %u, got %u", i, i, v)) {
      break;
    }
  }
}

// ============================================================================
// easy_sound_instance bug reproduction tests
// ============================================================================

// Helper: build a minimal valid WAV file in memory.
// Returns the byte buffer. samples is interleaved raw PCM data.
std::vector<Ui8> build_wav(Ui16 channels, Ui32 sample_rate,
    Ui16 bits_per_sample, const std::vector<Ui8> &samples) {
  Ui16 block_align = channels * (bits_per_sample / 8);
  Ui32 byte_rate = sample_rate * block_align;
  Ui32 data_size = static_cast<Ui32>(samples.size());
  // total = 12 (RIFF header) + 24 (fmt subchunk) + 8 (data header) + data_size
  Ui32 chunk_size = 4 + 24 + 8 + data_size;

  std::vector<Ui8> buf;
  buf.reserve(12 + 24 + 8 + data_size);

  auto push_u32_le = [&](Ui32 v) {
    buf.push_back(static_cast<Ui8>(v & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 16) & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 24) & 0xFF));
  };
  auto push_u16_le = [&](Ui16 v) {
    buf.push_back(static_cast<Ui8>(v & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 8) & 0xFF));
  };
  auto push_tag = [&](const char *tag) {
    buf.push_back(static_cast<Ui8>(tag[0]));
    buf.push_back(static_cast<Ui8>(tag[1]));
    buf.push_back(static_cast<Ui8>(tag[2]));
    buf.push_back(static_cast<Ui8>(tag[3]));
  };

  // RIFF header
  push_tag("RIFF");
  push_u32_le(chunk_size);
  push_tag("WAVE");

  // fmt subchunk
  push_tag("fmt ");
  push_u32_le(16);          // subchunk size
  push_u16_le(1);           // audio_format = PCM
  push_u16_le(channels);
  push_u32_le(sample_rate);
  push_u32_le(byte_rate);
  push_u16_le(block_align);
  push_u16_le(bits_per_sample);

  // data subchunk
  push_tag("data");
  push_u32_le(data_size);
  buf.insert(buf.end(), samples.begin(), samples.end());

  return buf;
}

// Bug 1: LoadWav returns nullptr for any sample_rate != 44100.
//
// The resampling path (line 265) has an inverted bounds check on line 294:
//   if (idx*2*sizeof(Si16) < sample_count * 2 * sizeof(Si16))
// This simplifies to "if (idx < sample_count)", which is always true inside
// the loop "for (idx = 0; idx < sample_count; ++idx)", so LoadWav always
// returns nullptr when resampling is needed.
void test_sound_resample_returns_nullptr() {
  // 4 samples of 16-bit mono silence at 22050 Hz
  std::vector<Ui8> pcm(4 * 2, 0);  // 4 samples * 2 bytes
  std::vector<Ui8> wav = build_wav(1, 22050, 16, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));

  // A valid WAV at 22050 Hz should load successfully after resampling to
  // 44100 Hz. The bug causes it to return nullptr instead.
  TEST_CHECK_(sound != nullptr,
      "LoadWav must not return nullptr for a valid WAV at 22050 Hz; "
      "the inverted bounds check on line 294 causes resampling to always fail");
}

// Bug 2: 8-bit stereo WAV reads wrong channel offset.
//
// For 8-bit audio each sample is 1 byte, so the right channel should be
// at in_data + 1 (sizeof(Ui8)). But lines 234-235 and 277-278 use
// sizeof(Ui16) = 2, reading the left channel of the NEXT sample instead.
void test_sound_8bit_stereo_wrong_offset() {
  // 2 stereo samples at 44100 Hz, 8-bit:
  //   sample 0: L=200, R=50
  //   sample 1: L=100, R=150
  // block_align = 2 (1 byte per channel * 2 channels)
  std::vector<Ui8> pcm = {200, 50, 100, 150};
  std::vector<Ui8> wav = build_wav(2, 44100, 8, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));
  if (!TEST_CHECK(sound != nullptr)) {
    return;
  }

  Si16 *out = sound->GetWavData();
  if (!TEST_CHECK(out != nullptr)) {
    return;
  }

  // 8-bit WAV PCM is unsigned. Correct conversion: (byte - 128) * 256.
  // For sample 0:
  //   L = (200 - 128) * 256 = 18432
  //   R = (50  - 128) * 256 = -19968
  //
  // The bug reads the right channel from in_data + sizeof(Ui16) (offset 2)
  // instead of in_data + sizeof(Ui8) (offset 1), so it picks up byte 100
  // (left channel of the next sample) instead of byte 50.
  Si16 left_ch_sample0 = out[0];
  Si16 right_ch_sample0 = out[1];

  Si16 expected_left  = (200 - 128) * 256;  // = 18432
  Si16 expected_right = (50  - 128) * 256;  // = -19968

  TEST_CHECK_(left_ch_sample0 == expected_left,
      "Sample 0 left channel: expected %d, got %d",
      (int)expected_left, (int)left_ch_sample0);
  TEST_CHECK_(right_ch_sample0 == expected_right,
      "Sample 0 right channel: expected %d (from byte 50 at offset 1), "
      "got %d (bug reads from wrong offset)",
      (int)expected_right, (int)right_ch_sample0);
}

// Bug 3: 8-bit WAV samples are treated as signed (Si8) instead of
// unsigned (Ui8).
//
// The WAV spec says 8-bit PCM is unsigned: 0=min, 128=silence, 255=max.
// Correct conversion: (Ui8_value - 128) * 256.
// The code casts to Si8* and multiplies by 256, so:
//   128 (silence) -> Si8(-128) * 256 = -32768 (should be 0)
//   0   (min)     -> Si8(0)    * 256 =  0     (should be -32768)
//   255 (max)     -> Si8(-1)   * 256 = -256   (should be +32512)
void test_sound_8bit_signed_vs_unsigned() {
  // 3 mono samples at 44100 Hz, 8-bit:
  //   sample 0: 128 (silence in unsigned 8-bit WAV)
  //   sample 1: 0   (minimum)
  //   sample 2: 255 (maximum)
  std::vector<Ui8> pcm = {128, 0, 255};
  std::vector<Ui8> wav = build_wav(1, 44100, 8, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));
  if (!TEST_CHECK(sound != nullptr)) {
    return;
  }

  Si16 *out = sound->GetWavData();
  if (!TEST_CHECK(out != nullptr)) {
    return;
  }

  Si16 silence_sample = out[0];  // sample 0, left channel
  Si16 min_sample = out[2];      // sample 1, left channel
  Si16 max_sample = out[4];      // sample 2, left channel

  // Correct values (unsigned interpretation per WAV spec):
  //   (128 - 128) * 256 =  0
  //   (0   - 128) * 256 = -32768
  //   (255 - 128) * 256 =  32512
  TEST_CHECK_(silence_sample == 0,
      "8-bit silence (128) should convert to 0, got %d",
      (int)silence_sample);
  TEST_CHECK_(min_sample == -32768,
      "8-bit minimum (0) should convert to -32768, got %d",
      (int)min_sample);
  TEST_CHECK_(max_sample == 32512,
      "8-bit maximum (255) should convert to 32512, got %d",
      (int)max_sample);
}

// ============================================================================
// Quaternion bug tests
// ============================================================================

// Bug: ToMat33F has sign errors in the w*x terms at positions [5] and [7].
//
// The correct rotation matrix from unit quaternion (x,y,z,w) is:
//   [5] = 2*y*z - 2*w*x
//   [7] = 2*y*z + 2*w*x
// But the code has the signs swapped:
//   [5] = 2*y*z + 2*w*x   (wrong)
//   [7] = 2*y*z - 2*w*x   (wrong)
//
// Test: 90-degree rotation around X should map (0,1,0) to (0,0,1).
// The bug flips the rotation direction, mapping (0,1,0) to (0,0,-1).
void test_quat_to_mat33f_sign() {
  float angle = static_cast<float>(kPi / 2.0);
  Vec3F axis(1.0f, 0.0f, 0.0f);
  QuaternionF q(axis, angle);

  Mat33F mat = q.ToMat33F();
  Vec3F v(0.0f, 1.0f, 0.0f);
  Vec3F result = mat * v;

  // 90-deg rotation around X: (0,1,0) -> (0,0,1)
  TEST_CHECK_(fabsf(result.x) < 0.001f,
      "Quat X-rot: result.x should be ~0, got %f", result.x);
  TEST_CHECK_(fabsf(result.y) < 0.001f,
      "Quat X-rot: result.y should be ~0, got %f", result.y);
  TEST_CHECK_(fabsf(result.z - 1.0f) < 0.001f,
      "Quat X-rot: result.z should be ~1, got %f", result.z);
}

// Same sign bug exists in ToPartialMatrix33F (copy of ToMat33F logic).
void test_quat_to_partial_mat33f_sign() {
  float angle = static_cast<float>(kPi / 2.0);
  Vec3F axis(1.0f, 0.0f, 0.0f);
  QuaternionF q(axis, angle);

  Mat33F mat;
  q.ToPartialMatrix33F(mat);
  Vec3F v(0.0f, 1.0f, 0.0f);
  Vec3F result = mat * v;

  TEST_CHECK_(fabsf(result.x) < 0.001f,
      "Partial mat X-rot: result.x should be ~0, got %f", result.x);
  TEST_CHECK_(fabsf(result.y) < 0.001f,
      "Partial mat X-rot: result.y should be ~0, got %f", result.y);
  TEST_CHECK_(fabsf(result.z - 1.0f) < 0.001f,
      "Partial mat X-rot: result.z should be ~1, got %f", result.z);
}

// Bug: slerp computes normalizedA and normalizedB but then uses the
// original a and b in all subsequent calculations. The normalization is
// dead code.
//
// Test: slerp two non-unit quaternions that represent orthogonal rotations.
// At t=0, the result should be a unit quaternion equivalent to the first
// input rotation. The bug returns an unnormalized quaternion.
void test_quat_slerp_unnormalized() {
  // Identity rotation, scaled by 2. Normalized form: (0,0,0,1).
  QuaternionF a(0.0f, 0.0f, 0.0f, 2.0f);
  // 180-degree rotation around Y, unit length.
  QuaternionF b(0.0f, 1.0f, 0.0f, 0.0f);

  // dot(a,b) = 0*0 + 0*1 + 0*0 + 2*0 = 0, so this takes the slerp path
  // (not the linear fallback). alpha = acos(0) = pi/2.
  //
  // At t=0: result = a*cos(0) + c*sin(0) = a = (0,0,0,2).
  // A correct slerp should return a normalized quaternion ~(0,0,0,1).
  QuaternionF result = slerp(a, b, 0.0f);
  float modulus = result.Modulus();

  TEST_CHECK_(fabsf(modulus - 1.0f) < 0.01f,
      "slerp(t=0) should return a unit quaternion (modulus ~1), got %f. ",
      modulus);
}

// Bug: slerp does not handle negative dot products. When dot(a,b) < 0,
// one quaternion should be negated to interpolate along the shorter arc.
// Without this, slerp between antipodal quaternions (same rotation,
// opposite signs) produces NaN because it tries to normalize a zero vector.
//
// Test: (0,0,0,1) and (0,0,0,-1) represent the same rotation (identity).
// slerp at t=0.5 should produce identity. The bug produces NaN.
void test_quat_slerp_negative_dot() {
  QuaternionF a(0.0f, 0.0f, 0.0f, 1.0f);
  QuaternionF b(0.0f, 0.0f, 0.0f, -1.0f);

  QuaternionF result = slerp(a, b, 0.5f);

  // The result should be a valid quaternion, not NaN.
  bool is_finite = std::isfinite(result.x) && std::isfinite(result.y)
      && std::isfinite(result.z) && std::isfinite(result.w);
  TEST_CHECK_(is_finite,
      "slerp of antipodal quaternions should not produce NaN. "
      "Got (%f, %f, %f, %f).",
      result.x, result.y, result.z, result.w);

  if (is_finite) {
    // Both represent identity, so the interpolation should also be identity.
    float modulus = result.Modulus();
    TEST_CHECK_(fabsf(modulus - 1.0f) < 0.01f,
        "slerp of two identity quaternions should be unit length, got %f",
        modulus);
  }
}

// Bug: Inverse(Transform3F) was computed as Transform3F(-d, R^{-1}), simply
// negating the displacement.  The correct inverse of T(x) = R*x + d is
// T^{-1}(x) = R^{-1}*(x - d), which means the displacement must be
// -R^{-1}*d, not just -d.  The old code only worked when rotation was
// identity.
//
// Test: create a transform with a 90-degree rotation around Z and a non-zero
// displacement, apply it to a point, then apply the inverse.  The result
// should be the original point.
void test_transform3f_inverse() {
  // 90 degrees around Z: q = (0, 0, sin(pi/4), cos(pi/4))
  float s = sinf(3.14159265f / 4.0f);
  float c = cosf(3.14159265f / 4.0f);
  QuaternionF rot(0.0f, 0.0f, s, c);
  Vec3F disp(5.0f, 3.0f, -2.0f);
  Transform3F t(disp, rot);

  Vec3F original(1.0f, 2.0f, 3.0f);
  Vec3F transformed = t.Transform(original);
  Transform3F inv = Inverse(t);
  Vec3F recovered = inv.Transform(transformed);

  float dx = recovered.x - original.x;
  float dy = recovered.y - original.y;
  float dz = recovered.z - original.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "Inverse(t).Transform(t.Transform(p)) should return p. "
      "Original (%.3f, %.3f, %.3f), recovered (%.3f, %.3f, %.3f), error %.6f",
      original.x, original.y, original.z,
      recovered.x, recovered.y, recovered.z, error);
}

// Additional check: composing a transform with its inverse should produce
// identity (no rotation, zero displacement).
void test_transform3f_inverse_composition() {
  float s = sinf(3.14159265f / 3.0f);  // 60 degrees
  float c = cosf(3.14159265f / 3.0f);
  // Rotation around axis (1,1,0)/sqrt(2)
  float norm = 1.0f / sqrtf(2.0f);
  QuaternionF rot(s * norm, s * norm, 0.0f, c);
  Vec3F disp(10.0f, -7.0f, 4.0f);
  Transform3F t(disp, rot);

  Transform3F inv = Inverse(t);
  Transform3F composed = inv.Transform(t);

  // Displacement should be ~zero.
  float disp_len = sqrtf(composed.displacement.x * composed.displacement.x
      + composed.displacement.y * composed.displacement.y
      + composed.displacement.z * composed.displacement.z);
  TEST_CHECK_(disp_len < 0.001f,
      "Inverse(t).Transform(t) displacement should be ~zero, got (%.4f, %.4f, %.4f), "
      "length %.6f",
      composed.displacement.x, composed.displacement.y, composed.displacement.z,
      disp_len);

  // Rotation should be ~identity: (0, 0, 0, +/-1).
  float identity_error = sqrtf(composed.rotation.x * composed.rotation.x
      + composed.rotation.y * composed.rotation.y
      + composed.rotation.z * composed.rotation.z);
  TEST_CHECK_(identity_error < 0.001f,
      "Inverse(t).Transform(t) rotation should be ~identity, "
      "got (%.4f, %.4f, %.4f, %.4f)",
      composed.rotation.x, composed.rotation.y,
      composed.rotation.z, composed.rotation.w);
}

// Bug: piSkeleton::AddBone checks "if (parentID > 0)" instead of
// "if (parentID >= 0)".  This means bone 0 can never be a parent.
// When you add a bone with parentID=0 (intending it to be a child of the
// first bone), the condition 0 > 0 is false, so the new bone replaces
// mRoot.  The original root (bone 0) is orphaned and never traversed
// during Update().
//
// Test: create a 3-bone chain: bone 0 (root) -> bone 1 -> bone 2.
// Set bone 0 to translate by (10, 0, 0) and bone 1 to translate by (0, 5, 0).
// Bone 2's global matrix should include both parents' translations, giving
// (10, 5, 0).  With the bug, bone 0 is orphaned, so bone 2 only gets
// bone 1's translation (0, 5, 0).
void test_skeleton_bone0_cannot_be_parent() {
  piSkeleton skel;
  skel.Init(4);

  int b0 = skel.AddBone(-1);  // root
  int b1 = skel.AddBone(0);   // should be child of bone 0
  int b2 = skel.AddBone(1);   // child of bone 1

  // Bone 0: translate by (10, 0, 0)
  skel.UpdateBone(b0, SetTranslation(10.0f, 0.0f, 0.0f));
  // Bone 1: translate by (0, 5, 0)
  skel.UpdateBone(b1, SetTranslation(0.0f, 5.0f, 0.0f));
  // Bone 2: identity
  skel.UpdateBone(b2, SetIdentity());

  skel.Update();

  // Read back global matrices.
  Mat44F globals[3];
  skel.GetData(globals);

  // Bone 2's global matrix should be the composition of all parents.
  // Correct: translation(10, 5, 0).  Buggy: translation(0, 5, 0).
  Vec3F bone2_translation = ExtractTranslation(globals[2]);

  TEST_CHECK_(fabsf(bone2_translation.x - 10.0f) < 0.001f,
      "Bone 2 global translation X should be 10.0 (from bone 0), got %.3f. "
      "If 0.0, bone 0 was orphaned because parentID=0 failed the > 0 check.",
      bone2_translation.x);
  TEST_CHECK_(fabsf(bone2_translation.y - 5.0f) < 0.001f,
      "Bone 2 global translation Y should be 5.0 (from bone 1), got %.3f.",
      bone2_translation.y);
  TEST_CHECK_(fabsf(bone2_translation.z) < 0.001f,
      "Bone 2 global translation Z should be 0.0, got %.3f.",
      bone2_translation.z);
}

// Verify that bone 0's own global matrix is computed (i.e. bone 0 is
// reachable from mRoot).  With the bug, adding any bone with parentID=0
// replaces mRoot, so bone 0 is never visited by Update() and its
// global matrix remains uninitialized / zero.
void test_skeleton_root_not_orphaned() {
  piSkeleton skel;
  skel.Init(4);

  skel.AddBone(-1);  // bone 0, root
  skel.AddBone(0);   // bone 1, should be child of bone 0

  // Set bone 0 to a known translation.
  skel.UpdateBone(0, SetTranslation(7.0f, 3.0f, 1.0f));
  skel.UpdateBone(1, SetIdentity());

  skel.Update();

  Mat44F globals[2];
  skel.GetData(globals);

  // Bone 0 is root, so its global matrix == its local matrix.
  Vec3F bone0_translation = ExtractTranslation(globals[0]);

  TEST_CHECK_(fabsf(bone0_translation.x - 7.0f) < 0.001f,
      "Bone 0 global translation X should be 7.0, got %.3f. "
      "If wrong, bone 0 was not visited during Update() -- it was orphaned.",
      bone0_translation.x);
  TEST_CHECK_(fabsf(bone0_translation.y - 3.0f) < 0.001f,
      "Bone 0 global translation Y should be 3.0, got %.3f.",
      bone0_translation.y);
  TEST_CHECK_(fabsf(bone0_translation.z - 1.0f) < 0.001f,
      "Bone 0 global translation Z should be 1.0, got %.3f.",
      bone0_translation.z);
}

// ---------------------------------------------------------------------------
// Unicode tests
// ---------------------------------------------------------------------------

// Utf16ToUtf8 includes a spurious null byte at the end of the string.
//
// In the size-counting pass (lines 214-221 of unicode.cpp), the code does:
//   size += cp.size;
//   if (d == 0) { break; }
//
// When the null terminator is read (d == 0), WriteUtf32(0) sets cp.size = 1,
// so size is incremented by 1 before the break.  The string is then resized
// to include this extra byte.  In the second pass the null byte is written
// into the string body.  The result is a std::string whose .size() is one
// more than expected, with a '\0' as part of the content.
//
// Compare with Utf32ToUtf8 which correctly checks "if (d == 0) break;"
// BEFORE adding to size.
void test_utf16_to_utf8_spurious_null() {
  // UTF-16LE for "AB" = { 'A', 0, 'B', 0, 0, 0 }
  const Ui8 utf16le[] = { 0x41, 0x00, 0x42, 0x00, 0x00, 0x00 };

  std::string result = Utf16ToUtf8(utf16le);

  TEST_CHECK_(result.size() == 2,
      "Utf16ToUtf8(\"AB\") should return a string of size 2, got %zu. "
      "A spurious null byte is included in the string.",
      result.size());
  TEST_CHECK(result == "AB");
}

// Utf32ToUtf8 basic correctness -- ASCII.
void test_utf32_to_utf8_ascii() {
  // UTF-32LE for "Hi" = { 'H', 0,0,0, 'i', 0,0,0, 0,0,0,0 }
  const Ui32 utf32[] = { 0x48, 0x69, 0x00 };

  std::string result = Utf32ToUtf8(utf32);

  TEST_CHECK_(result.size() == 2,
      "Utf32ToUtf8(\"Hi\") should be size 2, got %zu", result.size());
  TEST_CHECK(result == "Hi");
}

// Utf32ToUtf8 multi-byte codepoints.
void test_utf32_to_utf8_multibyte() {
  // U+00E9 = e-acute (2-byte UTF-8: 0xC3 0xA9)
  // U+4E16 = Chinese "world" (3-byte UTF-8: 0xE4 0xB8 0x96)
  // U+1F600 = grinning face emoji (4-byte UTF-8: 0xF0 0x9F 0x98 0x80)
  const Ui32 utf32[] = { 0x00E9, 0x4E16, 0x1F600, 0x00 };

  std::string result = Utf32ToUtf8(utf32);

  // Expected: 2 + 3 + 4 = 9 bytes
  TEST_CHECK_(result.size() == 9,
      "Expected 9 bytes, got %zu", result.size());

  const Ui8 *r = reinterpret_cast<const Ui8 *>(result.data());
  // e-acute
  TEST_CHECK(r[0] == 0xC3 && r[1] == 0xA9);
  // U+4E16
  TEST_CHECK(r[2] == 0xE4 && r[3] == 0xB8 && r[4] == 0x96);
  // U+1F600
  TEST_CHECK(r[5] == 0xF0 && r[6] == 0x9F && r[7] == 0x98 && r[8] == 0x80);
}

// Utf16ToUtf8 multi-byte: BMP codepoint (2-byte UTF-16, multi-byte UTF-8).
void test_utf16_to_utf8_bmp() {
  // U+00E9 (e-acute): UTF-16LE = { 0xE9, 0x00 }, UTF-8 = { 0xC3, 0xA9 }
  const Ui8 utf16le[] = { 0xE9, 0x00, 0x00, 0x00 };

  std::string result = Utf16ToUtf8(utf16le);

  TEST_CHECK_(result.size() == 2,
      "Expected 2 bytes for U+00E9 in UTF-8, got %zu", result.size());

  const Ui8 *r = reinterpret_cast<const Ui8 *>(result.data());
  TEST_CHECK(r[0] == 0xC3 && r[1] == 0xA9);
}

// Utf16ToUtf8 surrogate pair (supplementary plane).
void test_utf16_to_utf8_surrogate() {
  // U+1F600: UTF-16LE = { 0x3D, 0xD8, 0x00, 0xDE }, UTF-8 = { 0xF0, 0x9F, 0x98, 0x80 }
  const Ui8 utf16le[] = { 0x3D, 0xD8, 0x00, 0xDE, 0x00, 0x00 };

  std::string result = Utf16ToUtf8(utf16le);

  TEST_CHECK_(result.size() == 4,
      "Expected 4 bytes for U+1F600 in UTF-8, got %zu", result.size());

  const Ui8 *r = reinterpret_cast<const Ui8 *>(result.data());
  TEST_CHECK(r[0] == 0xF0 && r[1] == 0x9F && r[2] == 0x98 && r[3] == 0x80);
}

// Utf32Reader round-trip: encode codepoints as UTF-8, read them back.
void test_utf32_reader_roundtrip() {
  // Hand-encode U+0041 (A), U+00E9 (e-acute), U+4E16, U+1F600 as UTF-8.
  const Ui8 utf8[] = {
    0x41,                         // U+0041
    0xC3, 0xA9,                   // U+00E9
    0xE4, 0xB8, 0x96,             // U+4E16
    0xF0, 0x9F, 0x98, 0x80,       // U+1F600
    0x00                           // null terminator
  };

  Utf32Reader reader;
  reader.Reset(utf8);

  Ui32 c1 = reader.ReadOne();
  TEST_CHECK_(c1 == 0x0041, "Expected U+0041, got U+%04X", c1);

  Ui32 c2 = reader.ReadOne();
  TEST_CHECK_(c2 == 0x00E9, "Expected U+00E9, got U+%04X", c2);

  Ui32 c3 = reader.ReadOne();
  TEST_CHECK_(c3 == 0x4E16, "Expected U+4E16, got U+%04X", c3);

  Ui32 c4 = reader.ReadOne();
  TEST_CHECK_(c4 == 0x1F600, "Expected U+1F600, got U+%04X", c4);

  Ui32 c5 = reader.ReadOne();
  TEST_CHECK_(c5 == 0, "Expected null terminator (0), got U+%04X", c5);
}

// Utf8Codepoint::WriteUtf32 produces correct byte counts.
void test_utf8_codepoint_sizes() {
  Utf8Codepoint cp;

  cp.WriteUtf32(0x41);     // ASCII
  TEST_CHECK(cp.size == 1);

  cp.WriteUtf32(0x00E9);   // 2-byte
  TEST_CHECK(cp.size == 2);

  cp.WriteUtf32(0x4E16);   // 3-byte
  TEST_CHECK(cp.size == 3);

  cp.WriteUtf32(0x1F600);  // 4-byte
  TEST_CHECK(cp.size == 4);

  cp.WriteUtf32(0x110000); // out of range
  TEST_CHECK(cp.size == 0);
}

void test_is_utf8_continuation() {
  for (Ui32 b = 0x00; b <= 0x7F; ++b) {
    TEST_CHECK_(!IsUtf8Continuation(static_cast<Ui8>(b)),
      "ASCII byte 0x%02X should not be continuation", b);
  }
  for (Ui32 b = 0x80; b <= 0xBF; ++b) {
    TEST_CHECK_(IsUtf8Continuation(static_cast<Ui8>(b)),
      "Byte 0x%02X should be continuation", b);
  }
  for (Ui32 b = 0xC0; b <= 0xFF; ++b) {
    TEST_CHECK_(!IsUtf8Continuation(static_cast<Ui8>(b)),
      "Leading byte 0x%02X should not be continuation", b);
  }
}

void test_utf8_next_char_pos() {
  // "Aй\xe2\x98\x83\xf0\x9f\x98\x80" = A(1) + й(2) + snowman(3) + grinning(4)
  std::string s = "A\xD0\xB9\xE2\x98\x83\xF0\x9F\x98\x80";
  TEST_CHECK_(s.length() == 10, "test string should be 10 bytes, got %d",
    (int)s.length());

  Si32 p = 0;
  p = Utf8NextCharPos(s, p);
  TEST_CHECK_(p == 1, "after A: expected 1, got %d", p);

  p = Utf8NextCharPos(s, p);
  TEST_CHECK_(p == 3, "after й: expected 3, got %d", p);

  p = Utf8NextCharPos(s, p);
  TEST_CHECK_(p == 6, "after snowman: expected 6, got %d", p);

  p = Utf8NextCharPos(s, p);
  TEST_CHECK_(p == 10, "after grinning: expected 10, got %d", p);

  p = Utf8NextCharPos(s, p);
  TEST_CHECK_(p == 10, "past end: expected 10, got %d", p);

  std::string empty;
  TEST_CHECK_(Utf8NextCharPos(empty, 0) == 0,
    "empty string: expected 0, got %d", Utf8NextCharPos(empty, 0));
}

void test_utf8_prev_char_pos() {
  std::string s = "A\xD0\xB9\xE2\x98\x83\xF0\x9F\x98\x80";

  Si32 p = 10;
  p = Utf8PrevCharPos(s, p);
  TEST_CHECK_(p == 6, "before grinning: expected 6, got %d", p);

  p = Utf8PrevCharPos(s, p);
  TEST_CHECK_(p == 3, "before snowman: expected 3, got %d", p);

  p = Utf8PrevCharPos(s, p);
  TEST_CHECK_(p == 1, "before й: expected 1, got %d", p);

  p = Utf8PrevCharPos(s, p);
  TEST_CHECK_(p == 0, "before A: expected 0, got %d", p);

  p = Utf8PrevCharPos(s, p);
  TEST_CHECK_(p == 0, "at start: expected 0, got %d", p);

  TEST_CHECK_(Utf8PrevCharPos(s, 8) == 6,
    "mid-continuation of 4-byte char: expected 6, got %d",
    Utf8PrevCharPos(s, 8));

  TEST_CHECK_(Utf8PrevCharPos(s, 2) == 1,
    "mid-continuation of 2-byte char: expected 1, got %d",
    Utf8PrevCharPos(s, 2));

  std::string empty;
  TEST_CHECK_(Utf8PrevCharPos(empty, 0) == 0,
    "empty string: expected 0, got %d", Utf8PrevCharPos(empty, 0));
}

// ---------------------------------------------------------------------------
// Mesh bug regression tests
// ---------------------------------------------------------------------------

// Reproduce the readline() bug from mesh_ply.cpp: the while-loop that strips
// trailing CR/LF never decrements 'l', so only the last character is removed.
// On files with \r\n line endings, \r survives and all strcmp() comparisons
// in the PLY parser fail.
static int ply_readline_fixed(char *str, int max, const char *src) {
  strncpy(str, src, (size_t)max);
  str[max - 1] = 0;
  size_t l = strlen(str);
  while (l > 0 && (str[l - 1] == 10 || str[l - 1] == 13)) {
    str[l - 1] = 0;
    l--;
  }
  return 1;
}

void test_mesh_ply_readline_crlf(void) {
  char buf[256];

  // Unix line ending: should strip \n, result is "ply"
  ply_readline_fixed(buf, 256, "ply\n");
  TEST_CHECK(strcmp(buf, "ply") == 0);
  TEST_MSG("Unix ending: got \"%s\"", buf);

  // Windows line ending: should strip \r\n, result should be "ply"
  ply_readline_fixed(buf, 256, "ply\r\n");
  TEST_CHECK(strcmp(buf, "ply") == 0);
  TEST_MSG("Windows ending: got \"%s\" (len=%zu), expected \"ply\" (len=3)",
           buf, strlen(buf));
}

// Reproduce the mesh_ply.cpp bug: vertex format declares element 1 as 2 floats,
// but 3 floats are written, overwriting vertex (i+1)'s position data.
void test_mesh_vertex_attrib_write_overflow(void) {
  Mesh mesh;

  // Same format as mesh_ply.cpp:
  // element 0: 3 floats (position), element 1: 3 floats (normal)
  // stride = 6 * sizeof(float) = 24 bytes
  const MeshVertexFormat vf = {6 * (int)sizeof(float), 2, 0,
    {{3, kRMVEDT_Float, false}, {3, kRMVEDT_Float, false}}};

  bool ok = mesh.Init(1, 4, &vf, kRMVEDT_Polys, 1, 1);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // Write known position values for vertex 0 and vertex 1
  float *v0 = (float *)mesh.GetVertexData(0, 0, 0);  // vertex 0, element 0 (pos)
  float *v1 = (float *)mesh.GetVertexData(0, 1, 0);  // vertex 1, element 0 (pos)
  v0[0] = 1.0f;  v0[1] = 2.0f;  v0[2] = 3.0f;
  v1[0] = 10.0f; v1[1] = 20.0f; v1[2] = 30.0f;

  // Now write to element 1 of vertex 0 (texcoord, 2 floats)
  float *nt = (float *)mesh.GetVertexData(0, 0, 1);  // vertex 0, element 1

  // Correct: write only 2 floats (within the 2-float attribute)
  nt[0] = 100.0f;
  nt[1] = 200.0f;

  // Verify vertex 1 position is intact after writing 2 floats
  TEST_CHECK(v1[0] == 10.0f);
  TEST_MSG("After 2-float write: v1[0] = %f, expected 10.0", (double)v1[0]);

  // Reproduce mesh_ply.cpp pattern: write 3 floats into a 2-float attribute.
  // The vertex format must have room for 3 floats in element 1 for this to
  // be safe.  If it only has 2, nt[2] overwrites the next vertex.
  nt[0] = 100.0f;
  nt[1] = 200.0f;
  nt[2] = 300.0f;  // Would overwrite v1[0] if element 1 is only 2 floats

  TEST_CHECK(v1[0] == 10.0f);
  TEST_MSG("After 3-float write into element 1: v1[0] = %f, expected 10.0",
           (double)v1[0]);
}

// Test Mesh_ExtrudeFace: extrusion of a single triangle should create 3 side
// quads (6 triangles) connecting original and extruded vertices.  With the
// i+=2 bug, only edges 0 and 2 are processed, edge 1 is skipped.
void test_mesh_extrude_face_covers_all_edges(void) {
  Mesh mesh;

  // Simple format: 3 floats per vertex (position only)
  const MeshVertexFormat vf = {3 * (int)sizeof(float), 1, 0,
    {{3, kRMVEDT_Float, false}}};

  // Allocate room for 6 verts (3 original + 3 extruded) and
  // 7 faces (1 original + 6 side faces = 3 quads * 2 triangles each).
  // We over-allocate so we can check what Mesh_ExtrudeFace actually writes.
  bool ok = mesh.Init(1, 128, &vf, kRMVEDT_Polys, 1, 128);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // Set up a single triangle: vertices 0, 1, 2
  float *v0 = (float *)mesh.GetVertexData(0, 0, 0);
  float *v1 = (float *)mesh.GetVertexData(0, 1, 0);
  float *v2 = (float *)mesh.GetVertexData(0, 2, 0);
  v0[0] = 0.0f; v0[1] = 0.0f; v0[2] = 0.0f;
  v1[0] = 1.0f; v1[1] = 0.0f; v1[2] = 0.0f;
  v2[0] = 0.0f; v2[1] = 1.0f; v2[2] = 0.0f;

  mesh.mVertexData.mVertexArray[0].mNum = 3;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[0] = 0;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[1] = 1;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[2] = 2;
  mesh.mFaceData.mIndexArray[0].mNum = 1;

  // Extrude face 0
  ok = Mesh_ExtrudeFace(&mesh, 0);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // After extrusion we expect:
  //   - 3 new vertices (indices 3, 4, 5)
  //   - 3 new side faces (one per edge of the original triangle)
  //   - The original face now references the new vertices (3, 4, 5)
  TEST_CHECK(mesh.mVertexData.mVertexArray[0].mNum == 6);
  TEST_MSG("Expected 6 vertices, got %u", mesh.mVertexData.mVertexArray[0].mNum);

  // The extrude function adds num=3 to mNum, making total = 1 + 3 = 4.
  // But a correct extrusion of a triangle needs 2*3=6 side faces,
  // so total should be 1 + 6 = 7.
  // With the i+=2 bug, only 4 faces are written but mNum is set to 4.
  Si32 face_count = (Si32)mesh.mFaceData.mIndexArray[0].mNum;
  TEST_MSG("Face count after extrude: %d", face_count);

  // Verify that every original edge (0-1, 1-2, 2-0) is covered by side faces.
  // Original vertex indices are 0, 1, 2; new copies are 3, 4, 5.
  // For edge between original vertex i and i+1 (mod 3), we expect two side
  // triangles connecting (orig_i, orig_j, new_j) and (new_j, new_i, orig_i).
  //
  // Collect all edges from the side faces (faces 1..face_count-1) and check
  // that edges (0,1), (1,2), (2,0) all appear in some side face.
  bool edge01_found = false;
  bool edge12_found = false;
  bool edge20_found = false;

  for (Si32 fi = 1; fi < face_count; ++fi) {
    MeshFace *f = &mesh.mFaceData.mIndexArray[0].mBuffer[fi];
    for (int e = 0; e < 3; ++e) {
      int a = f->mIndex[e];
      int b = f->mIndex[(e + 1) % 3];
      // Check if this edge matches any original edge (in either direction)
      if ((a == 0 && b == 1) || (a == 1 && b == 0)) {
        edge01_found = true;
      }
      if ((a == 1 && b == 2) || (a == 2 && b == 1)) {
        edge12_found = true;
      }
      if ((a == 2 && b == 0) || (a == 0 && b == 2)) {
        edge20_found = true;
      }
    }
  }

  TEST_CHECK(edge01_found);
  TEST_MSG("Edge 0-1 covered by side faces: %s", edge01_found ? "yes" : "NO (BUG: i+=2 skips edge 1)");
  TEST_CHECK(edge12_found);
  TEST_MSG("Edge 1-2 covered by side faces: %s", edge12_found ? "yes" : "NO (BUG: i+=2 skips edge 1)");
  TEST_CHECK(edge20_found);
  TEST_MSG("Edge 2-0 covered by side faces: %s", edge20_found ? "yes" : "NO");
}


// A named element remembers the attribute name for Mesh::Draw, while an unnamed
// one keeps the old positional binding, and both count towards the stride.
void test_mesh_named_elements(void) {
  MeshVertexFormat format;
  int position = format.AddElement("vPosition", 3, kRMVEDT_Float);
  int normal = format.AddElement("vNormal", 3, kRMVEDT_Float);
  int color = format.AddElement(4, kRMVEDT_UByte, true);

  TEST_CHECK_(position == 0 && normal == 1 && color == 2,
      "elements got indices %d, %d, %d instead of 0, 1, 2",
      position, normal, color);
  TEST_CHECK_(format.mElems[position].mName != nullptr
      && strcmp(format.mElems[position].mName, "vPosition") == 0,
      "the name of element 0 was lost");
  TEST_CHECK_(format.mElems[normal].mName != nullptr
      && strcmp(format.mElems[normal].mName, "vNormal") == 0,
      "the name of element 1 was lost");
  TEST_CHECK_(format.mElems[color].mName == nullptr,
      "an unnamed element got a name out of nowhere");
  TEST_CHECK_(format.mElems[normal].mOffset == 3 * sizeof(float),
      "element 1 sits at offset %u instead of %u",
      format.mElems[normal].mOffset,
      (unsigned int)(3 * sizeof(float)));
  TEST_CHECK_(format.mStride == (int)(6 * sizeof(float) + 4),
      "the stride is %d instead of %d", format.mStride,
      (int)(6 * sizeof(float) + 4));
}

// Init fixes the capacity: AddVertex and AddFace refuse to write past it and
// say so with -1 instead of growing or overflowing the buffer.
void test_mesh_capacity_is_final(void) {
  MeshVertexFormat format;
  format.AddElement("vPosition", 3, kRMVEDT_Float);

  Mesh mesh;
  TEST_CHECK(mesh.Init(1, 3, &format, kRMVEDT_Polys, 1, 1));

  for (int i = 0; i < 3; ++i) {
    int id = mesh.AddVertex(0, 0.0f, 0.0f, (float)i);
    TEST_CHECK_(id == i, "vertex %d got index %d", i, id);
  }
  TEST_CHECK_(mesh.AddFace(0, 0, 1, 2) == 0, "the only face was refused");

  TEST_CHECK_(mesh.AddVertex(0, 1.0f, 1.0f, 1.0f) == -1,
      "a vertex past the capacity was accepted");
  TEST_CHECK_(mesh.AddFace(0, 0, 1, 2) == -1,
      "a face past the capacity was accepted");
  TEST_CHECK_(mesh.GetCurrentVertexCount(0) == 3,
      "the vertex count moved to %d", mesh.GetCurrentVertexCount(0));
  TEST_CHECK_(mesh.GetCurrentFaceCount(0) == 1,
      "the face count moved to %d", mesh.GetCurrentFaceCount(0));

  // Expand is the way to get more room, and it has to serve every stream and
  // every index array, not only the first one.
  TEST_CHECK(mesh.Expand(64, 64));
  TEST_CHECK_(mesh.AddVertex(0, 1.0f, 1.0f, 1.0f) == 3,
      "the vertex after Expand was still refused");
  TEST_CHECK_(mesh.AddFace(0, 1, 2, 3) == 1,
      "the face after Expand was still refused");
}

void test_mesh_expand_every_stream(void) {
  MeshVertexFormat format[2];
  format[0].AddElement("vPosition", 3, kRMVEDT_Float);
  format[1].AddElement("vTexCoord", 2, kRMVEDT_Float);

  Mesh mesh;
  TEST_CHECK(mesh.Init(2, 4, format, kRMVEDT_Polys, 2, 2));

  const unsigned int vertex_max_before = mesh.mVertexData.mVertexArray[1].mMax;
  const unsigned int face_max_before = mesh.mFaceData.mIndexArray[1].mMax;
  TEST_CHECK(mesh.Expand(64, 64));
  TEST_CHECK_(mesh.mVertexData.mVertexArray[1].mMax > vertex_max_before,
      "the second vertex stream stayed at %u vertices", vertex_max_before);
  TEST_CHECK_(mesh.mFaceData.mIndexArray[1].mMax > face_max_before,
      "the second index array stayed at %u faces", face_max_before);
}

// The same seed has to give the same numbers, and a different one different
// numbers, or a level generator cannot be reproduced.
void test_random_seed_determinism(void) {
  const Ui64 seed = 20250823ull;
  std::vector<Ui64> first;
  SetRandomSeed(seed);
  for (int i = 0; i < 16; ++i) {
    first.push_back(Random64());
    first.push_back((Ui64)Random32());
    first.push_back((Ui64)Random16());
    first.push_back((Ui64)Random8());
    first.push_back((Ui64)Random(0, 1000000));
  }

  std::vector<Ui64> second;
  SetRandomSeed(seed);
  for (int i = 0; i < 16; ++i) {
    second.push_back(Random64());
    second.push_back((Ui64)Random32());
    second.push_back((Ui64)Random16());
    second.push_back((Ui64)Random8());
    second.push_back((Ui64)Random(0, 1000000));
  }
  TEST_CHECK_(first == second,
      "the same seed gave a different sequence the second time");

  SetRandomSeed(seed + 1ull);
  std::vector<Ui64> other;
  for (size_t i = 0; i < first.size(); ++i) {
    other.push_back(Random64());
  }
  TEST_CHECK_(first != other, "a different seed gave the very same sequence");
}

// Numbers drawn after a state is put back have to repeat the ones drawn after it
// was taken, and every width has to be covered: a state that carries only one of
// the four generators looks right until the first Random8.
static std::vector<Ui64> DrawEveryWidth(int count) {
  std::vector<Ui64> drawn;
  for (int i = 0; i < count; ++i) {
    drawn.push_back(Random64());
    drawn.push_back((Ui64)Random32());
    drawn.push_back((Ui64)Random16());
    drawn.push_back((Ui64)Random8());
    drawn.push_back((Ui64)Random(0, 1000000));
  }
  return drawn;
}

void test_random_state_continues_the_sequence(void) {
  SetRandomSeed(777ull);
  DrawEveryWidth(3);

  RandomState state = GetRandomState();
  std::vector<Ui64> first = DrawEveryWidth(8);
  SetRandomState(state);
  std::vector<Ui64> again = DrawEveryWidth(8);
  TEST_CHECK_(first == again,
      "the restored state gave a different sequence");

  // A state is a value, so the copy taken above still holds while the sequence
  // moves on, and it can be put back a second time.
  DrawEveryWidth(5);
  SetRandomState(state);
  std::vector<Ui64> third = DrawEveryWidth(8);
  TEST_CHECK_(first == third,
      "the state stopped working after the sequence moved on");

  // The state has to be a snapshot, not a reset: continuing from it differs from
  // seeding the same seed again.
  SetRandomSeed(777ull);
  std::vector<Ui64> from_seed = DrawEveryWidth(8);
  TEST_CHECK_(first != from_seed,
      "the state behaves as a fresh seed instead of a snapshot");
}

void test_random_state_text_round_trip(void) {
  SetRandomSeed(31415ull);
  DrawEveryWidth(2);

  const std::string text = GetRandomState().ToString();
  std::vector<Ui64> first = DrawEveryWidth(6);

  RandomState restored;
  TEST_CHECK_(restored.FromString(text), "a state written by ToString failed to read");
  SetRandomState(restored);
  std::vector<Ui64> again = DrawEveryWidth(6);
  TEST_CHECK_(first == again,
      "the state read from text gave a different sequence");

  // Garbage is refused and the state it was read into is left as it was.
  RandomState keeper = GetRandomState();
  const std::string kept = keeper.ToString();
  TEST_CHECK_(!keeper.FromString("not a state at all"),
      "FromString accepted a text that is not a state");
  TEST_CHECK_(keeper.ToString() == kept,
      "a refused FromString changed the state anyway");

  // A truncated text is refused as well: three generators out of four are not a
  // state, and taking them would move the sequence somewhere unpredictable.
  const size_t half = text.size() / 2;
  TEST_CHECK_(!keeper.FromString(text.substr(0, half)),
      "FromString accepted a truncated state");
  TEST_CHECK_(keeper.ToString() == kept,
      "a refused truncated FromString changed the state anyway");
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

// ---------------------------------------------------------------------------
// Mat44F rotation consistency tests
// ---------------------------------------------------------------------------

// SetRotationX/Y should produce the same rotation as SetRotationAxisAngle4
// with the corresponding unit axis. SetRotationZ is already consistent.
//
// The bug: SetRotationX(t) and SetRotationY(t) rotate in the opposite
// direction compared to SetRotationAxisAngle4(axis, t). For example,
// SetRotationAxisAngle4(Vec3F(1,0,0), pi/4) applied to (0,1,0) gives
// (0, cos, sin) ~ (0, 0.707, 0.707), but SetRotationX(pi/4) applied to
// (0,1,0) gives (0, cos, -sin) ~ (0, 0.707, -0.707). The sign of sin is
// flipped in m[6] and m[9] for X, and similarly for Y.
void test_rotation_x_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(0.0f, 1.0f, 0.0f);

  Mat44F mat_x = SetRotationX(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(1.0f, 0.0f, 0.0f), angle);

  Vec3F result_x = Transform(mat_x, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_x.x - result_aa.x;
  float dy = result_x.y - result_aa.y;
  float dz = result_x.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationX(pi/4) and SetRotationAxisAngle4(X, pi/4) should produce "
      "the same result for point (0,1,0). "
      "SetRotationX -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_x.x, result_x.y, result_x.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

void test_rotation_y_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(0.0f, 0.0f, 1.0f);

  Mat44F mat_y = SetRotationY(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(0.0f, 1.0f, 0.0f), angle);

  Vec3F result_y = Transform(mat_y, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_y.x - result_aa.x;
  float dy = result_y.y - result_aa.y;
  float dz = result_y.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationY(pi/4) and SetRotationAxisAngle4(Y, pi/4) should produce "
      "the same result for point (0,0,1). "
      "SetRotationY -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_y.x, result_y.y, result_y.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

// SetRotationZ is already consistent with SetRotationAxisAngle4.
// This test serves as a control to confirm the test methodology is correct.
void test_rotation_z_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(1.0f, 0.0f, 0.0f);

  Mat44F mat_z = SetRotationZ(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(0.0f, 0.0f, 1.0f), angle);

  Vec3F result_z = Transform(mat_z, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_z.x - result_aa.x;
  float dy = result_z.y - result_aa.y;
  float dz = result_z.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationZ(pi/4) and SetRotationAxisAngle4(Z, pi/4) should produce "
      "the same result for point (1,0,0). "
      "SetRotationZ -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_z.x, result_z.y, result_z.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

// Composing SetRotationX and SetRotationY should be equivalent to composing
// the corresponding SetRotationAxisAngle4 calls in the same order.
// This test catches the inconsistency in a more realistic usage scenario.
void test_rotation_xy_composition_vs_axis_angle() {
  const float angle_x = 3.14159265f / 6.0f;  // 30 degrees
  const float angle_y = 3.14159265f / 3.0f;  // 60 degrees
  Vec3F point(1.0f, 2.0f, 3.0f);

  Mat44F composed_xyz = SetRotationX(angle_x) * SetRotationY(angle_y);
  Mat44F composed_aa = SetRotationAxisAngle4(Vec3F(1.0f, 0.0f, 0.0f), angle_x)
                     * SetRotationAxisAngle4(Vec3F(0.0f, 1.0f, 0.0f), angle_y);

  Vec3F result_xyz = Transform(composed_xyz, point);
  Vec3F result_aa = Transform(composed_aa, point);

  float dx = result_xyz.x - result_aa.x;
  float dy = result_xyz.y - result_aa.y;
  float dz = result_xyz.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "Composing SetRotationX * SetRotationY should match composing "
      "AxisAngle4(X) * AxisAngle4(Y). "
      "XY -> (%.4f, %.4f, %.4f), AA -> (%.4f, %.4f, %.4f), error = %.6f",
      result_xyz.x, result_xyz.y, result_xyz.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
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

void test_quat_to_axis_angle_acos_out_of_range() {
  // QuaternionF::ToAxisAngle calls acos(w) without clamping w to [-1, 1].
  // For unnormalized quaternions, or normalized ones that have drifted
  // past 1.0 due to floating-point accumulation, |w| > 1 causes acos()
  // to return NaN.

  // Construct a quaternion with w slightly above 1.0, simulating
  // floating-point drift after many rotations.
  QuaternionF q(0.0f, 0.0f, 0.0001f, 1.00001f);

  Vec3F axis;
  float angle;
  q.ToAxisAngle(axis, angle);

  // acos(1.00001) is undefined and returns NaN on IEEE 754 systems.
  // A robust implementation should clamp w to [-1, 1] and return a
  // valid angle.
  bool angle_is_nan = (angle != angle);  // NaN != NaN is true

  TEST_CHECK_(!angle_is_nan,
      "ToAxisAngle returned NaN angle for w=1.00001 (slightly above 1.0). "
      "acos(w) is undefined for |w| > 1; w should be clamped to [-1, 1].");

  // Also test w slightly below -1.0
  QuaternionF q2(0.0f, 0.0f, 0.0001f, -1.00001f);
  q2.ToAxisAngle(axis, angle);

  angle_is_nan = (angle != angle);
  TEST_CHECK_(!angle_is_nan,
      "ToAxisAngle returned NaN angle for w=-1.00001 (slightly below -1.0). "
      "acos(w) is undefined for |w| > 1; w should be clamped to [-1, 1].");
}

void test_quat_to_axis_angle_normalized_roundtrip() {
  // A properly normalized quaternion should round-trip through
  // ToAxisAngle without issues. This serves as a control test.
  Vec3F original_axis = Normalize(Vec3F(1.0f, 2.0f, 3.0f));
  float original_angle = 1.23f;
  QuaternionF q(original_axis, original_angle);

  Vec3F axis;
  float angle;
  q.ToAxisAngle(axis, angle);

  float angle_error = fabsf(angle - original_angle);
  float axis_error = sqrtf(
      (axis.x - original_axis.x) * (axis.x - original_axis.x) +
      (axis.y - original_axis.y) * (axis.y - original_axis.y) +
      (axis.z - original_axis.z) * (axis.z - original_axis.z));

  TEST_CHECK_(angle_error < 0.001f,
      "ToAxisAngle angle round-trip error = %.6f, expected < 0.001",
      angle_error);
  TEST_CHECK_(axis_error < 0.001f,
      "ToAxisAngle axis round-trip error = %.6f, expected < 0.001",
      axis_error);
}

void test_transform3f_scale_affects_point() {
  // Transform3F has a public `scale` member (default 1.0), but
  // Transform(Vec3F) ignores it entirely. Setting scale to 2.0
  // should double the point before applying rotation and displacement.

  Transform3F t;
  t.displacement = Vec3F(0.f, 0.f, 0.f);
  t.rotation.Clear();  // identity rotation
  t.scale = 2.0f;

  Vec3F input(1.f, 0.f, 0.f);
  Vec3F result = t.Transform(input);

  // With identity rotation, zero displacement, and scale=2:
  //   expected = rotate(input * scale) + displacement
  //            = (2, 0, 0) + (0, 0, 0) = (2, 0, 0)
  // But the bug makes Transform ignore scale, producing (1, 0, 0).

  TEST_CHECK_(fabsf(result.x - 2.0f) < 0.001f,
      "Transform(Vec3F) with scale=2 should produce x=2.0, got x=%.4f "
      "(scale member is ignored)",
      result.x);
}

void test_transform3f_scale_affects_transform_composition() {
  // When composing two transforms via Transform(Transform3F),
  // the parent's scale should affect the child's displacement.

  Transform3F parent;
  parent.displacement = Vec3F(0.f, 0.f, 0.f);
  parent.rotation.Clear();  // identity
  parent.scale = 3.0f;

  Transform3F child;
  child.displacement = Vec3F(1.f, 0.f, 0.f);
  child.rotation.Clear();  // identity
  child.scale = 1.0f;

  Transform3F composed = parent.Transform(child);

  // With identity parent rotation and parent scale=3:
  //   composed.displacement = parent.rotation.Rotate(child.displacement * parent.scale) + parent.displacement
  //                         = (3, 0, 0) + (0, 0, 0) = (3, 0, 0)
  //   composed.scale = parent.scale * child.scale = 3.0
  // But the bug makes Transform ignore scale, giving displacement=(1,0,0)
  // and not propagating scale at all.

  TEST_CHECK_(fabsf(composed.displacement.x - 3.0f) < 0.001f,
      "Parent scale=3 should scale child displacement to x=3.0, got x=%.4f "
      "(scale member is ignored in Transform(Transform3F))",
      composed.displacement.x);

  TEST_CHECK_(fabsf(composed.scale - 3.0f) < 0.001f,
      "Composed scale should be parent*child = 3.0, got %.4f "
      "(scale is not propagated in Transform(Transform3F))",
      composed.scale);
}

void test_transform3f_inverse_respects_scale() {
  // Inverse() also ignores scale. Applying a transform and then its
  // inverse should return to the original point, but this fails when
  // scale != 1.0.

  Transform3F t;
  t.displacement = Vec3F(1.f, 2.f, 3.f);
  t.rotation = QuaternionF(Normalize(Vec3F(0.f, 1.f, 0.f)), 0.5f);
  t.scale = 2.0f;

  Transform3F inv = Inverse(t);

  Vec3F original(4.f, 5.f, 6.f);
  Vec3F transformed = t.Transform(original);
  Vec3F roundtrip = inv.Transform(transformed);

  float dx = roundtrip.x - original.x;
  float dy = roundtrip.y - original.y;
  float dz = roundtrip.z - original.z;
  float error = sqrtf(dx*dx + dy*dy + dz*dz);

  TEST_CHECK_(error < 0.01f,
      "Transform then Inverse should round-trip with scale=2.0, "
      "but error=%.4f (Inverse ignores scale)",
      error);
}

// ============================================================
// Matrix / rotation consistency tests
// ============================================================

static float mat44_max_diff(const Mat44F &a, const Mat44F &b) {
  float d = 0.f;
  for (int i = 0; i < 16; ++i) {
    float ad = fabsf(a.m[i] - b.m[i]);
    if (ad > d) {
      d = ad;
    }
  }
  return d;
}

static Mat44F mat44_mul(const Mat44F &a, const Mat44F &b) {
  return a * b;
}

// 1. SetRotationQuaternion agrees with SetRotationAxisAngle4
void test_quat_matrix_vs_axis_angle() {
  Vec3F axis = Normalize(Vec3F(1.f, 2.f, 3.f));
  float angle = 0.73f;
  Mat44F from_aa = SetRotationAxisAngle4(axis, angle);
  Vec4F q(sinf(angle / 2) * axis.x,
          sinf(angle / 2) * axis.y,
          sinf(angle / 2) * axis.z,
          cosf(angle / 2));
  Mat44F from_q = SetRotationQuaternion(q);
  float d = mat44_max_diff(from_aa, from_q);
  TEST_CHECK_(d < 1e-5f,
      "SetRotationQuaternion vs SetRotationAxisAngle4: max diff=%.7f "
      "(should be consistent)", d);
}

// 2. SetRotationX/Y/Z agree with SetRotationAxisAngle4
void test_rotation_xyz_vs_axis_angle_consistency() {
  float t = 1.1f;
  float dx = mat44_max_diff(SetRotationX(t),
      SetRotationAxisAngle4(Vec3F(1, 0, 0), t));
  float dy = mat44_max_diff(SetRotationY(t),
      SetRotationAxisAngle4(Vec3F(0, 1, 0), t));
  float dz = mat44_max_diff(SetRotationZ(t),
      SetRotationAxisAngle4(Vec3F(0, 0, 1), t));
  TEST_CHECK_(dx < 1e-5f && dy < 1e-5f && dz < 1e-5f,
      "Rx/Ry/Rz vs AxisAngle: dx=%.7f dy=%.7f dz=%.7f", dx, dy, dz);
}

// 3. SetRotationEuler4 must equal Rz * Ry * Rx (same convention as all
//    other rotation-building functions in the engine).
void test_euler4_equals_composition() {
  float x = 0.3f, y = 0.5f, z = 0.7f;
  Mat44F euler = SetRotationEuler4(Vec3F(x, y, z));
  Mat44F composed = mat44_mul(SetRotationZ(z),
                   mat44_mul(SetRotationY(y), SetRotationX(x)));
  float d = mat44_max_diff(euler, composed);
  TEST_CHECK_(d < 1e-5f,
      "SetRotationEuler4(%.1f,%.1f,%.1f) must equal Rz*Ry*Rx, "
      "max diff=%.7f", x, y, z, d);
}

// 4. QuaternionF::ToMat33F matches top-left 3x3 of SetRotationQuaternion
void test_quat_tomat33_vs_setrotationquat() {
  Vec3F axis = Normalize(Vec3F(-1.f, 0.5f, 2.f));
  float angle = 1.2f;
  QuaternionF q(axis, angle);
  Mat33F m3 = q.ToMat33F();
  Vec4F q4(q.x, q.y, q.z, q.w);
  Mat44F m4 = SetRotationQuaternion(q4);
  float d = 0.f;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      float ad = fabsf(m3.m[r * 3 + c] - m4.m[r * 4 + c]);
      if (ad > d) {
        d = ad;
      }
    }
  }
  TEST_CHECK_(d < 1e-5f,
      "ToMat33F vs SetRotationQuaternion(4x4): max diff=%.7f", d);
}

// 5. ExtractRotationEuler must round-trip with SetRotationEuler4:
//    ExtractRotationEuler(SetRotationEuler4(x,y,z)) == (x,y,z).
void test_extract_euler_roundtrip_euler4() {
  float x = 0.4f, y = 0.3f, z = 0.6f;
  Mat44F m = SetRotationEuler4(Vec3F(x, y, z));
  Vec3F e = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e.x - x) < 1e-4f &&
              fabsf(e.y - y) < 1e-4f &&
              fabsf(e.z - z) < 1e-4f,
      "ExtractRotationEuler(SetRotationEuler4(%.1f,%.1f,%.1f)) = "
      "(%.4f,%.4f,%.4f) -- must recover original angles",
      x, y, z, e.x, e.y, e.z);
}

// 6. ExtractRotationEuler for a pure X-axis rotation must return (angle,0,0).
void test_extract_euler_pure_x() {
  float angle = 0.6f;
  Mat44F rx = SetRotationX(angle);
  Vec3F e = ExtractRotationEuler(rx);
  TEST_CHECK_(fabsf(e.x - angle) < 1e-4f &&
              fabsf(e.y) < 1e-4f &&
              fabsf(e.z) < 1e-4f,
      "ExtractRotationEuler(SetRotationX(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (%.1f, 0, 0)",
      angle, e.x, e.y, e.z, angle);
}

// 6b. ExtractRotationEuler for a pure Y-axis rotation must return (0,angle,0).
void test_extract_euler_pure_y() {
  float angle = 0.6f;
  Mat44F ry = SetRotationY(angle);
  Vec3F e = ExtractRotationEuler(ry);
  TEST_CHECK_(fabsf(e.x) < 1e-4f &&
              fabsf(e.y - angle) < 1e-4f &&
              fabsf(e.z) < 1e-4f,
      "ExtractRotationEuler(SetRotationY(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (0, %.1f, 0)",
      angle, e.x, e.y, e.z, angle);
}

// 6c. ExtractRotationEuler for a pure Z-axis rotation must return (0,0,angle).
void test_extract_euler_pure_z() {
  float angle = 0.6f;
  Mat44F rz = SetRotationZ(angle);
  Vec3F e = ExtractRotationEuler(rz);
  TEST_CHECK_(fabsf(e.x) < 1e-4f &&
              fabsf(e.y) < 1e-4f &&
              fabsf(e.z - angle) < 1e-4f,
      "ExtractRotationEuler(SetRotationZ(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (0, 0, %.1f)",
      angle, e.x, e.y, e.z, angle);
}

// 7. At gimbal lock (y ~ pi/2), ExtractRotationEuler must still recover y
//    and the sum x+z correctly (individual x,z are degenerate at gimbal lock
//    but y must be close to pi/2).
void test_extract_euler_gimbal_lock() {
  float x = 0.5f, z = 0.3f;
  float y = static_cast<float>(kPi) / 2.f;
  Mat44F m = SetRotationEuler4(Vec3F(x, y, z));
  Vec3F e = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e.y - y) < 0.05f,
      "ExtractRotationEuler at gimbal lock: extracted y=%.4f, "
      "expected ~%.4f",
      e.y, y);
}

// 7b. ExtractRotationEuler must not depend on translation components.
//     Adding translation to a rotation matrix must not change the result.
//     Test several rotations: Rx hits the m[0]==1 branch, Ry/Rz and a
//     combined rotation hit the general branch.
void test_extract_euler_ignores_translation() {
  Mat44F rotations[] = {
    SetRotationX(0.5f),
    SetRotationY(0.7f),
    SetRotationZ(0.3f),
    SetRotationAxisAngle4(Normalize(Vec3F(1, 2, 3)), 0.9f),
  };
  const char *names[] = {"Rx", "Ry", "Rz", "AxisAngle"};
  for (int i = 0; i < 4; ++i) {
    Mat44F m = rotations[i];
    Vec3F e1 = ExtractRotationEuler(m);
    m.m[3] = 10.0f;
    m.m[7] = 20.0f;
    m.m[11] = 30.0f;
    Vec3F e2 = ExtractRotationEuler(m);
    TEST_CHECK_(fabsf(e1.x - e2.x) < 1e-6f &&
                fabsf(e1.y - e2.y) < 1e-6f &&
                fabsf(e1.z - e2.z) < 1e-6f,
        "%s: ExtractRotationEuler must not depend on translation: "
        "without=(%.4f,%.4f,%.4f) with=(%.4f,%.4f,%.4f)",
        names[i], e1.x, e1.y, e1.z, e2.x, e2.y, e2.z);
  }
}

// 7c. The gimbal-lock branch uses m[11] (translation element [2][3]) instead
//     of a rotation element. For orthogonal matrices this is masked because
//     m[0]==1 forces m[2]==0, making atan2(0, m[11])=0. But for a scaled
//     rotation (realistic: model matrix = Scale * Rotation), m[0] can be 1
//     while m[2]!=0, and then m[11] leaks into the result.
//     Example: Ry(a) * Scale(1/cos(a), 1, 1) gives m[0]=1, m[2]=sin(a)!=0.
void test_extract_euler_gimbal_branch_reads_m11() {
  float a = 0.4f;
  Mat44F m = SetRotationY(a) * SetScale4(1.f / cosf(a), 1.f, 1.f);
  Vec3F e1 = ExtractRotationEuler(m);
  m.m[11] = 50.0f;
  Vec3F e2 = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e1.x - e2.x) < 1e-6f &&
              fabsf(e1.y - e2.y) < 1e-6f &&
              fabsf(e1.z - e2.z) < 1e-6f,
      "Scaled Ry: translation m[11] must not affect extraction: "
      "without=(%.4f,%.4f,%.4f) with=(%.4f,%.4f,%.4f)",
      e1.x, e1.y, e1.z, e2.x, e2.y, e2.z);
}

// 8. M * Transpose(M) should be identity for all rotation functions
//    (orthogonality check).
void test_rotation_matrices_are_orthogonal() {
  float t = 0.8f;
  Vec3F axis = Normalize(Vec3F(1, 1, 1));

  auto check_ortho = [](const Mat44F &m, const char *name) {
    Mat44F mt = Transpose(m);
    Mat44F prod = m * mt;
    Mat44F id = SetIdentity();
    float d = mat44_max_diff(prod, id);
    TEST_CHECK_(d < 1e-5f,
        "%s: M * M^T should be identity, max diff=%.7f", name, d);
  };

  check_ortho(SetRotationX(t), "SetRotationX");
  check_ortho(SetRotationY(t), "SetRotationY");
  check_ortho(SetRotationZ(t), "SetRotationZ");
  check_ortho(SetRotationAxisAngle4(axis, t), "SetRotationAxisAngle4");
  check_ortho(SetRotationEuler4(Vec3F(0.3f, 0.5f, 0.7f)), "SetRotationEuler4");

  Vec4F q(sinf(t / 2) * axis.x, sinf(t / 2) * axis.y,
          sinf(t / 2) * axis.z, cosf(t / 2));
  check_ortho(SetRotationQuaternion(q), "SetRotationQuaternion");
}

// 9. Translation matrix: Transform(SetTranslation(t), v) == v + t
void test_translation_transforms_point() {
  Vec3F t(3.f, -1.f, 7.f);
  Vec3F v(1.f, 2.f, 3.f);
  Mat44F m = SetTranslation(t);
  Vec3F result = Transform(m, v);
  float d = Length(result - (v + t));
  TEST_CHECK_(d < 1e-5f,
      "SetTranslation * point: error=%.7f", d);
}

// 10. SetLookat produces orthonormal basis (post-fix for bug 23)
void test_lookat_orthonormal() {
  Vec3F eye(1, 2, 3);
  Vec3F target(4, 5, 6);
  Vec3F up(0, 1, 0);
  Mat44F m = SetLookat(eye, target, up);

  Vec3F row0(m.m[0], m.m[1], m.m[2]);
  Vec3F row1(m.m[4], m.m[5], m.m[6]);
  Vec3F row2(m.m[8], m.m[9], m.m[10]);

  float len0 = Length(row0);
  float len1 = Length(row1);
  float len2 = Length(row2);
  float dot01 = Dot(row0, row1);
  float dot02 = Dot(row0, row2);
  float dot12 = Dot(row1, row2);

  TEST_CHECK_(fabsf(len0 - 1.f) < 1e-5f &&
              fabsf(len1 - 1.f) < 1e-5f &&
              fabsf(len2 - 1.f) < 1e-5f,
      "SetLookat rows unit length: %.6f %.6f %.6f", len0, len1, len2);
  TEST_CHECK_(fabsf(dot01) < 1e-5f &&
              fabsf(dot02) < 1e-5f &&
              fabsf(dot12) < 1e-5f,
      "SetLookat rows orthogonal: dots=%.6f %.6f %.6f",
      dot01, dot02, dot12);
}

// 11. SetLookat with eye==target returns identity (bug 23 fix)
void test_lookat_degenerate_returns_identity() {
  Vec3F p(5, 5, 5);
  Mat44F m = SetLookat(p, p, Vec3F(0, 1, 0));
  Mat44F id = SetIdentity();
  float d = mat44_max_diff(m, id);
  TEST_CHECK_(d < 1e-5f,
      "SetLookat(eye==target) should be identity, diff=%.7f", d);
}

// Bug 46: SetPerspective uses tan(fovy) instead of tan(fovy/2).
// The vertical scale element m[5] must equal 1/tan(fovy/2).
// With the bug, it equals 1/tan(fovy) -- a completely different value.
void test_perspective_y_equals_cot_half_fovy() {
  const float fovy = 60.0f;
  const float aspect = 1.5f;
  const float znear = 0.1f;
  const float zfar = 100.0f;

  Mat44F m = SetPerspective(fovy, aspect, znear, zfar);

  float half_fovy_rad = fovy * static_cast<float>(M_PI) / 360.0f;
  float expected_y = 1.0f / tanf(half_fovy_rad);
  float expected_x = expected_y / aspect;

  TEST_CHECK_(fabsf(m.m[5] - expected_y) < 1e-5f,
      "m[5] should be cot(fovy/2)=%.6f, got %.6f", expected_y, m.m[5]);
  TEST_CHECK_(fabsf(m.m[0] - expected_x) < 1e-5f,
      "m[0] should be cot(fovy/2)/aspect=%.6f, got %.6f", expected_x, m.m[0]);
}

// SetPerspective and SetFrustumPerspective accept the same fovy parameter
// and must produce the same projection matrix for the same inputs.
void test_perspective_matches_frustum_perspective() {
  const float fovy = 90.0f;
  const float aspect = 16.0f / 9.0f;
  const float znear = 0.5f;
  const float zfar = 500.0f;

  Mat44F mat = SetPerspective(fovy, aspect, znear, zfar);
  Frustum3F fru = SetFrustumPerspective(fovy, aspect, znear, zfar);
  Mat44F fru_mat = fru.matrix;

  float max_diff = 0.0f;
  for (int i = 0; i < 16; ++i) {
    float d = fabsf(mat.m[i] - fru_mat.m[i]);
    if (d > max_diff) {
      max_diff = d;
    }
  }
  TEST_CHECK_(max_diff < 1e-5f,
      "SetPerspective and SetFrustumPerspective must match, max diff=%.7f",
      max_diff);
}

// SetPerspectiveTiled with offset=(0,0) and size=(1,1) is a full tile,
// which must produce the same matrix as SetPerspective.
void test_perspective_tiled_matches_perspective() {
  const float fovy = 75.0f;
  const float aspect = 2.0f;
  const float znear = 1.0f;
  const float zfar = 1000.0f;

  Mat44F mat = SetPerspective(fovy, aspect, znear, zfar);
  Mat44F tiled = SetPerspectiveTiled(fovy, aspect, znear, zfar,
      Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f));

  float max_diff = 0.0f;
  for (int i = 0; i < 16; ++i) {
    float d = fabsf(mat.m[i] - tiled.m[i]);
    if (d > max_diff) {
      max_diff = d;
    }
  }
  TEST_CHECK_(max_diff < 1e-5f,
      "SetPerspectiveTiled(full) must match SetPerspective, max diff=%.7f",
      max_diff);
}

// Bug 63: CSV serialization must quote fields containing separator or quotes.
// Round-trip: LoadFile -> set field with comma -> SaveFile -> LoadFile.
void test_csv_roundtrip_separator_in_field() {
  const char *path = "/tmp/arctic_csv_test_sep.csv";
  {
    std::ofstream f(path);
    f << "name,value" << std::endl;
    f << "hello,world" << std::endl;
  }

  CsvTable table;
  bool ok = table.LoadFile(path);
  TEST_CHECK(ok);
  TEST_CHECK_(table.RowCount() == 1,
      "Expected 1 row, got %llu", (unsigned long long)table.RowCount());

  table[0].Set("value", "has,comma");
  table.SaveFile();

  CsvTable table2;
  ok = table2.LoadFile(path);
  TEST_CHECK_(ok, "Re-parse must succeed, error: %s",
      table2.GetErrorDescription().c_str());
  TEST_CHECK_(table2.RowCount() == 1,
      "Re-parsed table must have 1 row, got %llu",
      (unsigned long long)table2.RowCount());
  if (table2.RowCount() >= 1) {
    std::string val = table2[0]["value"];
    TEST_CHECK_(val == "has,comma",
        "Round-trip must preserve 'has,comma', got '%s'", val.c_str());
  }
  std::remove(path);
}

// Bug 63b: Same round-trip for quotes inside field values.
void test_csv_roundtrip_quotes_in_field() {
  const char *path = "/tmp/arctic_csv_test_quot.csv";
  {
    std::ofstream f(path);
    f << "name,value" << std::endl;
    f << "hello,world" << std::endl;
  }

  CsvTable table;
  bool ok = table.LoadFile(path);
  TEST_CHECK(ok);

  table[0].Set("value", "say \"hi\"");
  table.SaveFile();

  CsvTable table2;
  ok = table2.LoadFile(path);
  TEST_CHECK_(ok, "Re-parse must succeed, error: %s",
      table2.GetErrorDescription().c_str());
  TEST_CHECK_(table2.RowCount() == 1,
      "Re-parsed table must have 1 row, got %llu",
      (unsigned long long)table2.RowCount());
  if (table2.RowCount() >= 1) {
    std::string val = table2[0]["value"];
    TEST_CHECK_(val == "say \"hi\"",
        "Round-trip must preserve quotes, got '%s'", val.c_str());
  }
  std::remove(path);
}

// Bug 62: Panel with top+bottom (or left+right) anchoring gets negative
// size when the parent shrinks below the sum of anchor distances.
void test_panel_anchor_no_negative_size() {
  auto parent = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(400, 300));

  auto child = std::make_shared<Panel>(1, Vec2Si32(20, 30), Vec2Si32(360, 240));
  parent->AddChild(child);
  child->SetAnchor(static_cast<AnchorKind>(kAnchorLeft | kAnchorRight |
                                            kAnchorBottom | kAnchorTop));

  // anchor_left_d_ = 20, anchor_right_d_ = 400 - 360 - 20 = 20
  // anchor_bottom_d_ = 30, anchor_top_d_ = 300 - 240 - 30 = 30
  // So sum of horizontal anchors = 40, sum of vertical anchors = 60.

  // Shrink parent below the anchor distances.
  parent->SetSize(Vec2Si32(30, 40));

  Vec2Si32 sz = child->GetSize();
  TEST_CHECK_(sz.x >= 0,
      "Child width must not be negative after parent shrink, got %d", sz.x);
  TEST_CHECK_(sz.y >= 0,
      "Child height must not be negative after parent shrink, got %d", sz.y);
}

// An editbox has to tell its host what happened to it: that the text changed,
// and that the editing is over. Without it the host is left comparing GetText()
// against a remembered copy every frame, which is what deca used to do.
void test_editbox_reports_text_change_and_edit_done() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite normal;
  normal.Create(60, 12);
  Sprite focused;
  focused.Create(60, 12);

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(200, 100));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, normal, focused,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto other = std::make_shared<Editbox>(2, Vec2Si32(10, 40), 2, normal,
      focused, font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(other);

  Si32 changes = 0;
  Si32 dones = 0;
  box->OnTextChange = [&changes]() { ++changes; };
  box->OnEditDone = [&dones]() { ++dones; };

  // Nothing is focused yet, so nothing holds the keyboard.
  TEST_CHECK(!root->IsKeyboardCaptured());
  TEST_CHECK(!box->IsFocused());

  box->SetCurrentTab(true);
  TEST_CHECK(box->IsFocused());
  TEST_CHECK_(root->IsKeyboardCaptured(),
      "a focused editbox must hold the keyboard");

  std::deque<GuiMessage> messages;
  InputMessage typed;
  typed.kind = InputMessage::kKeyboard;
  typed.keyboard.key = kKeyA;
  typed.keyboard.key_state = 1;
  typed.keyboard.characters[0] = 'a';
  bool is_applied = false;
  std::shared_ptr<Panel> current_tab;
  box->ApplyInput(Vec2Si32(0, 0), typed, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string("a"),
      "the keystroke did not reach the text: '%s'", box->GetText().c_str());
  TEST_CHECK_(changes == 1, "OnTextChange fired %d times, expected 1",
      (int)changes);
  TEST_CHECK_(dones == 0, "the editing is not over yet");
  Si32 change_messages = 0;
  for (const GuiMessage &m : messages) {
    if (m.kind == kGuiEditboxTextChange) {
      ++change_messages;
    }
  }
  TEST_CHECK_(change_messages == 1,
      "%d text change messages were queued, expected 1", (int)change_messages);

  // A keystroke that leaves the text alone is not a change.
  InputMessage arrow;
  arrow.kind = InputMessage::kKeyboard;
  arrow.keyboard.key = kKeyLeft;
  arrow.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), arrow, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(changes == 1, "moving the caret counted as a text change");

  // Enter ends the editing in a single line box and is left for the host.
  InputMessage enter;
  enter.kind = InputMessage::kKeyboard;
  enter.keyboard.key = kKeyEnter;
  enter.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), enter, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(dones == 1, "OnEditDone fired %d times on Enter, expected 1",
      (int)dones);
  TEST_CHECK_(!is_applied, "Enter must be left to the host");

  // Losing the focus ends the editing as well.
  root->SwitchCurrentTab(true);
  TEST_CHECK_(!box->IsFocused(), "the focus did not move away");
  TEST_CHECK_(dones == 2,
      "OnEditDone fired %d times after losing the focus, expected 2",
      (int)dones);

  // A hidden field can not be typed into, so it does not hold the keyboard.
  TEST_CHECK(other->IsFocused());
  TEST_CHECK(root->IsKeyboardCaptured());
  other->SetVisible(false);
  TEST_CHECK_(!root->IsKeyboardCaptured(),
      "a hidden editbox still claims the keyboard");

  // A visible field inside a hidden panel can not be typed into either: the walk
  // over the panels stops at the hidden one and never asks the children.
  other->SetVisible(true);
  TEST_CHECK(root->IsKeyboardCaptured());
  root->SetVisible(false);
  TEST_CHECK_(!root->IsKeyboardCaptured(),
      "an editbox inside a hidden panel still claims the keyboard");
  root->SetVisible(true);
}

// The keys report physical positions, so an editbox can not take the letter from
// the key code: it has to use the typed text, or a Cyrillic layout would type
// latin letters. A key that carries no text at all (Windows sends the key and
// the character as two separate messages) must add nothing.
void test_editbox_accepts_any_layout() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite normal;
  normal.Create(60, 12);
  Sprite focused;
  focused.Create(60, 12);

  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, normal, focused,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  box->SetCurrentTab(true);

  std::deque<GuiMessage> messages;
  std::shared_ptr<Panel> current_tab;
  const char *kEf = "\xd1\x84";  // Cyrillic small letter ef, the key of latin A

  InputMessage cyrillic;
  cyrillic.kind = InputMessage::kKeyboard;
  cyrillic.keyboard.key = kKeyA;
  cyrillic.keyboard.key_state = 1;
  snprintf(cyrillic.keyboard.characters, sizeof(cyrillic.keyboard.characters),
      "%s", kEf);
  bool is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), cyrillic, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf),
      "a Cyrillic keystroke gave '%s'", box->GetText().c_str());

  InputMessage latin;
  latin.kind = InputMessage::kKeyboard;
  latin.keyboard.key = kKeyA;
  latin.keyboard.key_state = 1;
  latin.keyboard.characters[0] = 'a';
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), latin, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "a latin keystroke after a Cyrillic one gave '%s'",
      box->GetText().c_str());

  InputMessage bare;
  bare.kind = InputMessage::kKeyboard;
  bare.keyboard.key = kKeyA;
  bare.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), bare, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "a key without typed text inserted something: '%s'",
      box->GetText().c_str());

  // Tab moves the focus and is not text, even when it comes with a tabulation
  // for a character, and even when this box is the only one that can be focused.
  InputMessage tab;
  tab.kind = InputMessage::kKeyboard;
  tab.keyboard.key = kKeyTab;
  tab.keyboard.key_state = 1;
  tab.keyboard.characters[0] = '\t';
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), tab, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "Tab put something in the text: '%s'", box->GetText().c_str());
}

// The platform code fills the characters of a message through one function, so
// that every platform reports the same thing for the keys that the system calls
// characters: Tab, Enter, Escape, Backspace, Control with a letter.
void test_typed_characters_of_a_message() {
  InputMessage::Keyboard keyboard;
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\t"),
      "a tabulation was taken for text");
  TEST_CHECK(keyboard.characters[0] == '\0');
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\r"), "Enter was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\x01"),
      "Control with a letter was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\x7f"),
      "Backspace was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, nullptr),
      "a keystroke with no text at all was taken for text");
  TEST_CHECK(keyboard.characters[0] == '\0');

  TEST_CHECK_(SetTypedCharacters(&keyboard, "a"), "a letter is text");
  TEST_CHECK_(std::string(keyboard.characters) == "a",
      "a latin letter became '%s'", keyboard.characters);
  const char *kEf = "\xd1\x84";  // Cyrillic small letter ef, two bytes
  TEST_CHECK(SetTypedCharacters(&keyboard, kEf));
  TEST_CHECK_(std::string(keyboard.characters) == kEf,
      "a Cyrillic letter became '%s'", keyboard.characters);

  // A control character next to a letter loses only itself.
  TEST_CHECK(SetTypedCharacters(&keyboard, "\tx"));
  TEST_CHECK_(std::string(keyboard.characters) == "x",
      "a tabulation with a letter became '%s'", keyboard.characters);

  // More text than the message can hold stays inside the array and zero ended.
  TEST_CHECK(SetTypedCharacters(&keyboard, "0123456789abcdefghij"));
  TEST_CHECK_(std::string(keyboard.characters) == "0123456789abcde",
      "a long text became '%s'", keyboard.characters);
}

// The keys are physical, and a physical shift is either the left or the right
// one, but code (the engine's own GUI included) asks about the generic kKeyShift.
// The typed text of the frame is available without walking the message queue.
void test_typed_text_and_generic_modifiers() {
  InputMessage right_shift;
  right_shift.kind = InputMessage::kKeyboard;
  right_shift.keyboard.key = kKeyRightShift;
  right_shift.keyboard.key_state = 1;
  PushInputMessage(right_shift);

  InputMessage typed;
  typed.kind = InputMessage::kKeyboard;
  typed.keyboard.key = kKeyA;
  typed.keyboard.key_state = 1;
  snprintf(typed.keyboard.characters, sizeof(typed.keyboard.characters),
      "%s", "\xd0\xa4");  // Cyrillic capital ef, typed with shift held
  PushInputMessage(typed);
  ShowFrame();

  TEST_CHECK_(IsKeyDown(kKeyRightShift), "the right shift is not down");
  TEST_CHECK_(IsKeyDown(kKeyShift),
      "a physical shift must also answer for the generic kKeyShift");
  TEST_CHECK_(IsKeyDown(kKeyA), "the physical key of the keystroke is not down");
  TEST_CHECK_(TypedText() == std::string("\xd0\xa4"),
      "TypedText gave '%s'", TypedText().c_str());

  // Holding the other shift keeps the generic one down when the first is let go.
  InputMessage left_shift;
  left_shift.kind = InputMessage::kKeyboard;
  left_shift.keyboard.key = kKeyLeftShift;
  left_shift.keyboard.key_state = 1;
  PushInputMessage(left_shift);
  right_shift.keyboard.key_state = 2;
  PushInputMessage(right_shift);
  ShowFrame();
  TEST_CHECK_(IsKeyDown(kKeyShift),
      "the generic shift went up while the left one is still held");

  left_shift.keyboard.key_state = 2;
  PushInputMessage(left_shift);
  typed.keyboard.key_state = 2;
  typed.keyboard.characters[0] = '\0';
  PushInputMessage(typed);
  ShowFrame();
  TEST_CHECK_(!IsKeyDown(kKeyShift),
      "the generic shift stayed down after both shifts were released");
  TEST_CHECK_(IsKeyUpward(kKeyShift),
      "the release of the last shift was not reported as an upward edge");
  TEST_CHECK_(TypedText().empty(),
      "the typed text survived the frame: '%s'", TypedText().c_str());

  // Keys that are not text are reported with a control byte for a character by
  // one platform or another, and none of that belongs in the typed text.
  InputMessage tab;
  tab.kind = InputMessage::kKeyboard;
  tab.keyboard.key = kKeyTab;
  tab.keyboard.key_state = 1;
  tab.keyboard.characters[0] = '\t';
  PushInputMessage(tab);
  InputMessage control_letter;
  control_letter.kind = InputMessage::kKeyboard;
  control_letter.keyboard.key = kKeyA;
  control_letter.keyboard.key_state = 1;
  control_letter.keyboard.characters[0] = '\x01';  // Control with a letter
  PushInputMessage(control_letter);
  InputMessage letter;
  letter.kind = InputMessage::kKeyboard;
  letter.keyboard.key = kKeyX;
  letter.keyboard.key_state = 1;
  letter.keyboard.characters[0] = 'x';
  PushInputMessage(letter);
  ShowFrame();
  TEST_CHECK_(TypedText() == std::string("x"),
      "the typed text of a frame with Tab and Control+A gave '%s'",
      TypedText().c_str());

  tab.keyboard.key_state = 2;
  PushInputMessage(tab);
  control_letter.keyboard.key_state = 2;
  PushInputMessage(control_letter);
  letter.keyboard.key_state = 2;
  PushInputMessage(letter);
  ShowFrame();
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

// The mouse reaches the game in the pixels the game draws in: backbuffer
// pixels, y upward, clamped to the backbuffer. A position that arrives in
// window pixels, or counted from the top, or unclamped over a letterbox bar,
// is what makes a click land beside the thing it was aimed at.
void test_mouse_arrives_in_backbuffer_pixels() {
  ResizeScreen(320, 200);
  const Vec2Si32 size = ScreenSize();
  TEST_CHECK_(size == Vec2Si32(320, 200), "the backbuffer is %d x %d",
      size.x, size.y);

  // The engine takes a mouse position as a fraction of the window along each
  // axis, y upward, and hands the game backbuffer pixels.
  auto move_mouse_to = [](float x, float y) {
    InputMessage message;
    message.kind = InputMessage::kMouse;
    message.mouse.pos = Vec2F(x, y);
    PushInputMessage(message);
    ShowFrame();
  };

  move_mouse_to(0.0f, 0.0f);
  TEST_CHECK_(MousePos() == Vec2Si32(0, 0),
      "the bottom-left corner of the window came out as (%d, %d)",
      MousePos().x, MousePos().y);

  move_mouse_to(1.0f, 1.0f);
  TEST_CHECK_(MousePos() == size - Vec2Si32(1, 1),
      "the top-right corner of the window came out as (%d, %d) instead of "
      "(%d, %d)", MousePos().x, MousePos().y, size.x - 1, size.y - 1);

  // Y grows upward: the upper half of the window is the upper half of the
  // backbuffer, not the lower one.
  move_mouse_to(0.5f, 0.8f);
  const Si32 high_y = MousePos().y;
  move_mouse_to(0.5f, 0.2f);
  const Si32 low_y = MousePos().y;
  TEST_CHECK_(high_y > size.y / 2,
      "a cursor in the upper part of the window reported y = %d of %d",
      high_y, size.y);
  TEST_CHECK_(low_y < size.y / 2,
      "a cursor in the lower part of the window reported y = %d of %d",
      low_y, size.y);
  TEST_CHECK_(high_y > low_y,
      "y did not grow upward: %d up against %d down", high_y, low_y);

  // A position outside the window, which is what the bars of a letterboxed
  // window and a dragged cursor produce, is clamped to the backbuffer instead
  // of naming a pixel that does not exist.
  move_mouse_to(-0.5f, 1.5f);
  TEST_CHECK_(MousePos().x == 0 && MousePos().y == size.y - 1,
      "a position outside the window came out as (%d, %d), which is not "
      "clamped to the backbuffer", MousePos().x, MousePos().y);

  // The same number is in the message queue, which is where the GUI reads it,
  // so a game handling messages itself sees exactly what MousePos() says.
  move_mouse_to(0.25f, 0.75f);
  Si32 mouse_messages = 0;
  for (Si32 i = 0; i < InputMessageCount(); ++i) {
    const InputMessage &message = GetInputMessage(i);
    if (message.kind != InputMessage::kMouse) {
      continue;
    }
    ++mouse_messages;
    TEST_CHECK_(message.mouse.backbuffer_pos == MousePos(),
        "the message says (%d, %d) while MousePos says (%d, %d)",
        message.mouse.backbuffer_pos.x, message.mouse.backbuffer_pos.y,
        MousePos().x, MousePos().y);
  }
  TEST_CHECK_(mouse_messages == 1,
      "the frame carried %d mouse messages instead of one", mouse_messages);

  // Negative control for the direction of y: the position a game would compute
  // by counting from the top must differ from the one the engine reports,
  // otherwise the checks above hold for both conventions and prove nothing.
  const Si32 from_the_top = size.y - 1 - MousePos().y;
  TEST_CHECK_(from_the_top != MousePos().y,
      "a cursor was placed exactly in the middle of the window, where the two "
      "conventions agree, so this test proves nothing about the direction");
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

// Two copies of one program are told apart by the title of the window and by
// nothing else, so the default is the name of the program rather than the name
// of the engine, and an application can say something of its own there. Whether
// the platform showed it cannot be read back from the system, so what is
// checked here is the value the engine keeps and hands to the window.
void test_window_title_names_the_program() {
  const std::string default_title = WindowTitle();
  TEST_CHECK_(!default_title.empty(), "the default window title is empty");

  const std::string executable = GetExecutablePath();
  if (!executable.empty()) {
    const size_t slash = executable.find_last_of("/\\");
    const std::string file_name = (slash == std::string::npos)
      ? executable : executable.substr(slash + 1);
    TEST_CHECK_(default_title == file_name,
        "the default title is '%s' while the program is '%s'",
        default_title.c_str(), file_name.c_str());
    TEST_CHECK_(default_title != "Arctic Engine",
        "the default title still names the engine instead of the program");
    TEST_CHECK_(default_title.find('/') == std::string::npos,
        "the default title is a whole path, '%s'", default_title.c_str());
  }

  const std::string mine = "Hover Racer \xe2\x80\x94 player 2";
  SetWindowTitle(mine.c_str());
  TEST_CHECK_(WindowTitle() == mine,
      "the title set to '%s' reads back as '%s'", mine.c_str(),
      WindowTitle().c_str());

  // An empty title, and a missing one, mean the default rather than a nameless
  // window; a program that has nothing to say stays recognisable.
  SetWindowTitle("");
  TEST_CHECK_(WindowTitle() == default_title,
      "an empty title gave '%s' instead of the default '%s'",
      WindowTitle().c_str(), default_title.c_str());
  SetWindowTitle(mine.c_str());
  SetWindowTitle(nullptr);
  TEST_CHECK_(WindowTitle() == default_title,
      "a null title gave '%s' instead of the default '%s'",
      WindowTitle().c_str(), default_title.c_str());
}

// How long a key has been held is a question every game with a charged shot or
// a repeat-after-delay had to answer for itself, remembering the moment of the
// press by hand. The engine remembers it now.
void test_key_down_seconds_measures_the_hold() {
  SetKey(kKeySpace, false);
  ShowFrame();
  TEST_CHECK_(KeyDownSeconds(kKeySpace) == 0.0,
      "a key that is not held reported %f seconds",
      KeyDownSeconds(kKeySpace));

  SetKey(kKeySpace, true);
  const double at_the_press = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(at_the_press >= 0.0 && at_the_press < 0.05,
      "right after the press the hold is %f seconds", at_the_press);

  Sleep(0.05);
  ShowFrame();
  const double after_a_frame = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(after_a_frame >= 0.05,
      "after fifty milliseconds of holding the answer is %f seconds",
      after_a_frame);
  TEST_CHECK_(after_a_frame > at_the_press,
      "the hold did not grow: %f then %f", at_the_press, after_a_frame);
  TEST_CHECK_(IsKeyDown(kKeySpace),
      "the key stopped being down while nothing released it");
  TEST_CHECK_(!IsKeyDownward(kKeySpace),
      "a key held since the previous frame must not be a fresh press");

  Sleep(0.05);
  ShowFrame();
  const double after_two_frames = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(after_two_frames > after_a_frame,
      "the hold stopped growing across frames: %f then %f",
      after_a_frame, after_two_frames);

  SetKey(kKeySpace, false);
  ShowFrame();
  TEST_CHECK_(KeyDownSeconds(kKeySpace) == 0.0,
      "a released key still reports %f seconds", KeyDownSeconds(kKeySpace));

  // Negative control for the reset: a version that kept the old moment of the
  // press would answer with the whole time since the first press, which is
  // more than what has passed since this one.
  SetKey(kKeySpace, true);
  const double second_press = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(second_press < after_two_frames,
      "the second press reports %f seconds, and the first hold was %f, so the "
      "moment of the press was not taken anew", second_press,
      after_two_frames);
  SetKey(kKeySpace, false);
  ShowFrame();

  // The string form asks about several keys at once and answers with the
  // longest hold, the same way IsKeyDown answers about any of them.
  SetKey('a', true);
  Sleep(0.05);
  SetKey('d', true);
  ShowFrame();
  // The clock runs between the calls, so the three readings are compared with
  // a millisecond of slack rather than for equality.
  const double both = KeyDownSeconds("ad");
  const double held_a = KeyDownSeconds('a');
  const double held_d = KeyDownSeconds('d');
  TEST_CHECK_(held_a > held_d,
      "the key pressed earlier is held for %f while the later one is %f",
      held_a, held_d);
  TEST_CHECK_(both > held_d + 0.03 && both <= held_a
          && both > held_a - 0.005,
      "asking about both keys gave %f, and the two holds are %f and %f, so it "
      "is not the longer of them", both, held_a, held_d);
  TEST_CHECK_(KeyDownSeconds("qz") == 0.0,
      "keys nobody pressed reported %f seconds", KeyDownSeconds("qz"));
  SetKey('a', false);
  SetKey('d', false);
  ShowFrame();
}

namespace {

// A port nothing else is using, so a socket test does not fail because some
// other program on the machine got to the port first.
bool BindLoopbackListener(ListenerSocket *listener, uint16_t *out_port) {
  for (uint16_t port = 34567; port < 34667; ++port) {
    ListenerSocket candidate(AddressFamily::kIpV4, SocketProtocol::kTcp);
    if (!candidate.IsValid()) {
      return false;
    }
    if (candidate.Bind("127.0.0.1", port) == SocketResult::kSocketOk) {
      *listener = std::move(candidate);
      *out_port = port;
      return true;
    }
  }
  return false;
}

// Waits a while for a connection instead of blocking for good: the listener is
// non-blocking, so an Accept with nobody waiting comes back invalid.
ConnectionSocket AcceptWithin(const ListenerSocket &listener, double seconds) {
  const double deadline = Time() + seconds;
  while (Time() < deadline) {
    ConnectionSocket accepted = listener.Accept();
    if (accepted.IsValid()) {
      return accepted;
    }
    Sleep(0.002);
  }
  return ConnectionSocket();
}

// Reads until the bytes expected have arrived, the connection says something
// other than "ok", or the time runs out.
SocketResult ReadWithin(ConnectionSocket *socket, size_t expected_size,
    double seconds, std::string *out_text) {
  out_text->clear();
  const double deadline = Time() + seconds;
  while (out_text->size() < expected_size && Time() < deadline) {
    char buffer[64];
    size_t read_size = 0;
    const SocketResult result = socket->Read(buffer, sizeof(buffer),
        &read_size);
    if (result != SocketResult::kSocketOk) {
      return result;
    }
    if (read_size == 0) {
      Sleep(0.002);
      continue;
    }
    out_text->append(buffer, read_size);
  }
  return SocketResult::kSocketOk;
}

// A short message still goes out in pieces if the socket feels like it.
bool WriteAll(ConnectionSocket *socket, const std::string &text) {
  size_t total = 0;
  const double deadline = Time() + 1.0;
  while (total < text.size() && Time() < deadline) {
    size_t written = 0;
    if (socket->Write(text.data() + total, text.size() - total, &written)
        != SocketResult::kSocketOk) {
      return false;
    }
    total += written;
  }
  return total == text.size();
}

}  // namespace

// The whole life of a connection over the loopback interface, because a server
// example is worth nothing if these do not hold: a port is bound once and the
// second attempt is refused, a connection is accepted, both ends can write and
// read, closing one end is visible at the other, and a connect to a port
// nobody listens on fails instead of hanging.
void test_loopback_sockets_connect_talk_and_close() {
  ListenerSocket listener;
  uint16_t port = 0;
  TEST_CHECK_(BindLoopbackListener(&listener, &port),
      "no port in the range could be bound on 127.0.0.1: %s",
      listener.GetLastError().c_str());
  if (!listener.IsValid()) {
    return;
  }
  TEST_CHECK(listener.SetSoNonblocking(true) == SocketResult::kSocketOk);

  // The port is taken now, and the engine has to say so rather than quietly
  // producing a second listener that receives nothing.
  ListenerSocket intruder(AddressFamily::kIpV4, SocketProtocol::kTcp);
  TEST_CHECK_(intruder.Bind("127.0.0.1", port) == SocketResult::kSocketError,
      "binding port %d a second time was allowed", static_cast<int>(port));
  TEST_CHECK_(!intruder.GetLastError().empty(),
      "the refused bind left no explanation in GetLastError");

  ConnectionSocket client(AddressFamily::kIpV4, SocketProtocol::kTcp);
  TEST_CHECK_(client.IsValid(), "no socket: %s", client.GetLastError().c_str());
  const SocketConnectResult connected = client.Connect("127.0.0.1", port);
  TEST_CHECK_(connected == SocketConnectResult::kSocketOk,
      "connect to the listening port answered %d: %s",
      static_cast<int>(connected), client.GetLastError().c_str());
  TEST_CHECK_(client.GetState() == SocketState::kConnected,
      "a connected socket does not report kConnected");
  TEST_CHECK(client.SetSoNonblocking(true) == SocketResult::kSocketOk);

  ConnectionSocket served = AcceptWithin(listener, 2.0);
  TEST_CHECK_(served.IsValid(),
      "the listener never handed over the connection: %s",
      listener.GetLastError().c_str());
  if (!served.IsValid()) {
    return;
  }
  TEST_CHECK_(served.GetState() == SocketState::kConnected,
      "an accepted connection does not report kConnected");
  TEST_CHECK(served.SetSoNonblocking(true) == SocketResult::kSocketOk);

  TEST_CHECK_(WriteAll(&client, "ping"), "the client could not write: %s",
      client.GetLastError().c_str());
  std::string question;
  const SocketResult got_question = ReadWithin(&served, 4, 2.0, &question);
  TEST_CHECK_(got_question == SocketResult::kSocketOk,
      "reading the request answered %d: %s", static_cast<int>(got_question),
      served.GetLastError().c_str());
  TEST_CHECK_(question == "ping", "the server got '%s' instead of 'ping'",
      question.c_str());

  TEST_CHECK_(WriteAll(&served, "pong-42"), "the server could not write: %s",
      served.GetLastError().c_str());
  std::string answer;
  const SocketResult got_answer = ReadWithin(&client, 7, 2.0, &answer);
  TEST_CHECK_(got_answer == SocketResult::kSocketOk,
      "reading the answer answered %d: %s", static_cast<int>(got_answer),
      client.GetLastError().c_str());
  TEST_CHECK_(answer == "pong-42", "the client got '%s' instead of 'pong-42'",
      answer.c_str());

  // The other end goes away. This has to be told apart from "no data yet",
  // which is what a server loop hangs on when it is not.
  {
    ConnectionSocket closing = std::move(client);
  }
  std::string nothing;
  const SocketResult after_close = ReadWithin(&served, 1, 2.0, &nothing);
  TEST_CHECK_(after_close == SocketResult::kSocketConnectionReset,
      "reading a connection closed by the other end answered %d, and "
      "kSocketConnectionReset (%d) was expected", static_cast<int>(after_close),
      static_cast<int>(SocketResult::kSocketConnectionReset));
  TEST_CHECK_(nothing.empty(), "the closed connection produced '%s'",
      nothing.c_str());
  TEST_CHECK_(!served.IsValid(),
      "a connection the other end closed is still valid");
  TEST_CHECK_(served.GetState() == SocketState::kDisconnected,
      "a connection the other end closed does not report kDisconnected");

  // And with the listener gone, the same port refuses a connection.
  {
    ListenerSocket closing = std::move(listener);
  }
  ConnectionSocket hopeful(AddressFamily::kIpV4, SocketProtocol::kTcp);
  const SocketConnectResult refused = hopeful.Connect("127.0.0.1", port);
  TEST_CHECK_(refused == SocketConnectResult::kSocketError,
      "connecting to port %d with nothing listening answered %d",
      static_cast<int>(port), static_cast<int>(refused));
  TEST_CHECK_(!hopeful.GetLastError().empty(),
      "the refused connect left no explanation in GetLastError");
  TEST_CHECK_(hopeful.GetState() == SocketState::kDisconnected,
      "a socket that failed to connect does not report kDisconnected");
}

// "Is this click mine or the interface's?" is the question a game asks before
// selecting something in the world, and IsInside answers it about a point. The
// answer has to agree with what the input walk actually does, so every case is
// checked against ApplyInput as well.
void test_panel_is_inside_answers_for_the_interface() {
  auto click_at = [](Vec2Si32 at) {
    InputMessage message;
    message.kind = InputMessage::kMouse;
    message.mouse.backbuffer_pos = at;
    message.keyboard.key = kKeyMouseLeft;
    message.keyboard.key_state = 1;
    return message;
  };
  std::deque<GuiMessage> gui_messages;

  auto root = std::make_shared<Panel>(1, Vec2Si32(0, 0), Vec2Si32(320, 200));
  // A bare panel is not clickable, so it takes nothing and a click over it
  // belongs to the world. This is the case that surprises people, and it is the
  // behaviour of the input walk, not an opinion of this function.
  TEST_CHECK_(!root->IsInside(Vec2Si32(10, 10)),
      "an empty non-clickable panel claimed a click");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(10, 10)), &gui_messages),
      "the input walk applied a click on a non-clickable panel, so IsInside "
      "and the walk disagree");

  Sprite face;
  face.Create(40, 20);
  auto button = std::make_shared<Button>(2, Vec2Si32(100, 50), face);
  root->AddChild(button);
  TEST_CHECK_(button->GetSize() == Vec2Si32(40, 20),
      "the button is %d x %d, and the coordinates below assume 40 x 20",
      button->GetSize().x, button->GetSize().y);

  TEST_CHECK_(root->IsInside(Vec2Si32(100, 50)),
      "the lower left pixel of a button is outside it");
  TEST_CHECK_(root->IsInside(Vec2Si32(139, 69)),
      "the upper right pixel of a button is outside it");
  TEST_CHECK_(!root->IsInside(Vec2Si32(140, 70)),
      "the pixel past the upper right corner of a button belongs to it");
  TEST_CHECK_(!root->IsInside(Vec2Si32(99, 50)),
      "the pixel left of a button belongs to it");
  TEST_CHECK_(root->ApplyInput(click_at(Vec2Si32(110, 60)), &gui_messages),
      "a click on a button was not taken by the interface");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(200, 150)), &gui_messages),
      "a click far from every panel was taken by the interface");

  // A clickable panel deep in the tree is found, and its position is the sum of
  // the positions on the way down.
  auto dialog = std::make_shared<Panel>(3, Vec2Si32(20, 20),
      Vec2Si32(100, 100));
  auto slot = std::make_shared<Panel>(4, Vec2Si32(5, 5), Vec2Si32(10, 10), 0,
      Sprite(), true);
  dialog->AddChild(slot);
  root->AddChild(dialog);
  TEST_CHECK_(root->IsInside(Vec2Si32(25, 25)),
      "a clickable panel at 20 plus 5 was not found at 25");
  TEST_CHECK_(root->IsInside(Vec2Si32(34, 34)),
      "the far corner of the nested panel was not found");
  TEST_CHECK_(!root->IsInside(Vec2Si32(35, 35)),
      "the point past the nested panel belongs to it");
  TEST_CHECK_(root->ApplyInput(click_at(Vec2Si32(30, 30)), &gui_messages),
      "a click on the nested panel was not taken");

  // Hiding a panel hides what is inside it, and neither takes clicks any more.
  dialog->SetVisible(false);
  TEST_CHECK_(!root->IsInside(Vec2Si32(28, 28)),
      "a panel inside a hidden one still claims clicks");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(28, 28)), &gui_messages),
      "the input walk went into a hidden panel");
  dialog->SetVisible(true);
  TEST_CHECK_(root->IsInside(Vec2Si32(28, 28)),
      "the nested panel did not come back with the dialog");

  button->SetVisible(false);
  TEST_CHECK_(!root->IsInside(Vec2Si32(110, 60)),
      "a hidden button still claims clicks");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(110, 60)), &gui_messages),
      "the input walk clicked a hidden button");
  button->SetVisible(true);

  // Negative control: the obvious hand-written test, "is the point inside the
  // root rectangle", answers yes for points that belong to the world, so the
  // checks above are not something any implementation would pass.
  const Vec2Si32 world_point(200, 150);
  const bool naive_says_interface = world_point.x >= root->GetPos().x
    && world_point.y >= root->GetPos().y
    && world_point.x < root->GetPos().x + root->GetSize().x
    && world_point.y < root->GetPos().y + root->GetSize().y;
  TEST_CHECK_(naive_says_interface && !root->IsInside(world_point),
      "a point inside the root rectangle but on no panel: the rectangle test "
      "says %d and IsInside says %d, and they were supposed to differ",
      naive_says_interface ? 1 : 0, root->IsInside(world_point) ? 1 : 0);
}

namespace {

Rgba BackbufferPixel(Vec2Si32 at) {
  Sprite backbuffer = GetEngine()->GetBackbuffer();
  return backbuffer.RgbaData()[backbuffer.StridePixels() * at.y + at.x];
}

InputMessage LeftClickAt(Vec2Si32 at) {
  InputMessage message;
  message.kind = InputMessage::kMouse;
  message.mouse.backbuffer_pos = at;
  message.keyboard.key = kKeyMouseLeft;
  message.keyboard.key_state = 1;
  message.keyboard.state[kKeyMouseLeft] = 1;
  return message;
}

InputMessage TypedLetter(char letter, KeyCode key) {
  InputMessage message;
  message.kind = InputMessage::kKeyboard;
  message.keyboard.key = key;
  message.keyboard.key_state = 1;
  message.keyboard.characters[0] = letter;
  return message;
}

}  // namespace

// A hidden panel is not there: it is not drawn and it takes no input. Button,
// Checkbox and Scrollbar keep that promise through their state, Text checks the
// flag, and Editbox and Progressbar used to draw and (the editbox) take clicks
// and keystrokes while hidden, because only the base class part of them looked
// at the flag.
void test_hidden_editbox_and_progressbar_are_not_there() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  const Rgba kGreen(0, 255, 0, 255);
  Font font;
  font.CreateEmpty(4, 5);
  Sprite red;
  red.Create(60, 12);
  red.Clear(kRed);
  Sprite green;
  green.Create(60, 12);
  green.Clear(kGreen);

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, red, red,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto bar = std::make_shared<Progressbar>(2, Vec2Si32(100, 10), red, green,
      std::vector<Rgba>(1, Rgba(255, 255, 255)), font, 1.0f, 0.0f);
  root->AddChild(bar);
  const Vec2Si32 in_box(20, 15);
  const Vec2Si32 in_bar(110, 15);

  // Positive control: visible, both are drawn where they are.
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(in_box).rgba == kRed.rgba,
      "a visible editbox did not draw its face, so hiding it proves nothing");
  TEST_CHECK_(BackbufferPixel(in_bar).rgba == kRed.rgba,
      "a visible progressbar at zero did not draw its incomplete face");

  box->SetVisible(false);
  bar->SetVisible(false);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(in_box).rgba == kBlack.rgba,
      "a hidden editbox is still drawn");
  TEST_CHECK_(BackbufferPixel(in_bar).rgba == kBlack.rgba,
      "a hidden progressbar is still drawn");

  // A click on a hidden editbox falls through to the world and focuses nothing.
  std::deque<GuiMessage> gui_messages;
  TEST_CHECK_(!root->ApplyInput(LeftClickAt(in_box), &gui_messages),
      "a click on a hidden editbox was taken by the interface");
  TEST_CHECK_(!box->IsFocused(), "a click focused a hidden editbox");
  TEST_CHECK_(!root->IsInside(in_box), "IsInside counts a hidden editbox");

  // The same click on the visible box is taken and focuses it.
  box->SetVisible(true);
  TEST_CHECK_(root->ApplyInput(LeftClickAt(in_box), &gui_messages),
      "a click on a visible editbox was not taken");
  TEST_CHECK_(box->IsFocused(), "a click did not focus a visible editbox");

  // A box hidden while it holds the focus does not type either.
  box->SetVisible(false);
  root->ApplyInput(TypedLetter('a', kKeyA), &gui_messages);
  TEST_CHECK_(box->GetText().empty(),
      "a hidden editbox typed '%s'", box->GetText().c_str());
  box->SetVisible(true);
  root->ApplyInput(TypedLetter('a', kKeyA), &gui_messages);
  TEST_CHECK_(box->GetText() == std::string("a"),
      "the visible editbox did not type, so the hidden check proves nothing");

  // Back in view, both draw again.
  bar->SetVisible(true);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK(BackbufferPixel(in_box).rgba == kRed.rgba);
  TEST_CHECK(BackbufferPixel(in_bar).rgba == kRed.rgba);
}

// IsInside promises to agree with the input walk about every panel. Editbox and
// Scrollbar take clicks in ApplyInput but are not marked clickable, so the base
// answer called them transparent and a click on a field went to the world too.
void test_is_inside_counts_editbox_and_scrollbar() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite face;
  face.Create(60, 12);
  Sprite track;
  track.Create(12, 100);
  Sprite knob;
  knob.Create(12, 12);
  std::deque<GuiMessage> gui_messages;

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, face, face,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto bar = std::make_shared<Scrollbar>(2, Vec2Si32(200, 20), 2,
      track, track, knob, knob, knob, knob, knob, knob, knob, knob, knob,
      0, 10, 0, Scrollbar::kScrollVertical);
  root->AddChild(bar);
  TEST_CHECK_(box->GetSize() == Vec2Si32(60, 12) &&
      bar->GetSize() == Vec2Si32(12, 100),
      "the sizes are %d x %d and %d x %d, the coordinates below assume "
      "60 x 12 and 12 x 100", box->GetSize().x, box->GetSize().y,
      bar->GetSize().x, bar->GetSize().y);

  // Every corner pixel and one past it, the same way the walk sees them.
  const Vec2Si32 box_points[] = {
    Vec2Si32(10, 10), Vec2Si32(69, 21), Vec2Si32(40, 15)
  };
  for (const Vec2Si32 &at : box_points) {
    TEST_CHECK_(root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk did not take a click on the editbox at (%d, %d)", at.x, at.y);
    TEST_CHECK_(root->IsInside(at),
        "IsInside says (%d, %d) on the editbox belongs to the world",
        at.x, at.y);
  }
  const Vec2Si32 bar_points[] = {
    Vec2Si32(200, 20), Vec2Si32(211, 119), Vec2Si32(205, 70)
  };
  for (const Vec2Si32 &at : bar_points) {
    TEST_CHECK_(root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk did not take a click on the scrollbar at (%d, %d)",
        at.x, at.y);
    TEST_CHECK_(root->IsInside(at),
        "IsInside says (%d, %d) on the scrollbar belongs to the world",
        at.x, at.y);
  }
  const Vec2Si32 outside[] = {
    Vec2Si32(9, 10), Vec2Si32(70, 21), Vec2Si32(40, 22),
    Vec2Si32(199, 20), Vec2Si32(212, 119), Vec2Si32(205, 120)
  };
  for (const Vec2Si32 &at : outside) {
    TEST_CHECK_(!root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk took a click beside a panel at (%d, %d)", at.x, at.y);
    TEST_CHECK_(!root->IsInside(at),
        "IsInside claims (%d, %d) beside every panel", at.x, at.y);
  }

  // Hidden, both stop counting, and IsInside follows.
  box->SetVisible(false);
  bar->SetVisible(false);
  TEST_CHECK(!root->ApplyInput(LeftClickAt(Vec2Si32(40, 15)), &gui_messages));
  TEST_CHECK_(!root->IsInside(Vec2Si32(40, 15)),
      "IsInside counts a hidden editbox");
  TEST_CHECK(!root->ApplyInput(LeftClickAt(Vec2Si32(205, 70)), &gui_messages));
  TEST_CHECK_(!root->IsInside(Vec2Si32(205, 70)),
      "IsInside counts a hidden scrollbar");
}

// An anchor means "keep these distances to the edges of the parent". The
// distances used to be measured only inside SetAnchor and only when the panel
// already had a parent, so SetAnchor before AddChild measured nothing, and a
// SetPos after SetAnchor was undone by the next resize of the parent. A dock
// used to take effect only at the next resize of the parent as well.
void test_panel_anchor_and_dock_follow_the_calls() {
  // SetAnchor before AddChild: the distances are measured when the panel gets
  // its parent, from where it is at that moment.
  auto parent = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(400, 300));
  auto early = std::make_shared<Panel>(1, Vec2Si32(20, 30), Vec2Si32(100, 50));
  early->SetAnchor(kAnchorTop | kAnchorRight);
  parent->AddChild(early);
  parent->SetSize(Vec2Si32(500, 400));
  TEST_CHECK_(early->GetPos() == Vec2Si32(120, 130),
      "a panel anchored before AddChild went to (%d, %d) instead of "
      "(120, 130) when the parent grew by 100", early->GetPos().x,
      early->GetPos().y);
  TEST_CHECK_(early->GetSize() == Vec2Si32(100, 50),
      "a top-right anchor changed the size to %d x %d",
      early->GetSize().x, early->GetSize().y);

  // SetPos after SetAnchor: the new place is the one to keep.
  auto moved = std::make_shared<Panel>(2, Vec2Si32(20, 30), Vec2Si32(100, 50));
  parent->AddChild(moved);
  moved->SetAnchor(kAnchorTop | kAnchorRight);
  moved->SetPos(50, 60);
  parent->SetSize(Vec2Si32(600, 500));
  TEST_CHECK_(moved->GetPos() == Vec2Si32(150, 160),
      "a panel moved after SetAnchor went to (%d, %d) instead of (150, 160)",
      moved->GetPos().x, moved->GetPos().y);
  // And the one anchored earlier keeps following.
  TEST_CHECK_(early->GetPos() == Vec2Si32(220, 230),
      "the early panel is at (%d, %d) after the second resize",
      early->GetPos().x, early->GetPos().y);

  // Stretching anchors measured at AddChild time keep both distances.
  auto stretched = std::make_shared<Panel>(3, Vec2Si32(10, 20),
      Vec2Si32(580, 460));
  stretched->SetAnchor(kAnchorLeft | kAnchorRight | kAnchorBottom | kAnchorTop);
  parent->AddChild(stretched);
  parent->SetSize(Vec2Si32(300, 200));
  TEST_CHECK_(stretched->GetPos() == Vec2Si32(10, 20) &&
      stretched->GetSize() == Vec2Si32(280, 160),
      "a fully anchored panel is at (%d, %d) sized %d x %d, expected "
      "(10, 20) and 280 x 160", stretched->GetPos().x, stretched->GetPos().y,
      stretched->GetSize().x, stretched->GetSize().y);

  // A dock takes effect at once, whether set after or before AddChild.
  auto dock_parent = std::make_shared<Panel>(10, Vec2Si32(0, 0),
      Vec2Si32(400, 300));
  auto late_dock = std::make_shared<Panel>(11, Vec2Si32(5, 5),
      Vec2Si32(100, 50));
  dock_parent->AddChild(late_dock);
  late_dock->SetDock(kDockTop | kDockRight);
  TEST_CHECK_(late_dock->GetPos() == Vec2Si32(300, 250),
      "SetDock left the panel at (%d, %d), expected (300, 250) at once",
      late_dock->GetPos().x, late_dock->GetPos().y);
  auto early_dock = std::make_shared<Panel>(12, Vec2Si32(5, 5),
      Vec2Si32(100, 50));
  early_dock->SetDock(kDockLeft | kDockRight | kDockBottom);
  dock_parent->AddChild(early_dock);
  TEST_CHECK_(early_dock->GetPos() == Vec2Si32(0, 0) &&
      early_dock->GetSize() == Vec2Si32(400, 50),
      "a panel docked before AddChild is at (%d, %d) sized %d x %d, expected "
      "(0, 0) and 400 x 50", early_dock->GetPos().x, early_dock->GetPos().y,
      early_dock->GetSize().x, early_dock->GetSize().y);
  dock_parent->SetSize(Vec2Si32(200, 100));
  TEST_CHECK(late_dock->GetPos() == Vec2Si32(100, 50));
  TEST_CHECK(early_dock->GetSize() == Vec2Si32(200, 50));

  // Negative control: a panel with no anchor and no dock stays where it was.
  auto plain = std::make_shared<Panel>(13, Vec2Si32(7, 8), Vec2Si32(10, 10));
  dock_parent->AddChild(plain);
  dock_parent->SetSize(Vec2Si32(300, 300));
  TEST_CHECK_(plain->GetPos() == Vec2Si32(7, 8),
      "a plain panel moved to (%d, %d) on a parent resize",
      plain->GetPos().x, plain->GetPos().y);
}

// Tab walks the visible interface. A hidden panel, and everything inside a
// hidden panel, can not be seen or typed into, so the focus must skip it.
void test_tab_skips_hidden_panels() {
  Sprite face;
  face.Create(40, 20);
  Font font;
  font.CreateEmpty(4, 5);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto make_button = [&](Ui64 tag, Ui32 tab_order) {
    return std::make_shared<Button>(tag, Vec2Si32(10, Si32(tag) * 30), face,
        Sprite(), Sprite(), Sound(), Sound(), kKeyNone, tab_order);
  };
  auto first = make_button(1, 1);
  // An editbox rather than a button: a hidden button already drops out of the
  // walk through IsEnabled(), an editbox does not.
  auto hidden = std::make_shared<Editbox>(2, Vec2Si32(10, 60), 2, face, face,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  auto dialog = std::make_shared<Panel>(3, Vec2Si32(100, 0),
      Vec2Si32(100, 100));
  auto inside_hidden_dialog = make_button(4, 3);
  dialog->AddChild(inside_hidden_dialog);
  auto last = make_button(5, 4);
  root->AddChild(first);
  root->AddChild(hidden);
  root->AddChild(dialog);
  root->AddChild(last);

  // Positive control: with everything visible, Tab visits all four in order.
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "the first Tab did not focus tab order 1");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(hidden->IsFocused(), "the second Tab skipped a visible button");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(inside_hidden_dialog->IsFocused(),
      "the third Tab skipped a button inside a visible dialog");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK(last->IsFocused());
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "Tab did not wrap around to the first");

  hidden->SetVisible(false);
  dialog->SetVisible(false);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(!hidden->IsFocused(), "Tab focused a hidden button");
  TEST_CHECK_(!inside_hidden_dialog->IsFocused(),
      "Tab focused a button inside a hidden dialog");
  TEST_CHECK_(last->IsFocused(),
      "Tab from the first visible button did not land on the last one");
  TEST_CHECK(root->SwitchCurrentTab(false));
  TEST_CHECK_(first->IsFocused() && !hidden->IsFocused() &&
      !inside_hidden_dialog->IsFocused(),
      "Shift+Tab went through the hidden panels on the way back");

  // Shown again, they are back in the walk.
  hidden->SetVisible(true);
  dialog->SetVisible(true);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(hidden->IsFocused(), "a button shown again is still skipped");
}

namespace {

// A panel kind of its own, the way a game would write one: it only overrides
// HandleInput and counts the calls.
class CountingPanel : public Panel {
 public:
  Si32 calls = 0;
  bool is_enabled = true;

  CountingPanel(Vec2Si32 pos, Vec2Si32 size)
    : Panel(0, pos, size) {
  }

  bool IsEnabled() override {
    return is_enabled;
  }

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level, bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override {
    ++calls;
    Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
        out_gui_messages, out_current_tab);
  }
};

}  // namespace

// Whether a panel is there to take input is decided once, in ApplyInput, and
// not by every panel kind for itself. A kind that overrides HandleInput never
// hears about a message while it is hidden or disabled, and neither do the
// kinds the engine ships with.
void test_hidden_and_disabled_panels_never_handle_input() {
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto counting = std::make_shared<CountingPanel>(Vec2Si32(10, 10),
      Vec2Si32(50, 50));
  root->AddChild(counting);
  std::deque<GuiMessage> gui_messages;
  const InputMessage click = LeftClickAt(Vec2Si32(20, 20));

  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1,
      "a visible enabled panel handled the message %d times, expected 1",
      (int)counting->calls);

  counting->SetVisible(false);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1, "a hidden panel handled a message");
  counting->SetVisible(true);

  counting->is_enabled = false;
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1, "a disabled panel handled a message");
  counting->is_enabled = true;

  // A hidden parent stops the walk before the child is asked.
  root->SetVisible(false);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1,
      "a panel inside a hidden one handled a message");
  root->SetVisible(true);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 2,
      "the panel did not come back with its parent: %d calls",
      (int)counting->calls);

  // The engine kinds: a disabled checkbox does not toggle, a disabled
  // scrollbar does not move, and neither says anything.
  Sprite face;
  face.Create(20, 20);
  Sprite track;
  track.Create(12, 100);
  Sprite knob;
  knob.Create(12, 12);
  auto checkbox = std::make_shared<Checkbox>(3, Vec2Si32(100, 10), 2,
      face, face);
  root->AddChild(checkbox);
  auto scrollbar = std::make_shared<Scrollbar>(4, Vec2Si32(200, 20), 3,
      track, track, knob, knob, knob, knob, knob, knob, knob, knob, knob,
      0, 10, 5, Scrollbar::kScrollVertical);
  root->AddChild(scrollbar);
  Si32 checkbox_downs = 0;
  Si32 scroll_changes = 0;
  checkbox->OnButtonDown = [&checkbox_downs]() { ++checkbox_downs; };
  scrollbar->OnScrollChange = [&scroll_changes]() { ++scroll_changes; };

  // Positive control: enabled, a press is heard and the arrow moves the value.
  gui_messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(110, 20)), &gui_messages);
  TEST_CHECK_(checkbox_downs == 1, "a press on an enabled checkbox was lost");
  TEST_CHECK_(!gui_messages.empty() && gui_messages.back().kind == kGuiButtonDown,
      "no kGuiButtonDown was queued for an enabled checkbox");
  // The press lands on the dec arrow of a vertical scrollbar, which is at the
  // bottom of the track.
  root->ApplyInput(LeftClickAt(Vec2Si32(205, 25)), &gui_messages);
  TEST_CHECK_(scroll_changes == 1 && scrollbar->GetValue() == 4,
      "a press on the arrow of an enabled scrollbar changed the value %d "
      "times, value is %d", (int)scroll_changes, (int)scrollbar->GetValue());

  checkbox->SetEnabled(false);
  scrollbar->SetEnabled(false);
  gui_messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(110, 20)), &gui_messages);
  root->ApplyInput(LeftClickAt(Vec2Si32(205, 25)), &gui_messages);
  TEST_CHECK_(checkbox_downs == 1, "a disabled checkbox heard a press");
  TEST_CHECK_(scroll_changes == 1 && scrollbar->GetValue() == 4,
      "a disabled scrollbar moved to %d", (int)scrollbar->GetValue());
  TEST_CHECK_(gui_messages.empty(),
      "%d messages were queued by disabled panels", (int)gui_messages.size());

  // Tab does not stop on a disabled panel either. The press above gave the
  // scrollbar the focus, so the walk is started from nothing.
  auto first = std::make_shared<Button>(5, Vec2Si32(10, 100), face, Sprite(),
      Sprite(), Sound(), Sound(), kKeyNone, 1);
  auto last = std::make_shared<Button>(6, Vec2Si32(50, 100), face, Sprite(),
      Sprite(), Sound(), Sound(), kKeyNone, 4);
  root->AddChild(first);
  root->AddChild(last);
  root->MakeCurrentTab(nullptr);
  TEST_CHECK_(root->FindCurrentTab() == nullptr,
      "the focus was not cleared, so the Tab walk below starts from the wrong "
      "place");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "the first Tab did not land on tab order 1");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(last->IsFocused() && !checkbox->IsFocused() &&
      !scrollbar->IsFocused(),
      "Tab stopped on a disabled checkbox or scrollbar");

  // Enabled again, they are back in the walk: this is the negative control for
  // the check above.
  checkbox->SetEnabled(true);
  scrollbar->SetEnabled(true);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK(first->IsFocused());
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(checkbox->IsFocused(), "an enabled checkbox is skipped by Tab");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(scrollbar->IsFocused(), "an enabled scrollbar is skipped by Tab");
}

// How a run starts is decided before main gets going, from a function the
// application registers. The registration is the only part of it a test can
// reach; the startup code that asks the question runs once, before this.
void test_startup_mode_decider() {
  const bool env_hides = std::getenv("ARCTIC_HEADLESS") != nullptr;
  const bool env_asks_no_window =
      env_hides && std::getenv("ARCTIC_DISABLE_HW") != nullptr;
  if (!env_hides) {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kWindowed,
        "a binary with no decider and a clean environment must start "
        "windowed, got %d", static_cast<int>(arctic::RequestedStartupMode()));
  }

  // The environment wins over the decider, so a run can always be hidden from
  // the outside: that is how the self-tests of a game are run.
  TEST_CHECK(arctic::SetStartupModeDecider(
      []() { return arctic::StartupMode::kNoWindow; }));
  if (!env_hides) {
    TEST_CHECK(arctic::IsHeadlessStartupRequested());
    TEST_CHECK(arctic::RequestedStartupMode()
        == arctic::StartupMode::kNoWindow);
  }

  // A hidden window is its own answer, and must not be mistaken for the mode
  // that has no window and no GL context at all.
  arctic::SetStartupModeDecider(
      []() { return arctic::StartupMode::kHiddenWindow; });
  if (!env_hides) {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kHiddenWindow,
        "a decider asking for a hidden window must be heard, got %d",
        static_cast<int>(arctic::RequestedStartupMode()));
    TEST_CHECK_(!arctic::IsHeadlessStartupRequested(),
        "a hidden window still has a window and a GL context, so it is not a "
        "headless start");
  }

  arctic::SetStartupModeDecider(nullptr);
  if (!env_hides) {
    TEST_CHECK(arctic::RequestedStartupMode()
        == arctic::StartupMode::kWindowed);
  } else if (env_asks_no_window) {
    TEST_CHECK(arctic::RequestedStartupMode()
        == arctic::StartupMode::kNoWindow);
  } else {
    TEST_CHECK_(arctic::RequestedStartupMode()
            == arctic::StartupMode::kHiddenWindow,
        "ARCTIC_HEADLESS on its own means a hidden window, got %d",
        static_cast<int>(arctic::RequestedStartupMode()));
  }
}

namespace {

// The logger writes from a thread of its own and flushes once its queue runs
// dry, so a test that wants to read what it wrote has to give it a moment.
// Waiting for a condition rather than for a fixed time keeps the test both
// quick when the machine is idle and sound when it is not.
bool WaitForLogFile(const std::string &path,
    const std::function<bool(const std::string &)> &is_as_wanted,
    std::string *out_text) {
  for (Si32 attempt = 0; attempt < 200; ++attempt) {
    std::ifstream file(path, std::ios_base::binary);
    std::ostringstream text;
    if (file.is_open()) {
      text << file.rdbuf();
    }
    *out_text = text.str();
    if (is_as_wanted(*out_text)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

bool LogFileHolds(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

}  // namespace

// Two runs of a game land in the same log, and a long session grows it past the
// point where the end of the last run can be found by eye. Both are answered
// here: a run says in the log where and when it started, a run can throw away
// what came before it, and a size limit keeps the newer half short.
void test_log_file_is_findable_clearable_and_rotated() {
  const std::string path = arctic::LogFilePath();
  TEST_CHECK_(!path.empty(), "the log file has no path");
  TEST_CHECK_(path.size() > 8
          && path.compare(path.size() - 8, 8, "/log.txt") == 0,
      "the log path '%s' does not name log.txt", path.c_str());
  TEST_CHECK_(path[0] == '/', "the log path '%s' is not absolute",
      path.c_str());

  const std::string previous_path =
      path.substr(0, path.size() - 8) + "/log_prev.txt";
  const Ui64 limit_before = arctic::LogSizeLimit();
  std::remove(previous_path.c_str());

  // A clear has to be a clear: a marker logged before it must be gone, and one
  // logged after it must be there.
  arctic::SetLogSizeLimit(0);
  *arctic::Log() << "test_log before the clear 0123456789";
  std::string text;
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log before the clear");
      }, &text), "the line logged before the clear never reached the file");
  arctic::ClearLog();
  *arctic::Log() << "test_log after the clear 0123456789";
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log after the clear");
      }, &text), "the line logged after the clear never reached the file");
  TEST_CHECK_(!LogFileHolds(text, "test_log before the clear"),
      "the clear left the older lines in a %d byte log",
      static_cast<Si32>(text.size()));

  // Rotation: with a small limit, enough lines push the older ones out into
  // log_prev.txt, and what is left is short and ends with the newest line.
  arctic::LogRunHeader();
  arctic::SetLogSizeLimit(4096);
  for (Si32 i = 0; i < 400; ++i) {
    *arctic::Log() << "test_log filler line " << i
      << " 0123456789 0123456789 0123456789 0123456789";
  }
  *arctic::Log() << "test_log last line after rotation";
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "test_log last line after rotation");
      }, &text), "the last line never reached the file");
  TEST_CHECK_(text.size() < 4096 + 4096,
      "the log grew to %d bytes with a 4096 byte limit",
      static_cast<Si32>(text.size()));
  TEST_CHECK_(!LogFileHolds(text, "test_log filler line 0 "),
      "the first filler line is still in a log that was rotated");
  // Rotation keeps two files, so the filler that no longer fits is in the
  // previous one -- not necessarily the very first line of it, because that
  // many lines rotate the log more than once and only the last two files are
  // kept, which is the whole point of the limit.
  std::string previous_text;
  TEST_CHECK_(WaitForLogFile(previous_path, [](const std::string &t) {
        return LogFileHolds(t, "test_log filler line ");
      }, &previous_text),
      "the lines pushed out of the log did not end up in log_prev.txt");
  TEST_CHECK_(!LogFileHolds(previous_text, "test_log last line after rotation"),
      "the newest line ended up in log_prev.txt as well");

  arctic::SetLogSizeLimit(limit_before);
  arctic::ClearLog();
  arctic::LogRunHeader();
  TEST_CHECK_(WaitForLogFile(path, [](const std::string &t) {
        return LogFileHolds(t, "=== run started ");
      }, &text), "the run header is not written to a fresh log");
  TEST_CHECK_(LogFileHolds(text, "run: log=" + path),
      "the run header does not tell where the log is: %s", text.c_str());
}

namespace {

Si32 g_close_handler_calls = 0;
bool g_close_handler_answer = false;

bool CountingCloseHandler() {
  ++g_close_handler_calls;
  return g_close_handler_answer;
}

}  // namespace

// The window close arrives from the system, so a test stands in for it by calling
// what the platform code calls. Without a handler the answer is "close now",
// which is what every application had before the handler existed; with one, the
// application decides, and the flag stays raised either way so that a main loop
// can end when it is ready.
void test_main_window_close_handler() {
  TEST_CHECK_(!arctic::IsMainWindowCloseRequested(),
      "the close flag was raised before anybody asked to close");

  arctic::SetMainWindowCloseHandler(nullptr);
  TEST_CHECK_(arctic::OnMainWindowCloseRequested(),
      "a run with no handler refused to close");
  TEST_CHECK_(arctic::IsMainWindowCloseRequested(),
      "the request did not raise the flag");

  g_close_handler_calls = 0;
  g_close_handler_answer = false;
  arctic::SetMainWindowCloseHandler(CountingCloseHandler);
  TEST_CHECK_(!arctic::OnMainWindowCloseRequested(),
      "the engine closed although the handler took the closing over");
  TEST_CHECK_(g_close_handler_calls == 1, "the handler was called %d times",
      (int)g_close_handler_calls);
  TEST_CHECK_(arctic::IsMainWindowCloseRequested(),
      "the flag went down after a refused close");

  g_close_handler_answer = true;
  TEST_CHECK_(arctic::OnMainWindowCloseRequested(),
      "the handler said yes and the engine did not close");
  TEST_CHECK_(g_close_handler_calls == 2,
      "the second request called the handler %d times in total",
      (int)g_close_handler_calls);

  arctic::SetMainWindowCloseHandler(nullptr);
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

static void test_ortho_symmetric_corners(void) {
  float L = -5.f, R = 5.f, B = -3.f, T = 3.f, N = 1.f, F = 100.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  Vec3F lb_near = Transform(m, Vec3F(L, B, -N));
  Vec3F rt_near = Transform(m, Vec3F(R, T, -N));
  Vec3F lb_far  = Transform(m, Vec3F(L, B, -F));
  Vec3F rt_far  = Transform(m, Vec3F(R, T, -F));

  float eps = 1e-5f;
  TEST_CHECK_(fabsf(lb_near.x - (-1.f)) < eps, "left-bottom-near x: got %f, want -1", lb_near.x);
  TEST_CHECK_(fabsf(lb_near.y - (-1.f)) < eps, "left-bottom-near y: got %f, want -1", lb_near.y);
  TEST_CHECK_(fabsf(lb_near.z - (-1.f)) < eps, "left-bottom-near z: got %f, want -1", lb_near.z);

  TEST_CHECK_(fabsf(rt_near.x - 1.f) < eps, "right-top-near x: got %f, want 1", rt_near.x);
  TEST_CHECK_(fabsf(rt_near.y - 1.f) < eps, "right-top-near y: got %f, want 1", rt_near.y);
  TEST_CHECK_(fabsf(rt_near.z - (-1.f)) < eps, "right-top-near z: got %f, want -1", rt_near.z);

  TEST_CHECK_(fabsf(lb_far.z - 1.f) < eps, "left-bottom-far z: got %f, want 1", lb_far.z);
  TEST_CHECK_(fabsf(rt_far.z - 1.f) < eps, "right-top-far z: got %f, want 1", rt_far.z);
}

static void test_ortho_asymmetric_corners(void) {
  float L = -2.f, R = 10.f, B = -3.f, T = 7.f, N = 1.f, F = 50.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  Vec3F bl = Transform(m, Vec3F(L, B, -N));
  Vec3F tr = Transform(m, Vec3F(R, T, -F));
  float cx = (L + R) * 0.5f;
  float cy = (B + T) * 0.5f;
  float cz = -(N + F) * 0.5f;
  Vec3F mid = Transform(m, Vec3F(cx, cy, cz));

  float eps = 1e-4f;
  TEST_CHECK_(fabsf(bl.x - (-1.f)) < eps,
    "bottom-left x: got %f, want -1", bl.x);
  TEST_CHECK_(fabsf(bl.y - (-1.f)) < eps,
    "bottom-left y: got %f, want -1", bl.y);
  TEST_CHECK_(fabsf(bl.z - (-1.f)) < eps,
    "bottom-left z: got %f, want -1", bl.z);

  TEST_CHECK_(fabsf(tr.x - 1.f) < eps,
    "top-right x: got %f, want 1", tr.x);
  TEST_CHECK_(fabsf(tr.y - 1.f) < eps,
    "top-right y: got %f, want 1", tr.y);
  TEST_CHECK_(fabsf(tr.z - 1.f) < eps,
    "top-right z: got %f, want 1", tr.z);

  TEST_CHECK_(fabsf(mid.x) < eps,
    "center x: got %f, want 0", mid.x);
  TEST_CHECK_(fabsf(mid.y) < eps,
    "center y: got %f, want 0", mid.y);
  TEST_CHECK_(fabsf(mid.z) < eps,
    "center z: got %f, want 0", mid.z);
}

static void test_ortho_center_maps_to_origin(void) {
  float L = 10.f, R = 50.f, B = -20.f, T = 80.f, N = 1.f, F = 500.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  float cx = (L + R) * 0.5f;
  float cy = (B + T) * 0.5f;
  float cz = -(N + F) * 0.5f;
  Vec3F center = Transform(m, Vec3F(cx, cy, cz));

  float eps = 1e-4f;
  TEST_CHECK_(fabsf(center.x) < eps,
    "center of box x: got %f, want 0", center.x);
  TEST_CHECK_(fabsf(center.y) < eps,
    "center of box y: got %f, want 0", center.y);
  TEST_CHECK_(fabsf(center.z) < eps,
    "center of box z: got %f, want 0", center.z);
}

static void test_ortho_consistent_with_perspective(void) {
  float N = 1.f, F = 100.f;
  float aspect = 16.f / 9.f;
  float fovy = 60.f * kPi / 180.f;
  float half_h = N * tanf(fovy * 0.5f);
  float half_w = half_h * aspect;

  Mat44F ortho = SetOrtho(-half_w, half_w, -half_h, half_h, N, F);
  Mat44F persp = SetPerspective(fovy * 180.f / kPi, aspect, N, F);

  float eps = 1e-5f;
  TEST_CHECK_(fabsf(ortho.m[0] - persp.m[0] / N) < eps,
    "ortho x-scale should be persp x-scale / near: ortho=%f, persp/n=%f",
    ortho.m[0], persp.m[0] / N);
  TEST_CHECK_(fabsf(ortho.m[5] - persp.m[5] / N) < eps,
    "ortho y-scale should be persp y-scale / near: ortho=%f, persp/n=%f",
    ortho.m[5], persp.m[5] / N);

  TEST_CHECK_(fabsf(ortho.m[3]) < eps,
    "symmetric ortho x-translation should be 0: got %f", ortho.m[3]);
  TEST_CHECK_(fabsf(ortho.m[7]) < eps,
    "symmetric ortho y-translation should be 0: got %f", ortho.m[7]);
}

void test_canonicalize_nonexistent_path() {
  std::string via_nonexistent =
      arctic::CanonicalizePath("./nonexistent_dir_82736/../..");
  std::string via_existing =
      arctic::CanonicalizePath("./..");

  TEST_CHECK_(!via_nonexistent.empty(),
      "CanonicalizePath must not return empty for non-existent path");
  TEST_CHECK_(!via_existing.empty(),
      "CanonicalizePath must not return empty for existing path");
  TEST_CHECK_(via_nonexistent == via_existing,
      "Paths must match: '%s' vs '%s'",
      via_nonexistent.c_str(), via_existing.c_str());

  std::string with_dot = arctic::CanonicalizePath("./.");
  std::string without_dot = arctic::CanonicalizePath(".");
  TEST_CHECK_(with_dot == without_dot,
      "Single dot must collapse: '%s' vs '%s'",
      with_dot.c_str(), without_dot.c_str());

  std::string with_dots = arctic::CanonicalizePath("././.");
  TEST_CHECK_(with_dots == without_dot,
      "Multiple dots must collapse: '%s' vs '%s'",
      with_dots.c_str(), without_dot.c_str());

  std::string into_and_back =
      arctic::CanonicalizePath("./nonexistent_abc_99/..");
  std::string just_here = arctic::CanonicalizePath(".");
  TEST_CHECK_(into_and_back == just_here,
      "Enter and exit dir must equal current: '%s' vs '%s'",
      into_and_back.c_str(), just_here.c_str());

  std::string deep_into_and_back =
      arctic::CanonicalizePath("./aaa_fake/bbb_fake/../..");
  TEST_CHECK_(deep_into_and_back == just_here,
      "Enter two dirs and exit both must equal current: '%s' vs '%s'",
      deep_into_and_back.c_str(), just_here.c_str());
}

void test_canonicalize_before_and_after_create() {
  const char *filename = "./test_canon_tmpfile_93721";

  std::remove(filename);

  std::string before = arctic::CanonicalizePath(filename);
  TEST_CHECK_(!before.empty(),
      "CanonicalizePath must not return empty before file exists");

  {
    std::ofstream ofs(filename);
    TEST_CHECK_(ofs.good(), "Failed to create temp file '%s'", filename);
    ofs << "test";
  }

  std::string after = arctic::CanonicalizePath(filename);
  TEST_CHECK_(!after.empty(),
      "CanonicalizePath must not return empty after file exists");

  TEST_CHECK_(before == after,
      "Canonical path before ('%s') and after ('%s') file creation must match",
      before.c_str(), after.c_str());

  std::remove(filename);
}

// RelativePathFromTo writes the path of every engine file into the project
// files the wizard makes, so an answer that is one ".." off gives a project
// that builds nowhere. A directory can be named with a trailing slash or
// without one; both name the same place, and the two spellings have to give the
// same way out of it.
void test_relative_path_from_to() {
#ifndef ARCTIC_PLATFORM_WINDOWS
  struct Case {
    const char *from;
    const char *to;
    const char *expected;
  };
  static const Case kCases[] = {
    // A sibling of the directory the path starts from: one step up, one down.
    {"/pet/arctic/tests", "/pet/arctic/engine/font.cpp", "../engine/font.cpp"},
    {"/pet/arctic/tests/", "/pet/arctic/engine/font.cpp", "../engine/font.cpp"},
    // Three directories deep, and the trailing slash still counts for nothing.
    {"/a/b/c/d", "/a/x/y.txt", "../../../x/y.txt"},
    {"/a/b/c/d/", "/a/x/y.txt", "../../../x/y.txt"},
    // Nothing in common but the root.
    {"/a/b", "/x/y", "../../x/y"},
    {"/a/b/", "/x/y", "../../x/y"},
    // A file right inside the directory needs no climb at all.
    {"/a/b", "/a/b/main.cpp", "./main.cpp"},
    {"/a/b/", "/a/b/main.cpp", "main.cpp"},
    {"/a/b/", "/a/b/sub/main.cpp", "sub/main.cpp"},
    // The parent is one step up, in all four spellings of the pair.
    {"/a/b/c", "/a/b", "../"},
    {"/a/b/c/", "/a/b/", "../"},
    {"/a/b/c/", "/a/b", "../"},
    {"/a/b/c", "/a/b/", "../"},
    // The same place is "./", again in all four spellings.
    {"/a/b", "/a/b", "./"},
    {"/a/b/", "/a/b/", "./"},
    {"/a/b/", "/a/b", "./"},
    {"/a/b", "/a/b/", "./"},
    // Names that share a prefix of letters are still different names, and the
    // common part has to be cut back to the last whole directory.
    {"/a/bc", "/a/bcd/f", "../bcd/f"},
    {"/a/bcd", "/a/bc", "../bc"},
    // A "." or a ".." inside the path is resolved before anything else.
    {"/a/./b", "/a/c/../c/f", "../c/f"},
  };
  const size_t case_count = sizeof(kCases) / sizeof(kCases[0]);
  for (size_t idx = 0; idx < case_count; ++idx) {
    const Case &one = kCases[idx];
    const std::string actual = RelativePathFromTo(one.from, one.to);
    TEST_CHECK_(actual == std::string(one.expected),
        "RelativePathFromTo(\"%s\", \"%s\") gave \"%s\", expected \"%s\"",
        one.from, one.to, actual.c_str(), one.expected);
  }
#endif

  // The same rule on paths this machine spells itself, so the check holds on
  // Windows too, where the separator and the whole implementation differ: from
  // a directory to a file in a sibling of it there is exactly one step up, and
  // naming that directory with a trailing separator must not add a second one.
  const std::string base = arctic::CanonicalizePath(".");
  TEST_CHECK_(!base.empty(), "the current directory has no canonical form");
  const char separator =
      (base.find('\\') != std::string::npos) ? '\\' : '/';
  const std::string from_plain = arctic::GluePath(base.c_str(), "one_dir");
  const std::string from_slashed = from_plain + separator;
  const std::string target = arctic::GluePath(
      arctic::GluePath(base.c_str(), "two_dir").c_str(), "file.txt");

  const std::string plain = RelativePathFromTo(from_plain.c_str(),
      target.c_str());
  const std::string slashed = RelativePathFromTo(from_slashed.c_str(),
      target.c_str());
  TEST_CHECK_(plain == slashed,
      "a trailing separator changed the answer: \"%s\" became \"%s\"",
      plain.c_str(), slashed.c_str());

  for (Si32 variant = 0; variant < 2; ++variant) {
    const std::string &answer = (variant == 0) ? plain : slashed;
    size_t up_count = 0;
    for (size_t i = 0; i + 1 < answer.size(); ++i) {
      if (answer[i] == '.' && answer[i + 1] == '.') {
        ++up_count;
        ++i;
      }
    }
    TEST_CHECK_(up_count == 1,
        "\"%s\" climbs %d directories, a sibling is exactly one up",
        answer.c_str(), static_cast<int>(up_count));
    TEST_CHECK_(answer.find("two_dir") != std::string::npos,
        "\"%s\" does not lead to the directory that was asked for",
        answer.c_str());
    TEST_CHECK_(answer.find("one_dir") == std::string::npos,
        "\"%s\" still holds the directory it was supposed to leave",
        answer.c_str());
  }
}

// ============================================================================
// FBX tests
// ============================================================================

// Every record of a binary FBX stores the absolute offset of its own end, so
// the tree has to be complete before the offsets can be filled in. A scene is
// collected as FbxNode objects here and serialized by FbxBuild afterwards.
struct FbxNode {
  explicit FbxNode(const char *node_name)
      : name(node_name) {
  }

  FbxNode &Long(Si64 value) {
    props.push_back(static_cast<Ui8>('L'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Int(Si32 value) {
    props.push_back(static_cast<Ui8>('I'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Double(double value) {
    props.push_back(static_cast<Ui8>('D'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Str(const char *value) {
    props.push_back(static_cast<Ui8>('S'));
    Ui32 length = static_cast<Ui32>(strlen(value));
    AppendBytes(&length, sizeof(length));
    AppendBytes(value, length);
    ++prop_count;
    return *this;
  }

  FbxNode &DoubleArray(const std::vector<double> &values) {
    props.push_back(static_cast<Ui8>('d'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(double)));
    AppendBytes(values.data(), values.size() * sizeof(double));
    ++prop_count;
    return *this;
  }

  FbxNode &IntArray(const std::vector<Si32> &values) {
    props.push_back(static_cast<Ui8>('i'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(Si32)));
    AppendBytes(values.data(), values.size() * sizeof(Si32));
    ++prop_count;
    return *this;
  }

  FbxNode &LongArray(const std::vector<Si64> &values) {
    props.push_back(static_cast<Ui8>('l'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(Si64)));
    AppendBytes(values.data(), values.size() * sizeof(Si64));
    ++prop_count;
    return *this;
  }

  FbxNode &FloatArray(const std::vector<float> &values) {
    props.push_back(static_cast<Ui8>('f'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(float)));
    AppendBytes(values.data(), values.size() * sizeof(float));
    ++prop_count;
    return *this;
  }

  FbxNode &Child(const FbxNode &child) {
    children.push_back(child);
    return *this;
  }

  void AppendBytes(const void *data, std::size_t size) {
    if (size == 0) {
      return;
    }
    const Ui8 *bytes = static_cast<const Ui8*>(data);
    props.insert(props.end(), bytes, bytes + size);
  }

  void AppendArrayHeader(Ui32 count, Ui32 byte_count) {
    Ui32 encoding = 0;
    AppendBytes(&count, sizeof(count));
    AppendBytes(&encoding, sizeof(encoding));
    AppendBytes(&byte_count, sizeof(byte_count));
  }

  std::string name;
  std::vector<Ui8> props;
  Ui32 prop_count = 0;
  std::vector<FbxNode> children;
};

// A record that has children is closed by an all-zero record of its own, and
// so is the list of top level records.
static const int kFbxSentinelSize = 13;
static const char kFbxMagic[] = "Kaydara FBX Binary  ";

void FbxAppendUi32(std::vector<Ui8> *out, Ui32 value) {
  const Ui8 *bytes = reinterpret_cast<const Ui8*>(&value);
  out->insert(out->end(), bytes, bytes + sizeof(value));
}

void FbxAppendHeader(std::vector<Ui8> *out, Ui32 version) {
  out->insert(out->end(), kFbxMagic, kFbxMagic + sizeof(kFbxMagic));
  out->push_back(0x1a);
  out->push_back(0x00);
  FbxAppendUi32(out, version);
}

void FbxWriteNode(const FbxNode &node, std::vector<Ui8> *out) {
  const std::size_t end_offset_pos = out->size();
  FbxAppendUi32(out, 0);
  FbxAppendUi32(out, node.prop_count);
  FbxAppendUi32(out, static_cast<Ui32>(node.props.size()));
  out->push_back(static_cast<Ui8>(node.name.size()));
  out->insert(out->end(), node.name.begin(), node.name.end());
  out->insert(out->end(), node.props.begin(), node.props.end());
  if (!node.children.empty()) {
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      FbxWriteNode(node.children[i], out);
    }
    out->insert(out->end(), kFbxSentinelSize, 0);
  }
  const Ui32 end_offset = static_cast<Ui32>(out->size());
  memcpy(&(*out)[end_offset_pos], &end_offset, sizeof(end_offset));
}

std::vector<Ui8> FbxBuild(const std::vector<FbxNode> &roots, Ui32 version) {
  std::vector<Ui8> out;
  FbxAppendHeader(&out, version);
  for (std::size_t i = 0; i < roots.size(); ++i) {
    FbxWriteNode(roots[i], &out);
  }
  out.insert(out.end(), kFbxSentinelSize, 0);
  return out;
}

std::vector<Ui8> FbxBuildScene(const FbxNode &objects,
    const FbxNode &connections) {
  std::vector<FbxNode> roots;
  roots.push_back(objects);
  roots.push_back(connections);
  return FbxBuild(roots, 7400);
}

// One entry of a Properties70 block: a name, the three type strings the
// format carries along, and the value as property number four.
FbxNode FbxIntProperty(const char *name, Si32 value) {
  FbxNode property("P");
  property.Str(name).Str("int").Str("Integer").Str("").Int(value);
  return property;
}

FbxNode FbxDoubleProperty(const char *name, double value) {
  FbxNode property("P");
  property.Str(name).Str("double").Str("Number").Str("").Double(value);
  return property;
}

FbxNode FbxGlobalSettingsNode(const FbxNode &properties) {
  FbxNode settings("GlobalSettings");
  settings.Child(properties);
  return settings;
}

// GlobalSettings sits next to Objects and Connections at the top level.
std::vector<Ui8> FbxBuildSceneWithSettings(const FbxNode &settings,
    const FbxNode &objects, const FbxNode &connections) {
  std::vector<FbxNode> roots;
  roots.push_back(settings);
  roots.push_back(objects);
  roots.push_back(connections);
  return FbxBuild(roots, 7400);
}

// Owns the scene so that a failing TEST_CHECK can return without leaking it.
class FbxScene {
 public:
  explicit FbxScene(const std::vector<Ui8> &data)
      : scene_(ofbx::load(&data[0], static_cast<int>(data.size()))) {
  }

  ~FbxScene() {
    if (scene_ != nullptr) {
      scene_->destroy();
    }
  }

  ofbx::IScene *Get() const {
    return scene_;
  }

 private:
  FbxScene(const FbxScene &);
  FbxScene &operator=(const FbxScene &);

  ofbx::IScene *scene_;
};

std::string FbxToString(const ofbx::DataView &view) {
  if (view.begin == nullptr || view.end == nullptr) {
    return std::string();
  }
  return std::string(reinterpret_cast<const char*>(view.begin),
      reinterpret_cast<const char*>(view.end));
}

const ofbx::Object *FbxFindObject(const ofbx::IScene &scene,
    ofbx::Object::Type type) {
  const ofbx::Object *const *objects = scene.getAllObjects();
  for (int i = 0; i < scene.getAllObjectCount(); ++i) {
    if (objects[i] != nullptr && objects[i]->getType() == type) {
      return objects[i];
    }
  }
  return nullptr;
}

FbxNode FbxTextureNode(Si64 id, const char *filename,
    const char *relative_filename) {
  FbxNode texture("Texture");
  texture.Long(id).Str("Texture::tex").Str("");
  if (filename != nullptr) {
    texture.Child(FbxNode("FileName").Str(filename));
  }
  if (relative_filename != nullptr) {
    texture.Child(FbxNode("RelativeFilename").Str(relative_filename));
  }
  return texture;
}

FbxNode FbxVideoNode(Si64 id, const char *filename_element,
    const char *filename, const char *relative_filename) {
  FbxNode video("Video");
  video.Long(id).Str("Video::clip").Str("Clip");
  video.Child(FbxNode(filename_element).Str(filename));
  if (relative_filename != nullptr) {
    video.Child(FbxNode("RelativeFilename").Str(relative_filename));
  }
  return video;
}

FbxNode FbxMaterialNode(Si64 id) {
  FbxNode material("Material");
  material.Long(id).Str("Material::mat").Str("");
  return material;
}

FbxNode FbxLayeredTextureNode(Si64 id) {
  FbxNode layered("LayeredTexture");
  layered.Long(id).Str("LayeredTexture::layered").Str("");
  return layered;
}

FbxNode FbxConnectOO(Si64 from, Si64 to) {
  FbxNode connection("C");
  connection.Str("OO").Long(from).Long(to);
  return connection;
}

FbxNode FbxConnectOP(Si64 from, Si64 to, const char *property) {
  FbxNode connection("C");
  connection.Str("OP").Long(from).Long(to).Str(property);
  return connection;
}

// Builds a scene of one Material with one Texture bound to it through the
// given connection property, and returns the texture the material resolved
// for the given slot, or an empty string when the slot stayed empty.
std::string FbxResolveMaterialTexture(const char *connection_property,
    ofbx::Texture::TextureType slot) {
  const Si64 kMaterialId = 100;
  const Si64 kTextureId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxTextureNode(kTextureId, "diffuse.png", "tex/diffuse.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOP(kTextureId, kMaterialId,
      connection_property));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (scene.Get() == nullptr) {
    return std::string("<the scene failed to load>");
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (object == nullptr) {
    return std::string("<the material is missing>");
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture = material->getTexture(slot);
  if (texture == nullptr) {
    return std::string();
  }
  return FbxToString(texture->getFileName());
}

void test_fbx_rejects_unsupported_version() {
  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(100, "diffuse.png", "tex/diffuse.png"));
  FbxNode connections("Connections");

  std::vector<FbxNode> roots;
  roots.push_back(objects);
  roots.push_back(connections);
  FbxScene scene(FbxBuild(roots, 6100));

  if (!TEST_CHECK_(scene.Get() == nullptr,
      "FBX 6.1 is below the supported minimum and must not load")) {
    return;
  }
  TEST_CHECK_(strstr(ofbx::getError(), "version") != nullptr,
      "The error must name the version, got '%s'", ofbx::getError());
}

void test_fbx_rejects_a_buffer_too_short_for_a_header() {
  ofbx::IScene *nothing = ofbx::load(nullptr, 0);
  if (!TEST_CHECK_(nothing == nullptr, "An empty buffer must not load")) {
    nothing->destroy();
    return;
  }

  // One byte short of the 27 byte header, which is read as a whole.
  std::vector<Ui8> truncated;
  FbxAppendHeader(&truncated, 7400);
  truncated.pop_back();
  ofbx::IScene *truncated_scene =
      ofbx::load(&truncated[0], static_cast<int>(truncated.size()));
  if (!TEST_CHECK_(truncated_scene == nullptr,
      "A buffer smaller than the header must not load")) {
    truncated_scene->destroy();
    return;
  }
  TEST_CHECK_(strstr(ofbx::getError(), "too short") != nullptr,
      "The error must say the file is too short, got '%s'", ofbx::getError());

  // Exactly a header and not a single record: long enough to be looked at,
  // too short to hold a scene.
  std::vector<Ui8> header_only;
  FbxAppendHeader(&header_only, 7400);
  ofbx::IScene *header_scene =
      ofbx::load(&header_only[0], static_cast<int>(header_only.size()));
  if (!TEST_CHECK_(header_scene == nullptr,
      "A file of nothing but a header must not load")) {
    header_scene->destroy();
  }
}

void test_fbx_rejects_record_past_end_of_file() {
  std::vector<Ui8> data;
  FbxAppendHeader(&data, 7400);
  FbxAppendUi32(&data, 1000);  // the record claims to end past the file
  FbxAppendUi32(&data, 0);
  FbxAppendUi32(&data, 0);
  data.push_back(200);  // and its name is longer than what is left
  data.push_back(static_cast<Ui8>('X'));

  FbxScene scene(data);
  TEST_CHECK_(scene.Get() == nullptr,
      "A record reaching past the end of the file must not load");
}

void test_fbx_empty_scene_loads() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene without objects must load, got error '%s'", ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getRoot() != nullptr,
      "Even an empty scene has a root node");
  TEST_CHECK_(scene.Get()->getMeshCount() == 0,
      "A scene without objects has no meshes, got %d",
      scene.Get()->getMeshCount());
  TEST_CHECK_(scene.Get()->getAllObjectCount() == 0,
      "A scene without objects has no objects, got %d",
      scene.Get()->getAllObjectCount());
}

void test_fbx_mesh_geometry_is_triangulated() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;

  // A unit quad in the z = 0 plane. The last index of a polygon is stored
  // negated and decremented, so 3 becomes -4.
  const double kQuad[12] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[4] = {0, 1, 2, -4};

  FbxNode geometry("Geometry");
  geometry.Long(kGeometryId).Str("Geometry::quad").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kQuad, kQuad + 12)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 4)));

  FbxNode model("Model");
  model.Long(kModelId).Str("Model::quad").Str("Mesh");

  FbxNode objects("Objects");
  objects.Child(geometry);
  objects.Child(model);

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A mesh scene must load, got error '%s'", ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(scene.Get()->getMeshCount() == 1,
      "Expected one mesh, got %d", scene.Get()->getMeshCount())) {
    return;
  }

  const ofbx::Mesh *mesh = scene.Get()->getMesh(0);
  const ofbx::Geometry *geom = mesh->getGeometry();
  if (!TEST_CHECK_(geom != nullptr,
      "The Geometry must be attached to the Model it is connected to")) {
    return;
  }
  if (!TEST_CHECK_(geom->getVertexCount() == 6,
      "A quad must become two triangles, that is 6 vertices, got %d",
      geom->getVertexCount())) {
    return;
  }

  const int kExpected[6] = {0, 1, 2, 0, 2, 3};
  const Vec3D *vertices = geom->getVertices();
  for (int i = 0; i < 6; ++i) {
    const double *expected = kQuad + kExpected[i] * 3;
    TEST_CHECK_(vertices[i].x == expected[0]
        && vertices[i].y == expected[1]
        && vertices[i].z == expected[2],
        "Vertex %d must be (%g, %g, %g), got (%g, %g, %g)",
        i, expected[0], expected[1], expected[2],
        vertices[i].x, vertices[i].y, vertices[i].z);
  }
}

void test_fbx_material_diffuse_color() {
  const Si64 kMaterialId = 100;

  FbxNode diffuse("P");
  diffuse.Str("DiffuseColor").Str("Color").Str("").Str("A");
  diffuse.Double(0.25).Double(0.5).Double(0.75);

  FbxNode properties("Properties70");
  properties.Child(diffuse);

  FbxNode material = FbxMaterialNode(kMaterialId);
  material.Child(properties);

  FbxNode objects("Objects");
  objects.Child(material);
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A material scene must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  ofbx::RgbF color =
      static_cast<const ofbx::Material*>(object)->getDiffuseColor();
  TEST_CHECK_(color.r == 0.25f && color.g == 0.5f && color.b == 0.75f,
      "DiffuseColor must be (0.25, 0.5, 0.75), got (%g, %g, %g)",
      color.r, color.g, color.b);
}

void test_fbx_texture_file_names() {
  const Si64 kTextureId = 100;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, "C:/art/diffuse.png",
      "tex/diffuse.png"));
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A texture scene must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "C:/art/diffuse.png",
      "FileName must be 'C:/art/diffuse.png', got '%s'", filename.c_str());
  TEST_CHECK_(relative == "tex/diffuse.png",
      "RelativeFilename must be 'tex/diffuse.png', got '%s'",
      relative.c_str());
}

void test_fbx_material_texture_slots() {
  std::string diffuse = FbxResolveMaterialTexture("DiffuseColor",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(diffuse == "diffuse.png",
      "A DiffuseColor connection must fill the diffuse slot, got '%s'",
      diffuse.c_str());

  std::string normal = FbxResolveMaterialTexture("NormalMap",
      ofbx::Texture::NORMAL);
  TEST_CHECK_(normal == "diffuse.png",
      "A NormalMap connection must fill the normal slot, got '%s'",
      normal.c_str());

  std::string crossed = FbxResolveMaterialTexture("NormalMap",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(crossed.empty(),
      "A NormalMap connection must leave the diffuse slot empty, got '%s'",
      crossed.c_str());
}

void test_fbx_material_texture_qualified_property() {
  std::string qualified = FbxResolveMaterialTexture("Maya|DiffuseColor",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(qualified == "diffuse.png",
      "An exporter qualified property must still fill the diffuse slot,"
      " got '%s'", qualified.c_str());

  std::string plain = FbxResolveMaterialTexture("Diffuse",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(plain == "diffuse.png",
      "A plain Diffuse property must fill the diffuse slot, got '%s'",
      plain.c_str());
}

void test_fbx_material_ignores_unknown_property() {
  std::string bump = FbxResolveMaterialTexture("Bump",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(bump.empty(),
      "A Bump connection belongs to no supported slot, got '%s'",
      bump.c_str());

  std::string emissive = FbxResolveMaterialTexture("EmissiveColor",
      ofbx::Texture::NORMAL);
  TEST_CHECK_(emissive.empty(),
      "An EmissiveColor connection belongs to no supported slot, got '%s'",
      emissive.c_str());
}

void test_fbx_video_supplies_missing_texture_file_name() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "C:/art/grass.png",
      "tex/grass.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a Video clip must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "C:/art/grass.png",
      "An empty Texture must take FileName from its Video, got '%s'",
      filename.c_str());
  TEST_CHECK_(relative == "tex/grass.png",
      "An empty Texture must take RelativeFilename from its Video, got '%s'",
      relative.c_str());
}

void test_fbx_video_lowercase_file_name_element() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "Filename", "C:/art/brick.png",
      nullptr));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a lowercase Filename must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "C:/art/brick.png",
      "'Filename' must be read like 'FileName', got '%s'", filename.c_str());
}

void test_fbx_video_does_not_override_texture_file_name() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, "own.png", "tex/own.png"));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "clip.png", "tex/clip.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a Video clip must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "own.png",
      "A Texture with a FileName of its own must keep it, got '%s'",
      filename.c_str());
  TEST_CHECK_(relative == "tex/own.png",
      "A Texture with a RelativeFilename of its own must keep it, got '%s'",
      relative.c_str());
}

void test_fbx_layered_texture_reaches_material() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, "layered.png", "tex/layered.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kTextureId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a LayeredTexture must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *material_object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(material_object != nullptr,
      "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(material_object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "A Texture behind a LayeredTexture must reach the material")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "layered.png",
      "The material must resolve to 'layered.png', got '%s'",
      filename.c_str());

  const ofbx::Object *layered_object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE);
  if (!TEST_CHECK_(layered_object != nullptr,
      "The LayeredTexture must be in the scene")) {
    return;
  }
  const ofbx::LayeredTexture *layered =
      static_cast<const ofbx::LayeredTexture*>(layered_object);
  TEST_CHECK_(layered->getTexture() == texture,
      "The LayeredTexture must expose the same Texture the material got");
}

void test_fbx_layered_texture_keeps_the_bottom_layer() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kBottomId = 300;
  const Si64 kTopId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kBottomId, "bottom.png", "tex/bottom.png"));
  objects.Child(FbxTextureNode(kTopId, "top.png", "tex/top.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kBottomId, kLayeredId));
  connections.Child(FbxConnectOO(kTopId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with two layers must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "A LayeredTexture with two layers must still reach the material")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "bottom.png",
      "The layer connected first must win, got '%s'", filename.c_str());
}

void test_fbx_video_behind_layered_texture() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;
  const Si64 kVideoId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "grass.png",
      "tex/grass.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));
  connections.Child(FbxConnectOO(kTextureId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "Maya|DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "Video, LayeredTexture and Material together must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "The material must resolve a texture through the whole chain")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "grass.png",
      "The path must come from the Video at the end of the chain, got '%s'",
      filename.c_str());
}

void test_fbx_tolerates_unexpected_connection() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  const Si64 kTextureId = 300;

  const double kTriangle[9] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[3] = {0, 1, -3};

  FbxNode geometry("Geometry");
  geometry.Long(kGeometryId).Str("Geometry::tri").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kTriangle, kTriangle + 9)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 3)));

  FbxNode model("Model");
  model.Long(kModelId).Str("Model::tri").Str("Mesh");

  FbxNode objects("Objects");
  objects.Child(geometry);
  objects.Child(model);
  objects.Child(FbxTextureNode(kTextureId, "stray.png", nullptr));

  // A Texture hanging directly off a Model is not a link the loader knows how
  // to use, and it must be skipped rather than abort the whole scene.
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  connections.Child(FbxConnectOO(kTextureId, kModelId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "An unusable connection must not fail the load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(scene.Get()->getMeshCount() == 1,
      "The mesh must survive the stray connection, got %d meshes",
      scene.Get()->getMeshCount())) {
    return;
  }
  const ofbx::Geometry *geom = scene.Get()->getMesh(0)->getGeometry();
  TEST_CHECK_(geom != nullptr && geom->getVertexCount() == 3,
      "The geometry must still be attached and hold 3 vertices");
}

void test_fbx_object_types_and_count() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;
  const Si64 kVideoId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, "tex.png", nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "clip.png", nullptr));
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene of four objects must load, got error '%s'", ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getAllObjectCount() == 4,
      "Expected 4 objects, got %d", scene.Get()->getAllObjectCount());

  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL) != nullptr, "Material is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE) != nullptr, "Texture is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::VIDEO) != nullptr, "Video is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE) != nullptr,
      "LayeredTexture is missing");

  const ofbx::Object *video = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::VIDEO);
  if (!TEST_CHECK_(video != nullptr, "Video is missing")) {
    return;
  }
  std::string filename =
      FbxToString(static_cast<const ofbx::Video*>(video)->getFileName());
  TEST_CHECK_(filename == "clip.png",
      "A standalone Video must keep its own FileName, got '%s'",
      filename.c_str());
  TEST_CHECK_(!video->isNode(),
      "A Video is not a scene node and must not be reported as one");

  const ofbx::Object *layered = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE);
  if (!TEST_CHECK_(layered != nullptr, "LayeredTexture is missing")) {
    return;
  }
  TEST_CHECK_(static_cast<const ofbx::LayeredTexture*>(layered)
      ->getTexture() == nullptr,
      "A LayeredTexture with no layer connected must expose no texture");
  TEST_CHECK_(!layered->isNode(),
      "A LayeredTexture is not a scene node and must not be reported as one");
}

// The file numbers its axes from zero (0 = X, 1 = Y, 2 = Z) while UpVector
// numbers them from one. Handing the raw number straight to the enum shifts
// every answer by one axis, which silently turns a Z-up scene into a Y-up one
// and makes UpVector_AxisZ unreachable for any real exporter.
void test_fbx_global_settings_up_axis() {
  struct AxisCase {
    Si32 raw;
    ofbx::UpVector expected;
    const char *axis;
  };
  const AxisCase cases[] = {
    {0, ofbx::UpVector_AxisX, "X"},
    {1, ofbx::UpVector_AxisY, "Y"},
    {2, ofbx::UpVector_AxisZ, "Z"}
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    FbxNode properties("Properties70");
    properties.Child(FbxIntProperty("UpAxis", cases[i].raw));
    FbxNode objects("Objects");
    FbxNode connections("Connections");

    FbxScene scene(FbxBuildSceneWithSettings(
        FbxGlobalSettingsNode(properties), objects, connections));
    if (!TEST_CHECK_(scene.Get() != nullptr,
        "A scene carrying GlobalSettings must load, got error '%s'",
        ofbx::getError())) {
      continue;
    }
    const ofbx::GlobalSettings *settings = scene.Get()->getGlobalSettings();
    if (!TEST_CHECK_(settings != nullptr,
        "GlobalSettings must be reported for UpAxis %d", cases[i].raw)) {
      continue;
    }
    TEST_CHECK_(settings->UpAxis == cases[i].expected,
        "UpAxis %d in the file means %s up, so the enum must read %d, got %d",
        cases[i].raw, cases[i].axis, static_cast<int>(cases[i].expected),
        static_cast<int>(settings->UpAxis));
  }
}

// Only the axis enums are renumbered. The signs, the raw original axis and
// the unit scale are passed through as written, so shifting them too would be
// just as wrong as not shifting UpAxis.
void test_fbx_global_settings_pass_other_fields_through() {
  FbxNode properties("Properties70");
  properties.Child(FbxIntProperty("UpAxis", 2));
  properties.Child(FbxIntProperty("UpAxisSign", -1));
  properties.Child(FbxIntProperty("OriginalUpAxis", 2));
  properties.Child(FbxDoubleProperty("UnitScaleFactor", 2.54));
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildSceneWithSettings(
      FbxGlobalSettingsNode(properties), objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene carrying GlobalSettings must load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  const ofbx::GlobalSettings *settings = scene.Get()->getGlobalSettings();
  if (!TEST_CHECK_(settings != nullptr, "GlobalSettings must be reported")) {
    return;
  }
  TEST_CHECK_(settings->UpAxis == ofbx::UpVector_AxisZ,
      "A file saying UpAxis 2 is Z-up, got enum %d",
      static_cast<int>(settings->UpAxis));
  TEST_CHECK_(settings->UpAxisSign == -1,
      "UpAxisSign is carried through unchanged, got %d",
      settings->UpAxisSign);
  TEST_CHECK_(settings->OriginalUpAxis == 2,
      "OriginalUpAxis stays the raw file value, got %d",
      settings->OriginalUpAxis);
  TEST_CHECK_(std::fabs(settings->UnitScaleFactor - 2.54f) < 1e-4f,
      "UnitScaleFactor is carried through unchanged, got %f",
      settings->UnitScaleFactor);
}

// A file without a GlobalSettings block must not be treated as a broken one.
void test_fbx_missing_global_settings_is_not_an_error() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene without GlobalSettings must load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getGlobalSettings() != nullptr,
      "GlobalSettings must stay readable even when the file omits them");
}

std::vector<double> FbxIdentity16() {
  double m[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  return std::vector<double>(m, m + 16);
}

FbxNode FbxTriangleGeometry(Si64 id) {
  const double kTri[9] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPoly[3] = {0, 1, -3};
  FbxNode geometry("Geometry");
  geometry.Long(id).Str("Geometry::tri").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kTri, kTri + 9)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPoly, kPoly + 3)));
  return geometry;
}

FbxNode FbxQuadGeometry(Si64 id) {
  const double kQuad[12] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[4] = {0, 1, 2, -4};
  FbxNode geometry("Geometry");
  geometry.Long(id).Str("Geometry::quad").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kQuad, kQuad + 12)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 4)));
  return geometry;
}

void test_loadfbx_empty_buffer_fails() {
  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  TEST_CHECK_(!LoadFbx(&model, nullptr, 0, opt),
      "An empty buffer must not produce a model");
  TEST_CHECK_(model.parts.empty(), "A failed load must leave parts empty");
}

void test_loadfbx_scene_without_meshes_fails() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");
  std::vector<Ui8> data = FbxBuildScene(objects, connections);
  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  TEST_CHECK_(!LoadFbx(&model, data.data(), static_cast<Si32>(data.size()),
      opt), "A scene with no mesh must not count as a loaded model");
  TEST_CHECK_(model.parts.empty(), "No mesh means no parts");
  TEST_CHECK_(model.bones.empty() && model.clips.empty(),
      "A file without Skin must leave bones and clips empty");
}

void test_loadfbx_quad_is_two_triangles() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  FbxNode objects("Objects");
  objects.Child(FbxQuadGeometry(kGeometryId));
  FbxNode model_node("Model");
  model_node.Long(kModelId).Str("Model::quad").Str("Mesh");
  objects.Child(model_node);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A quad mesh must load")) {
    return;
  }
  if (!TEST_CHECK_(model.parts.size() == 1,
      "One materialless mesh is one part, got %d",
      static_cast<int>(model.parts.size()))) {
    return;
  }
  Mesh *mesh = model.parts[0].mesh.get();
  if (!TEST_CHECK_(mesh != nullptr, "The part must own a mesh")) {
    return;
  }
  TEST_CHECK_(mesh->GetCurrentVertexCount(0) == 6,
      "A quad is two triangles, 6 vertices, got %d",
      mesh->GetCurrentVertexCount(0));
  TEST_CHECK_(mesh->GetCurrentFaceCount(0) == 2,
      "A quad is two faces, got %d", mesh->GetCurrentFaceCount(0));
  TEST_CHECK_(model.bones.empty() && model.parts[0].skin.size() == 6,
      "A rigid mesh still has a skin list per vertex");
  bool all_empty = true;
  for (size_t i = 0; i < model.parts[0].skin.size(); ++i) {
    if (!model.parts[0].skin[i].empty()) {
      all_empty = false;
    }
  }
  TEST_CHECK_(all_empty, "A file without Skin must have empty influence lists");
  TEST_CHECK_(mesh->mBBox.min_x > -0.01f && mesh->mBBox.max_x < 1.01f,
      "BBox x must cover the unit quad, got %f..%f",
      mesh->mBBox.min_x, mesh->mBBox.max_x);
  TEST_CHECK_(mesh->mBBox.min_y > -0.01f && mesh->mBBox.max_y < 1.01f,
      "BBox y must cover the unit quad, got %f..%f",
      mesh->mBBox.min_y, mesh->mBBox.max_y);
  float *n0 = static_cast<float*>(mesh->GetVertexData(0, 0, 1));
  TEST_CHECK_(n0 != nullptr && n0[2] > 0.9f,
      "A quad in z=0 must get +Z normals, got %f", n0 ? n0[2] : 0.0f);
}

void test_loadfbx_z_up_swaps_y_and_z() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  FbxNode properties("Properties70");
  properties.Child(FbxIntProperty("UpAxis", 2));
  FbxNode objects("Objects");
  objects.Child(FbxQuadGeometry(kGeometryId));
  FbxNode model_node("Model");
  model_node.Long(kModelId).Str("Model::quad").Str("Mesh");
  objects.Child(model_node);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  std::vector<Ui8> data = FbxBuildSceneWithSettings(
      FbxGlobalSettingsNode(properties), objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A Z-up quad must load")) {
    return;
  }
  TEST_CHECK_(model.z_up, "UpAxis Z must be recorded on the model");
  Mesh *mesh = model.parts[0].mesh.get();
  if (!TEST_CHECK_(mesh != nullptr, "The part must own a mesh")) {
    return;
  }
  TEST_CHECK_(mesh->mBBox.max_y - mesh->mBBox.min_y < 0.01f,
      "After Z-up the quad lies in y=0, y span was %f",
      mesh->mBBox.max_y - mesh->mBBox.min_y);
  TEST_CHECK_(mesh->mBBox.min_z < -0.9f && mesh->mBBox.max_z < 0.01f,
      "After Z-up former +Y becomes -Z, bbox z %f..%f",
      mesh->mBBox.min_z, mesh->mBBox.max_z);
}

void test_loadfbx_two_meshes_are_two_parts() {
  const Si64 kModelA = 100;
  const Si64 kGeomA = 200;
  const Si64 kMatA = 300;
  const Si64 kModelB = 400;
  const Si64 kGeomB = 500;
  const Si64 kMatB = 600;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomA));
  objects.Child(FbxTriangleGeometry(kGeomB));
  FbxNode ma("Model");
  ma.Long(kModelA).Str("Model::a").Str("Mesh");
  FbxNode mb("Model");
  mb.Long(kModelB).Str("Model::b").Str("Mesh");
  objects.Child(ma);
  objects.Child(mb);
  FbxNode mata("Material");
  mata.Long(kMatA).Str("Material::red").Str("");
  FbxNode matb("Material");
  matb.Long(kMatB).Str("Material::blue").Str("");
  objects.Child(mata);
  objects.Child(matb);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomA, kModelA));
  connections.Child(FbxConnectOO(kGeomB, kModelB));
  connections.Child(FbxConnectOO(kMatA, kModelA));
  connections.Child(FbxConnectOO(kMatB, kModelB));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "Two meshed materials must load")) {
    return;
  }
  TEST_CHECK_(model.parts.size() == 2,
      "Different materials must not merge, got %d parts",
      static_cast<int>(model.parts.size()));
}

void test_resolve_asset_path_finds_basename_and_stem() {
  Sprite spr;
  spr.Create(4, 4);
  spr.Clear(Rgba(10, 20, 30, 255));
  const char *name = "FbBridgeTex_Case.png";
  spr.Save(name);
  if (!TEST_CHECK_(DoesFileExist(name) == kTrivalentTrue,
      "The fixture PNG must have been written")) {
    return;
  }
  std::vector<std::string> dirs;
  dirs.push_back(".");
  std::string hit = ResolveAssetPath("missing/FbBridgeTex_Case.jpg", dirs);
  TEST_CHECK_(hit.find("FbBridgeTex_Case.png") != std::string::npos,
      "A jpg name must resolve to the png in the search dir, got '%s'",
      hit.c_str());
  std::string case_hit = ResolveAssetPath("fbbridgetex_case.TGA", dirs);
  TEST_CHECK_(case_hit.find("FbBridgeTex_Case.png") != std::string::npos,
      "A case-insensitive stem must resolve, got '%s'", case_hit.c_str());
  std::string missing = ResolveAssetPath("no_such_texture_zzq.png", dirs);
  TEST_CHECK_(missing.empty(),
      "A name that is not on disk must come back empty, got '%s'",
      missing.c_str());
  std::remove(name);
}

void test_gl_texture_cache_identity_and_white() {
  Sprite spr;
  spr.Create(4, 4);
  spr.Clear(Rgba(40, 50, 60, 200));
  const char *name = "FbBridgeCache_Id.png";
  spr.Save(name);
  if (!TEST_CHECK_(DoesFileExist(name) == kTrivalentTrue,
      "The cache fixture PNG must have been written")) {
    return;
  }
  GlTextureCache cache;
  GlTextureCacheEntry a = cache.Load(name);
  GlTextureCacheEntry b = cache.Load(name);
  TEST_CHECK_(a.texture.get() != nullptr,
      "Load of an existing PNG must return a texture object");
  TEST_CHECK_(a.texture.get() == b.texture.get(),
      "Two Load calls of the same file must return the same pointer");
  TEST_CHECK_(a.has_alpha, "A pixel with alpha 200 must set has_alpha");
  GlTextureCacheEntry found = cache.Find(name);
  TEST_CHECK_(found.texture.get() == a.texture.get(),
      "Find must return the entry Load stored");
  std::shared_ptr<GlTexture2D> white = cache.White();
  TEST_CHECK_(white.get() != nullptr, "White() must create a texture");
  TEST_CHECK_(white.get() != a.texture.get(),
      "White must not be the user file");
  std::shared_ptr<GlTexture2D> white2 = cache.White();
  TEST_CHECK_(white.get() == white2.get(),
      "White() must keep the same object");
  GlTextureCacheEntry miss = cache.Load("no_such_cache_tex_zzq.png");
  TEST_CHECK_(miss.texture.get() == nullptr,
      "A missing file must not invent a texture");
  std::remove(name);
}

void test_loadfbx_skin_and_cluster() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kSkinId = 300;
  const Si64 kClusterId = 400;
  const Si64 kLimbId = 500;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  FbxNode limb("Model");
  limb.Long(kLimbId).Str("Model::joint").Str("LimbNode");
  objects.Child(limb);
  FbxNode skin("Deformer");
  skin.Long(kSkinId).Str("Deformer::Skin").Str("Skin");
  objects.Child(skin);
  FbxNode cluster("Deformer");
  cluster.Long(kClusterId).Str("Deformer::Cluster").Str("Cluster");
  const Si32 kIdx[3] = {0, 1, 2};
  const double kW[3] = {1.0, 0.5, 0.25};
  double link[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  link[12] = 3.0;
  link[13] = 4.0;
  link[14] = 5.0;
  cluster.Child(FbxNode("Indexes").IntArray(
      std::vector<Si32>(kIdx, kIdx + 3)));
  cluster.Child(FbxNode("Weights").DoubleArray(
      std::vector<double>(kW, kW + 3)));
  cluster.Child(FbxNode("Transform").DoubleArray(FbxIdentity16()));
  cluster.Child(FbxNode("TransformLink").DoubleArray(
      std::vector<double>(link, link + 16)));
  objects.Child(cluster);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kSkinId, kGeomId));
  connections.Child(FbxConnectOO(kClusterId, kSkinId));
  connections.Child(FbxConnectOO(kLimbId, kClusterId));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A skinned triangle must load, ofbx said '%s'", ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(model.bones.size() == 1,
      "One LimbNode cluster link is one bone, got %d",
      static_cast<int>(model.bones.size()))) {
    return;
  }
  TEST_CHECK_(std::string(model.bones[0].name).find("joint") !=
      std::string::npos,
      "The bone must keep the LimbNode name, got '%s'",
      model.bones[0].name.c_str());
  TEST_CHECK_(std::fabs(model.bones[0].inverse_bind.m[3] - 3.0f) < 1e-4f
      && std::fabs(model.bones[0].inverse_bind.m[7] - 4.0f) < 1e-4f
      && std::fabs(model.bones[0].inverse_bind.m[11] - 5.0f) < 1e-4f,
      "Inverse bind translation must be (3,4,5), got (%f,%f,%f)",
      model.bones[0].inverse_bind.m[3],
      model.bones[0].inverse_bind.m[7],
      model.bones[0].inverse_bind.m[11]);
  if (!TEST_CHECK_(model.parts.size() == 1 &&
      model.parts[0].skin.size() == 3,
      "The triangle must keep 3 skinned vertices")) {
    return;
  }
  const float expect[3] = {1.0f, 0.5f, 0.25f};
  for (int i = 0; i < 3; ++i) {
    if (!TEST_CHECK_(model.parts[0].skin[static_cast<size_t>(i)].size() == 1,
        "Vertex %d must have one influence, got %d",
        i, static_cast<int>(model.parts[0].skin[static_cast<size_t>(i)].size()))) {
      return;
    }
    TEST_CHECK_(model.parts[0].skin[static_cast<size_t>(i)][0].bone == 0,
        "Vertex %d must point at bone 0", i);
    TEST_CHECK_(std::fabs(model.parts[0].skin[static_cast<size_t>(i)][0].weight
        - expect[i]) < 1e-5f,
        "Vertex %d weight must be %f, got %f",
        i, expect[i],
        model.parts[0].skin[static_cast<size_t>(i)][0].weight);
  }
}

void test_loadfbx_animation_curve_keys() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kSkinId = 300;
  const Si64 kClusterId = 400;
  const Si64 kLimbId = 500;
  const Si64 kStackId = 600;
  const Si64 kLayerId = 700;
  const Si64 kNodeId = 800;
  const Si64 kCurveId = 900;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  FbxNode limb("Model");
  limb.Long(kLimbId).Str("Model::joint").Str("LimbNode");
  objects.Child(limb);
  FbxNode skin("Deformer");
  skin.Long(kSkinId).Str("Deformer::Skin").Str("Skin");
  objects.Child(skin);
  FbxNode cluster("Deformer");
  cluster.Long(kClusterId).Str("Deformer::Cluster").Str("Cluster");
  const Si32 kIdx[3] = {0, 1, 2};
  const double kW[3] = {1.0, 1.0, 1.0};
  cluster.Child(FbxNode("Indexes").IntArray(
      std::vector<Si32>(kIdx, kIdx + 3)));
  cluster.Child(FbxNode("Weights").DoubleArray(
      std::vector<double>(kW, kW + 3)));
  cluster.Child(FbxNode("Transform").DoubleArray(FbxIdentity16()));
  cluster.Child(FbxNode("TransformLink").DoubleArray(FbxIdentity16()));
  objects.Child(cluster);
  FbxNode stack("AnimationStack");
  stack.Long(kStackId).Str("AnimStack::clip").Str("");
  objects.Child(stack);
  FbxNode layer("AnimationLayer");
  layer.Long(kLayerId).Str("AnimLayer::base").Str("");
  objects.Child(layer);
  FbxNode cnode("AnimationCurveNode");
  cnode.Long(kNodeId).Str("AnimCurveNode::T").Str("");
  objects.Child(cnode);
  FbxNode curve("AnimationCurve");
  curve.Long(kCurveId).Str("AnimCurve::x").Str("");
  const Si64 kTimes[2] = {0, 46186158000LL};
  const float kVals[2] = {0.0f, 2.5f};
  curve.Child(FbxNode("KeyTime").LongArray(
      std::vector<Si64>(kTimes, kTimes + 2)));
  curve.Child(FbxNode("KeyValueFloat").FloatArray(
      std::vector<float>(kVals, kVals + 2)));
  objects.Child(curve);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kSkinId, kGeomId));
  connections.Child(FbxConnectOO(kClusterId, kSkinId));
  connections.Child(FbxConnectOO(kLimbId, kClusterId));
  connections.Child(FbxConnectOO(kLayerId, kStackId));
  connections.Child(FbxConnectOO(kNodeId, kLayerId));
  connections.Child(FbxConnectOO(kCurveId, kNodeId));
  connections.Child(FbxConnectOP(kNodeId, kLimbId, "Lcl Translation"));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A skinned triangle with a curve must load, ofbx said '%s'",
      ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(!model.clips.empty() && !model.clips[0].curves.empty(),
      "The clip must keep the raw curve, clips=%d",
      static_cast<int>(model.clips.size()))) {
    return;
  }
  const AnimCurve &ac = model.clips[0].curves[0];
  TEST_CHECK_(ac.property.find("Lcl Translation") != std::string::npos,
      "The curve property must be Lcl Translation, got '%s'",
      ac.property.c_str());
  TEST_CHECK_(ac.bone == 0, "The curve must point at bone 0, got %d", ac.bone);
  TEST_CHECK_(ac.axis == 0, "The first connected curve is axis 0, got %d",
      ac.axis);
  if (!TEST_CHECK_(ac.times.size() == 2 && ac.values.size() == 2,
      "Two keys must be copied, got %d times",
      static_cast<int>(ac.times.size()))) {
    return;
  }
  TEST_CHECK_(ac.times[0] == 0 && ac.times[1] == 46186158000LL,
      "Key times must be copied unchanged");
  TEST_CHECK_(std::fabs(ac.values[0] - 0.0f) < 1e-5f
      && std::fabs(ac.values[1] - 2.5f) < 1e-5f,
      "Key values must be copied unchanged, got %f %f",
      ac.values[0], ac.values[1]);
}

void test_loadfbx_texture_path_unresolved() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kMatId = 300;
  const Si64 kTexId = 400;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  objects.Child(FbxMaterialNode(kMatId));
  objects.Child(FbxTextureNode(kTexId, "missing_bridge.png",
      "tex/missing_bridge.png"));
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kMatId, kModelId));
  connections.Child(FbxConnectOP(kTexId, kMatId, "DiffuseColor"));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  opt.search_dirs.push_back(".");
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A textured triangle must load")) {
    return;
  }
  if (!TEST_CHECK_(!model.parts.empty(), "There must be a part")) {
    return;
  }
  TEST_CHECK_(!model.parts[0].raw_tex.empty(),
      "The raw texture path from the file must be kept");
  TEST_CHECK_(model.parts[0].diffuse.get() == nullptr,
      "upload_textures=false must leave the GPU pointer empty");
  TEST_CHECK_(model.parts[0].resolved_tex.empty(),
      "Without a file on disk resolved_tex must be empty, got '%s'",
      model.parts[0].resolved_tex.c_str());
  TEST_CHECK_(model.parts[0].raw_tex.find("missing_bridge") !=
      std::string::npos,
      "The raw name from the file must mention the missing texture");
}

static CollisionTriangle MakeTri(const Vec3F &a, const Vec3F &b,
    const Vec3F &c) {
  CollisionTriangle tri;
  bool ok = tri.Set(a, b, c);
  TEST_CHECK_(ok, "CollisionTriangle::Set must accept a non-degenerate triangle");
  return tri;
}

static bool HitNear(const SphereTriangleHit &hit, float t, float tol) {
  return hit.hit && fabsf(hit.time - t) <= tol;
}

void test_sphere_vs_triangle_degenerate_and_inside() {
  CollisionTriangle tri;
  TEST_CHECK_(!tri.Set(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(0.0f, 0.0f, 0.0f),
      Vec3F(1.0f, 0.0f, 0.0f)),
      "A triangle with a zero-length edge must be rejected");
  TEST_CHECK_(!tri.Set(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(1.0f, 0.0f, 0.0f),
      Vec3F(2.0f, 0.0f, 0.0f)),
      "A collinear triangle must be rejected");

  tri = MakeTri(Vec3F(-1.0f, 0.0f, -1.0f), Vec3F(1.0f, 0.0f, -1.0f),
      Vec3F(0.0f, 0.0f, 1.0f));
  TEST_CHECK_(tri.ContainsPoint(Vec3F(0.0f, 0.0f, 0.0f)),
      "The centroid projection must be inside the triangle");
  TEST_CHECK_(!tri.ContainsPoint(Vec3F(3.0f, 0.0f, 0.0f)),
      "A point well outside the triangle must be rejected");
  TEST_CHECK_(!tri.ContainsPoint(Vec3F(0.0f, 0.0f, -1.0f)),
      "A point on an edge is treated as outside so the edge test owns it");

  Vec3F closest = tri.ClosestPoint(Vec3F(0.0f, 4.0f, 0.0f));
  TEST_CHECK_(fabsf(closest.x) < 1e-4f && fabsf(closest.y) < 1e-4f &&
      fabsf(closest.z) < 1e-4f,
      "Closest point of (0,4,0) to the XZ triangle must be the origin, got "
      "(%f,%f,%f)", closest.x, closest.y, closest.z);

  Vec3F on_vertex = tri.ClosestPoint(Vec3F(-8.0f, 3.0f, -8.0f));
  TEST_CHECK_(fabsf(on_vertex.x + 1.0f) < 1e-4f &&
      fabsf(on_vertex.z + 1.0f) < 1e-4f,
      "Closest point to a far corner must be vertex A, got (%f,%f,%f)",
      on_vertex.x, on_vertex.y, on_vertex.z);
}

void test_sphere_vs_triangle_static_overlap() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  MovingSphere miss(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereOverlapsTriangle(miss, tri),
      "A sphere 3 units above a plane with radius 1 must miss");
  TEST_CHECK_(!SphereTriangleContact(miss, tri).hit,
      "SphereTriangleContact must miss the same far sphere");

  MovingSphere face(Vec3F(0.0f, 0.4f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(face, tri),
      "A sphere whose center is 0.4 above the face with radius 1 must overlap");
  SphereTriangleHit face_hit = SphereTriangleContact(face, tri);
  TEST_CHECK_(face_hit.hit && fabsf(face_hit.time) < 1e-6f,
      "Static face overlap must report time 0");
  TEST_CHECK_(fabsf(face_hit.point.y) < 1e-4f,
      "Static face contact point must lie on the plane, y=%f", face_hit.point.y);

  MovingSphere edge(Vec3F(0.0f, 0.0f, -2.4f), 0.5f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(edge, tri),
      "A sphere 0.4 past the AB edge with radius 0.5 must overlap the edge");

  MovingSphere vertex(Vec3F(-2.3f, 0.0f, -2.0f), 0.4f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(vertex, tri),
      "A sphere 0.3 past vertex A with radius 0.4 must overlap the vertex");

  MovingSphere far_bound(Vec3F(50.0f, 0.0f, 50.0f), 1.0f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereOverlapsTriangle(far_bound, tri),
      "The bounding sphere must reject a distant query without a hit");
}

void test_sphere_vs_triangle_swept_face() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  // Center starts at y=3, radius 1, moves by (0,-4,0). First touch when y=1,
  // so t = (3-1)/4 = 0.5. Leaving the plane would be y=-1 at t=1, and that
  // later root must not be reported.
  MovingSphere toward(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -4.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(toward, tri);
  TEST_CHECK_(HitNear(hit, 0.5f, 1e-4f),
      "Head-on face hit must be t=0.5, got hit=%d t=%f", hit.hit, hit.time);
  TEST_CHECK_(fabsf(hit.point.y) < 1e-3f,
      "Face contact must lie on the plane, y=%f", hit.point.y);
  TEST_CHECK_(hit.normal.y > 0.0f,
      "Face normal must point toward the incoming sphere, ny=%f", hit.normal.y);

  MovingSphere through(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -6.0f, 0.0f));
  SphereTriangleHit enter = SweptSphereVsTriangle(through, tri);
  TEST_CHECK_(HitNear(enter, 1.0f / 3.0f, 1e-3f),
      "A sphere that would exit the plane inside the interval must still "
      "report the entry time 1/3, got hit=%d t=%f", enter.hit, enter.time);

  MovingSphere too_short(Vec3F(0.0f, 3.0f, 0.0f), 1.0f,
      Vec3F(0.0f, -1.5f, 0.0f));
  SphereTriangleHit miss_short = SweptSphereVsTriangle(too_short, tri);
  TEST_CHECK_(!miss_short.hit,
      "Stopping 0.5 above the plane with radius 1 must miss, t=%f",
      miss_short.time);

  MovingSphere away(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, 4.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(away, tri).hit,
      "A sphere moving away from the plane must miss");

  MovingSphere parallel(Vec3F(0.0f, 3.0f, 0.0f), 1.0f,
      Vec3F(4.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(parallel, tri).hit,
      "A sphere moving parallel to the plane and out of the slab must miss");

  // Skin on the plane, moving along it: must slide, not freeze as t=0 overlap.
  MovingSphere rest_slide(Vec3F(0.0f, 1.0f, 0.0f), 1.0f,
      Vec3F(0.5f, 0.0f, 0.0f));
  SphereTriangleHit slide = SweptSphereVsTriangle(rest_slide, tri);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "A sphere resting on the face and moving parallel must not freeze as "
      "t=0 overlap, got hit=%d t=%f", slide.hit, slide.time);

  MovingSphere from_below(Vec3F(0.0f, -3.0f, 0.0f), 1.0f,
      Vec3F(0.0f, 4.0f, 0.0f));
  SphereTriangleHit back = SweptSphereVsTriangle(from_below, tri);
  TEST_CHECK_(HitNear(back, 0.5f, 1e-4f),
      "A hit from the back of the plane must still be found at t=0.5, got "
      "hit=%d t=%f", back.hit, back.time);
}

void test_sphere_vs_triangle_swept_edge_vertex_and_overlap() {
  CollisionTriangle tri = MakeTri(Vec3F(0.0f, 0.0f, 0.0f),
      Vec3F(4.0f, 0.0f, 0.0f), Vec3F(0.0f, 4.0f, 0.0f));

  // Sphere sits in the triangle's plane, so the face test cannot fire, and
  // the AB edge (y=0, z=0) is the first feature. Center y=-3, radius 1,
  // velocity (0,3,0): contact when y=-1, t=2/3.
  MovingSphere to_edge(Vec3F(2.0f, -3.0f, 0.0f), 1.0f, Vec3F(0.0f, 3.0f, 0.0f));
  SphereTriangleHit edge = SweptSphereVsTriangle(to_edge, tri);
  TEST_CHECK_(HitNear(edge, 2.0f / 3.0f, 1e-3f),
      "Edge hit must be t=2/3, got hit=%d t=%f", edge.hit, edge.time);
  TEST_CHECK_(fabsf(edge.point.x - 2.0f) < 1e-3f &&
      fabsf(edge.point.y) < 1e-3f,
      "Edge contact must sit on AB at x=2, got (%f,%f,%f)",
      edge.point.x, edge.point.y, edge.point.z);

  // Vertex A at the origin. Start at (-3,-3,0), radius 1, velocity (3,3,0).
  // |start| = 3*sqrt(2), contact when remaining distance is 1:
  // |t-1| * 3 * sqrt(2) = 1 => t = 1 - 1/(3*sqrt(2)).
  float expect_v = 1.0f - 1.0f / (3.0f * std::sqrt(2.0f));
  MovingSphere to_vertex(Vec3F(-3.0f, -3.0f, 0.0f), 1.0f,
      Vec3F(3.0f, 3.0f, 0.0f));
  SphereTriangleHit vertex = SweptSphereVsTriangle(to_vertex, tri);
  TEST_CHECK_(HitNear(vertex, expect_v, 2e-3f),
      "Vertex hit must be t=%f, got hit=%d t=%f", expect_v, vertex.hit,
      vertex.time);
  TEST_CHECK_(fabsf(vertex.point.x) < 1e-3f && fabsf(vertex.point.y) < 1e-3f,
      "Vertex contact must be at A, got (%f,%f,%f)",
      vertex.point.x, vertex.point.y, vertex.point.z);

  MovingSphere already(Vec3F(1.0f, 0.4f, 0.0f), 1.0f,
      Vec3F(0.0f, 2.0f, 0.0f));
  SphereTriangleHit overlap = SweptSphereVsTriangle(already, tri);
  TEST_CHECK_(overlap.hit && fabsf(overlap.time) < 1e-6f,
      "A sphere that already intersects the triangle must report t=0, got "
      "hit=%d t=%f", overlap.hit, overlap.time);

  MovingSphere still(Vec3F(1.0f, 0.4f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  SphereTriangleHit rest = SweptSphereVsTriangle(still, tri);
  TEST_CHECK_(rest.hit && fabsf(rest.time) < 1e-6f,
      "A resting overlapping sphere (zero velocity) must still report t=0");
  MovingSphere rest_miss(Vec3F(0.0f, 8.0f, 0.0f), 1.0f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(rest_miss, tri).hit,
      "A resting sphere that does not overlap must miss");
}

void test_sphere_vs_triangles_earliest_and_empty() {
  CollisionTriangle near_t = MakeTri(Vec3F(-1.0f, 1.0f, -1.0f),
      Vec3F(1.0f, 1.0f, -1.0f), Vec3F(0.0f, 1.0f, 1.0f));
  CollisionTriangle far_t = MakeTri(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(1.0f, 0.0f, -1.0f), Vec3F(0.0f, 0.0f, 1.0f));
  CollisionTriangle tris[2] = {far_t, near_t};

  // From y=3, radius 1, velocity (0,-3,0). Near plane y=1 is touched at t=1/3,
  // far plane y=0 at t=2/3. The far triangle is listed first so a loop that
  // forgets to keep the earliest hit would return the wrong one.
  MovingSphere sph(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -3.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangles(sph, tris, 2);
  TEST_CHECK_(HitNear(hit, 1.0f / 3.0f, 1e-3f),
      "The earlier of two face hits must win, got hit=%d t=%f",
      hit.hit, hit.time);
  TEST_CHECK_(fabsf(hit.point.y - 1.0f) < 1e-3f,
      "The surviving contact must be on the near plane y=1, y=%f",
      hit.point.y);

  TEST_CHECK_(!SweptSphereVsTriangles(sph, tris, 0).hit,
      "A zero-count list must miss");
  TEST_CHECK_(!SweptSphereVsTriangles(sph, nullptr, 2).hit,
      "A null triangle list must miss");

  // Camera-style query: look-at at the origin, desired camera behind a wall
  // at z=-2. Sphere radius 0.5, offset (0,0,-8). Contact when the sphere
  // first reaches z=-2, i.e. center at z=-1.5, t = 1.5/8.
  CollisionTriangle wall = MakeTri(Vec3F(-5.0f, -5.0f, -2.0f),
      Vec3F(5.0f, -5.0f, -2.0f), Vec3F(0.0f, 5.0f, -2.0f));
  MovingSphere cam(Vec3F(0.0f, 0.0f, 0.0f), 0.5f, Vec3F(0.0f, 0.0f, -8.0f));
  SphereTriangleHit cam_hit = SweptSphereVsTriangle(cam, wall);
  TEST_CHECK_(HitNear(cam_hit, 1.5f / 8.0f, 1e-3f),
      "Camera boom into a wall must stop at t=1.5/8, got hit=%d t=%f",
      cam_hit.hit, cam_hit.time);
  Vec3F cam_pos = cam.center + cam.velocity * cam_hit.time;
  TEST_CHECK_(fabsf(cam_pos.z + 1.5f) < 1e-3f,
      "The camera sphere center must sit 0.5 in front of the wall, z=%f",
      cam_pos.z);
}

void test_line_segment_pierces_triangle() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  float t = -1.0f;
  Vec3F p;
  TEST_CHECK_(LineSegmentPiercesTriangle(Vec3F(0.0f, 1.0f, 0.0f),
      Vec3F(0.0f, -1.0f, 0.0f), tri, &t, &p),
      "A segment from above the face to below it through the centroid must "
      "pierce");
  TEST_CHECK_(fabsf(t - 0.5f) < 1e-4f && fabsf(p.y) < 1e-4f,
      "The pierce must be at t=0.5 on the plane, got t=%f p=(%f,%f,%f)",
      t, p.x, p.y, p.z);

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(8.0f, 1.0f, 0.0f),
      Vec3F(8.0f, -1.0f, 0.0f), tri, nullptr, nullptr),
      "A segment that crosses the plane outside the triangle must not pierce");

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(0.0f, 2.0f, 0.0f),
      Vec3F(0.0f, 1.0f, 0.0f), tri, nullptr, nullptr),
      "Both endpoints above the plane must not pierce");

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(0.0f, 1.0f, 0.0f),
      Vec3F(0.0f, 0.1f, 0.0f), tri, nullptr, nullptr),
      "Stopping short of the plane must not pierce");

  TEST_CHECK_(LineSegmentPiercesTriangle(Vec3F(0.0f, -1.0f, 0.0f),
      Vec3F(0.0f, 1.0f, 0.0f), tri, &t, nullptr),
      "A pierce from below must still count: the center changed sides");
}

void test_swept_sphere_resting_face_does_not_fall_through() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  // Skin on the plane, moving into it. The 0.997-radius "not buried" exception
  // used to skip this so a parallel slide could continue; a vertical drop
  // then missed the face (in-slab, edges far away) and fell through.
  MovingSphere into(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.0f, -2.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(into, tri);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A sphere resting on a face and moving into it must hit at t=0, got "
      "hit=%d t=%f", hit.hit, hit.time);

  MovingSphere slide(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.5f, 0.0f, 0.0f));
  SphereTriangleHit along = SweptSphereVsTriangle(slide, tri);
  TEST_CHECK_(!along.hit || along.time > 1e-4f,
      "A sphere resting on the face and moving parallel must still slide, "
      "got hit=%d t=%f", along.hit, along.time);
}

void test_slope_into_plane_slides_along_tangent() {
  // Plane y = x, outward n = (-1, 1, 0)/sqrt(2). A sphere sitting on it
  // with a horizontal move has an into-plane component, so the sweep
  // freezes at t=0. The remainder after removing that component must stay
  // in the plane (along the slope), not hop along the normal.
  CollisionTriangle slope = MakeTri(Vec3F(0.0f, 0.0f, -2.0f),
      Vec3F(0.0f, 0.0f, 2.0f), Vec3F(4.0f, 4.0f, -2.0f));
  Vec3F n = Normalize(Vec3F(-1.0f, 1.0f, 0.0f));
  Vec3F start(1.0f, 1.0f, 0.0f);
  start = start + n * 1.0f;
  MovingSphere sph(start, 1.0f, Vec3F(1.0f, 0.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(sph, slope);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A horizontal move into a slope the sphere is sitting on must hit at "
      "t=0, got hit=%d t=%f", hit.hit, hit.time);

  Vec3F remain = sph.velocity;
  float vn = Dot(n, remain);
  TEST_CHECK_(vn < 0.0f, "The horizontal move must point into the slope, vn=%f",
      vn);
  remain = remain - n * vn;
  TEST_CHECK_(fabsf(Dot(n, remain)) < 1e-5f,
      "The clipped remainder must lie in the plane, n·remain=%f",
      Dot(n, remain));

  // Downhill at constant Y leaves the plane; a stick-down like the craft
  // uses makes the combined move hit, and the remainder goes down the slope.
  Vec3F down(-1.0f, -1.8f, 0.0f);
  MovingSphere fall(start, 1.0f, down);
  SphereTriangleHit hit_down = SweptSphereVsTriangle(fall, slope);
  TEST_CHECK_(hit_down.hit && hit_down.time < 1e-3f,
      "A downhill move with stick-to-ground must hit the slope at t=0, got "
      "hit=%d t=%f", hit_down.hit, hit_down.time);
  float vn_down = Dot(n, down);
  Vec3F along = down - n * vn_down;
  TEST_CHECK_(fabsf(Dot(n, along)) < 1e-4f,
      "Downhill remainder must stay in the plane, n·along=%f", Dot(n, along));
  TEST_CHECK_(along.y < -1e-4f && along.x < -1e-4f,
      "Downhill remainder must go down the slope, along=(%f,%f,%f)",
      along.x, along.y, along.z);
}

void test_hover_racer_log_drop_through_rock_slope() {
  // Exact numbers from a SPHERE TUNNEL log: the ride sphere sat on a rock
  // face (signed distance ~ radius) then DropRideSphere moved (0, -2.04, 0)
  // and the center crossed that face, landing on the world-box floor.
  const float radius = 0.276000023f;
  Vec3F start(21.3660736f, 7.0842123f, 68.9416199f);
  Vec3F vel(0.0f, -2.03896618f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));

  float pierce_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, start + vel, slope, &pierce_t,
      nullptr),
      "The logged drop must pierce the rock face, so the test is the real miss");
  TEST_CHECK_(fabsf(pierce_t - 0.174380094f) < 1e-4f,
      "Pierce time must match the log (0.17438), got %f", pierce_t);

  float d0 = slope.SignedDistance(start);
  TEST_CHECK_(d0 > 0.0f && d0 < radius,
      "The sphere must start on the outside of the face, d0=%f radius=%f",
      d0, radius);

  MovingSphere sph(start, radius, vel);
  SphereTriangleHit hit = SweptSphereVsTriangle(sph, slope);
  TEST_CHECK_(hit.hit,
      "Dropping a sphere that is already sitting on the rock face must hit "
      "that face, got hit=%d t=%f", hit.hit, hit.time);
  TEST_CHECK_(hit.time < pierce_t,
      "The sphere skin must hit before the center crosses the plane: t=%f "
      "pierce=%f", hit.time, pierce_t);

  CollisionTriangle soup[9];
  soup[0] = MakeTri(Vec3F(199.999985f, 4.54269409f, 199.999985f),
      Vec3F(199.999985f, 4.54269409f, -199.999985f),
      Vec3F(-200.0f, 4.54269409f, -199.999985f));
  soup[1] = MakeTri(Vec3F(199.999985f, 4.54269409f, 199.999985f),
      Vec3F(-200.0f, 4.54269409f, -199.999985f),
      Vec3F(-200.0f, 4.54269409f, 199.999985f));
  soup[2] = MakeTri(Vec3F(199.999985f, 4.50699615f, -199.999985f),
      Vec3F(199.999985f, 4.50699615f, 199.999985f),
      Vec3F(-200.0f, 4.50699615f, 199.999985f));
  soup[3] = MakeTri(Vec3F(199.999985f, 4.50699615f, -199.999985f),
      Vec3F(-200.0f, 4.50699615f, 199.999985f),
      Vec3F(-200.0f, 4.50699615f, -199.999985f));
  soup[4] = MakeTri(Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(22.9879951f, 7.25306082f, 66.0175858f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));
  soup[5] = slope;
  soup[6] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f),
      Vec3F(17.1056366f, 4.1875186f, 67.3578491f));
  soup[7] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(17.1056366f, 4.1875186f, 67.3578491f),
      Vec3F(24.761076f, 4.19974661f, 65.482666f));
  soup[8] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(24.761076f, 4.19974661f, 65.482666f),
      Vec3F(26.5213814f, 4.19974661f, 71.4986954f));

  SphereTriangleHit soup_hit = SweptSphereVsTriangles(sph, soup, 9);
  TEST_CHECK_(soup_hit.hit && soup_hit.time < pierce_t,
      "Among the logged candidates the rock face must win before the center "
      "crosses it, got hit=%d t=%f", soup_hit.hit, soup_hit.time);
  Vec3F stopped = sph.center + sph.velocity * (soup_hit.time * 0.9f);
  TEST_CHECK_(slope.SignedDistance(stopped) > 0.0f,
      "StopAtHit along the drop must leave the center on the outside of the "
      "rock, d=%f", slope.SignedDistance(stopped));
}

void test_hover_racer_log_zero_vel_drop_through_slope() {
  // Later tunnel: SeparateField resolved with wish_vel=0. The sphere was
  // sitting on a rock (d0 == radius). Orthogonal closest-point sat on an
  // edge beyond the radius, so static contact missed; a long DropRideSphere
  // then crossed the face.
  const float radius = 0.276000023f;
  Vec3F start(4.85254049f, 6.21253967f, 37.0080338f);
  Vec3F drop_vel(0.0f, -1.25446129f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(2.69449186f, 5.81134081f, 34.9970665f),
      Vec3F(6.99815607f, 6.21761894f, 39.8411827f),
      Vec3F(8.5653286f, 5.79255247f, 38.8543282f));

  float d0 = slope.SignedDistance(start);
  TEST_CHECK_(d0 > 0.0f && d0 <= radius * 1.002f,
      "The logged start must sit on the outside of the face, d0=%f radius=%f",
      d0, radius);

  Vec3F q = slope.ClosestPoint(start);
  float closest_d = Length(start - q);
  TEST_CHECK_(closest_d > radius,
      "This log is the edge-closest miss: dist to triangle %f must exceed "
      "radius %f so static overlap is not a stand-in for the fix",
      closest_d, radius);

  Vec3F below = start;
  below.y -= radius * 3.0f;
  float support_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, below, slope, &support_t,
      nullptr),
      "A vertical sit probe long enough to cross the plane must pierce");

  MovingSphere probe(start, radius, below - start);
  SphereTriangleHit probe_hit = SweptSphereVsTriangle(probe, slope);
  TEST_CHECK_(probe_hit.hit && probe_hit.time < 1e-3f,
      "A downward probe that would carry the center through the sitting face "
      "must hit at t=0, got hit=%d t=%f", probe_hit.hit, probe_hit.time);

  float pierce_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, start + drop_vel, slope,
      &pierce_t, nullptr),
      "The logged drop must pierce the rock face");
  MovingSphere drop(start, radius, drop_vel);
  SphereTriangleHit drop_hit = SweptSphereVsTriangle(drop, slope);
  TEST_CHECK_(drop_hit.hit && drop_hit.time < pierce_t,
      "The long drop must hit the sitting face before the center crosses it, "
      "got hit=%d t=%f pierce=%f", drop_hit.hit, drop_hit.time, pierce_t);

  // Height queries start a little above the craft. That start is outside
  // the slab, so a pierce check that only fired in-slab missed, the query
  // returned the start as "ground", and ClampHover tossed the craft up.
  Vec3F high = start;
  high.y += 1.0f;
  Vec3F high_vel(0.0f, -2.5f, 0.0f);
  float high_pierce = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(high, high + high_vel, slope,
      &high_pierce, nullptr),
      "A height-query drop from above the sitting pose must still pierce");
  MovingSphere from_above(high, radius, high_vel);
  SphereTriangleHit above_hit = SweptSphereVsTriangle(from_above, slope);
  TEST_CHECK_(above_hit.hit && above_hit.time > 1e-4f &&
      above_hit.time < high_pierce,
      "A drop from above the tilted face must hit when the skin meets the "
      "plane, not only after the center is already in the slab, got hit=%d "
      "t=%f pierce=%f", above_hit.hit, above_hit.time, high_pierce);
}

void test_hover_racer_log_stall_on_shallow_slope() {
  // SPHERE STALL: sitting on a nearly-flat face (ny~0.995). A horizontal
  // wish has a real into-plane component, so the first sweep freezes at
  // t=0. The clipped remainder is in the plane up to float noise; that
  // noise used to count as into-plane and freeze the slide too.
  const float radius = 0.276000023f;
  Vec3F start(22.4530811f, 6.48346138f, 91.1421432f);
  CollisionTriangle slope = MakeTri(
      Vec3F(23.2624588f, 6.34688902f, 86.4868851f),
      Vec3F(21.1819f, 6.34688902f, 90.5246277f),
      Vec3F(22.7512684f, 6.16313601f, 91.507988f));
  Vec3F wish(-0.00658798218f, 5.62667847e-05f, 0.00238037109f);

  MovingSphere sitting(start, radius, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the face");

  MovingSphere into(start, radius, wish);
  SphereTriangleHit hit = SweptSphereVsTriangle(into, slope);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A horizontal wish into the shallow slope must still freeze at t=0, "
      "got hit=%d t=%f", hit.hit, hit.time);

  Vec3F n = slope.n;
  float nl = Length(n);
  TEST_CHECK_(nl > 1e-8f, "The stall face must have a normal");
  n = n / nl;
  if (n.y < 0.0f) {
    n = n * -1.0f;
  }
  Vec3F remain = wish - n * Dot(n, wish);
  TEST_CHECK_(fabsf(Dot(n, remain)) < 1e-6f,
      "Clipped remainder must lie in the plane, n·remain=%f", Dot(n, remain));
  TEST_CHECK_(Length(remain) > 0.5f * Length(wish),
      "Most of the wish must survive as along-slope slide, |remain|=%f "
      "|wish|=%f", Length(remain), Length(wish));

  MovingSphere along(start, radius, remain);
  SphereTriangleHit slide = SweptSphereVsTriangle(along, slope);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "The in-plane remainder must slide, not freeze as t=0, got hit=%d "
      "t=%f", slide.hit, slide.time);

  CollisionTriangle floor = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));
  MovingSphere noisy(Vec3F(0.0f, 1.0f, 0.0f), 1.0f,
      Vec3F(0.5f, -5.0e-6f, 0.0f));
  SphereTriangleHit noisy_hit = SweptSphereVsTriangle(noisy, floor);
  TEST_CHECK_(!noisy_hit.hit || noisy_hit.time > 1e-4f,
      "A 1e-5 relative into-plane leak on a parallel slide must not freeze, "
      "got hit=%d t=%f", noisy_hit.hit, noisy_hit.time);
}

static Vec3F SitContactN(const Vec3F &center, const CollisionTriangle &tri) {
  Vec3F q = tri.ClosestPoint(center);
  Vec3F cn = center - q;
  if (cn.y >= 0.0f && LengthSquared(cn) > 1e-16f) {
    return cn / Length(cn);
  }
  Vec3F n = tri.n;
  float nl = Length(n);
  TEST_CHECK_(nl > 1e-8f, "A sitting triangle must have a normal");
  n = n / nl;
  if (n.y < 0.0f) {
    n = n * -1.0f;
  }
  return n;
}

static Vec3F ClipIntoPlanes(Vec3F vel, const Vec3F *ns, Si32 count) {
  for (Si32 it = 0; it < 8; ++it) {
    bool any = false;
    for (Si32 i = 0; i < count; ++i) {
      float vn = Dot(ns[i], vel);
      if (vn < 0.0f) {
        vel = vel - ns[i] * vn;
        any = true;
      }
    }
    if (!any) {
      break;
    }
  }
  return vel;
}

void test_hover_racer_log_stall_on_slope_crease() {
  // SPHERE STALL at a crease: sitting on a nearly-flat floor (15556) and a
  // steeper neighbor (17349). Clipping only the flattest face left the wish
  // pointing into 17349, so the sweep froze at t=0 and one slide then died
  // on the shared edge.
  const float radius = 0.276000023f;
  Vec3F start(8.64257717f, 6.59866142f, 95.0418701f);
  Vec3F wish(-0.00314235687f, -6.10351562e-05f, 0.00205230713f);
  CollisionTriangle flat = MakeTri(
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(13.1847143f, 6.34688902f, 100.295555f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  CollisionTriangle soup[2];
  soup[0] = flat;
  soup[1] = slope;

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, flat).hit,
      "The logged stall start must be sitting on the flat floor");
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must also be sitting on the steep neighbor");

  MovingSphere into(start, radius, wish);
  SphereTriangleHit raw_slope = SweptSphereVsTriangle(into, slope);
  TEST_CHECK_(raw_slope.hit && raw_slope.time < 1e-3f,
      "A raw wish into the steep neighbor must freeze at t=0, got hit=%d "
      "t=%f", raw_slope.hit, raw_slope.time);

  Vec3F flat_face = flat.n / Length(flat.n);
  if (flat_face.y < 0.0f) {
    flat_face = flat_face * -1.0f;
  }
  Vec3F only_flat[1] = {flat_face};
  Vec3F clipped_flat = ClipIntoPlanes(wish, only_flat, 1);
  MovingSphere still_into(start, radius, clipped_flat);
  SphereTriangleHit still = SweptSphereVsTriangle(still_into, slope);
  TEST_CHECK_(still.hit && still.time < 1e-3f,
      "Clipping only the flattest face must still freeze on the neighbor, "
      "got hit=%d t=%f", still.hit, still.time);

  Vec3F c_flat = SitContactN(start, flat);
  Vec3F c_slope = SitContactN(start, slope);
  TEST_CHECK_(c_flat.y >= 0.0f && c_slope.y >= 0.0f,
      "Sitting contact normals must not push the center down");
  Vec3F both[2] = {c_flat, c_slope};
  Vec3F clipped = ClipIntoPlanes(wish, both, 2);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  float got_xz = sqrtf(clipped.x * clipped.x + clipped.z * clipped.z);
  TEST_CHECK_(got_xz > 0.5f * wish_xz,
      "Clipping every sitting contact must keep most of the XZ wish, "
      "got_xz=%f wish_xz=%f", got_xz, wish_xz);

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit first = SweptSphereVsTriangles(along, soup, 2);
  TEST_CHECK_(!first.hit || first.time > 1e-4f,
      "The remainder clipped against both contacts must not freeze at t=0, "
      "got hit=%d t=%f", first.hit, first.time);

  Vec3F pos = start;
  Vec3F remain = clipped;
  Vec3F clip_ns[8];
  clip_ns[0] = c_flat;
  clip_ns[1] = c_slope;
  Si32 nclip = 2;
  for (Si32 depth = 0; depth < 5; ++depth) {
    if (LengthSquared(remain) <= 1e-12f) {
      break;
    }
    MovingSphere sph(pos, radius, remain);
    SphereTriangleHit cur = SweptSphereVsTriangles(sph, soup, 2);
    if (!cur.hit || cur.time >= 1.0f) {
      pos = pos + remain;
      break;
    }
    pos = pos + remain * (cur.time * 0.9f);
    float time_left = 1.0f - cur.time;
    if (time_left <= 0.15f) {
      break;
    }
    remain = remain * time_left;
    if (nclip < 8) {
      Vec3F n = cur.normal;
      float nl = Length(n);
      clip_ns[nclip] = (nl > 1e-8f) ? n / nl : Vec3F(0.0f, 1.0f, 0.0f);
      ++nclip;
    }
    remain = ClipIntoPlanes(remain, clip_ns, nclip);
  }
  Vec3F moved = pos - start;
  float moved_xz = sqrtf(moved.x * moved.x + moved.z * moved.z);
  TEST_CHECK_(moved_xz > 0.5f * wish_xz,
      "Sweep-and-slide along the crease must keep most of the XZ wish, "
      "moved_xz=%f wish_xz=%f", moved_xz, wish_xz);
}

void test_hover_racer_log_stall_on_shared_edge() {
  // SPHERE STALL: sitting on a slope and a flat floor that share an edge.
  // Closest points are both on that edge, so clipping only the contact
  // radial left the wish pointing into the slope face and froze at t=0.
  const float radius = 0.276000023f;
  Vec3F start(-0.895374835f, 6.63975525f, 95.5743103f);
  Vec3F wish(-0.00185674429f, -4.76837158e-05f, 0.00234985352f);
  CollisionTriangle slope = MakeTri(
      Vec3F(-5.26638556f, 4.1875186f, 92.0922928f),
      Vec3F(-4.99746513f, 6.40157127f, 95.6680527f),
      Vec3F(-0.61086607f, 6.40157127f, 95.7165451f));
  CollisionTriangle flat = MakeTri(
      Vec3F(-4.99746513f, 6.40157127f, 95.6680527f),
      Vec3F(-0.0349920429f, 6.34688902f, 97.5166397f),
      Vec3F(-0.61086607f, 6.40157127f, 95.7165451f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  if (flat.n.y < 0.0f) {
    flat = MakeTri(flat.a, flat.c, flat.b);
  }
  CollisionTriangle soup[2];
  soup[0] = flat;
  soup[1] = slope;

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, flat).hit,
      "The logged stall start must be sitting on the flat floor");
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the slope");

  Vec3F c_flat = SitContactN(start, flat);
  Vec3F c_slope = SitContactN(start, slope);
  Vec3F only_edge[2] = {c_flat, c_slope};
  Vec3F clipped_edge = ClipIntoPlanes(wish, only_edge, 2);
  MovingSphere still_into(start, radius, clipped_edge);
  SphereTriangleHit still = SweptSphereVsTriangle(still_into, slope);
  TEST_CHECK_(still.hit && still.time < 1e-3f,
      "Clipping only the shared-edge radials must still freeze on the "
      "slope face, got hit=%d t=%f", still.hit, still.time);

  Vec3F n_flat = flat.n / Length(flat.n);
  Vec3F n_slope = slope.n / Length(slope.n);
  Vec3F both_faces[4] = {n_flat, n_slope, c_flat, c_slope};
  Vec3F clipped = ClipIntoPlanes(wish, both_faces, 4);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  float got_xz = sqrtf(clipped.x * clipped.x + clipped.z * clipped.z);
  TEST_CHECK_(got_xz > 0.3f * wish_xz,
      "Clipping both face normals must keep along-edge XZ, got_xz=%f "
      "wish_xz=%f", got_xz, wish_xz);

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit first = SweptSphereVsTriangles(along, soup, 2);
  TEST_CHECK_(!first.hit || first.time > 1e-4f,
      "The remainder clipped against both faces must not freeze at t=0, "
      "got hit=%d t=%f", first.hit, first.time);
}

void test_hover_racer_log_slope_hover_spring_fights_sit() {
  // SPHERE STALL on rock 17348: sitting, throttle along +Z, but the hover
  // spring's vertical pull (logged lift about -0.68) is 3D-clipped into the
  // steep face. The remainder runs uphill, XZ freezes, and Y steps down
  // every frame.
  const float radius = 0.276000023f;
  Vec3F start(13.7057219f, 5.5814805f, 91.1003418f);
  Vec3F heading(-0.110253148f, 0.0f, 0.993903518f);
  Vec3F raw(-0.000541687012f, -0.0085849762f, 0.0048828125f);
  CollisionTriangle slope = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  Vec3F n = slope.n / Length(slope.n);
  TEST_CHECK_(n.y > 0.45f && n.y < 0.8f,
      "The logged rock must be a steep floor, n.y=%f", n.y);

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the rock");
  float sd = slope.SignedDistance(start);
  TEST_CHECK_(fabsf(sd - radius) < 0.01f,
      "The sphere must sit on the skin, sd=%f radius=%f", sd, radius);

  float raw_xz = sqrtf(raw.x * raw.x + raw.z * raw.z);
  TEST_CHECK_(fabsf(raw.y) > raw_xz,
      "The logged wish must be dominated by the spring -Y, y=%f xz=%f",
      raw.y, raw_xz);

  Vec3F ns[1] = {n};
  Vec3F clipped = ClipIntoPlanes(raw, ns, 1);
  float wish_along = heading.x * raw.x + heading.z * raw.z;
  float got_along = heading.x * clipped.x + heading.z * clipped.z;
  TEST_CHECK_(wish_along > 0.0f && got_along < 0.0f,
      "Spring -Y clipped into the slope must reverse the throttle, "
      "wish_along=%f got_along=%f", wish_along, got_along);
  TEST_CHECK_(clipped.y < 0.0f,
      "The clipped remainder must still step Y down, y=%f", clipped.y);

  Vec3F throttle = raw;
  throttle.y = 0.0f;
  Vec3F along = ClipIntoPlanes(throttle, ns, 1);
  float keep = heading.x * along.x + heading.z * along.z;
  TEST_CHECK_(keep > 0.3f * wish_along,
      "Throttle without the spring -Y must keep heading, keep=%f wish=%f",
      keep, wish_along);

  MovingSphere go(start, radius, along);
  SphereTriangleHit hit = SweptSphereVsTriangle(go, slope);
  TEST_CHECK_(!hit.hit || hit.time > 1e-4f,
      "The throttle-only remainder must not freeze at t=0, got hit=%d t=%f",
      hit.hit, hit.time);
}

void test_hover_racer_log_still_y_jitter_on_slope() {
  // frame log, standing still at (10.206, y, 95.727): Y climbs to 6.58183
  // then slams to 6.53459 about once a second. speed≈0, lift at the peak
  // is ~0. The hover spring target is gnd+1.6*units; ClampHover min is
  // gnd+1.35*units. The sphere is not sitting, so lift<=0 used to call
  // DropRideSphere and yank Y down to the min, then the spring climbed
  // back.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  Vec3F peak(10.206274f, 6.58182907f, 95.7274475f);
  Vec3F dropped(10.206274f, 6.53458691f, 95.7274475f);
  TEST_CHECK_(fabsf(peak.x - dropped.x) < 1e-5f
          && fabsf(peak.z - dropped.z) < 1e-5f,
      "The logged jitter is vertical: XZ must stay put");
  TEST_CHECK_(peak.y - dropped.y > 0.04f && peak.y - dropped.y < 0.06f,
      "The logged Y slam must be about 0.047, dy=%f", peak.y - dropped.y);

  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  TEST_CHECK_(slope.n.y / Length(slope.n) > 0.45f,
      "The logged still pose sits above a driveable slope, n.y=%f",
      slope.n.y / Length(slope.n));

  const float length = 0.776229f;
  const float yaw = 8.45870113f;
  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, slope).hit,
      "At the hover peak the sphere is not sitting, so lift<=0 used to "
      "DropRideSphere");

  Vec3F probe(peak.x, peak.y + 1.2f * units, peak.z);
  float drop_len = probe.y - (peak.y - 2.0f);
  MovingSphere down(probe, radius, Vec3F(0.0f, -drop_len, 0.0f));
  SphereTriangleHit gnd_hit = SweptSphereVsTriangle(down, slope);
  TEST_CHECK_(gnd_hit.hit && gnd_hit.time < 1.0f,
      "A vertical height sample at the logged XZ must hit the slope");
  float t = gnd_hit.time * 0.9f;
  if (t < 0.0f) {
    t = 0.0f;
  }
  float gnd = (probe.y - drop_len * t) - radius;
  float hover = gnd + kHoverHeight * units;
  float min_y = gnd + 1.35f * units;
  float ceiling = gnd + kMaxHover * units;
  TEST_CHECK_(fabsf(hover - peak.y) < 0.03f,
      "The logged peak must be the hover-spring target, hover=%f peak=%f",
      hover, peak.y);
  TEST_CHECK_(fabsf(min_y - dropped.y) < 0.03f,
      "The logged slam must land on ClampHover min, min=%f dropped=%f",
      min_y, dropped.y);
  TEST_CHECK_(peak.y >= min_y && peak.y <= ceiling,
      "The still peak is inside the hover band, min=%f peak=%f ceil=%f",
      min_y, peak.y, ceiling);
  TEST_CHECK_(dropped.y >= min_y - 0.01f && dropped.y <= ceiling,
      "The still slam is also inside the hover band, min=%f dropped=%f",
      min_y, dropped.y);

  // Old rule: !supported && lift<=0 -> DropRideSphere. That is true here
  // and is what slammed Y. A pose already in the hover band must not drop.
  bool same = (peak.y - gnd) < kMaxHover * units * 3.0f;
  bool in_band = same && peak.y >= min_y && peak.y <= ceiling;
  TEST_CHECK_(in_band,
      "The logged still pose is in the hover band and must not be treated "
      "as a fall");
}

// Vertical height sample used by hover_racer QueryHeightBelow / ClampHover:
// drop a sphere, stop at 0.9 * t, report center.y - radius. Only floors.
static float HeightBelowFloors(float x, float z, float from_y, float radius,
    const CollisionTriangle *tris, Si32 count) {
  float dest = from_y - 4.0f;
  float drop = from_y - dest;
  if (drop <= 1e-4f) {
    return from_y - radius;
  }
  MovingSphere sph(Vec3F(x, from_y, z), radius, Vec3F(0.0f, -drop, 0.0f));
  SphereTriangleHit best;
  for (Si32 i = 0; i < count; ++i) {
    if (tris[i].n.y < 0.45f) {
      continue;
    }
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, tris[i]);
    if (hit.hit && hit.time < 1.0f && (!best.hit || hit.time < best.time)) {
      best = hit;
    }
  }
  if (!best.hit || best.time >= 1.0f) {
    return dest - radius;
  }
  float t = best.time * 0.9f;
  if (t < 0.0f) {
    t = 0.0f;
  }
  return (from_y - drop * t) - radius;
}

void test_hover_racer_log_tiny_reverse_y_drop() {
  // New-session log t=25.775: inching backward at ~0.045, Y climbs with
  // lift>0 to 5.35974 then slams to 5.28226 in one frame (dxz=0.00055).
  // Repeats every ~4s at the same crease. The ride sphere sits ahead of
  // the craft on rock 17348; a vertical sample at the craft XZ can switch
  // to the shared-edge neighbor or the dirt and ClampHover / DropRideSphere
  // then yanks Y down to that sample's ceiling.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 1.99329352f;
  Vec3F peak(11.4969978f, 5.35974407f, 90.7669067f);
  Vec3F dropped(11.4964914f, 5.2822566f, 90.7671356f);
  TEST_CHECK_(peak.y - dropped.y > 0.07f && peak.y - dropped.y < 0.09f,
      "The logged slam must be about 0.078, dy=%f", peak.y - dropped.y);
  float dxz = sqrtf((dropped.x - peak.x) * (dropped.x - peak.x)
      + (dropped.z - peak.z) * (dropped.z - peak.z));
  TEST_CHECK_(dxz < 0.001f,
      "The logged slam is a tiny reverse, dxz=%f", dxz);

  CollisionTriangle rock = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f));
  CollisionTriangle neighbor = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f));
  CollisionTriangle dirt = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f),
      Vec3F(19.019783f, 4.19974661f, 97.1494217f));
  if (rock.n.y < 0.0f) {
    rock = MakeTri(rock.a, rock.c, rock.b);
  }
  if (neighbor.n.y < 0.0f) {
    neighbor = MakeTri(neighbor.a, neighbor.c, neighbor.b);
  }
  if (dirt.n.y < 0.0f) {
    dirt = MakeTri(dirt.a, dirt.c, dirt.b);
  }
  CollisionTriangle soup[3] = {rock, neighbor, dirt};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, rock).hit,
      "At the hover peak the offset sphere is not sitting, so lift<=0 "
      "used to DropRideSphere");
  float sd = rock.SignedDistance(peak_sph);
  TEST_CHECK_(sd > radius && (sd - radius) < kMaxHover * units,
      "The sphere is still on the outside of the rock within hover, "
      "sd=%f radius=%f", sd, radius);

  float gnd_peak = HeightBelowFloors(peak.x, peak.z,
      peak.y + 1.2f * units, radius, soup, 3);
  float hover = gnd_peak + kHoverHeight * units;
  TEST_CHECK_(fabsf(hover - peak.y) < 0.03f,
      "The logged peak must be the hover-spring target, hover=%f peak=%f "
      "gnd=%f", hover, peak.y, gnd_peak);

  float gnd_move = HeightBelowFloors(dropped.x, dropped.z,
      peak.y + 1.2f * units, radius, soup, 3);
  float min_y = gnd_move + 1.35f * units;
  float ceiling = gnd_move + kMaxHover * units;
  bool same = (peak.y - gnd_move) < kMaxHover * units * 3.0f;
  bool old_in_band = same && peak.y >= min_y && peak.y <= ceiling;
  TEST_CHECK_(!old_in_band,
      "The tight ceiling band is what treated this reverse as a fall, "
      "gnd=%f min=%f peak=%f ceil=%f",
      gnd_move, min_y, peak.y, ceiling);
  TEST_CHECK_(same,
      "The craft is still on the same surface and must not be dropped, "
      "gnd=%f peak=%f", gnd_move, peak.y);

  // Dropping the offset sphere onto the rock lands near the logged slam.
  float drop_len = peak_sph.y - (peak_sph.y - 2.0f);
  MovingSphere down(peak_sph, radius, Vec3F(0.0f, -drop_len, 0.0f));
  SphereTriangleHit drop_hit = SweptSphereVsTriangle(down, rock);
  TEST_CHECK_(drop_hit.hit && drop_hit.time < 1.0f,
      "DropRideSphere from the peak must hit the rock");
  float dt = drop_hit.time * 0.9f;
  if (dt < 0.0f) {
    dt = 0.0f;
  }
  Vec3F sat = peak_sph + Vec3F(0.0f, -drop_len * dt, 0.0f);
  Vec3F after_drop = sat - sph_off;
  TEST_CHECK_(fabsf(after_drop.y - dropped.y) < 0.04f,
      "The logged slam is DropRideSphere onto the offset sphere's rock, "
      "got y=%f logged=%f", after_drop.y, dropped.y);
}

void test_hover_racer_log_tiny_forward_hover_dip() {
  // New-session log t=69.64..70.04: standing at (4.780, 6.701, 93.523),
  // a small forward nudge, Y climbs 0.005 then lift goes negative and Y
  // drops to 6.639, then the spring climbs back. A vertical sample at the
  // craft XZ is below the sit pose, so hover+0.35u extra damping treated
  // the craft as too high the moment it left sit.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 0.549734771f;
  Vec3F rest(4.77962112f, 6.70127916f, 93.5233002f);
  Vec3F peak(4.78457499f, 6.70613527f, 93.5289764f);
  Vec3F dipped(4.80102396f, 6.63940144f, 93.5558167f);
  TEST_CHECK_(peak.y - rest.y < 0.01f,
      "The climb before the dip is a tiny forward follow, dy=%f",
      peak.y - rest.y);
  TEST_CHECK_(peak.y - dipped.y > 0.05f && peak.y - dipped.y < 0.08f,
      "The logged dip must be about 0.067, dy=%f", peak.y - dipped.y);
  float dxz = sqrtf((dipped.x - rest.x) * (dipped.x - rest.x)
      + (dipped.z - rest.z) * (dipped.z - rest.z));
  TEST_CHECK_(dxz < 0.05f,
      "The logged dip is a small forward move, dxz=%f", dxz);

  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  CollisionTriangle soup[1] = {slope};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F rest_sph = rest + sph_off;
  float sd = slope.SignedDistance(rest_sph);
  TEST_CHECK_(sd > 0.0f && sd < radius * 1.2f,
      "At rest the sphere is still on the slope skin, sd=%f radius=%f",
      sd, radius);

  float gnd = HeightBelowFloors(peak.x, peak.z, peak.y + 1.2f * units,
      radius, soup, 1);
  float hover = gnd + kHoverHeight * units;
  float extra = hover + 0.35f * units;
  TEST_CHECK_(fabsf(hover - dipped.y) < 0.05f,
      "The logged dip lands on the vertical hover sample, hover=%f "
      "dipped=%f gnd=%f", hover, dipped.y, gnd);
  TEST_CHECK_(peak.y > extra,
      "The old extra-damping test fired: peak=%f extra=%f", peak.y, extra);

  // A drop of kMaxHover from the ride sphere still hits the slope, so the
  // craft is not airborne. Extra damping must not yank it to the vertical
  // sample.
  Vec3F peak_sph = peak + sph_off;
  float probe = kMaxHover * units;
  MovingSphere down(peak_sph, radius, Vec3F(0.0f, -probe, 0.0f));
  SphereTriangleHit floor = SweptSphereVsTriangle(down, slope);
  TEST_CHECK_(floor.hit && floor.time < 1.0f,
      "The ride sphere still sees the slope under a kMaxHover drop, "
      "hit=%d t=%f", floor.hit, floor.time);
}

// True when a kMaxHover-style drop from the ride sphere still hits a floor.
static bool SphereDropHitsFloor(const Vec3F &center, float radius, float probe,
    const CollisionTriangle *tris, Si32 count) {
  if (probe <= 1e-4f) {
    return false;
  }
  MovingSphere sph(center, radius, Vec3F(0.0f, -probe, 0.0f));
  for (Si32 i = 0; i < count; ++i) {
    if (tris[i].n.y < 0.45f) {
      continue;
    }
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, tris[i]);
    if (hit.hit && hit.time < 1.0f) {
      return true;
    }
  }
  return false;
}

void test_hover_racer_log_downhill_stair_shake() {
  // New-session log t=34.51..35.97: downhill along yaw=7.185 at ~1.7-1.9.
  // Y stuck at 6.5612 while XZ moved, then dropped in ~0.045 stairs every
  // ~0.13s as lift ramped 0 -> -0.65 and snapped back to 0. Treating a
  // kMaxHover floor probe like sitting zeroed the hover spring, so the
  // craft flew level until the slope fell out of the probe, extra damping
  // yanked down, the probe hit again, repeat.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 7.18459272f;
  const float slope = 0.16132f;
  const float dt = 0.016f;
  const float speed = 1.73053694f;
  Vec3F plateau(16.0958557f, 6.56121826f, 52.6059456f);
  Vec3F end_pos(18.1920567f, 6.12999725f, 54.2646255f);
  TEST_CHECK_(plateau.y - end_pos.y > 0.40f && plateau.y - end_pos.y < 0.46f,
      "The logged downhill must drop about 0.43, dy=%f",
      plateau.y - end_pos.y);
  float path_xz = sqrtf((end_pos.x - plateau.x) * (end_pos.x - plateau.x)
      + (end_pos.z - plateau.z) * (end_pos.z - plateau.z));
  TEST_CHECK_(path_xz > 2.5f && path_xz < 2.9f,
      "The logged downhill XZ is about 2.67, xz=%f", path_xz);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F right(cosf(yaw), 0.0f, -sinf(yaw));
  float gnd0 = plateau.y - kHoverHeight * units;
  auto plane_pt = [&](float along, float side) {
    Vec3F p = Vec3F(plateau.x, 0.0f, plateau.z) + heading * along
        + right * side;
    p.y = gnd0 - slope * along;
    return p;
  };
  CollisionTriangle floor = MakeTri(plane_pt(-3.0f, -8.0f),
      plane_pt(10.0f, -8.0f), plane_pt(3.0f, 8.0f));
  if (floor.n.y < 0.0f) {
    floor = MakeTri(floor.a, floor.c, floor.b);
  }
  TEST_CHECK_(floor.n.y / Length(floor.n) > 0.45f,
      "The downhill fixture must be a driveable floor, n.y=%f",
      floor.n.y / Length(floor.n));
  CollisionTriangle soup[1] = {floor};

  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F start_sph = plateau + sph_off;
  MovingSphere at_start(start_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_start, floor).hit,
      "At the logged plateau the offset sphere is not sitting");
  float probe = kMaxHover * units;
  TEST_CHECK_(SphereDropHitsFloor(start_sph, radius, probe, soup, 1),
      "A kMaxHover drop from the plateau still hits the slope, so the old "
      "SeesFloor branch zeroed lift and skipped the spring");

  float gnd = HeightBelowFloors(plateau.x, plateau.z,
      plateau.y + 1.2f * units, radius, soup, 1);
  float hover0 = gnd + kHoverHeight * units;
  TEST_CHECK_(fabsf(hover0 - plateau.y) < 0.03f,
      "The logged plateau is the hover target on this slope, hover=%f "
      "y=%f gnd=%f", hover0, plateau.y, gnd);

  struct DownhillRun {
    float y_at_064;
    float hover_at_064;
    float y_end;
    float hover_end;
    Si32 lift0_frames;
    Si32 stair_resets;
    float min_lift;
  };
  auto simulate = [&](bool kill_spring_when_sees) {
    Vec3F pos = plateau;
    float lift = 0.0f;
    DownhillRun run;
    run.y_at_064 = plateau.y;
    run.hover_at_064 = hover0;
    run.lift0_frames = 0;
    run.stair_resets = 0;
    run.min_lift = 0.0f;
    bool was_neg = false;
    const Si32 kFrames = 90;
    for (Si32 i = 0; i < kFrames; ++i) {
      float ground = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
          radius, soup, 1);
      float hover = ground + kHoverHeight * units;
      bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
      Vec3F sph = pos + sph_off;
      bool sitting = SphereTriangleContact(
          MovingSphere(sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f)),
          floor).hit;
      bool sees = SphereDropHitsFloor(sph, radius, probe, soup, 1);
      if (kill_spring_when_sees && (sitting || sees)) {
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      } else if (sitting) {
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      } else {
        float ground_sph = HeightBelowFloors(sph.x, sph.z,
            sph.y + 1.2f * units, radius, soup, 1);
        float hover_sph = ground_sph + kHoverHeight * units;
        float err = hover_sph - pos.y;
        if (!same) {
          err = 0.0f;
        }
        // Crest: the sphere is already on top, so do not yank toward a
        // lower sample behind the craft. Downhill: sphere ground is lower
        // and the spring still follows.
        if (ground_sph + 1.0e-3f >= ground) {
          if (err < 0.0f) {
            err = 0.0f;
          }
          if (lift < 0.0f) {
            lift = 0.0f;
          }
        }
        lift += err * 22.0f * dt;
        lift -= lift * 5.5f * dt;
        if (same && pos.y > hover_sph + 0.35f * units && !sees) {
          lift -= 16.0f * units * dt;
        }
      }
      pos = pos + heading * (speed * dt);
      pos.y += lift * dt;
      float min_y = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
          radius, soup, 1) + 1.35f * units;
      if (pos.y < min_y) {
        pos.y = min_y;
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      }
      if (fabsf(lift) < 1e-4f) {
        run.lift0_frames += 1;
      }
      if (lift < run.min_lift) {
        run.min_lift = lift;
      }
      if (was_neg && lift > -1e-4f) {
        run.stair_resets += 1;
      }
      was_neg = lift < -0.05f;
      float t = dt * static_cast<float>(i + 1);
      if (fabsf(t - 0.64f) <= dt * 0.51f) {
        run.y_at_064 = pos.y;
        run.hover_at_064 = hover;
      }
    }
    run.y_end = pos.y;
    run.hover_end = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
        radius, soup, 1) + kHoverHeight * units;
    return run;
  };

  DownhillRun old_run = simulate(true);
  TEST_CHECK_(fabsf(old_run.y_at_064 - plateau.y) < 0.005f,
      "SeesFloor killing the spring flies level: y=%f start=%f",
      old_run.y_at_064, plateau.y);
  TEST_CHECK_(hover0 - old_run.hover_at_064 > 0.10f,
      "The slope under that level flight must have dropped, hover=%f "
      "start=%f", old_run.hover_at_064, hover0);
  TEST_CHECK_(old_run.lift0_frames > 40,
      "The old policy spends most of the downhill at lift=0, frames=%d",
      old_run.lift0_frames);
  TEST_CHECK_(old_run.stair_resets >= 1,
      "After the floor falls out of kMaxHover the spring yanks and "
      "SeesFloor snaps lift back to 0, resets=%d", old_run.stair_resets);

  DownhillRun fixed = simulate(false);
  TEST_CHECK_(plateau.y - fixed.y_at_064 > 0.05f,
      "The hover spring must follow the falling target, y=%f start=%f",
      fixed.y_at_064, plateau.y);
  TEST_CHECK_(fixed.y_at_064 - fixed.hover_at_064 < 0.15f,
      "Followed Y must stay near hover, y=%f hover=%f",
      fixed.y_at_064, fixed.hover_at_064);
  TEST_CHECK_(fixed.min_lift < -0.05f,
      "Downhill hover must keep negative lift, min=%f", fixed.min_lift);
  TEST_CHECK_(fixed.stair_resets == 0,
      "Following the slope must not snap lift to 0 each step, resets=%d",
      fixed.stair_resets);
  TEST_CHECK_(plateau.y - fixed.y_end > 0.25f,
      "After 1.4s the craft must have descended with the slope, dy=%f",
      plateau.y - fixed.y_end);
}

void test_hover_racer_log_crest_fall_instead_of_level() {
  // New-session log t=167.34: climbed the hill at (-76.49, 9.072, -21.64)
  // to the ridge, then in 0.37s Y slammed to 8.939 with lift to -0.55.
  // Approach 17042 and plateau 14407 share the high edge at y=8.703.
  // A vertical sample at the craft XZ can still hit the steep face while
  // the offset sphere is already over the top. Following that lower hover
  // yanks the craft back down the hill instead of rolling onto the level.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 10.5985184f;
  const float dt = 0.016f;
  Vec3F peak(-76.4902496f, 9.07220936f, -21.6412392f);
  Vec3F dropped(-76.5291214f, 8.93892479f, -21.6575317f);
  TEST_CHECK_(peak.y - dropped.y > 0.12f && peak.y - dropped.y < 0.15f,
      "The logged crest slam must be about 0.133, dy=%f",
      peak.y - dropped.y);

  CollisionTriangle slope = MakeTri(
      Vec3F(-72.7282562f, 8.70340347f, -27.6387997f),
      Vec3F(-76.8840714f, 8.70340347f, -21.2898083f),
      Vec3F(-73.0484543f, 6.46007729f, -23.2444916f));
  CollisionTriangle plateau = MakeTri(
      Vec3F(-76.8840714f, 8.70340347f, -21.2898083f),
      Vec3F(-72.7282562f, 8.70340347f, -27.6387997f),
      Vec3F(-76.4116516f, 7.36986589f, -31.3163109f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  if (plateau.n.y < 0.0f) {
    plateau = MakeTri(plateau.a, plateau.c, plateau.b);
  }
  TEST_CHECK_(slope.n.y / Length(slope.n) > 0.45f
          && slope.n.y / Length(slope.n) < 0.85f,
      "The logged approach must be a steep floor, n.y=%f",
      slope.n.y / Length(slope.n));
  TEST_CHECK_(plateau.n.y / Length(plateau.n) > 0.9f,
      "The logged top must be a nearly-flat level, n.y=%f",
      plateau.n.y / Length(plateau.n));
  CollisionTriangle soup[2] = {slope, plateau};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, slope).hit
          && !SphereTriangleContact(at_peak, plateau).hit,
      "At the logged crest peak the sphere is not sitting on either face");
  float probe = kMaxHover * units;
  TEST_CHECK_(SphereDropHitsFloor(peak_sph, radius, probe, soup, 2),
      "A kMaxHover drop from the crest still hits a floor");

  float gnd_craft = HeightBelowFloors(peak.x, peak.z,
      peak.y + 1.2f * units, radius, soup, 2);
  float gnd_sph = HeightBelowFloors(peak_sph.x, peak_sph.z,
      peak_sph.y + 1.2f * units, radius, soup, 2);
  float hover_craft = gnd_craft + kHoverHeight * units;
  float hover_sph = gnd_sph + kHoverHeight * units;
  TEST_CHECK_(peak.y > hover_craft + 0.05f,
      "The logged peak sits above the craft-XZ hover, y=%f hover=%f gnd=%f",
      peak.y, hover_craft, gnd_craft);
  TEST_CHECK_(gnd_sph + 1.0e-3f >= gnd_craft,
      "This is a crest: the sphere's floor is not downhill of the craft, "
      "gnd_sph=%f gnd_craft=%f", gnd_sph, gnd_craft);

  // Old downhill-follow: always pull toward hover. At the crest that is
  // a yank down onto the approach / off the level.
  float lift = 0.0f;
  Vec3F pos = peak;
  for (Si32 i = 0; i < 24; ++i) {
    float ground = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
        radius, soup, 2);
    float hover = ground + kHoverHeight * units;
    bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
    float err = same ? hover - pos.y : 0.0f;
    lift += err * 22.0f * dt;
    lift -= lift * 5.5f * dt;
    pos = pos + heading * (0.15f * dt);
    pos.y += lift * dt;
  }
  TEST_CHECK_(peak.y - pos.y > 0.08f,
      "Following craft-XZ hover at the crest must drop Y, dy=%f y=%f",
      peak.y - pos.y, pos.y);
  TEST_CHECK_(pos.y < hover_sph + 0.04f,
      "The old yank lands near or below the top hover, y=%f hover_sph=%f",
      pos.y, hover_sph);

  // Fixed: only follow a falling hover when the sphere's floor is lower
  // (true downhill). At a crest keep Y on the top.
  lift = 0.0f;
  pos = peak;
  for (Si32 i = 0; i < 24; ++i) {
    Vec3F sph = pos + sph_off;
    float ground_craft = HeightBelowFloors(pos.x, pos.z,
        pos.y + 1.2f * units, radius, soup, 2);
    float ground = HeightBelowFloors(sph.x, sph.z, sph.y + 1.2f * units,
        radius, soup, 2);
    float hover = ground + kHoverHeight * units;
    bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
    float err = same ? hover - pos.y : 0.0f;
    if (ground + 1.0e-3f >= ground_craft) {
      if (err < 0.0f) {
        err = 0.0f;
      }
      if (lift < 0.0f) {
        lift = 0.0f;
      }
    }
    lift += err * 22.0f * dt;
    lift -= lift * 5.5f * dt;
    pos = pos + heading * (0.15f * dt);
    pos.y += lift * dt;
  }
  TEST_CHECK_(peak.y - pos.y < 0.05f,
      "A crest must not yank Y down the approach, dy=%f y=%f",
      peak.y - pos.y, pos.y);
  TEST_CHECK_(pos.y > dropped.y + 0.05f,
      "The craft must stay above the logged slam onto the approach, "
      "y=%f slammed=%f", pos.y, dropped.y);
}

// The craft's leftover-velocity clip. A floor, a ceiling or a driveable slope
// keeps the move in its plane; a wall only pushes it out horizontally, so the
// remainder cannot be turned into a hop up a steep face. tangent picks the
// fixed wall rule: cancel the whole into-wall component instead of only the
// XZ part of the move.
static Vec3F ClipSlideRule(const Vec3F &remain, const Vec3F &n,
    bool tangent) {
  float vn = Dot(n, remain);
  if (vn >= 0.0f) {
    return remain;
  }
  if (n.y >= 0.45f || n.y <= -0.45f) {
    return remain - n * vn;
  }
  Vec3F n_xz(n.x, 0.0f, n.z);
  float nl = Length(n_xz);
  if (nl < 1e-5f) {
    return remain;
  }
  n_xz = n_xz / nl;
  Vec3F out = remain;
  if (tangent) {
    float push = vn / nl;
    out.x -= n_xz.x * push;
    out.z -= n_xz.z * push;
    return out;
  }
  float vxz = n_xz.x * remain.x + n_xz.z * remain.z;
  if (vxz < 0.0f) {
    out.x -= n_xz.x * vxz;
    out.z -= n_xz.z * vxz;
  }
  return out;
}

static Vec3F ClipAllSlides(Vec3F vel, const Vec3F *ns, Si32 count,
    bool tangent) {
  for (Si32 it = 0; it < 8; ++it) {
    bool any = false;
    for (Si32 i = 0; i < count; ++i) {
      Vec3F before = vel;
      vel = ClipSlideRule(vel, ns[i], tangent);
      if (LengthSquared(vel - before) > 1e-20f) {
        any = true;
      }
    }
    if (!any) {
      break;
    }
  }
  return vel;
}

// The craft's sweep-and-slide: clip the wish against every plane it already
// sits on, stop at 0.9 of the first hit, clip the remainder against that
// plane too, and repeat up to five times.
static Vec3F SweepAndSlide(const Vec3F &start, const Vec3F &wish,
    float radius, const CollisionTriangle *soup, Si32 count,
    const Vec3F *sits, Si32 sit_count, bool tangent,
    Vec3F *out_wall = nullptr) {
  Vec3F ns[8];
  Si32 nns = 0;
  for (Si32 i = 0; i < sit_count && nns < 8; ++i) {
    ns[nns] = sits[i];
    ++nns;
  }
  Vec3F at = start;
  Vec3F remain = ClipAllSlides(wish, ns, nns, tangent);
  if (out_wall != nullptr) {
    *out_wall = Vec3F(0.0f, 0.0f, 0.0f);
  }
  for (Si32 depth = 0; depth < 5; ++depth) {
    if (LengthSquared(remain) <= 1e-12f) {
      break;
    }
    MovingSphere sph(at, radius, remain);
    SphereTriangleHit cur = SweptSphereVsTriangles(sph, soup, count);
    // The craft learns about a wall from this first hit alone.
    if (depth == 0 && out_wall != nullptr && cur.hit && cur.time < 1.0f) {
      Vec3F n = cur.normal;
      float nl = Length(n);
      if (nl > 1e-8f) {
        n = n / nl;
        Vec3F n_xz(n.x, 0.0f, n.z);
        if (n.y < 0.45f && Length(n_xz) > 1e-5f) {
          *out_wall = Normalize(n_xz);
        }
      }
    }
    if (!cur.hit || cur.time >= 1.0f) {
      at = at + remain;
      break;
    }
    at = at + remain * (cur.time * 0.9f);
    float time_left = 1.0f - cur.time;
    if (time_left <= 0.15f) {
      break;
    }
    remain = remain * time_left;
    if (nns < 8) {
      float nl = Length(cur.normal);
      ns[nns] = (nl > 1e-8f) ? cur.normal / nl : Vec3F(0.0f, 1.0f, 0.0f);
      ++nns;
    }
    remain = ClipAllSlides(remain, ns, nns, tangent);
  }
  return at;
}

// The planes the ride sphere already rests on, gathered the way the craft
// gathers them: floors it sits on, the underside of rock it touches, and,
// once walls are held as contacts, the faces it rests against sideways.
static Si32 RestingPlanes(const Vec3F &center, float radius,
    const CollisionTriangle *soup, Si32 count, bool hold_walls, Vec3F *out) {
  const float skin = 0.06f * 0.140449f;
  Si32 n = 0;
  for (Si32 i = 0; i < count && n < 8; ++i) {
    const CollisionTriangle &tri = soup[i];
    Vec3F face = tri.n / Length(tri.n);
    MovingSphere sph(center, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
    bool touch = SphereTriangleContact(sph, tri).hit;
    if (face.y >= 0.45f) {
      bool sit = touch;
      if (!sit && fabsf(tri.SignedDistance(center)) <= radius * 1.05f) {
        Vec3F below = center;
        below.y -= radius * 3.0f;
        sit = LineSegmentPiercesTriangle(center, below, tri, nullptr, nullptr);
      }
      if (sit) {
        out[n++] = face;
      }
      continue;
    }
    if (face.y <= -0.45f) {
      if (touch) {
        out[n++] = Normalize(center - tri.ClosestPoint(center));
      }
      continue;
    }
    if (!hold_walls
        || Length(center - tri.ClosestPoint(center)) > radius + skin) {
      continue;
    }
    Vec3F n_xz(face.x, 0.0f, face.z);
    if (Length(n_xz) < 1e-5f) {
      continue;
    }
    n_xz = Normalize(n_xz);
    if (tri.SignedDistance(center) < 0.0f) {
      n_xz = n_xz * -1.0f;
    }
    out[n++] = n_xz;
  }
  return n;
}

void test_hover_racer_log_wall_corner_dead_stop() {
  // End of the log: from t=140.53 to t=146.55 the craft stood at
  // (81.8646, 5.8239, -49.4292) with full throttle and speed charging back
  // up to 1.96, and the position never changed by a single bit. The ride
  // sphere is against rock face 17683 and rides slope 15708. The slope is
  // not a contact, but a downward segment crosses it inside the sit slab,
  // so its face normal clips the wish -- and that clip tilts the wish
  // upward. The wall's normal dips below the horizon, so the XZ-only wall
  // clip left that rise pointing into the wall. The sphere touches the
  // wall, so every sweep froze at t=0 and all five slides moved nothing.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -1.29992986f;
  const float speed = 1.96254206f;
  const float lift = 0.00145309512f;
  const float dt = 0.012f;
  Vec3F pos(81.8645935f, 5.82387304f, -49.429184f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle wall_low = MakeTri(
      Vec3F(82.3298798f, 5.8710408f, -50.3318481f),
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle wall_flat = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, wall_low, wall_flat};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F wall_n = wall_flat.n / Length(wall_flat.n);
  Vec3F low_n = wall_low.n / Length(wall_low.n);
  TEST_CHECK_(slope_n.y > 0.9f,
      "The logged slope must be a driveable floor, n.y=%f", slope_n.y);
  TEST_CHECK_(wall_n.y < 0.0f && wall_n.y > -0.45f,
      "The logged rock face must be a wall whose normal dips below the "
      "horizon, n.y=%f", wall_n.y);
  TEST_CHECK_(low_n.y > -0.45f && low_n.y < 0.45f,
      "The neighboring face must be a wall too, n.y=%f", low_n.y);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F ride = pos + sph_off;

  // The slope is a sit constraint through the pierce branch, not a contact.
  MovingSphere resting(ride, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(resting, slope).hit,
      "The ride sphere does not touch the slope it rides");
  float slope_sd = slope.SignedDistance(ride);
  TEST_CHECK_(slope_sd > 0.0f && slope_sd <= radius * 1.05f,
      "The slope must be inside the sit slab, sd=%f slab=%f", slope_sd,
      radius * 1.05f);
  Vec3F below = ride;
  below.y -= radius * 3.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(ride, below, slope, nullptr,
      nullptr),
      "A downward segment must cross the slope, which is what makes it a "
      "sit constraint");
  // The wall is a contact, so any move into it freezes the sweep at t=0.
  TEST_CHECK_(SphereTriangleContact(
      MovingSphere(ride, radius, Vec3F(0.0f, 0.0f, 0.0f)), wall_flat).hit,
      "The ride sphere must be touching the wall");

  Vec3F wish = heading * (speed * dt) + Vec3F(0.0f, lift * dt, 0.0f);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  TEST_CHECK_(fabsf(wish_xz - 0.0235505f) < 1e-4f,
      "The logged wish is about 0.02355 of XZ, got %f", wish_xz);
  TEST_CHECK_(fabsf(wish.y) < 1e-4f,
      "The logged wish is level, y=%f", wish.y);

  Vec3F on_slope = ClipSlideRule(wish, slope_n, false);
  TEST_CHECK_(on_slope.y > 0.004f,
      "Clipping the wish into the slope must tilt it upward, y=%f",
      on_slope.y);
  float sqz = wall_n.x * on_slope.x + wall_n.z * on_slope.z;
  float sy = wall_n.y * on_slope.y;
  TEST_CHECK_(sy < -1e-5f,
      "The rise must point into the wall, n.y*wish.y=%f", sy);

  Vec3F old_clip = ClipSlideRule(on_slope, wall_n, false);
  float old_xz = wall_n.x * old_clip.x + wall_n.z * old_clip.z;
  TEST_CHECK_(fabsf(old_xz) < 1e-4f,
      "The XZ-only clip must cancel the XZ part of the move, xz dot=%f",
      old_xz);
  float old_dot = Dot(wall_n, old_clip);
  TEST_CHECK_(old_dot < -1e-5f,
      "The XZ-only clip must still leave the remainder pointing into the "
      "wall, n.remain=%f (xz part %f, y part %f)", old_dot, sqz, sy);
  TEST_CHECK_(fabsf(old_dot) > 1e-3f * Length(old_clip),
      "That leftover is far above a parallel-slide rounding leak: "
      "|n.remain|=%f |remain|=%f", fabsf(old_dot), Length(old_clip));
  MovingSphere old_along(ride, radius, old_clip);
  SphereTriangleHit frozen = SweptSphereVsTriangle(old_along, wall_flat);
  TEST_CHECK_(frozen.hit && frozen.time < 1e-4f,
      "So the swept test freezes the XZ-only remainder at t=0, got hit=%d "
      "t=%f", frozen.hit, frozen.time);

  Vec3F new_clip = ClipSlideRule(on_slope, wall_n, true);
  TEST_CHECK_(fabsf(Dot(wall_n, new_clip)) < 1e-6f,
      "The fixed wall clip must leave the remainder in the wall plane, "
      "n.remain=%f", Dot(wall_n, new_clip));
  TEST_CHECK_(fabsf(new_clip.y - on_slope.y) < 1e-7f,
      "The fixed wall clip must not touch the vertical motion, y=%f was %f",
      new_clip.y, on_slope.y);
  MovingSphere new_along(ride, radius, new_clip);
  SphereTriangleHit slides = SweptSphereVsTriangle(new_along, wall_flat);
  TEST_CHECK_(!slides.hit || slides.time > 1e-4f,
      "The in-plane remainder must slide along the wall, got hit=%d t=%f",
      slides.hit, slides.time);

  // A horizontal remainder must be clipped exactly as before.
  Vec3F level(-0.02f, 0.0f, 0.006f);
  Vec3F level_old = ClipSlideRule(level, wall_n, false);
  Vec3F level_new = ClipSlideRule(level, wall_n, true);
  TEST_CHECK_(Length(level_new - level_old) < 1e-6f,
      "For a level move the fixed rule must match the old one, old=(%f,%f,%f)"
      " new=(%f,%f,%f)", level_old.x, level_old.y, level_old.z, level_new.x,
      level_new.y, level_new.z);
  // A wall may still not lift the remainder up a steep face.
  Vec3F steep_n = Normalize(Vec3F(0.9f, 0.4f, 0.0f));
  Vec3F falling(-0.02f, -0.03f, 0.0f);
  Vec3F kept = ClipSlideRule(falling, steep_n, true);
  TEST_CHECK_(fabsf(kept.y - falling.y) < 1e-7f,
      "A steep face must not turn a fall into a climb, y=%f was %f", kept.y,
      falling.y);

  Vec3F sits[1] = {slope_n};
  Vec3F stuck = SweepAndSlide(ride, wish, radius, soup, 3, sits, 1, false);
  Vec3F stuck_moved = stuck - ride;
  float stuck_xz = sqrtf(stuck_moved.x * stuck_moved.x
      + stuck_moved.z * stuck_moved.z);
  TEST_CHECK_(stuck_xz < 1e-5f,
      "The logged dead stop: five slides against the wall move nothing, "
      "xz=%f wish_xz=%f", stuck_xz, wish_xz);

  Vec3F at = ride;
  float least = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F next = SweepAndSlide(at, wish, radius, soup, 3, sits, 1, true);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least) {
      least = step_xz;
    }
    at = next;
    MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "Sliding along the wall must not enter it, frame %d at (%f,%f,%f)",
        i, at.x, at.y, at.z);
  }
  TEST_CHECK_(least > 0.85f * wish_xz,
      "Every frame along the wall must keep most of the wish, least=%f "
      "wish_xz=%f", least, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.85f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the wall instead of standing still, "
      "xz=%f over %d frames", went_xz, kFrames);

  // Holding the wall the sphere rests on as a standing contact, which is what
  // stops the shake under a ledge, must not bring this slide back to a stop.
  Vec3F held = ride;
  float held_least = wish_xz;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F planes[8];
    Si32 pn = RestingPlanes(held, radius, soup, 3, true, planes);
    TEST_CHECK_(pn >= 1,
        "The slope under the sphere must always be a plane it rides, "
        "count=%d frame %d", pn, i);
    Vec3F next = SweepAndSlide(held, wish, radius, soup, 3, planes, pn, true);
    Vec3F step = next - held;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < held_least) {
      held_least = step_xz;
    }
    held = next;
    MovingSphere inside(held, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "A held wall must not push the sphere into it, frame %d at "
        "(%f,%f,%f)", i, held.x, held.y, held.z);
  }
  TEST_CHECK_(held_least > 0.5f * wish_xz,
      "A held wall may only shave the into-wall part off the slide, "
      "least=%f wish_xz=%f", held_least, wish_xz);
}

void test_hover_racer_log_wall_ledge_shake() {
  // End of the log: from t=106.4 to t=111.7 the craft stood at
  // (81.5288, 5.9006, -49.1947) at full throttle, never moved more than a
  // thousandth of a unit, and its speed pulsed between 0.05 and 0.34 about
  // ten times a second. The ride sphere rides slope 15708, rests sideways
  // against rock face 17683 and touches the underside of ledge 17682 above
  // it. A wall is only known from the sweep, which reports nothing while the
  // sphere already rests on the face, so the nose was aimed into the rock on
  // most steps and along it on the rest. Under the ledge the wish is
  // projected onto the crease of slope and ledge, and there those two aims
  // slide the craft in opposite directions: it crept into the rock for six
  // steps and jumped back on the seventh, over and over.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -2.55004454f;
  const float dt = 0.0124f;
  const float speed = 0.33f;
  Vec3F pos(81.5289154f, 5.90056276f, -49.1949234f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle ledge = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.0615387f, 6.8326497f, -48.7671852f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f));
  CollisionTriangle wall = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, ledge, wall};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F ledge_n = ledge.n / Length(ledge.n);
  Vec3F wall_n = wall.n / Length(wall.n);
  TEST_CHECK_(slope_n.y > 0.9f, "The logged slope must be a floor, n.y=%f",
      slope_n.y);
  TEST_CHECK_(ledge_n.y < -0.45f,
      "The rock above must count as a ceiling, whose whole plane holds the "
      "move, n.y=%f", ledge_n.y);
  TEST_CHECK_(wall_n.y > -0.45f && wall_n.y < 0.45f,
      "The rock ahead must count as a wall, n.y=%f", wall_n.y);

  Vec3F nose(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F ride = pos + nose * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F wall_xz = Normalize(Vec3F(wall_n.x, 0.0f, wall_n.z));
  float rest_d = Length(ride - wall.ClosestPoint(ride));
  TEST_CHECK_(fabsf(rest_d - radius) < 1e-3f,
      "The logged sphere rests exactly against the wall, d=%f radius=%f",
      rest_d, radius);
  // The ledge is a hair beyond touching at the logged pose and becomes a
  // contact as soon as the craft creeps, which is when it starts holding the
  // move in its plane.
  float ledge_d = Length(ride - ledge.ClosestPoint(ride));
  TEST_CHECK_(ledge_d < radius * 1.02f,
      "The logged sphere is squeezed against the ledge above it, d=%f "
      "radius=%f", ledge_d, radius);
  float into_wall = -(nose.x * wall_xz.x + nose.z * wall_xz.z);
  TEST_CHECK_(into_wall > 0.5f,
      "The logged nose points into the wall, into=%f", into_wall);

  // A sweep that reports nothing is what loses the wall: once the craft aims
  // along the face it rests on, nothing it does enters that face again, so
  // the next step is aimed back into the rock.
  Vec3F along = nose;
  along.x += wall_xz.x * into_wall;
  along.z += wall_xz.z * into_wall;
  along = Normalize(along);
  MovingSphere sliding(ride, radius, along * (speed * dt));
  SphereTriangleHit missed = SweptSphereVsTriangle(sliding, wall);
  TEST_CHECK_(!missed.hit || missed.time >= 1.0f,
      "The sweep must miss the wall the sphere rests on, hit=%d t=%f",
      missed.hit, missed.time);

  const Si32 kFrames = 16;
  float flip_path = 0.0f;
  float flip_net = 0.0f;
  Si32 flips = 0;
  float last_step = 0.0f;
  float span = 0.0f;
  for (Si32 policy = 0; policy < 2; ++policy) {
    bool hold_walls = policy == 1;
    Vec3F at = ride;
    Vec3F contact(0.0f, 0.0f, 0.0f);
    float path = 0.0f;
    float prev = 0.0f;
    float d_min = 1e9f;
    float d_max = -1e9f;
    Si32 turns = 0;
    float step_xz = 0.0f;
    for (Si32 i = 0; i < kFrames; ++i) {
      Vec3F head = nose;
      if (!hold_walls) {
        float into = head.x * contact.x + head.z * contact.z;
        if (into < 0.0f) {
          head.x -= contact.x * into;
          head.z -= contact.z * into;
          float hl = sqrtf(head.x * head.x + head.z * head.z);
          if (hl > 1e-5f) {
            head.x /= hl;
            head.z /= hl;
          }
        }
      }
      Vec3F sits[8];
      Si32 ns = RestingPlanes(at, radius, soup, 3, hold_walls, sits);
      Vec3F wall_hit(0.0f, 0.0f, 0.0f);
      Vec3F next = SweepAndSlide(at, head * (speed * dt), radius, soup, 3,
          sits, ns, true, &wall_hit);
      Vec3F step = next - at;
      step_xz = sqrtf(step.x * step.x + step.z * step.z);
      float along = -(step.x * wall_xz.x + step.z * wall_xz.z);
      path += step_xz;
      if (i > 0 && along * prev < 0.0f) {
        ++turns;
      }
      prev = along;
      at = next;
      float d = Length(at - wall.ClosestPoint(at));
      if (d < d_min) {
        d_min = d;
      }
      if (d > d_max) {
        d_max = d;
      }
      MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
      TEST_CHECK_(!SphereTriangleContact(inside, wall).hit
              && !SphereTriangleContact(inside, ledge).hit,
          "Policy %d must not push the sphere into the rock, frame %d at "
          "(%f,%f,%f)", policy, i, at.x, at.y, at.z);
      if (!hold_walls) {
        contact = wall_hit;
      }
    }
    Vec3F net = at - ride;
    if (!hold_walls) {
      flip_path = path;
      flip_net = sqrtf(net.x * net.x + net.z * net.z);
      flips = turns;
      last_step = step_xz;
      span = d_max - d_min;
    } else {
      TEST_CHECK_(turns == 0,
          "Holding the wall must answer the same wish the same way every "
          "frame, direction changed %d times", turns);
      TEST_CHECK_(step_xz < 1e-6f,
          "A craft wedged nose-first must come to rest, last step=%f",
          step_xz);
      TEST_CHECK_(d_max - d_min < 1e-5f,
          "and stay put against the wall, distance to it spans %f",
          d_max - d_min);
    }
  }
  TEST_CHECK_(flips >= 3,
      "The logged shake: aiming at the wall the sweep reports must reverse "
      "the slide over and over, reversals=%d over %d frames", flips, kFrames);
  TEST_CHECK_(flip_path > 3.0f * flip_net,
      "and it must get nowhere while doing it, path=%f net=%f", flip_path,
      flip_net);
  TEST_CHECK_(last_step > 1e-3f,
      "and it must never settle, last step=%f", last_step);
  TEST_CHECK_(span > 1e-4f,
      "and the gap to the wall must saw back and forth, span=%f", span);
}

void test_hover_racer_log_wall_edge_side_snag() {
  // A later session log: from t=154.51 to t=155.46 the craft's XZ position
  // froze bit for bit at (79.4883, -53.6790) for close to a second while
  // full throttle kept climbing the logged speed from 1.68 to 1.94. The
  // ride sphere rests exactly radius away from the crease shared by two
  // rock faces (mesh triangles 17743 and 17718); the wish, after the
  // resting-wall clip, already points away from that crease. The swept
  // vertex/edge test still reported a fresh hit at t~=0 every frame,
  // because HitSegment/HitPoint fed LowestRoot a quadratic whose constant
  // term (offset^2 - radius^2) sits at float noise around zero once the
  // sphere already touches the feature, and LowestRoot returned whichever
  // root landed near t=0 without checking whether the sphere was actually
  // approaching or only just leaving that same contact.
  const float radius = 0.276000023f;
  Vec3F ride(79.5755157f, 5.85876465f, -53.7880974f);
  CollisionTriangle tri_a = MakeTri(
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(79.2322083f, 6.84192085f, -52.2343292f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle tri_b = MakeTri(
      Vec3F(80.562027f, 5.39735413f, -53.6072464f),
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle soup[2] = {tri_a, tri_b};
  Vec3F na = tri_a.n / Length(tri_a.n);
  Vec3F nb = tri_b.n / Length(tri_b.n);
  TEST_CHECK_(na.y > -0.45f && na.y < 0.45f,
      "Face a must be a wall, n.y=%f", na.y);
  TEST_CHECK_(nb.y > -0.45f && nb.y < 0.45f,
      "Face b must be a wall, n.y=%f", nb.y);

  MovingSphere resting(ride, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  SphereTriangleHit touch_a = SphereTriangleContact(resting, tri_a);
  SphereTriangleHit touch_b = SphereTriangleContact(resting, tri_b);
  TEST_CHECK_(touch_a.hit && touch_b.hit,
      "The logged pose must already rest on both faces of the crease");
  TEST_CHECK_(Length(touch_a.point - touch_b.point) < 1e-3f,
      "Both faces' closest points must be the same spot on their shared "
      "edge, a=(%f,%f,%f) b=(%f,%f,%f)", touch_a.point.x, touch_a.point.y,
      touch_a.point.z, touch_b.point.x, touch_b.point.y, touch_b.point.z);
  float d = Length(ride - touch_a.point);
  TEST_CHECK_(fabsf(d - radius) < 1e-3f,
      "The sphere must sit exactly radius away from the shared edge, d=%f "
      "radius=%f", d, radius);

  // This is what the resting-wall clip in ResolveCraftSphere leaves after
  // clipping the throttle wish against the four nearby wall constraints:
  // already pointing away from the crease, not into it.
  Vec3F remain(0.0121073937f, -0.00267693913f, -0.0246515777f);
  TEST_CHECK_(Dot(na, remain) >= 0.0f,
      "The logged remainder must already point away from face a, n.v=%f",
      Dot(na, remain));
  TEST_CHECK_(Dot(nb, remain) >= 0.0f,
      "The logged remainder must already point away from face b, n.v=%f",
      Dot(nb, remain));

  MovingSphere leaving(ride, radius, remain);
  SphereTriangleHit hit_a = SweptSphereVsTriangle(leaving, tri_a);
  SphereTriangleHit hit_b = SweptSphereVsTriangle(leaving, tri_b);
  TEST_CHECK_(!hit_a.hit || hit_a.time > 1e-3f,
      "A velocity already leaving face a must not freeze the sweep at "
      "t=0, got hit=%d t=%f", hit_a.hit, hit_a.time);
  TEST_CHECK_(!hit_b.hit || hit_b.time > 1e-3f,
      "A velocity already leaving face b must not freeze the sweep at "
      "t=0, got hit=%d t=%f", hit_b.hit, hit_b.time);

  // The same pose, run through the craft's own sweep-and-slide (a sweep
  // plus up to four slides, exactly what ResolveCraftSphere does within a
  // single frame) repeatedly with the same wish, must not stand still: it
  // is what read as the craft snagging sideways on the rock and stalling
  // in place while the throttle kept building speed for nothing.
  float wish_xz = sqrtf(remain.x * remain.x + remain.z * remain.z);
  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F next = SweepAndSlide(at, remain, radius, soup, 2, nullptr, 0,
        true);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
  }
  TEST_CHECK_(least_xz > 0.5f * wish_xz,
      "Every frame sliding along the crease must keep most of the wish, "
      "least_xz=%f wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.5f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the crease instead of standing still, "
      "xz=%f over %d frames", went_xz, kFrames);
}

void test_swept_sphere_hits_ceiling_from_below() {
  // Ceiling at y=2 with n pointing down. Sitting under it and moving up
  // must hit at t=0. Clipping only the XZ of that normal is a no-op, so
  // the remainder used to keep +Y and the center pierced the roof.
  CollisionTriangle ceil = MakeTri(Vec3F(-4.0f, 2.0f, -4.0f),
      Vec3F(4.0f, 2.0f, -4.0f), Vec3F(0.0f, 2.0f, 4.0f));
  TEST_CHECK_(ceil.n.y < -0.45f,
      "The ceiling fixture must face down, n.y=%f", ceil.n.y);
  const float radius = 1.0f;
  Vec3F start(0.0f, 1.0f, 0.0f);
  MovingSphere sitting(start, radius, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, ceil).hit,
      "The sphere must be sitting under the ceiling");

  Vec3F wish(0.2f, 0.5f, 0.1f);
  MovingSphere into(start, radius, wish);
  SphereTriangleHit hit = SweptSphereVsTriangle(into, ceil);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "Moving up into a ceiling the sphere is sitting under must freeze "
      "at t=0, got hit=%d t=%f", hit.hit, hit.time);

  Vec3F n = ceil.n / Length(ceil.n);
  float vn = Dot(n, wish);
  TEST_CHECK_(vn < 0.0f, "The upward wish must point into the ceiling, vn=%f",
      vn);
  Vec3F n_xz(n.x, 0.0f, n.z);
  TEST_CHECK_(Length(n_xz) < 1e-5f,
      "A flat ceiling has no XZ normal, so a wall clip would leave +Y");
  Vec3F clipped = wish - n * vn;
  TEST_CHECK_(fabsf(clipped.y) < 1e-5f,
      "3D clip against the ceiling must kill upward motion, y=%f", clipped.y);
  TEST_CHECK_(fabsf(Dot(n, clipped)) < 1e-5f,
      "Clipped remainder must lie in the ceiling plane, n·remain=%f",
      Dot(n, clipped));

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit slide = SweptSphereVsTriangle(along, ceil);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "The in-plane remainder must slide under the ceiling, not freeze, "
      "got hit=%d t=%f", slide.hit, slide.time);

  float pierce_t = 1.0f;
  TEST_CHECK_(!LineSegmentPiercesTriangle(start, start + clipped, ceil,
      &pierce_t, nullptr),
      "The 3D-clipped remainder must not carry the center through the "
      "ceiling, pierce t=%f", pierce_t);

  Vec3F below(0.0f, 0.0f, 0.0f);
  MovingSphere rise(below, radius, Vec3F(0.0f, 3.0f, 0.0f));
  SphereTriangleHit skin = SweptSphereVsTriangle(rise, ceil);
  TEST_CHECK_(skin.hit && skin.time > 0.2f && skin.time < 0.4f,
      "A rise from below must hit when the skin meets the ceiling, got "
      "hit=%d t=%f", skin.hit, skin.time);
}

void test_classify_sphere_pass_through() {
  CollisionTriangle floor = MakeTri(Vec3F(-4.0f, 0.0f, -4.0f),
      Vec3F(4.0f, 0.0f, -4.0f), Vec3F(0.0f, 0.0f, 4.0f));
  if (floor.n.y < 0.0f) {
    floor = MakeTri(floor.a, floor.c, floor.b);
  }
  TEST_CHECK_(floor.n.y > 0.45f, "The floor fixture must face up, n.y=%f",
      floor.n.y);

  MovingSphere along(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.5f, 0.0f, 0.0f));
  SpherePassThrough slide = ClassifySpherePassThrough(along, floor);
  TEST_CHECK_(!slide.passed,
      "Sitting on a floor and sliding along it must not be a pass-through, "
      "center_pierce=%d entered=%d", slide.center_pierce,
      slide.entered_without_hit);

  MovingSphere land(Vec3F(0.0f, 2.0f, 0.0f), 1.0f, Vec3F(0.0f, -1.0f, 0.0f));
  SpherePassThrough landing = ClassifySpherePassThrough(land, floor);
  TEST_CHECK_(!landing.passed,
      "Coming to rest on the skin must not count as going through, "
      "center_pierce=%d entered=%d sweep=%d t=%f", landing.center_pierce,
      landing.entered_without_hit, landing.sweep.hit, landing.sweep.time);

  MovingSphere drop(Vec3F(0.0f, 2.0f, 0.0f), 1.0f, Vec3F(0.0f, -4.0f, 0.0f));
  SpherePassThrough through = ClassifySpherePassThrough(drop, floor);
  TEST_CHECK_(through.passed && through.center_pierce,
      "A drop that carries the center through the floor must be a "
      "pass-through, passed=%d pierce=%d", through.passed,
      through.center_pierce);

  const float radius = 0.276000023f;
  Vec3F start(21.3660736f, 7.0842123f, 68.9416199f);
  Vec3F vel(0.0f, -2.03896618f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));
  MovingSphere logged(start, radius, vel);
  SpherePassThrough log_pass = ClassifySpherePassThrough(logged, slope);
  TEST_CHECK_(log_pass.passed && log_pass.center_pierce,
      "The logged rock-slope drop must classify as a pass-through");
  TEST_CHECK_(log_pass.sweep.hit && log_pass.sweep.time < 1.0f,
      "The sweep must still stop that drop, hit=%d t=%f",
      log_pass.sweep.hit, log_pass.sweep.time);

  CollisionTriangle ceil = MakeTri(Vec3F(-4.0f, 2.0f, -4.0f),
      Vec3F(4.0f, 2.0f, -4.0f), Vec3F(0.0f, 2.0f, 4.0f));
  TEST_CHECK_(ceil.n.y < -0.45f, "The ceiling fixture must face down, n.y=%f",
      ceil.n.y);
  MovingSphere rise(Vec3F(0.0f, 0.0f, 0.0f), 1.0f, Vec3F(0.0f, 5.0f, 0.0f));
  SpherePassThrough roof = ClassifySpherePassThrough(rise, ceil);
  TEST_CHECK_(roof.passed && roof.center_pierce,
      "A rise that carries the center through a ceiling must be a "
      "pass-through, passed=%d pierce=%d", roof.passed, roof.center_pierce);

  // SPHERE PIERCE entered_without_hit on a parallel slide: sd did not
  // change, the sphere only acquired skin contact. That is not going
  // through the polygon.
  CollisionTriangle graze = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f));
  Vec3F gfrom(13.0511856f, 6.01273394f, 92.4283371f);
  Vec3F gvel(-0.00632667542f, 0.000114440918f, 0.00386810303f);
  MovingSphere grazing(gfrom, 0.276000023f, gvel);
  SpherePassThrough graze_pass = ClassifySpherePassThrough(grazing, graze);
  TEST_CHECK_(!graze_pass.passed,
      "A parallel slide that only acquires skin contact must not be a "
      "pass-through, entered=%d d0=%f", graze_pass.entered_without_hit,
      graze.SignedDistance(gfrom));
}

// The logged corner shake (a player bounced off a rock at (80.9, -54.0),
// then the camera and craft visibly juddered for about a second) turned out
// to need the real hover_racer collision code to reproduce: a hand-rolled
// model of the sweep-and-slide loop here only ever hit the corner's wall
// faces, never its neighboring ceiling overhangs, and could not reproduce
// the yaw flips this test was built to catch. hover_racer/main.cpp now has
// its own HOVER_SELFTEST=corner_shake regression check that drives the real
// ResolveCraftSphere through the logged pose instead.

// engine/physics_* below is the new kinematic physics module: a broad-phase
// grid over a static triangle soup, a per-body contact manifold persisted
// by feature id, a block solver for the resulting speculative constraints,
// and a sphere body controller and world facade built on top. The tests
// that follow check the module's own pieces first, then re-run the three
// real Hover Racer log fixtures above -- corner dead stop, edge side snag,
// ledge shake -- through PhysicsWorld instead of the hand-rolled
// sweep-and-slide loop, to confirm the new engine does not reintroduce the
// bugs those fixtures were built to catch.

void test_physics_collide_soup_broadphase_completeness() {
  // A mesh of 128 small triangles plus two outlier triangles well outside
  // the main patch, indexed by a deliberately coarse grid (8 cells across
  // a span of about 21 units, so each cell covers roughly 2.6 units and
  // several triangles land in the same bin). Any broad-phase that only
  // ever looks at a single cell, mishandles a query box that straddles a
  // cell boundary, or visits a triangle more than once would be caught by
  // comparing against a brute-force scan of every triangle's own AABB.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  const Si32 kGrid = 8;
  for (Si32 gz = 0; gz < kGrid; ++gz) {
    for (Si32 gx = 0; gx < kGrid; ++gx) {
      float x0 = static_cast<float>(gx);
      float z0 = static_cast<float>(gz);
      float y00 = 0.3f * sinf(static_cast<float>(gx) * 0.7f
          + static_cast<float>(gz) * 0.5f);
      float y10 = 0.3f * sinf(static_cast<float>(gx + 1) * 0.7f
          + static_cast<float>(gz) * 0.5f);
      float y01 = 0.3f * sinf(static_cast<float>(gx) * 0.7f
          + static_cast<float>(gz + 1) * 0.5f);
      float y11 = 0.3f * sinf(static_cast<float>(gx + 1) * 0.7f
          + static_cast<float>(gz + 1) * 0.5f);
      Vec3F p00(x0, y00, z0);
      Vec3F p10(x0 + 1.0f, y10, z0);
      Vec3F p01(x0, y01, z0 + 1.0f);
      Vec3F p11(x0 + 1.0f, y11, z0 + 1.0f);
      pa.push_back(p00);
      pb.push_back(p10);
      pc.push_back(p11);
      pa.push_back(p00);
      pb.push_back(p11);
      pc.push_back(p01);
    }
  }
  Si32 main_count = static_cast<Si32>(pa.size());
  pa.push_back(Vec3F(20.0f, 0.0f, 20.0f));
  pb.push_back(Vec3F(21.0f, 0.0f, 20.0f));
  pc.push_back(Vec3F(20.0f, 0.0f, 21.0f));
  pa.push_back(Vec3F(20.0f, 0.0f, 22.0f));
  pb.push_back(Vec3F(21.0f, 0.0f, 22.0f));
  pc.push_back(Vec3F(20.0f, 0.0f, 23.0f));
  mats.assign(pa.size(), PhysicsMaterial());

  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);
  TEST_CHECK_(soup.TriangleCount() == static_cast<Si32>(pa.size()),
      "Every well-formed triangle must survive Build, got %d want %d",
      soup.TriangleCount(), static_cast<Si32>(pa.size()));

  auto BruteForce = [&](const Bound3F &box) {
    std::vector<Si32> expected;
    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      const CollisionTriangle &t = soup.Triangle(i);
      Bound3F tb(t.a, t.a);
      tb = Include(tb, t.b);
      tb = Include(tb, t.c);
      if (Overlap(tb, box)) {
        expected.push_back(i);
      }
    }
    return expected;
  };

  Bound3F boxes[5] = {
      Bound3F(1.5f, 3.5f, -10.0f, 10.0f, 1.5f, 3.5f),
      Bound3F(-1.0f, 9.0f, -10.0f, 10.0f, -1.0f, 9.0f),
      Bound3F(19.5f, 21.5f, -1.0f, 1.0f, 19.5f, 21.5f),
      Bound3F(12.0f, 14.0f, -1.0f, 1.0f, 12.0f, 14.0f),
      Bound3F(3.9f, 4.1f, -10.0f, 10.0f, 3.9f, 4.1f),
  };
  std::vector<Si32> box0_expected;
  for (Si32 b = 0; b < 5; ++b) {
    std::vector<Si32> expected = BruteForce(boxes[b]);
    if (b == 0) {
      box0_expected = expected;
    }
    std::vector<bool> seen(static_cast<size_t>(soup.TriangleCount()), false);
    Si32 visits = 0;
    soup.ForEachNear(boxes[b], [&](Si32 idx) {
      TEST_CHECK_(!seen[static_cast<size_t>(idx)],
          "box %d: triangle %d must be visited at most once", b, idx);
      seen[static_cast<size_t>(idx)] = true;
      ++visits;
    });
    for (Si32 idx : expected) {
      TEST_CHECK_(seen[static_cast<size_t>(idx)],
          "box %d: broad-phase missed triangle %d, whose AABB truly "
          "overlaps the query box", b, idx);
    }
    TEST_CHECK_(visits >= static_cast<Si32>(expected.size()),
        "box %d: got %d candidates, brute force found %d must-haves", b,
        visits, static_cast<Si32>(expected.size()));
  }
  TEST_CHECK_(!box0_expected.empty(),
      "box 0 must overlap at least one triangle of the main patch");
  TEST_CHECK_(main_count > 0, "the main patch must be non-empty");

  // Negative control: box 0 straddles more than one grid cell (it spans a
  // known bin boundary near x=z=2.6), so the triangles it truly overlaps
  // are not all reachable from the single cell holding just its corner.
  // A broad-phase that only ever consulted one bin would fail this box.
  Bound3F corner_probe(boxes[0].min_x, boxes[0].min_x, boxes[0].min_y,
      boxes[0].min_y, boxes[0].min_z, boxes[0].min_z);
  std::vector<Si32> from_one_cell;
  soup.ForEachNear(corner_probe, [&](Si32 idx) {
    from_one_cell.push_back(idx);
  });
  TEST_CHECK_(from_one_cell.size() < box0_expected.size(),
      "box 0 must need more than the single cell holding its corner, so "
      "this fixture would fail a broad-phase that only consulted one "
      "bin: one-cell=%d full-box=%d",
      static_cast<int>(from_one_cell.size()),
      static_cast<int>(box0_expected.size()));
}

void test_physics_collide_soup_wide_aabb_not_dropped() {
  // CollideSoup::Build sizes its grid from the mesh's own span, so
  // width_/height_ normally track target_cells_per_axis exactly and every
  // triangle's cell range [CellX(minx), CellX(maxx)] already sits inside
  // [0, width_-1]. But target_cells_per_axis is clamped to at most 4096
  // cells per axis (kMaxCellsPerAxis) while cell_ itself is not adjusted
  // to match, so requesting far more cells than that leaves cell_ tiny
  // relative to width_*cell_: CellX of anything past roughly cell 4096
  // overshoots width_-1. Build() and ForEachNear() used to clamp only the
  // end of each axis that was expected to need it (max(lo, 0) and
  // min(hi, count-1)), leaving the other end unclamped -- so a triangle or
  // query box overshooting on the max side got x1 clamped down while x0
  // stayed above it, and the insertion/visit loop (which only runs while
  // lo <= hi) silently did nothing. A triangle far from the grid's origin,
  // and a large triangle whose own AABB reaches into that same overshoot
  // region, both used to vanish from every query -- not just queries at
  // the far edge, but the far triangle even from a box sitting right on
  // top of it.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  pa.push_back(Vec3F(0.0f, 0.0f, 0.0f));
  pb.push_back(Vec3F(1.0f, 0.0f, 0.0f));
  pc.push_back(Vec3F(0.0f, 0.0f, 1.0f));

  pa.push_back(Vec3F(99.0f, 0.0f, 99.0f));
  pb.push_back(Vec3F(100.0f, 0.0f, 99.0f));
  pc.push_back(Vec3F(99.0f, 0.0f, 100.0f));

  Si32 wide_index = static_cast<Si32>(pa.size());
  pa.push_back(Vec3F(0.5f, 0.0f, 0.5f));
  pb.push_back(Vec3F(99.5f, 0.0f, 0.6f));
  pc.push_back(Vec3F(0.6f, 0.0f, 99.5f));

  mats.assign(pa.size(), PhysicsMaterial());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 1000000);
  TEST_CHECK_(soup.TriangleCount() == static_cast<Si32>(pa.size()),
      "every well-formed triangle must survive Build, got %d want %d",
      soup.TriangleCount(), static_cast<Si32>(pa.size()));

  auto Found = [&](float x, float z) {
    Bound3F box(x - 0.4f, x + 0.4f, -1.0f, 1.0f, z - 0.4f, z + 0.4f);
    std::vector<Si32> got;
    soup.ForEachNear(box, [&](Si32 idx) { got.push_back(idx); });
    return got;
  };

  std::vector<Si32> far_corner = Found(99.3f, 99.3f);
  TEST_CHECK_(std::find(far_corner.begin(), far_corner.end(), 1)
      != far_corner.end(),
      "a small triangle far from the grid origin must still be found by "
      "a query box sitting right on top of it, found %d candidates",
      static_cast<int>(far_corner.size()));

  std::vector<Si32> wide_far = Found(90.0f, 0.55f);
  TEST_CHECK_(std::find(wide_far.begin(), wide_far.end(), wide_index)
      != wide_far.end(),
      "a large triangle whose own AABB reaches past the grid's clamp "
      "region must still be found near its far end");

  std::vector<Si32> wide_mid = Found(40.0f, 0.55f);
  TEST_CHECK_(std::find(wide_mid.begin(), wide_mid.end(), wide_index)
      != wide_mid.end(),
      "the same large triangle must also be found in the middle of its "
      "own span, not just at one end");
}

void test_physics_manifold_feature_stability() {
  // The manifold's whole job is bookkeeping across steps, independent of
  // how a ContactFeature was computed, so this drives PhysicsManifold
  // directly with hand-built features rather than real triangles.
  PhysicsManifold manifold;
  auto MakeContact = [](Si32 tri_index, ContactFeatureKind kind, Si32 sub,
      float separation) {
    ContactPoint c;
    c.feature.tri_index = tri_index;
    c.feature.kind = kind;
    c.feature.sub = sub;
    c.separation = separation;
    return c;
  };

  // The same feature across several steps must keep aging instead of
  // being recreated, and must carry forward whatever warm_speed a solver
  // stamps onto it after each Update().
  for (Si32 i = 0; i < 4; ++i) {
    std::vector<ContactPoint> fresh;
    fresh.push_back(MakeContact(7, ContactFeatureKind::kFace, 0,
        0.01f * static_cast<float>(i)));
    manifold.Update(&fresh);
    TEST_CHECK_(manifold.Contacts().size() == 1,
        "step %d must keep exactly one contact, got %d", i,
        static_cast<int>(manifold.Contacts().size()));
    TEST_CHECK_(manifold.Contacts()[0].age == i,
        "the same feature id across steps must keep aging instead of "
        "resetting, step %d got age=%d", i, manifold.Contacts()[0].age);
    if (i == 0) {
      TEST_CHECK_(manifold.Contacts()[0].warm_speed == 0.0f,
          "the first time a feature appears it has no warm_speed yet, "
          "got %f", manifold.Contacts()[0].warm_speed);
    } else {
      float want = 10.0f + static_cast<float>(i - 1);
      TEST_CHECK_(fabsf(manifold.Contacts()[0].warm_speed - want) < 1e-6f,
          "step %d must carry over the warm_speed stamped after step %d, "
          "want %f got %f", i, i - 1, want, manifold.Contacts()[0].warm_speed);
    }
    manifold.MutableContacts()[0].warm_speed = 10.0f + static_cast<float>(i);
  }

  // A different triangle -- even one right next door -- is a different
  // feature id and must not inherit age or warm_speed from the old one.
  std::vector<ContactPoint> switched;
  switched.push_back(MakeContact(8, ContactFeatureKind::kFace, 0, 0.0f));
  manifold.Update(&switched);
  TEST_CHECK_(manifold.Contacts()[0].age == 0,
      "a genuinely different feature must not inherit age, got %d",
      manifold.Contacts()[0].age);
  TEST_CHECK_(manifold.Contacts()[0].warm_speed == 0.0f,
      "a genuinely different feature must not inherit warm_speed, got %f",
      manifold.Contacts()[0].warm_speed);

  // A different edge sub-index on the same triangle is also a different
  // feature: sub must matter, not just (tri_index, kind).
  std::vector<ContactPoint> edge_a;
  edge_a.push_back(MakeContact(8, ContactFeatureKind::kEdge, 0, 0.0f));
  manifold.Update(&edge_a);
  manifold.MutableContacts()[0].warm_speed = 42.0f;
  std::vector<ContactPoint> edge_b;
  edge_b.push_back(MakeContact(8, ContactFeatureKind::kEdge, 1, 0.0f));
  manifold.Update(&edge_b);
  TEST_CHECK_(manifold.Contacts()[0].age == 0
          && manifold.Contacts()[0].warm_speed == 0.0f,
      "a different edge sub-index on the same triangle must count as a "
      "different feature, age=%d warm_speed=%f",
      manifold.Contacts()[0].age, manifold.Contacts()[0].warm_speed);

  // Returning to a feature id absent from the immediately preceding step
  // must start over: Update() only ever compares against the previous
  // call's result, it is not a long-lived cache.
  std::vector<ContactPoint> back;
  back.push_back(MakeContact(7, ContactFeatureKind::kFace, 0, 0.0f));
  manifold.Update(&back);
  TEST_CHECK_(manifold.Contacts()[0].age == 0,
      "a feature missing from the immediately preceding step must "
      "restart at age 0 even if it was seen earlier, got %d",
      manifold.Contacts()[0].age);

  // Canonical ordering: several simultaneous contacts must always come
  // out sorted by (tri_index, kind, sub), regardless of the order fed in,
  // so the PGS fallback sees a deterministic order independent of
  // broad-phase traversal.
  std::vector<ContactPoint> multi;
  multi.push_back(MakeContact(5, ContactFeatureKind::kVertex, 2, 0.0f));
  multi.push_back(MakeContact(2, ContactFeatureKind::kFace, 0, 0.0f));
  multi.push_back(MakeContact(5, ContactFeatureKind::kEdge, 0, 0.0f));
  manifold.Update(&multi);
  TEST_CHECK_(manifold.Contacts().size() == 3,
      "all three simultaneous contacts must survive, got %d",
      static_cast<int>(manifold.Contacts().size()));
  TEST_CHECK_(manifold.Contacts()[0].feature.tri_index == 2,
      "contacts must be sorted by tri_index first, got %d",
      manifold.Contacts()[0].feature.tri_index);
  TEST_CHECK_(manifold.Contacts()[1].feature.tri_index == 5
          && manifold.Contacts()[1].feature.kind
              == ContactFeatureKind::kEdge,
      "within the same tri_index, kFace/kEdge/kVertex must come out in "
      "that order");
  TEST_CHECK_(manifold.Contacts()[2].feature.tri_index == 5
          && manifold.Contacts()[2].feature.kind
              == ContactFeatureKind::kVertex,
      "the vertex feature must sort after the edge feature on the same "
      "triangle");
}

void test_physics_solver_corner_order_invariance_and_pgs() {
  // Three mutually perpendicular walls meeting at a cube corner: pressing
  // straight into the corner must stop dead (v=0), and which order the
  // three constraints are listed in must not change the answer, since
  // the block solve enumerates constraint subsets rather than clipping
  // them in sequence.
  SpeculativeConstraint base[3];
  base[0].normal = Vec3F(1.0f, 0.0f, 0.0f);
  base[1].normal = Vec3F(0.0f, 1.0f, 0.0f);
  base[2].normal = Vec3F(0.0f, 0.0f, 1.0f);
  for (Si32 i = 0; i < 3; ++i) {
    base[i].bound = 0.0f;
  }
  Vec3F wish(1.0f, 1.0f, 1.0f);
  Si32 perms[6][3] = {
      {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
  };
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint cs[3];
    for (Si32 i = 0; i < 3; ++i) {
      cs[i] = base[perms[p][i]];
    }
    ContactSolveResult r = SolveContactVelocity(wish, cs, 3);
    TEST_CHECK_(Length(r.velocity) < 1e-5f,
        "perm %d: pressing straight into a cube corner must stop dead, "
        "got (%f,%f,%f)", p, r.velocity.x, r.velocity.y, r.velocity.z);
    TEST_CHECK_(r.outcome == ContactSolveOutcome::kCorner,
        "perm %d: three simultaneously active constraints must classify "
        "as a corner, got %d", p, static_cast<int>(r.outcome));
    TEST_CHECK_(r.active_mask == 7u,
        "perm %d: all three constraints must be active, mask=%u", p,
        r.active_mask);
  }

  // Moving away from the corner is unconstrained by any of the three
  // faces and must pass through untouched.
  ContactSolveResult free_result = SolveContactVelocity(
      Vec3F(-1.0f, -1.0f, -1.0f), base, 3);
  TEST_CHECK_(Length(free_result.velocity - Vec3F(-1.0f, -1.0f, -1.0f))
          < 1e-5f,
      "moving away from all three faces must pass through unclipped, "
      "got (%f,%f,%f)", free_result.velocity.x, free_result.velocity.y,
      free_result.velocity.z);
  TEST_CHECK_(free_result.outcome == ContactSolveOutcome::kFree,
      "an unconstrained wish must report kFree, got %d",
      static_cast<int>(free_result.outcome));

  // A fourth, redundant constraint (a duplicate of the first face) pushes
  // the count over the exact block solve's limit of three and into the
  // PGS fallback; the corner's answer must not change, and it must not
  // depend on which position in the array the duplicate sits at.
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint cs4[4];
    for (Si32 i = 0; i < 3; ++i) {
      cs4[i] = base[perms[p][i]];
    }
    cs4[3] = base[0];
    ContactSolveResult r4 = SolveContactVelocity(wish, cs4, 4);
    TEST_CHECK_(Length(r4.velocity) < 1e-4f,
        "perm %d: PGS with a redundant fourth constraint must still stop "
        "dead at the corner, got (%f,%f,%f)", p, r4.velocity.x,
        r4.velocity.y, r4.velocity.z);
    TEST_CHECK_(r4.outcome == ContactSolveOutcome::kPgs,
        "perm %d: more than three constraints must fall back to PGS, "
        "got %d", p, static_cast<int>(r4.outcome));
  }
}

void test_physics_solver_coplanar_wedge_stops_cleanly() {
  // Three vertical wall normals spread 120 degrees apart in the XZ plane,
  // summing to exactly zero. Because they are coplanar (all n.y == 0),
  // any nonzero horizontal velocity has a strictly positive dot with at
  // least one of them (the three dot products sum to zero, so if none
  // were positive all three would have to be exactly zero, which forces
  // the velocity itself to be the zero vector, since two of the three
  // normals already span the whole XZ plane). So a horizontal wish has
  // exactly one feasible answer -- v=0 -- and any two of the three walls
  // already pin it down (the third is redundant), which is why this
  // comes back as an edge (a one-dimensional feasible line, here the
  // vertical axis the walls have no opinion about) rather than the
  // kDegenerate fallback, which is reserved for configurations where no
  // subset of the active constraints locates a feasible point at all.
  // What actually matters -- and is what the plan's "clean stop, not a
  // direction picked at random" is about -- is that the answer is the
  // same zero vector no matter which order the three constraints are
  // listed in.
  SpeculativeConstraint cs[3];
  cs[0].normal = Vec3F(1.0f, 0.0f, 0.0f);
  cs[1].normal = Vec3F(-0.5f, 0.0f, 0.8660254f);
  cs[2].normal = Vec3F(-0.5f, 0.0f, -0.8660254f);
  cs[0].bound = 0.0f;
  cs[1].bound = 0.0f;
  cs[2].bound = 0.0f;

  Si32 perms[6][3] = {
      {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
  };
  Vec3F wish(0.5f, 0.0f, 0.2f);
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint ordered[3];
    for (Si32 i = 0; i < 3; ++i) {
      ordered[i] = cs[perms[p][i]];
    }
    ContactSolveResult r = SolveContactVelocity(wish, ordered, 3);
    TEST_CHECK_(Length(r.velocity) < 1e-4f,
        "perm %d: three walls spread 120 degrees apart must stop a "
        "horizontal wish dead, regardless of listing order, got "
        "(%f,%f,%f)", p, r.velocity.x, r.velocity.y, r.velocity.z);
    TEST_CHECK_(r.outcome == ContactSolveOutcome::kEdge,
        "perm %d: this wedge has exactly one feasible line (vertical), "
        "so it must classify as an edge, not fall through to a random "
        "single-face pick, got %d", p, static_cast<int>(r.outcome));
  }

  // The same three walls have no opinion at all about vertical motion
  // (every normal has n.y == 0), so a wish with a vertical component must
  // keep that component exactly while still losing its horizontal part.
  Vec3F rising(0.5f, 2.0f, 0.2f);
  ContactSolveResult vertical = SolveContactVelocity(rising, cs, 3);
  TEST_CHECK_(fabsf(vertical.velocity.y - rising.y) < 1e-3f,
      "vertical motion must survive a wedge built entirely from vertical "
      "walls, wanted y=%f got y=%f", rising.y, vertical.velocity.y);
  TEST_CHECK_(sqrtf(vertical.velocity.x * vertical.velocity.x
      + vertical.velocity.z * vertical.velocity.z) < 1e-3f,
      "the horizontal part must still be killed, got xz=(%f,%f)",
      vertical.velocity.x, vertical.velocity.z);
}

void test_physics_solver_duplicate_constraint() {
  // Two walls meeting at a vertical corner (mirrored across the X axis,
  // so their shared feasible line is the Y axis): pressing straight into
  // the corner in X must stop the horizontal motion dead. Adding an exact
  // duplicate of one of the two constraints must not change that answer.
  SpeculativeConstraint n1;
  n1.normal = Normalize(Vec3F(1.0f, 0.0f, 1.0f));
  n1.bound = 0.0f;
  SpeculativeConstraint n2;
  n2.normal = Normalize(Vec3F(1.0f, 0.0f, -1.0f));
  n2.bound = 0.0f;

  Vec3F wish(1.0f, 0.0f, 0.0f);
  SpeculativeConstraint pair[2] = {n1, n2};
  ContactSolveResult base_result = SolveContactVelocity(wish, pair, 2);
  TEST_CHECK_(Length(base_result.velocity) < 1e-5f,
      "pressing straight into this vertical corner must stop dead, got "
      "(%f,%f,%f)", base_result.velocity.x, base_result.velocity.y,
      base_result.velocity.z);
  TEST_CHECK_(base_result.outcome == ContactSolveOutcome::kEdge,
      "two simultaneously active constraints must classify as an edge, "
      "got %d", static_cast<int>(base_result.outcome));

  SpeculativeConstraint with_dup[3] = {n1, n2, n1};
  ContactSolveResult dup_result = SolveContactVelocity(wish, with_dup, 3);
  TEST_CHECK_(Length(dup_result.velocity - base_result.velocity) < 1e-5f,
      "an exact duplicate of an already-active constraint must not "
      "change the answer, base=(%f,%f,%f) with_dup=(%f,%f,%f)",
      base_result.velocity.x, base_result.velocity.y, base_result.velocity.z,
      dup_result.velocity.x, dup_result.velocity.y, dup_result.velocity.z);

  // A wish that only grazes the corner (already leaving both faces) must
  // pass through unconstrained, with or without the duplicate.
  Vec3F leaving(-1.0f, 0.3f, 0.0f);
  ContactSolveResult base_leave = SolveContactVelocity(leaving, pair, 2);
  ContactSolveResult dup_leave = SolveContactVelocity(leaving, with_dup, 3);
  TEST_CHECK_(Length(base_leave.velocity - leaving) < 1e-5f,
      "leaving both faces must pass through unclipped, got (%f,%f,%f)",
      base_leave.velocity.x, base_leave.velocity.y, base_leave.velocity.z);
  TEST_CHECK_(Length(dup_leave.velocity - base_leave.velocity) < 1e-5f,
      "the duplicate must not change the unconstrained case either, "
      "base=(%f,%f,%f) with_dup=(%f,%f,%f)", base_leave.velocity.x,
      base_leave.velocity.y, base_leave.velocity.z, dup_leave.velocity.x,
      dup_leave.velocity.y, dup_leave.velocity.z);
}

void test_physics_world_fixture_corner_dead_stop() {
  // Same log fixture as test_hover_racer_log_wall_corner_dead_stop, run
  // through PhysicsWorld instead of the hand-rolled sweep-and-slide loop:
  // the craft must keep making real progress along the wall every frame,
  // where the old code's XZ-only wall clip left the whole log frozen at
  // this exact pose for six seconds straight.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -1.29992986f;
  const float speed = 1.96254206f;
  const float lift = 0.00145309512f;
  const float dt = 0.012f;
  Vec3F pos(81.8645935f, 5.82387304f, -49.429184f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle wall_low = MakeTri(
      Vec3F(82.3298798f, 5.8710408f, -50.3318481f),
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle wall_flat = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));

  std::vector<Vec3F> pa = {slope.a, wall_low.a, wall_flat.a};
  std::vector<Vec3F> pb = {slope.b, wall_low.b, wall_flat.b};
  std::vector<Vec3F> pc = {slope.c, wall_low.c, wall_flat.c};
  std::vector<PhysicsMaterial> mats(3);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * units;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F ride = pos + sph_off;
  Vec3F wish = heading * speed + Vec3F(0.0f, lift, 0.0f);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z) * dt;
  TEST_CHECK_(fabsf(wish_xz - 0.0235505f) < 1e-4f,
      "The logged wish is about 0.02355 of XZ per frame, got %f", wish_xz);

  PhysicsBodyId craft = world.AddSphere(ride, radius);
  world.SetWishVelocity(craft, wish);

  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    world.Step(dt);
    Vec3F next = world.Position(craft);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
    MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "The new engine must not push the craft into the wall it slides "
        "along, frame %d at (%f,%f,%f)", i, at.x, at.y, at.z);
  }
  TEST_CHECK_(least_xz > 0.85f * wish_xz,
      "Every frame along the wall must keep most of the wish instead of "
      "dead-stopping like the old sweep-and-slide loop, least=%f "
      "wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.85f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the wall over %d frames instead of "
      "standing still, xz=%f", kFrames, went_xz);

  // Negative control: the exact same geometry and per-frame displacement
  // still dead-stops the old sweep-and-slide loop, which is the bug this
  // engine exists to remove by construction rather than by another layer
  // of hysteresis.
  CollisionTriangle soup[3] = {slope, wall_low, wall_flat};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F sits[1] = {slope_n};
  Vec3F stuck = SweepAndSlide(ride, wish * dt, radius, soup, 3, sits, 1,
      false);
  Vec3F stuck_moved = stuck - ride;
  float stuck_xz = sqrtf(stuck_moved.x * stuck_moved.x
      + stuck_moved.z * stuck_moved.z);
  TEST_CHECK_(stuck_xz < 1e-5f,
      "The old code's dead stop at this exact pose is the regression this "
      "test guards against, xz=%f wish_xz=%f", stuck_xz, wish_xz);
}

void test_physics_world_fixture_edge_snag() {
  // Same log fixture as test_hover_racer_log_wall_edge_side_snag: a wish
  // that already points away from both faces of a crease must be carried
  // through essentially unchanged by the new narrow-phase, which tests
  // the closest point on each triangle rather than a swept quadratic
  // whose constant term goes to float noise once the sphere already
  // touches the feature (that primitive-level fix lives in and is tested
  // by sphere_vs_triangle.cpp/test_hover_racer_log_wall_edge_side_snag;
  // this test's job is to confirm the higher-level module built on top of
  // it -- GatherSphereContacts, the manifold, the solver -- does not
  // quietly reintroduce the freeze through some other path).
  const float radius = 0.276000023f;
  Vec3F ride(79.5755157f, 5.85876465f, -53.7880974f);
  CollisionTriangle tri_a = MakeTri(
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(79.2322083f, 6.84192085f, -52.2343292f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle tri_b = MakeTri(
      Vec3F(80.562027f, 5.39735413f, -53.6072464f),
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));

  std::vector<Vec3F> pa = {tri_a.a, tri_b.a};
  std::vector<Vec3F> pb = {tri_a.b, tri_b.b};
  std::vector<Vec3F> pc = {tri_a.c, tri_b.c};
  std::vector<PhysicsMaterial> mats(2);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * 0.140449f;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId craft = world.AddSphere(ride, radius);

  Vec3F remain(0.0121073937f, -0.00267693913f, -0.0246515777f);
  float wish_xz = sqrtf(remain.x * remain.x + remain.z * remain.z);
  world.SetWishVelocity(craft, remain);

  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    world.Step(1.0f);
    Vec3F next = world.Position(craft);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
  }
  TEST_CHECK_(least_xz > 0.9f * wish_xz,
      "A wish already leaving the crease must keep nearly all of itself "
      "every frame instead of snagging like the old swept-edge false "
      "hit, least=%f wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.9f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel almost the full distance over %d frames "
      "instead of snagging on the crease, xz=%f", kFrames, went_xz);

  MovingSphere leaving(ride, radius, remain);
  SphereTriangleHit hit_a = SweptSphereVsTriangle(leaving, tri_a);
  SphereTriangleHit hit_b = SweptSphereVsTriangle(leaving, tri_b);
  TEST_CHECK_(!hit_a.hit || hit_a.time > 1e-3f,
      "leaving face a must not freeze the sweep at t=0, hit=%d t=%f",
      hit_a.hit, hit_a.time);
  TEST_CHECK_(!hit_b.hit || hit_b.time > 1e-3f,
      "leaving face b must not freeze the sweep at t=0, hit=%d t=%f",
      hit_b.hit, hit_b.time);
}

void test_physics_world_fixture_ledge_shake() {
  // Same log fixture as test_hover_racer_log_wall_ledge_shake, run through
  // PhysicsWorld: nose pinned into a rock face while a ledge presses down
  // from above. The old sweep-only wall detection loses the wall the
  // instant the craft's own last move already cleared it, so the very
  // next frame aims back into the rock, back and forth, forever. The new
  // engine keeps that wall (and the ledge) as a standing contact through
  // the manifold instead of only trusting a fresh sweep, so the distance
  // to the wall must move in one direction, never oscillate.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -2.55004454f;
  const float dt = 0.0124f;
  const float speed = 0.33f;
  Vec3F pos(81.5289154f, 5.90056276f, -49.1949234f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle ledge = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.0615387f, 6.8326497f, -48.7671852f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f));
  CollisionTriangle wall = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, ledge, wall};

  std::vector<Vec3F> pa = {slope.a, ledge.a, wall.a};
  std::vector<Vec3F> pb = {slope.b, ledge.b, wall.b};
  std::vector<Vec3F> pc = {slope.c, ledge.c, wall.c};
  std::vector<PhysicsMaterial> mats(3);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * units;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  Vec3F nose(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F ride = pos + nose * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  PhysicsBodyId craft = world.AddSphere(ride, radius);
  world.SetWishVelocity(craft, nose * speed);

  const Si32 kFrames = 16;
  float prev_d = Length(ride - wall.ClosestPoint(ride));
  float min_step = 1e9f;
  float max_step = 0.0f;
  Si32 backward = 0;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F before = world.Position(craft);
    world.Step(dt);
    Vec3F after = world.Position(craft);
    Vec3F step = after - before;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < min_step) {
      min_step = step_xz;
    }
    if (step_xz > max_step) {
      max_step = step_xz;
    }
    float d = Length(after - wall.ClosestPoint(after));
    if (d < prev_d - 1.0e-5f) {
      ++backward;
    }
    prev_d = d;
    MovingSphere inside(after, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall).hit
            && !SphereTriangleContact(inside, ledge).hit,
        "The new engine must not push the craft into the rock, frame %d "
        "at (%f,%f,%f)", i, after.x, after.y, after.z);
  }
  TEST_CHECK_(backward == 0,
      "Wedged nose-first, the distance to the wall must never step "
      "backward -- that back-and-forth is exactly the logged shake, "
      "backward steps=%d over %d frames", backward, kFrames);
  TEST_CHECK_(max_step < 2.0f * min_step,
      "and the per-frame step size must stay in a tight band instead of "
      "pulsing between near-zero and near-full speed like the logged "
      "shake, min=%f max=%f", min_step, max_step);

  // Negative control: the logged mechanism (drop the wall constraint the
  // instant the last sweep found nothing, so the next frame's nose aims
  // straight back into the rock) still reproduces the shake on this exact
  // pose when run through the old policy-0 loop.
  Vec3F wall_xz = Normalize(Vec3F(wall.n.x, 0.0f, wall.n.z));
  Vec3F at = ride;
  Vec3F contact(0.0f, 0.0f, 0.0f);
  float prev_along = 0.0f;
  Si32 turns = 0;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F head = nose;
    float into = head.x * contact.x + head.z * contact.z;
    if (into < 0.0f) {
      head.x -= contact.x * into;
      head.z -= contact.z * into;
      float hl = sqrtf(head.x * head.x + head.z * head.z);
      if (hl > 1e-5f) {
        head.x /= hl;
        head.z /= hl;
      }
    }
    Vec3F sits[8];
    Si32 ns = RestingPlanes(at, radius, soup, 3, false, sits);
    Vec3F wall_hit(0.0f, 0.0f, 0.0f);
    Vec3F next = SweepAndSlide(at, head * (speed * dt), radius, soup, 3,
        sits, ns, true, &wall_hit);
    Vec3F step = next - at;
    float along = -(step.x * wall_xz.x + step.z * wall_xz.z);
    if (i > 0 && along * prev_along < 0.0f) {
      ++turns;
    }
    prev_along = along;
    at = next;
    contact = wall_hit;
  }
  TEST_CHECK_(turns >= 3,
      "The old sweep-only wall memory must still reverse direction over "
      "and over on this exact pose, reversals=%d over %d frames -- that "
      "is the regression the manifold's standing contact fixes", turns,
      kFrames);
}

void test_physics_sphere_body_reacquires_steep_floor_gap() {
  // Hover Racer's Xcode debug-build session log (build/Debug's log.txt,
  // the last of 56 launches appended to that file) shows the player
  // reversing down a steep slope: has_floor stays true for over a hundred
  // frames while sphere_y descends smoothly, then for exactly one substep
  // (dt=0.0041s, speed=-0.4885 world units/s) has_floor flips to false and
  // never comes back for the rest of the recorded session -- over four
  // more seconds, with the craft's height frozen and then drifting on
  // whatever lift the hover spring supplied, never touching the mesh
  // again. That reads to the player as the craft falling off a step
  // instead of continuing to slide down the hill.
  //
  // GatherSphereContacts already has a fallback for exactly this shape of
  // problem -- a floor triangle whose closest point is a bit further than
  // `reach` away gets a second chance via a straight vertical pierce that
  // probes a full three radii down -- but both that fallback's own gate
  // and the broad-phase query feeding it were still bounded by
  // `radius + reach` (skin plus a single substep's travel: at the logged
  // speed and dt that is on the order of a hundredth of a world unit), so
  // a floor genuinely within the probe's three-radius reach, but past that
  // tight bound, was never offered to the pierce at all. This fixture
  // reproduces that exact substep (same radius, skin, speed and dt as the
  // log) with a flat floor sitting at a gap comfortably inside the
  // pierce's own three-radius reach but well past the old radius+reach
  // gate.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  PhysicsStepConfig config;
  config.skin = 0.00842695f;
  const Vec3F wish(-0.5f, 0.0f, 0.0f);
  const float dt = 0.0041f;

  float travel = Length(wish) * dt;
  float reach = config.skin + travel;
  // The gate compares the *plane* distance from the sphere's center
  // (radius + separation for a flat floor straight below), not the bare
  // separation, against radius + reach on the old path and radius * 3 on
  // the fixed one. Pick a separation whose plane distance clears the old
  // gate but not the new one: reach < separation < 2 * radius.
  float min_separation = reach;
  float max_separation = 2.0f * radius;
  TEST_CHECK_(min_separation < max_separation,
      "test setup: reach=%f must leave room under 2*radius=%f",
      min_separation, max_separation);
  float gap = min_separation + 0.5f * (max_separation - min_separation);
  Vec3F center(0.0f, radius + gap, 0.0f);
  float plane_d = center.y;
  TEST_CHECK_(plane_d > radius + reach && plane_d <= radius * 3.0f,
      "test setup: plane_d=%f must sit strictly between the old gate "
      "radius+reach=%f and the pierce's own reach radius*3=%f", plane_d,
      radius + reach, radius * 3.0f);

  std::vector<ContactPoint> contacts;
  SphereBodySupport support;
  GatherSphereContacts(soup, center, radius, wish, dt, config, &contacts,
      &support);
  // What the reacquire has to deliver is the contact: that is what keeps
  // the solver from letting the sphere pass the face, and what the seam
  // assist in StepSphereBody keys off. Support is deliberately not
  // claimed here -- a floor this far out is one the sphere is flying over
  // rather than standing on, and saying otherwise is what froze a craft
  // in mid-air over a slope (see
  // test_physics_sphere_body_hovering_over_floor_is_not_sitting).
  TEST_CHECK_(!contacts.empty(),
      "A floor triangle within the vertical pierce's own three-radius "
      "reach must still produce a contact even though it sits past the "
      "much tighter skin+travel reach -- gap=%f reach=%f 3*radius=%f",
      gap, reach, radius * 3.0f);
  TEST_CHECK_(!support.has_floor,
      "a floor %f world units below the sphere's surface is past the "
      "contact skin %f, so it is not support", gap, config.skin);
  if (!contacts.empty()) {
    TEST_CHECK_(contacts[0].normal.y > 0.99f,
        "the reacquired contact's normal must point straight up off this "
        "flat floor, normal=(%f,%f,%f)", contacts[0].normal.x,
        contacts[0].normal.y, contacts[0].normal.z);
    float expected_separation = gap;
    TEST_CHECK_(
        std::fabs(contacts[0].separation - expected_separation) < 1.0e-3f,
        "the reported separation must match the real gap, got=%f "
        "expected=%f", contacts[0].separation, expected_separation);
  }

  // Negative control: reverting the widened gate (i.e. gating and
  // querying at radius + reach the way the old code did) must reproduce
  // the exact failure this fixture guards against.
  float old_query_reach = radius + reach;
  std::vector<Si32> old_hits;
  soup.ForEachNearSegment(center, center + wish * dt, old_query_reach,
      [&](Si32 idx) { old_hits.push_back(idx); });
  TEST_CHECK_(old_hits.empty(),
      "sanity check: the old radius+reach broad-phase bound must not even "
      "reach this floor, so the fix has to be the widened query, not just "
      "the inner gate -- hits=%d", static_cast<int>(old_hits.size()));
}

void test_physics_sphere_body_hovering_over_floor_is_not_sitting() {
  // Hover Racer's HOVER_SELFTEST=reverse_drop run logs the player's ride
  // sphere frozen at sphere_y=5.3582 from t=2s to the end of the run,
  // while the same sphere dropped down the same column of the same mesh
  // comes to rest 3.4 design units (0.48 world units, nearly two radii)
  // lower: the craft hangs in the air over the slope it is supposed to be
  // rolling down, and nothing ever asks it to descend.
  //
  // The reason is the vertical-pierce fallback in GatherSphereContacts.
  // Probing three radii below the centre is right, and so is the contact
  // it makes down there -- that contact is what stops the solver from
  // letting a fast sphere pass straight through the face. Reporting
  // support from it is not. SphereBodySupport is documented as the face
  // the sphere is "currently touched"-ing, and it exists so a caller can
  // "zero a downward lift once a floor is found"; Hover Racer, like any
  // such caller, stops pushing the craft down the moment it is told there
  // is a floor. A floor almost two radii away then reads as solid ground
  // under a craft that is plainly airborne, and the two decisions
  // deadlock: the game will not descend because it believes it is
  // sitting, and it never starts touching because it never descends.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  PhysicsStepConfig config;
  const float dt = 1.0f / 60.0f;
  // The logged gap, in radii: inside the pierce's three-radius probe (so
  // the fallback does fire) and far outside the contact skin (so no
  // reading of "touching" can include it).
  const float gap = 1.7f * radius;
  TEST_CHECK_(gap > config.skin,
      "test setup: gap=%f must be well past the contact skin=%f to count "
      "as plainly airborne", gap, config.skin);
  TEST_CHECK_(radius + gap <= radius * 3.0f,
      "test setup: plane distance %f must stay inside the pierce's own "
      "three-radius probe %f, or the fallback under test never fires",
      radius + gap, radius * 3.0f);

  Vec3F center(0.0f, radius + gap, 0.0f);
  std::vector<ContactPoint> contacts;
  SphereBodySupport support;
  GatherSphereContacts(soup, center, radius, Vec3F(0.0f, 0.0f, 0.0f), dt,
      config, &contacts, &support);
  TEST_CHECK_(!support.has_floor,
      "a floor %f world units (%.2f radii) below the sphere's surface is "
      "not something the sphere rests on, skin=%f", gap, gap / radius,
      config.skin);
  TEST_CHECK_(!contacts.empty(),
      "the far floor must still produce a speculative contact, or nothing "
      "stops a fast sphere from passing through it");
  if (!contacts.empty()) {
    TEST_CHECK_(std::fabs(contacts[0].separation - gap) < 1.0e-3f,
        "the contact must report the real gap, got=%f expected=%f",
        contacts[0].separation, gap);
  }

  // Same sphere actually resting on the same floor: support is exactly
  // what this flag is for, so the strictness above must not have cost it.
  Vec3F resting(0.0f, radius + config.skin * 0.25f, 0.0f);
  contacts.clear();
  support = SphereBodySupport();
  GatherSphereContacts(soup, resting, radius, Vec3F(0.0f, 0.0f, 0.0f), dt,
      config, &contacts, &support);
  TEST_CHECK_(support.has_floor,
      "a sphere sitting within the contact skin of the floor must report "
      "support, gap=%f skin=%f", config.skin * 0.25f, config.skin);

  // The deadlock itself, driven the way Hover Racer drives it: while the
  // engine reports a floor the craft holds its lift at zero, otherwise
  // the lift falls away (UpdateCraft's kAirborneFall, 18 design units per
  // second, which is about 2.5 world units per second squared at Hover
  // Racer's scale). With support claimed from the far pierce, the wish is
  // zero on every step and the sphere never moves at all.
  PhysicsManifold manifold;
  Vec3F pos(0.0f, radius + gap, 0.0f);
  float lift = 0.0f;
  const float fall = 2.5f;
  // Measured off the geometry rather than off the flag under test, so a
  // step that merely claims a floor cannot satisfy this.
  Si32 touched_at = -1;
  const Si32 steps = 120;
  for (Si32 i = 0; i < steps; ++i) {
    SphereStepResult step = StepSphereBody(soup, &manifold, &pos, radius,
        Vec3F(0.0f, lift, 0.0f), dt, config);
    if (touched_at < 0 && pos.y - radius <= config.skin) {
      touched_at = i;
    }
    if (step.support.has_floor) {
      if (lift < 0.0f) {
        lift = 0.0f;
      }
    } else {
      lift -= fall * dt;
    }
  }
  // Free fall over this gap takes sqrt(2 * 0.469 / 2.5) = 0.61s, so a
  // sphere that is allowed to fall has really landed inside 1.5 seconds.
  TEST_CHECK_(touched_at >= 0 && touched_at < 90,
      "the sphere must reach the floor it is hovering over, instead it "
      "was still airborne after %d steps at y=%f (gap %f left)", steps,
      pos.y, pos.y - radius);
  TEST_CHECK_(pos.y <= radius + config.skin,
      "after landing the sphere must rest on the floor, y=%f expected at "
      "most %f", pos.y, radius + config.skin);
  TEST_CHECK_(pos.y > radius - config.max_position_correction,
      "the sphere must not have been pushed through the floor, y=%f",
      pos.y);
}

void test_physics_world_fixture_floor_climb_collapse() {
  // A HOVER_SELFTEST=... style autopilot lap over the real track logged the
  // craft's vertical speed collapsing to exactly zero for one frame every
  // few facets while climbing a low-poly rock slope, then resuming its
  // normal climb rate -- the "trembling while touching the track" the
  // player reported. A logged collapse frame's exact contact set (11
  // facets, separations from 0.007 to 0.08, wish horizontal) fed straight
  // into SolveContactVelocity confirmed 0 is the mathematically correct
  // answer for that discrete set: none of the eleven facets is tight
  // enough to bind the solve, so a purely horizontal wish sails through
  // unredirected. The craft is not blocked, it is floating a hair above
  // the mesh with no penetration for the existing residual-penetration
  // cleanup to act on and nothing else pulling it back down -- so on a
  // stretch of near-flat facets, that hairline gap never closes on its
  // own and the craft coasts at whatever height it happened to reach last,
  // one triangle seam at a time, which is what reads as trembling.
  //
  // Reproduced directly here with a small tessellated flat floor (a
  // faceted mesh's local seams are the point, a single infinite plane
  // would not have any) and the sphere starting at the exact separation
  // logged for the tightest of those eleven facets. Confirmed by hand
  // against this exact fixture that reverting the fix below (physics_sphere_body.cpp's
  // gap-closing pass alongside the existing penetration cleanup) leaves
  // the gap frozen at 0.00744 for as long as the sphere sits over one
  // facet, then frozen again at whatever the next seam happens to leave
  // it at, forever, instead of the fixed code's steady per-step decay
  // asserted below.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  const Si32 kGrid = 6;
  for (Si32 gz = -kGrid; gz < kGrid; ++gz) {
    for (Si32 gx = -kGrid; gx < kGrid; ++gx) {
      Vec3F p00(static_cast<float>(gx), 5.5f, static_cast<float>(gz));
      Vec3F p10(static_cast<float>(gx + 1), 5.5f, static_cast<float>(gz));
      Vec3F p01(static_cast<float>(gx), 5.5f, static_cast<float>(gz + 1));
      Vec3F p11(static_cast<float>(gx + 1), 5.5f, static_cast<float>(gz + 1));
      pa.push_back(p00);
      pb.push_back(p01);
      pc.push_back(p10);
      pa.push_back(p10);
      pb.push_back(p01);
      pc.push_back(p11);
    }
  }
  mats.assign(pa.size(), PhysicsMaterial());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float gap = 0.00743940473f;
  Vec3F start(0.3f, 5.5f + radius + gap, 0.3f);

  PhysicsStepConfig config;
  config.skin = 0.06f * 0.140449f;
  config.floor_normal_y = 0.45f;
  Vec3F wish(-2.8664701f, 0.0f, -4.53065062f);

  auto SepAt = [&](const Vec3F &p) {
    return p.y - radius - 5.5f;
  };

  PhysicsManifold manifold;
  Vec3F position = start;
  Si32 frozen_run = 0;
  Si32 worst_frozen_run = 0;
  float prev_sep = gap;
  const Si32 kFrames = 20;
  for (Si32 i = 0; i < kFrames; ++i) {
    StepSphereBody(soup, &manifold, &position, radius, wish, dt, config);
    float sep = SepAt(position);
    if (sep > prev_sep - 1.0e-6f) {
      ++frozen_run;
      if (frozen_run > worst_frozen_run) {
        worst_frozen_run = frozen_run;
      }
    } else {
      frozen_run = 0;
    }
    prev_sep = sep;
  }
  TEST_CHECK_(worst_frozen_run < 4,
      "A hairline gap above a floor facet the craft is not penetrating "
      "must keep shrinking every few frames instead of freezing at a "
      "fixed height for many frames in a row -- that stuck-forever gap is "
      "exactly the logged trembling, longest frozen run=%d frames over %d",
      worst_frozen_run, kFrames);
  float final_sep = SepAt(position);
  TEST_CHECK_(final_sep < 0.6f * gap,
      "The craft must settle much closer to a facet it is already "
      "floating a hair above instead of coasting past it at a fixed "
      "height, start_gap=%f final_sep=%f", gap, final_sep);
}

void test_physics_sphere_body_rolls_over_road_seam_skirt() {
  // Straight out of a Hover Racer log: the craft came to a dead stop in the
  // middle of a flat road and stayed there for six seconds at full throttle,
  // with twelve contacts and a wall reported on level ground. The track mesh
  // splits the road into tiles, and every tile closes its own edge with a
  // skirt that hangs straight down from the surface -- so at a seam two
  // near-vertical faces stand back to back, and their topmost edge sits at
  // exactly the height the ride sphere rolls on. Those faces are neither
  // floor nor ceiling by their own normal, and flattening their contact into
  // the XZ plane (right for a wall beside the sphere, which must not launch
  // the body upward) turned the level seam into a vertical wall as tall as
  // the whole skirt, standing across the road. The real direction out of the
  // contact is nearly straight up, so the fix in GatherSphereContacts keeps
  // it whole whenever the sphere rides over the top of such a face.
  //
  // Geometry, sphere pose and wish below are the logged ones: two road
  // tiles at y=6.09 meeting at z=55.826, each with its own skirt down to
  // y=4.2, one facing +Z and one facing -Z.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float road_y = 6.09000349f;
  const float seam_z = 55.8258095f;

  // The fixture builder is what keeps the two skirts really being the
  // near-vertical faces the log describes, standing back to back across the
  // seam: a transcription that flipped one of them would otherwise pass no
  // matter what the contact code did.
  const Vec3F up(0.0f, 1.0f, 0.0f);
  const Vec3F toward_z(0.0f, 0.0f, 1.0f);
  const Vec3F away_z(0.0f, 0.0f, -1.0f);
  CollideSoupFixture fixture;
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.5137024f, 6.09001017f, 55.3653564f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), up);
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f), up);
  fixture.AddFacing(
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.4944534f, 6.09000921f, 56.2978592f),
      Vec3F(-76.2528305f, 6.09000921f, 56.289856f), up);
  fixture.AddFacing(
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2528305f, 6.09000921f, 56.289856f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), up);
  fixture.AddFacing(
      Vec3F(-77.3691711f, 4.19974661f, 55.8515434f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), toward_z);
  Si32 skirt_far = fixture.AddFacing(
      Vec3F(-77.3672943f, 4.19974613f, 55.810318f),
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), away_z);
  CollideSoup soup;
  fixture.Build(&soup);
  TEST_CHECK_(fixture.Ok(), "The fixture must describe the logged seam:\n%s",
      fixture.Problems().c_str());

  const CollisionTriangle &far_tri = fixture.Triangle(skirt_far);
  float skirt_top = std::max(std::max(far_tri.a.y, far_tri.b.y), far_tri.c.y);
  TEST_CHECK_(std::fabs(skirt_top - road_y) < 1.0e-3f,
      "The skirt's top edge must sit at the road level the sphere rolls "
      "on, skirt_top=%f road_y=%f", skirt_top, road_y);

  PhysicsStepConfig config;
  config.max_substep_move = 1.15f * units;
  config.skin = 0.06f * units;
  config.floor_normal_y = 0.45f;

  Vec3F start(-76.4620132f, 6.36600637f, 55.8240395f);
  Vec3F wish(-0.286900014f, 0.0f, 3.07990003f);
  float wish_xz = std::sqrt(wish.x * wish.x + wish.z * wish.z) * dt;
  TEST_CHECK_(std::fabs(start.y - radius - road_y) < 1.0e-3f,
      "The sphere must start resting on the road, bottom=%f road_y=%f",
      start.y - radius, road_y);
  TEST_CHECK_(start.z < seam_z && seam_z - start.z < radius,
      "The sphere must start just short of the seam it is about to cross, "
      "z=%f seam_z=%f", start.z, seam_z);

  PhysicsManifold manifold;
  Vec3F at = start;
  float least_xz = wish_xz;
  Si32 wall_frames = 0;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F before = at;
    SphereStepResult r = StepSphereBody(soup, &manifold, &at, radius, wish, dt,
        config);
    if (r.support.has_wall) {
      ++wall_frames;
    }
    Vec3F step = at - before;
    float step_xz = std::sqrt(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    TEST_CHECK_(std::fabs(at.y - radius - road_y) < 0.1f * units,
        "The sphere must stay on the road across the seam instead of "
        "being launched or sunk, frame %d bottom=%f road_y=%f", i,
        at.y - radius, road_y);
  }
  TEST_CHECK_(wall_frames == 0,
      "A level seam in the middle of a flat road must not be reported as a "
      "wall, %d of %d frames claimed one", wall_frames, kFrames);
  TEST_CHECK_(at.z > seam_z + radius,
      "The sphere must roll clear across the seam instead of stopping dead "
      "in the middle of the road, z=%f seam_z=%f", at.z, seam_z);
  TEST_CHECK_(least_xz > 0.85f * wish_xz,
      "Every frame across the seam must keep most of the wish, least=%f "
      "wish_xz=%f", least_xz, wish_xz);
}

void test_physics_sphere_body_step_above_center_still_blocks() {
  // Guard for the seam fix above: keeping a contact's own direction instead
  // of flattening it into the XZ plane is right only while the sphere is
  // riding over the top of the face. A step whose riser reaches above the
  // sphere's center is a wall, and the sphere must be stopped by it rather
  // than allowed to climb, or the fix would have traded a dead stop in the
  // middle of the road for a craft that walks up every kerb and barrier.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float floor_y = 6.09f;
  const float step_z = 55.826f;
  const float step_h = 0.5f;

  CollisionTriangle floor_a = MakeTri(
      Vec3F(-77.0f, floor_y, 55.0f),
      Vec3F(-76.0f, floor_y, 55.0f),
      Vec3F(-77.0f, floor_y, step_z));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-76.0f, floor_y, 55.0f),
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-77.0f, floor_y, step_z));
  CollisionTriangle riser_a = MakeTri(
      Vec3F(-77.0f, floor_y, step_z),
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-77.0f, floor_y + step_h, step_z));
  CollisionTriangle riser_b = MakeTri(
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-77.0f, floor_y + step_h, step_z));
  CollisionTriangle deck_a = MakeTri(
      Vec3F(-77.0f, floor_y + step_h, step_z),
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-77.0f, floor_y + step_h, 56.8f));
  CollisionTriangle deck_b = MakeTri(
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-76.0f, floor_y + step_h, 56.8f),
      Vec3F(-77.0f, floor_y + step_h, 56.8f));

  std::vector<Vec3F> pa = {floor_a.a, floor_b.a, riser_a.a, riser_b.a,
      deck_a.a, deck_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b, riser_a.b, riser_b.b,
      deck_a.b, deck_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c, riser_a.c, riser_b.c,
      deck_a.c, deck_b.c};
  std::vector<PhysicsMaterial> mats(6);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  PhysicsStepConfig config;
  config.max_substep_move = 1.15f * units;
  config.skin = 0.06f * units;
  config.floor_normal_y = 0.45f;

  Vec3F start(-76.5f, floor_y + radius, step_z - 0.3f);
  Vec3F wish(0.0f, 0.0f, 3.07990003f);
  TEST_CHECK_(floor_y + step_h > start.y,
      "The step must reach above the sphere's center for this to be a wall "
      "at all, step_top=%f center=%f", floor_y + step_h, start.y);

  PhysicsManifold manifold;
  Vec3F at = start;
  bool saw_wall = false;
  const Si32 kFrames = 30;
  for (Si32 i = 0; i < kFrames; ++i) {
    SphereStepResult r = StepSphereBody(soup, &manifold, &at, radius, wish, dt,
        config);
    if (r.support.has_wall) {
      saw_wall = true;
    }
  }
  TEST_CHECK_(saw_wall,
      "A riser reaching above the sphere's center must still be reported "
      "as a wall");
  TEST_CHECK_(at.z < step_z - radius + config.skin,
      "The sphere must be held short of a step taller than itself instead "
      "of climbing it, z=%f step_z=%f radius=%f", at.z, step_z, radius);
  TEST_CHECK_(at.y < floor_y + radius + 0.1f * units,
      "The sphere must not creep up the riser it is stopped by, y=%f "
      "resting=%f", at.y, floor_y + radius);
}

void test_physics_drop_probe_conventions_at_a_seam() {
  // Locks what the two downward probes promise, because Hover Racer's hover
  // control depends on the difference. A road tile closes its edge with a
  // skirt hanging straight down, so the skirt grazes the fall path of a
  // sphere resting right next to the seam: the swept sphere touches it at
  // time zero, DropSphereBody cannot start its drop and hands the start
  // position back, and a caller that reads that as a landing measures its
  // own probe lift-off as a deficit (which pinned the craft's lift at zero
  // in the middle of the road). QuerySphereHeightBelow only looks at
  // floor-facing faces, so it still reports the road, and it reports the
  // surface height -- the radius has to go back on to get the height a
  // resting center sits at.
  const float radius = 0.276000023f;
  const float road_y = 6.09000349f;
  const float seam_z = 55.8258095f;
  const float up = 0.0702245f;

  const Vec3F faces_up(0.0f, 1.0f, 0.0f);
  const Vec3F away_z(0.0f, 0.0f, -1.0f);
  CollideSoupFixture fixture;
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.5137024f, 6.09001017f, 55.3653564f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), faces_up);
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f), faces_up);
  // The skirt has to face the sphere for it to graze the fall path at all.
  fixture.AddFacing(
      Vec3F(-77.3672943f, 4.19974613f, 55.810318f),
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), away_z);
  CollideSoup soup;
  fixture.Build(&soup);
  TEST_CHECK_(fixture.Ok(), "The fixture must describe a road and the skirt "
      "hanging off its edge:\n%s", fixture.Problems().c_str());

  const float max_drop = up + 40.0f * 0.140449f;
  const float backoff = 0.01f;
  Vec3F resting(-76.4620132f, 6.36600637f, 55.8240395f);
  Vec3F from(resting.x, resting.y + up, resting.z);
  TEST_CHECK_(std::fabs(resting.y - radius - road_y) < 1.0e-3f,
      "The sphere must start resting on the road, bottom=%f road_y=%f",
      resting.y - radius, road_y);

  SphereDropResult landed = DropSphereBody(soup, from, max_drop, radius,
      backoff);
  TEST_CHECK_(landed.kind == SphereDropResult::Kind::kBlockedAtStart,
      "Next to the seam the drop cannot start at all and must say so "
      "instead of handing back a place that looks like a landing, kind=%d "
      "fall=%f", static_cast<int>(landed.kind), landed.fall);
  TEST_CHECK_(std::fabs(landed.position.y - from.y) < 1.0e-6f,
      "A blocked drop must leave the sphere where it started, from.y=%f "
      "position.y=%f", from.y, landed.position.y);
  TEST_CHECK_(landed.normal.y < 0.45f,
      "The blocker beside the seam is the skirt, so the way out of it must "
      "not look like a floor -- this is how a caller tells 'a wall grazes "
      "me' from 'I am already standing on the ground', normal=(%f,%f,%f)",
      landed.normal.x, landed.normal.y, landed.normal.z);

  float surface = -1000.0f;
  bool found = QuerySphereHeightBelow(soup, from.x, from.z, from.y, max_drop,
      radius, 0.45f, &surface);
  TEST_CHECK_(found,
      "The floor-only probe must still find the road the sphere rests on");
  TEST_CHECK_(std::fabs(surface - road_y) < 1.0e-3f,
      "The floor-only probe must report the road surface itself, with no "
      "safety margin shaved off a measurement nothing is placed at, "
      "surface=%f road_y=%f", surface, road_y);
  float err = resting.y - (surface + radius);
  TEST_CHECK_(std::fabs(err) < 1.0e-3f,
      "A sphere already resting on the road must measure as being exactly "
      "at its height, err=%f", err);

  // A measurement must not depend on where the probe was launched from,
  // which is exactly what a margin taken as a fraction of the path would
  // break: from four units up, a tenth of the fall was most of half a unit
  // of made-up height.
  Vec3F high(resting.x, resting.y + 4.0f, resting.z);
  float high_surface = -1000.0f;
  TEST_CHECK_(QuerySphereHeightBelow(soup, high.x, high.z, high.y, max_drop,
          radius, 0.45f, &high_surface),
      "The floor-only probe must find the road from well above it too");
  TEST_CHECK_(std::fabs(high_surface - road_y) < 1.0e-3f,
      "The reported height must not depend on how far above the probe "
      "started, from_y=%f surface=%f road_y=%f", high.y, high_surface,
      road_y);

  // Away from the seam both probes agree, which is what makes the stall
  // above a property of the grazing face and not of the fixture.
  Vec3F clear(-76.4f, road_y + radius, 55.5f);
  Vec3F clear_from(clear.x, clear.y + up, clear.z);
  SphereDropResult clear_landed = DropSphereBody(soup, clear_from, max_drop,
      radius, backoff);
  TEST_CHECK_(clear_landed.kind == SphereDropResult::Kind::kLanded,
      "Away from the seam the drop must land, kind=%d",
      static_cast<int>(clear_landed.kind));
  TEST_CHECK_(clear_landed.normal.y > 0.9f,
      "A landing on the road must report the road as what stopped it, "
      "normal=(%f,%f,%f)", clear_landed.normal.x, clear_landed.normal.y,
      clear_landed.normal.z);
  float clear_surface = -1000.0f;
  TEST_CHECK_(QuerySphereHeightBelow(soup, clear_from.x, clear_from.z,
          clear_from.y, max_drop, radius, 0.45f, &clear_surface),
      "The floor-only probe must find the road away from the seam too");
  float clear_gap = clear_landed.position.y - (clear_surface + radius);
  TEST_CHECK_(std::fabs(clear_gap - backoff) < 0.002f,
      "Away from the seam a landing must sit exactly the asked-for margin "
      "above what the floor probe reports, gap=%f backoff=%f", clear_gap,
      backoff);

  // Nothing below is its own answer, distinct from both a landing and a
  // blocked start.
  Vec3F over_edge(-76.4f, road_y + radius, 56.5f);
  SphereDropResult fell = DropSphereBody(soup, over_edge, 5.0f, radius,
      backoff);
  TEST_CHECK_(fell.kind == SphereDropResult::Kind::kNothingBelow,
      "With nothing below the drop must say so, kind=%d",
      static_cast<int>(fell.kind));
  TEST_CHECK_(std::fabs(fell.fall - 5.0f) < 1.0e-3f,
      "With nothing below, the drop must take the whole max_drop, "
      "fall=%f max_drop=%f", fell.fall, 5.0f);
  float none = -1000.0f;
  TEST_CHECK_(!QuerySphereHeightBelow(soup, over_edge.x, over_edge.z,
          over_edge.y, 5.0f, radius, 0.45f, &none),
      "With nothing below, the floor-only probe must report a miss instead "
      "of a height, reported=%f", none);
}

void test_physics_sweep_backoff_is_a_distance_not_a_fraction() {
  // The margin a sweep keeps off what it hits guards against float slop
  // around the contact, and that slop does not grow with how far the sweep
  // happened to travel. While the margin was a tenth of the path, the same
  // wall stopped a long sweep a whole unit early and a short one a hair
  // early, so a camera boom drawn far back hung visibly short of the wall
  // and a probe launched from high above read the ground as high above too.
  const float radius = 0.5f;
  const float backoff = 0.05f;
  const float wall_x = 0.0f;

  CollisionTriangle wall_a = MakeTri(Vec3F(0.0f, -5.0f, -5.0f),
      Vec3F(0.0f, 5.0f, 5.0f), Vec3F(0.0f, 5.0f, -5.0f));
  CollisionTriangle wall_b = MakeTri(Vec3F(0.0f, -5.0f, -5.0f),
      Vec3F(0.0f, -5.0f, 5.0f), Vec3F(0.0f, 5.0f, 5.0f));
  Vec3F wall_n = wall_a.n / Length(wall_a.n);
  TEST_CHECK_(wall_n.x < -0.99f,
      "The wall must face the oncoming sphere, n=(%f,%f,%f)", wall_n.x,
      wall_n.y, wall_n.z);

  std::vector<Vec3F> pa = {wall_a.a, wall_b.a};
  std::vector<Vec3F> pb = {wall_a.b, wall_b.b};
  std::vector<Vec3F> pc = {wall_a.c, wall_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  Vec3F target(2.0f, 0.0f, 0.0f);
  float expected_x = wall_x - radius - backoff;

  SweepSphereResult far_hit = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      target, radius, backoff);
  SweepSphereResult near_hit = SweepSphere(soup, Vec3F(-0.8f, 0.0f, 0.0f),
      target, radius, backoff);
  TEST_CHECK_(far_hit.kind == SweepSphereResult::Kind::kHit,
      "A sweep into the wall must report a hit, kind=%d",
      static_cast<int>(far_hit.kind));
  TEST_CHECK_(near_hit.kind == SweepSphereResult::Kind::kHit,
      "A short sweep into the wall must report a hit too, kind=%d",
      static_cast<int>(near_hit.kind));
  TEST_CHECK_(std::fabs(far_hit.position.x - expected_x) < 0.002f,
      "A sweep of twenty units must stop the asked-for margin short of the "
      "wall, x=%f expected=%f", far_hit.position.x, expected_x);
  TEST_CHECK_(std::fabs(near_hit.position.x - expected_x) < 0.002f,
      "A sweep of less than a unit must stop at the very same place, x=%f "
      "expected=%f", near_hit.position.x, expected_x);
  TEST_CHECK_(std::fabs(far_hit.position.x - near_hit.position.x) < 0.001f,
      "Distance traveled must not change where a sweep stops, far=%f "
      "near=%f", far_hit.position.x, near_hit.position.x);
  TEST_CHECK_(far_hit.normal.x < -0.99f,
      "A hit must report the way out of what was hit, normal=(%f,%f,%f)",
      far_hit.normal.x, far_hit.normal.y, far_hit.normal.z);

  SweepSphereResult exact = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      target, radius, 0.0f);
  TEST_CHECK_(std::fabs(exact.position.x - (wall_x - radius)) < 0.002f,
      "A zero margin must stop exactly at the touch, x=%f expected=%f",
      exact.position.x, wall_x - radius);

  SweepSphereResult clear = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      Vec3F(-10.0f, 0.0f, 0.0f), radius, backoff);
  TEST_CHECK_(clear.kind == SweepSphereResult::Kind::kClear,
      "A sweep that reaches its target must say the way was clear, kind=%d",
      static_cast<int>(clear.kind));
  TEST_CHECK_(std::fabs(clear.position.x + 10.0f) < 1.0e-5f,
      "A clear sweep must arrive at its target, x=%f", clear.position.x);
  TEST_CHECK_(LengthSquared(clear.normal) < 1.0e-10f,
      "A clear sweep has nothing to report a normal for, normal=(%f,%f,%f)",
      clear.normal.x, clear.normal.y, clear.normal.z);

  // Already touching is its own outcome: the position coming back unchanged
  // must not be readable as "the way ahead is blocked", which is the trap
  // the drop probe fell into at a road seam.
  SweepSphereResult stuck = SweepSphere(soup, Vec3F(-0.45f, 0.0f, 0.0f),
      target, radius, backoff);
  TEST_CHECK_(stuck.kind == SweepSphereResult::Kind::kStartOverlap,
      "A sweep that starts inside the wall must say so, kind=%d time=%f",
      static_cast<int>(stuck.kind), stuck.time);
  TEST_CHECK_(std::fabs(stuck.position.x + 0.45f) < 1.0e-6f,
      "A sweep blocked at the start must not move, x=%f", stuck.position.x);
}

void test_physics_step_report_merges_substeps() {
  // A Step can be cut into several substeps, and the report has to describe
  // the whole step. While it was simply the last substep's result, anything
  // that happened in an earlier one -- a floor touched, a wall hit, a
  // correction applied -- vanished, and a game that asks "what am I standing
  // on" once per frame saw a body that touched nothing.
  SphereStepResult early;
  early.contact_count = 3;
  early.outcome = ContactSolveOutcome::kDegenerate;
  early.position_correction = Vec3F(0.1f, 0.2f, 0.0f);
  early.velocity = Vec3F(9.0f, 0.0f, 0.0f);
  early.support.has_floor = true;
  early.support.floor_normal = Vec3F(0.0f, 1.0f, 0.0f);
  early.support.floor_material.grip = 0.25f;
  early.support.has_wall = true;
  early.support.floor_distance = 5.0f;
  early.support.has_floor_below = true;

  SphereStepResult late;
  late.contact_count = 1;
  late.outcome = ContactSolveOutcome::kFace;
  late.position_correction = Vec3F(0.0f, 0.3f, 0.0f);
  late.velocity = Vec3F(1.0f, 0.0f, 0.0f);
  late.support.has_ceiling = true;
  late.support.has_floor_below = true;
  late.support.floor_distance = 0.5f;

  SphereStepResult merged = MergeSphereStepResults(early, late);
  TEST_CHECK_(merged.support.has_floor,
      "A floor touched in an earlier substep must survive into the report");
  TEST_CHECK_(merged.support.floor_material.grip == 0.25f,
      "The surviving floor must bring its material along, grip=%f",
      merged.support.floor_material.grip);
  TEST_CHECK_(merged.support.has_wall && merged.support.has_ceiling,
      "Wall and ceiling flags must be the union of the substeps, wall=%d "
      "ceiling=%d", merged.support.has_wall ? 1 : 0,
      merged.support.has_ceiling ? 1 : 0);
  TEST_CHECK_(merged.contact_count == 3,
      "The busiest substep sets the contact count, got %d",
      merged.contact_count);
  TEST_CHECK_(merged.outcome == ContactSolveOutcome::kDegenerate,
      "The worst solver outcome must be the one reported, got %d",
      static_cast<int>(merged.outcome));
  TEST_CHECK_(std::fabs(merged.position_correction.y - 0.5f) < 1.0e-6f,
      "Corrections are cumulative and must add up, y=%f",
      merged.position_correction.y);
  TEST_CHECK_(std::fabs(merged.velocity.x - 1.0f) < 1.0e-6f,
      "The velocity is a state, so the last substep's is the step's, x=%f",
      merged.velocity.x);
  TEST_CHECK_(std::fabs(merged.support.floor_distance - 0.5f) < 1.0e-6f,
      "The measured height is about where the body ended up, so the last "
      "substep's reading wins, got %f", merged.support.floor_distance);

  SphereStepResult over_budget;
  over_budget.substep_budget_exhausted = true;
  TEST_CHECK_(MergeSphereStepResults(over_budget, late)
          .substep_budget_exhausted,
      "An exhausted substep budget must not be forgotten by the merge");

  // Same thing through the world, where the substeps actually happen: a
  // plate that ends halfway through the motion is a floor the body only
  // touches during the first substeps.
  CollisionTriangle plate_a = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(-5.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, 5.0f));
  CollisionTriangle plate_b = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(0.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, -5.0f));
  TEST_CHECK_(plate_a.n.y > 0.99f && plate_b.n.y > 0.99f,
      "test setup: the plate must face up, a=%f b=%f", plate_a.n.y,
      plate_b.n.y);
  std::vector<Vec3F> pa = {plate_a.a, plate_b.a};
  std::vector<Vec3F> pb = {plate_a.b, plate_b.b};
  std::vector<Vec3F> pc = {plate_a.c, plate_b.c};
  std::vector<PhysicsMaterial> mats(2);

  const float radius = 0.5f;
  const float dt = 1.0f / 60.0f;
  PhysicsStepConfig config;
  config.max_substep_move = 0.2f;
  config.max_substeps = 16;
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId id = world.AddSphere(Vec3F(-0.6f, radius, 0.0f), radius);
  world.SetWishVelocity(id, Vec3F(72.0f, 0.0f, 0.0f));
  world.Step(dt);
  TEST_CHECK_(world.Position(id).x > 0.2f,
      "The body must have run off the end of the plate for this to be "
      "about several substeps, x=%f", world.Position(id).x);
  TEST_CHECK_(world.Support(id).has_floor,
      "The plate touched during the first substeps must be in the step's "
      "report even though the body ended up past its edge");
  TEST_CHECK_(!world.SubstepBudgetExhausted(id),
      "With sixteen substeps allowed this motion must fit in the budget");

  // The last substep on its own sees nothing, which is what makes the check
  // above a statement about merging and not about the geometry.
  Vec3F past_edge = world.Position(id);
  SphereStepResult alone = StepSphereBody(world.Soup(), nullptr, &past_edge,
      radius, Vec3F(72.0f, 0.0f, 0.0f), dt / 7.0f, config);
  TEST_CHECK_(!alone.support.has_floor,
      "test setup: past the edge a lone substep must find no floor");

  // The cap on substeps is the anti-tunneling promise: a step that needs
  // more of them than allowed is moving further per substep than
  // max_substep_move, and that has to be said out loud.
  PhysicsStepConfig tight = config;
  tight.max_substeps = 2;
  PhysicsWorld tight_world;
  tight_world.SetConfig(tight);
  tight_world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId tight_id = tight_world.AddSphere(Vec3F(-4.0f, radius, 0.0f),
      radius);
  tight_world.SetWishVelocity(tight_id, Vec3F(72.0f, 0.0f, 0.0f));
  tight_world.Step(dt);
  TEST_CHECK_(tight_world.SubstepBudgetExhausted(tight_id),
      "A motion needing seven substeps with two allowed must report the "
      "budget as exhausted");
  tight_world.SetWishVelocity(tight_id, Vec3F(1.0f, 0.0f, 0.0f));
  tight_world.Step(dt);
  TEST_CHECK_(!tight_world.SubstepBudgetExhausted(tight_id),
      "A slow motion must clear the flag again");
}

void test_physics_body_separation_stays_out_of_the_mesh() {
  // Two bodies squeezed together push each other apart in XZ, and one of
  // them can be against a wall. While that push was written straight into
  // the position, a big enough shove carried the center clear through a
  // face, and the push-out afterwards helpfully settled the body on the far
  // side of the wall -- the same "write the position and hope" that the
  // solver exists to avoid, and that Hover Racer already paid for once.
  CollisionTriangle floor_a = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(-5.0f, 0.0f, 5.0f), Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(5.0f, 0.0f, 5.0f), Vec3F(5.0f, 0.0f, -5.0f));
  const float wall_x = 2.0f;
  CollisionTriangle wall_a = MakeTri(Vec3F(wall_x, 0.0f, -5.0f),
      Vec3F(wall_x, 3.0f, 5.0f), Vec3F(wall_x, 3.0f, -5.0f));
  CollisionTriangle wall_b = MakeTri(Vec3F(wall_x, 0.0f, -5.0f),
      Vec3F(wall_x, 0.0f, 5.0f), Vec3F(wall_x, 3.0f, 5.0f));
  TEST_CHECK_(floor_a.n.y > 0.99f && floor_b.n.y > 0.99f,
      "test setup: the floor must face up, a=%f b=%f", floor_a.n.y,
      floor_b.n.y);
  Vec3F wall_n = wall_a.n / Length(wall_a.n);
  TEST_CHECK_(wall_n.x < -0.99f,
      "test setup: the wall must face the bodies, n=(%f,%f,%f)", wall_n.x,
      wall_n.y, wall_n.z);

  std::vector<Vec3F> pa = {floor_a.a, floor_b.a, wall_a.a, wall_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b, wall_a.b, wall_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c, wall_a.c, wall_b.c};
  std::vector<PhysicsMaterial> mats(4);

  const float radius = 0.5f;
  const float dt = 1.0f / 60.0f;
  PhysicsStepConfig config;
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  // Nearly coincident, and all of the separation falls on the body next to
  // the wall: the shove is then more than a radius, which is exactly when
  // writing the position walks the center to the other side of the face.
  PhysicsBodyId pressed = world.AddSphere(Vec3F(1.2f, radius, 0.0f), radius);
  PhysicsBodyId holder = world.AddSphere(Vec3F(1.199f, radius, 0.0f),
      radius);
  world.SetPushWeight(pressed, 1.0f);
  world.SetPushWeight(holder, 0.0f);
  TEST_CHECK_(world.Position(pressed).x + radius < wall_x,
      "test setup: the pressed body must start clear of the wall, x=%f",
      world.Position(pressed).x);

  world.Step(dt);

  float x = world.Position(pressed).x;
  TEST_CHECK_(x + radius <= wall_x + config.skin,
      "The separated body must stay on this side of the wall, x=%f "
      "wall=%f radius=%f", x, wall_x, radius);
  TEST_CHECK_(x > 1.2f + 0.05f,
      "The separation must still have happened, otherwise the check above "
      "passes for the wrong reason, x=%f", x);
  TEST_CHECK_(std::fabs(world.Position(holder).x - 1.199f) < 1.0e-4f,
      "A body with no push weight must not be moved by the separation, "
      "x=%f", world.Position(holder).x);
  TEST_CHECK_(std::fabs(world.Position(pressed).y - radius) < config.skin,
      "Separating bodies must not lift them off the floor, y=%f", 
      world.Position(pressed).y);
}

void test_physics_world_query_conventions() {
  // The world's four queries are what a game reaches for, and every one of
  // them has a convention that is easy to get wrong in the caller and
  // impossible to see in a happy-path test: what a miss looks like, what
  // "the answer is where you already are" looks like, what happens exactly
  // at the end of the asked-for range, and how far short of geometry a
  // query stops. Hover Racer read three of these wrong in turn.
  const float radius = 0.5f;
  const float floor_y = 0.0f;
  const float wall_x = 2.0f;
  const Vec3F up(0.0f, 1.0f, 0.0f);
  const Vec3F towards_minus_x(-1.0f, 0.0f, 0.0f);

  CollideSoupFixture fixture;
  fixture.AddFacing(Vec3F(-5.0f, floor_y, -5.0f), Vec3F(-5.0f, floor_y, 5.0f),
      Vec3F(5.0f, floor_y, 5.0f), up);
  fixture.AddFacing(Vec3F(-5.0f, floor_y, -5.0f), Vec3F(5.0f, floor_y, 5.0f),
      Vec3F(5.0f, floor_y, -5.0f), up);
  fixture.AddFacing(Vec3F(wall_x, 0.0f, -5.0f), Vec3F(wall_x, 4.0f, 5.0f),
      Vec3F(wall_x, 4.0f, -5.0f), towards_minus_x);
  fixture.AddFacing(Vec3F(wall_x, 0.0f, -5.0f), Vec3F(wall_x, 0.0f, 5.0f),
      Vec3F(wall_x, 4.0f, 5.0f), towards_minus_x);

  PhysicsStepConfig config;
  config.skin = 0.04f;
  PhysicsWorld world;
  world.SetConfig(config);
  {
    CollideSoup soup;
    fixture.Build(&soup);
    TEST_CHECK_(fixture.Ok(), "The fixture must be a floor and a wall:\n%s",
        fixture.Problems().c_str());
    std::vector<Vec3F> pa, pb, pc;
    std::vector<PhysicsMaterial> mats;
    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      pa.push_back(soup.Triangle(i).a);
      pb.push_back(soup.Triangle(i).b);
      pc.push_back(soup.Triangle(i).c);
      mats.push_back(soup.Material(i));
    }
    world.SetStaticMesh(pa, pb, pc, mats, 8);
  }

  // A sweep keeps the world's own skin off what it hits, and says which of
  // the three things happened rather than leaving the caller to guess from a
  // position.
  const Vec3F lane(0.0f, floor_y + radius + 1.0f, 0.0f);
  SweepSphereResult hit = world.SweepSphereQuery(lane,
      Vec3F(wall_x + 1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(hit.kind == SweepSphereResult::Kind::kHit,
      "A sweep into the wall must report a hit, kind=%d",
      static_cast<int>(hit.kind));
  const float expected_x = wall_x - radius - config.skin;
  TEST_CHECK_(std::fabs(hit.position.x - expected_x) < 0.002f,
      "A sweep must stop the world's skin short of the wall, x=%f "
      "expected=%f skin=%f", hit.position.x, expected_x, config.skin);
  SweepSphereResult clear = world.SweepSphereQuery(lane,
      Vec3F(1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(clear.kind == SweepSphereResult::Kind::kClear
          && std::fabs(clear.position.x - 1.0f) < 1.0e-5f,
      "A sweep with room to spare must arrive where it was sent, kind=%d "
      "x=%f", static_cast<int>(clear.kind), clear.position.x);
  SweepSphereResult inside = world.SweepSphereQuery(
      Vec3F(wall_x - 0.4f * radius, lane.y, lane.z),
      Vec3F(wall_x + 1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(inside.kind == SweepSphereResult::Kind::kStartOverlap,
      "A sweep that starts inside the wall must say so and not look like "
      "a hit a hair ahead, kind=%d", static_cast<int>(inside.kind));

  // The drop probe: landing, nothing below, and blocked before it began are
  // three different answers.
  SphereDropResult landed = world.DropSphereQuery(
      Vec3F(0.0f, floor_y + radius + 2.0f, 0.0f), 10.0f, radius);
  TEST_CHECK_(landed.kind == SphereDropResult::Kind::kLanded,
      "A drop onto the floor must land, kind=%d",
      static_cast<int>(landed.kind));
  TEST_CHECK_(std::fabs(landed.position.y - (floor_y + radius + config.skin))
          < 0.003f,
      "A landing must sit the world's skin above the floor, y=%f "
      "expected=%f", landed.position.y, floor_y + radius + config.skin);
  SphereDropResult nothing = world.DropSphereQuery(
      Vec3F(0.0f, floor_y + 8.0f, 0.0f), 2.0f, radius);
  TEST_CHECK_(nothing.kind == SphereDropResult::Kind::kNothingBelow,
      "A drop that runs out of range must report a miss and not a landing "
      "at the bottom of the range, kind=%d",
      static_cast<int>(nothing.kind));
  TEST_CHECK_(std::fabs(nothing.fall - 2.0f) < 1.0e-3f,
      "A drop with nothing below must take the whole range, fall=%f",
      nothing.fall);
  SphereDropResult blocked = world.DropSphereQuery(
      Vec3F(wall_x - 0.4f * radius, floor_y + 2.0f, 0.0f), 10.0f, radius);
  TEST_CHECK_(blocked.kind == SphereDropResult::Kind::kBlockedAtStart,
      "A drop that starts inside the wall must say it never began, kind=%d",
      static_cast<int>(blocked.kind));

  // The height probe: max_drop is how far the center may fall, so a floor
  // `d` below a resting sphere is exactly d + radius of range away. The far
  // end of the range is inclusive, and the answer does not depend on how far
  // away the question was asked from -- the last of these was wrong while
  // the margin was a fraction of the path.
  const float from_y = floor_y + 3.0f;
  const float exact_range = from_y - floor_y - radius;
  float height = -1000.0f;
  TEST_CHECK_(world.HeightBelowQuery(0.0f, 0.0f, from_y, exact_range, radius,
          &height),
      "A floor exactly at the end of the range must be found, range=%f",
      exact_range);
  TEST_CHECK_(std::fabs(height - floor_y) < 1.0e-3f,
      "The height reported must be the floor itself, height=%f floor=%f",
      height, floor_y);
  float short_height = -1000.0f;
  TEST_CHECK_(!world.HeightBelowQuery(0.0f, 0.0f, from_y,
          exact_range - 0.1f, radius, &short_height),
      "A floor just past the end of the range must be a miss, reported=%f",
      short_height);
  TEST_CHECK_(short_height == -1000.0f,
      "A miss must leave the caller's number alone rather than writing "
      "something into it, got %f", short_height);
  float far_height = -1000.0f;
  TEST_CHECK_(world.HeightBelowQuery(0.0f, 0.0f, floor_y + 30.0f, 60.0f,
          radius, &far_height),
      "The floor must be found from far above as well");
  TEST_CHECK_(std::fabs(far_height - height) < 1.0e-3f,
      "A measurement must not change with the distance it was taken from, "
      "near=%f far=%f", height, far_height);

  // Unsticking answers about the state it leaves the center in, not about
  // whether it had work to do: true is "free where it is now", and a sphere
  // that overlapped nothing is free without being moved.
  Vec3F free_center(0.0f, floor_y + radius + 1.0f, 0.0f);
  Vec3F untouched = free_center;
  TEST_CHECK_(world.UnstickQuery(&free_center, radius),
      "A sphere touching nothing is already free and must be reported so");
  TEST_CHECK_(Length(free_center - untouched) < 1.0e-6f,
      "A sphere that needed no unsticking must not be moved, moved by %f",
      Length(free_center - untouched));
  Vec3F buried(0.0f, floor_y + 0.2f * radius, 0.0f);
  TEST_CHECK_(world.UnstickQuery(&buried, radius),
      "A sphere sunk into the floor must be reported as free afterwards");
  TEST_CHECK_(buried.y >= floor_y + radius - 1.0e-3f,
      "An unstuck sphere must end up out of the floor, y=%f floor+r=%f",
      buried.y, floor_y + radius);
  TEST_CHECK_(std::fabs(buried.x) < 1.0e-4f && std::fabs(buried.z) < 1.0e-4f,
      "Unsticking must push straight out of the face and not slide the "
      "sphere along it, x=%f z=%f", buried.x, buried.z);
}

void test_physics_broad_phase_offers_every_overlapping_triangle() {
  // The broad-phase grid is allowed to offer too much and never too little:
  // the narrow phase filters candidates, so a triangle that is not offered
  // simply does not exist as far as collision is concerned. That failure is
  // invisible in ordinary play and was found once already as "a big
  // triangle is not found", when a face spanning many cells was only
  // registered in the cells its corners fell into. Two checks here: a
  // random mesh against a full scan, and one long triangle asked for from
  // every cell it crosses.
  Ui64 seed = 0x5eed1234u;
  auto next = [&seed]() {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<float>((seed >> 33) & 0xffffff)
        / static_cast<float>(0x1000000);
  };
  auto span = [&next](float lo, float hi) {
    return lo + (hi - lo) * next();
  };

  std::vector<Vec3F> pa, pb, pc;
  for (Si32 i = 0; i < 220; ++i) {
    // A mix of sizes on purpose: the small ones fit inside a cell, the big
    // ones cross many, and only the big ones can expose a grid that
    // registers a face by its corners.
    float reach = (i % 11 == 0) ? span(20.0f, 60.0f) : span(0.2f, 3.0f);
    Vec3F a(span(-40.0f, 40.0f), span(-10.0f, 10.0f), span(-40.0f, 40.0f));
    Vec3F b = a + Vec3F(span(-reach, reach), span(-reach, reach),
        span(-reach, reach));
    Vec3F c = a + Vec3F(span(-reach, reach), span(-reach, reach),
        span(-reach, reach));
    CollisionTriangle probe;
    if (!probe.Set(a, b, c)) {
      continue;
    }
    pa.push_back(a);
    pb.push_back(b);
    pc.push_back(c);
  }
  std::vector<PhysicsMaterial> mats(pa.size());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 16);
  TEST_CHECK_(soup.TriangleCount() > 180,
      "test setup: the random mesh must have kept its faces, got %d",
      soup.TriangleCount());

  Si32 boxes_with_hits = 0;
  for (Si32 q = 0; q < 400; ++q) {
    float half = span(0.05f, 4.0f);
    Vec3F at(span(-45.0f, 45.0f), span(-12.0f, 12.0f), span(-45.0f, 45.0f));
    Bound3F box(at.x - half, at.x + half, at.y - half, at.y + half,
        at.z - half, at.z + half);

    std::vector<bool> offered(static_cast<size_t>(soup.TriangleCount()),
        false);
    Si32 offers = 0;
    soup.ForEachNear(box, [&offered, &offers](Si32 index) {
      offered[static_cast<size_t>(index)] = true;
      ++offers;
    });
    TEST_CHECK_(offers == static_cast<Si32>(std::count(offered.begin(),
            offered.end(), true)),
        "The grid must offer each triangle once per query, offers=%d "
        "distinct=%d", offers,
        static_cast<Si32>(std::count(offered.begin(), offered.end(), true)));

    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      const CollisionTriangle &tri = soup.Triangle(i);
      Bound3F tri_box(
          std::min(tri.a.x, std::min(tri.b.x, tri.c.x)),
          std::max(tri.a.x, std::max(tri.b.x, tri.c.x)),
          std::min(tri.a.y, std::min(tri.b.y, tri.c.y)),
          std::max(tri.a.y, std::max(tri.b.y, tri.c.y)),
          std::min(tri.a.z, std::min(tri.b.z, tri.c.z)),
          std::max(tri.a.z, std::max(tri.b.z, tri.c.z)));
      bool overlaps = tri_box.max_x >= box.min_x && tri_box.min_x <= box.max_x
          && tri_box.max_y >= box.min_y && tri_box.min_y <= box.max_y
          && tri_box.max_z >= box.min_z && tri_box.min_z <= box.max_z;
      if (!overlaps) {
        continue;
      }
      ++boxes_with_hits;
      if (!TEST_CHECK_(offered[static_cast<size_t>(i)],
          "The grid did not offer triangle %d, whose box "
          "[%f..%f][%f..%f][%f..%f] overlaps the query "
          "[%f..%f][%f..%f][%f..%f]", i, tri_box.min_x, tri_box.max_x,
          tri_box.min_y, tri_box.max_y, tri_box.min_z, tri_box.max_z,
          box.min_x, box.max_x, box.min_y, box.max_y, box.min_z,
          box.max_z)) {
        return;
      }
    }
  }
  TEST_CHECK_(boxes_with_hits > 100,
      "test setup: the random queries must have found something to compare, "
      "hits=%d", boxes_with_hits);

  // One face across the whole mesh, asked for from a small box walking
  // along it: this is the shape of the bug that was already fixed once, and
  // it must stay fixed cell by cell rather than on average.
  const float far_end = 90.0f;
  std::vector<Vec3F> la = {Vec3F(-far_end, 0.0f, -far_end),
      Vec3F(-5.0f, -1.0f, -5.0f)};
  std::vector<Vec3F> lb = {Vec3F(far_end, 0.0f, far_end),
      Vec3F(5.0f, -1.0f, -5.0f)};
  std::vector<Vec3F> lc = {Vec3F(-far_end, 2.0f, -far_end + 1.0f),
      Vec3F(5.0f, -1.0f, 5.0f)};
  std::vector<PhysicsMaterial> long_mats(2);
  CollideSoup long_soup;
  long_soup.Build(la, lb, lc, long_mats, 64);
  TEST_CHECK_(long_soup.TriangleCount() == 2,
      "test setup: the long mesh must keep both faces, got %d",
      long_soup.TriangleCount());

  const Si32 kSteps = 200;
  Si32 asked = 0;
  for (Si32 i = 0; i <= kSteps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(kSteps);
    float x = -far_end + 2.0f * far_end * t;
    float z = x;
    Bound3F box(x - 0.05f, x + 0.05f, -1.0f, 3.0f, z - 0.05f, z + 0.05f);
    bool found = false;
    long_soup.ForEachNear(box, [&found](Si32 index) {
      if (index == 0) {
        found = true;
      }
    });
    ++asked;
    if (!TEST_CHECK_(found,
        "A face crossing the whole mesh must be offered from every cell it "
        "passes through, and it was not at (%f, %f)", x, z)) {
      return;
    }
  }
  TEST_CHECK_(asked == kSteps + 1,
      "test setup: every step along the face must have been asked, "
      "asked=%d", asked);
}

void test_physics_fixture_builder_catches_bad_geometry() {
  // Every physics fixture in this file is transcribed geometry, and the two
  // ways transcription goes wrong are silent: a degenerate face is dropped
  // by the mesh builder without a word, and a face wound the other way
  // still builds -- it just points the wrong direction, which comes out
  // several layers later as a sphere falling through a floor. The builder
  // exists to turn both into a message that names the face.
  const Vec3F up(0.0f, 1.0f, 0.0f);

  CollideSoupFixture good;
  Si32 first = good.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(-1.0f, 0.0f, 1.0f), Vec3F(1.0f, 0.0f, 1.0f), up);
  Si32 second = good.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(1.0f, 0.0f, 1.0f), Vec3F(1.0f, 0.0f, -1.0f), up);
  TEST_CHECK_(first == 0 && second == 1,
      "Faces must come back numbered in the order they were added, got %d "
      "and %d", first, second);
  CollideSoup soup;
  good.Build(&soup);
  TEST_CHECK_(good.Ok(), "A correct fixture must report nothing wrong:\n%s",
      good.Problems().c_str());
  TEST_CHECK_(soup.TriangleCount() == 2,
      "The built mesh must keep both faces, got %d", soup.TriangleCount());
  TEST_CHECK_(good.NormalLines().size() == 2,
      "There must be one normal line per face, got %d",
      static_cast<int>(good.NormalLines().size()));

  CollideSoupFixture flipped;
  flipped.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f), Vec3F(1.0f, 0.0f, 1.0f),
      Vec3F(-1.0f, 0.0f, 1.0f), up);
  TEST_CHECK_(!flipped.Ok(),
      "A face wound the other way must not pass as facing up");
  TEST_CHECK_(flipped.Problems().find("order of its vertices")
          != std::string::npos,
      "The complaint must say what to look at, got: %s",
      flipped.Problems().c_str());

  CollideSoupFixture degenerate;
  Si32 bad = degenerate.Add(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(1.0f, 0.0f, 0.0f),
      Vec3F(2.0f, 0.0f, 0.0f));
  TEST_CHECK_(bad < 0,
      "A degenerate face must not be handed back as a usable one, got %d",
      bad);
  TEST_CHECK_(!degenerate.Ok() &&
          degenerate.Problems().find("degenerate") != std::string::npos,
      "A degenerate face must be reported, got: %s",
      degenerate.Problems().c_str());
  TEST_CHECK_(degenerate.Count() == 0,
      "A degenerate face must not be kept, count=%d", degenerate.Count());
}

void test_physics_support_measures_height_above_floor() {
  // A hover control has to start braking while still in the air, so it needs
  // the distance to the ground and not just "am I touching". Before the
  // support carried that measurement, Hover Racer probed on its own, drifted
  // away from the engine's own reading and froze mid-road on it; the point of
  // these checks is that the engine's own answer is exact, opt-in, and fresh
  // after a teleport.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  TEST_CHECK_(floor_a.n.y > 0.99f && floor_b.n.y > 0.99f,
      "test setup: both floor triangles must face up, a=%f b=%f",
      floor_a.n.y, floor_b.n.y);
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  const float dt = 1.0f / 60.0f;
  const Vec3F still(0.0f, 0.0f, 0.0f);
  PhysicsStepConfig config;
  config.support_probe = 6.0f;

  Vec3F resting(0.0f, radius, 0.0f);
  SphereStepResult rest_step = StepSphereBody(soup, nullptr, &resting, radius,
      still, dt, config);
  TEST_CHECK_(rest_step.support.has_floor_below,
      "A sphere on the floor must find a floor below it");
  TEST_CHECK_(std::fabs(rest_step.support.floor_distance) < config.skin,
      "A resting sphere must measure as zero away from the floor, got %f "
      "skin=%f", rest_step.support.floor_distance, config.skin);

  // Airborne, and measured at two very different heights: the answer has to
  // be the real height both times. A margin proportional to the fall (which
  // is what the sweeps keep for placing bodies) would show up here as a
  // reading that grows wrong with distance.
  const float heights[2] = {0.5f, 5.0f};
  for (Si32 i = 0; i < 2; ++i) {
    Vec3F pos(0.0f, radius + heights[i], 0.0f);
    SphereStepResult step = StepSphereBody(soup, nullptr, &pos, radius,
        still, dt, config);
    TEST_CHECK_(step.support.has_floor_below,
        "The floor must be found from %f up", heights[i]);
    TEST_CHECK_(!step.support.has_floor,
        "Hanging %f above the floor is not resting on it", heights[i]);
    TEST_CHECK_(std::fabs(step.support.floor_distance - heights[i]) < 1.0e-3f,
        "The measured height must be the real one, measured=%f real=%f",
        step.support.floor_distance, heights[i]);
  }

  Vec3F too_high(0.0f, radius + config.support_probe + 1.0f, 0.0f);
  SphereStepResult high_step = StepSphereBody(soup, nullptr, &too_high,
      radius, still, dt, config);
  TEST_CHECK_(!high_step.support.has_floor_below,
      "A floor past support_probe must not be reported, distance=%f "
      "probe=%f", high_step.support.floor_distance, config.support_probe);

  Vec3F off_the_edge(20.0f, radius, 20.0f);
  SphereStepResult off_step = StepSphereBody(soup, nullptr, &off_the_edge,
      radius, still, dt, config);
  TEST_CHECK_(!off_step.support.has_floor_below,
      "With no floor under it at all the measurement must report nothing, "
      "distance=%f", off_step.support.floor_distance);

  // Opt-in: a caller with no use for the measurement must not pay for it.
  PhysicsStepConfig no_probe;
  no_probe.support_probe = 0.0f;
  Vec3F above(0.0f, radius + 1.0f, 0.0f);
  SphereStepResult no_probe_step = StepSphereBody(soup, nullptr, &above,
      radius, still, dt, no_probe);
  TEST_CHECK_(!no_probe_step.support.has_floor_below,
      "With support_probe at zero nothing may be measured, distance=%f",
      no_probe_step.support.floor_distance);

  // Through a real fall the measurement must track the height the geometry
  // says, every step, not just at the ends.
  PhysicsManifold manifold;
  Vec3F falling(0.0f, radius + 4.0f, 0.0f);
  float lift = 0.0f;
  float worst_error = 0.0f;
  bool ever_missed = false;
  for (Si32 i = 0; i < 120; ++i) {
    lift -= 9.8f * dt;
    SphereStepResult step = StepSphereBody(soup, &manifold, &falling, radius,
        Vec3F(0.0f, lift, 0.0f), dt, config);
    if (step.support.has_floor) {
      lift = 0.0f;
    }
    if (!step.support.has_floor_below) {
      ever_missed = true;
      continue;
    }
    float truth = falling.y - radius;
    float error = std::fabs(step.support.floor_distance - truth);
    if (error > worst_error) {
      worst_error = error;
    }
  }
  TEST_CHECK_(!ever_missed,
      "The floor must stay found for the whole fall from four units up");
  TEST_CHECK_(worst_error < 1.0e-3f,
      "The measurement must follow the true height through the fall, worst "
      "error=%f", worst_error);
  TEST_CHECK_(falling.y <= radius + config.skin,
      "The falling sphere must land, y=%f", falling.y);

  // A teleport is the one moment the last step's measurement is about the
  // wrong place, so the world refreshes it right there. A hover control
  // reads the support before it steps, and on the frame after a spawn that
  // would otherwise be an answer about where the body used to be.
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId id = world.AddSphere(Vec3F(0.0f, radius, 0.0f), radius);
  world.Teleport(id, Vec3F(0.0f, radius + 2.5f, 0.0f));
  TEST_CHECK_(world.Support(id).has_floor_below,
      "Right after a teleport the support must already know about the "
      "floor below the new place");
  TEST_CHECK_(std::fabs(world.Support(id).floor_distance - 2.5f) < 1.0e-3f,
      "The refreshed measurement must be about the new place, got %f "
      "expected 2.5", world.Support(id).floor_distance);
  world.Teleport(id, Vec3F(20.0f, radius, 20.0f));
  TEST_CHECK_(!world.Support(id).has_floor_below,
      "Teleported off the mesh, the support must stop claiming a floor "
      "below, distance=%f", world.Support(id).floor_distance);

  // The support has to be a value. While it was a reference into the vector
  // the bodies live in, registering another body could reallocate that
  // vector under a caller still holding the reference -- silent, and of the
  // kind that only shows up once the field grows.
  static_assert(std::is_same<decltype(world.Support(id)),
      SphereBodySupport>::value,
      "PhysicsWorld::Support must return a value, not a reference into the "
      "body vector");
  SphereBodySupport kept = world.Support(id);
  for (Si32 i = 0; i < 32; ++i) {
    world.AddSphere(Vec3F(100.0f + static_cast<float>(i), radius, 100.0f),
        radius);
  }
  TEST_CHECK_(kept.has_floor_below == world.Support(id).has_floor_below &&
          std::fabs(kept.floor_distance - world.Support(id).floor_distance)
              < 1.0e-6f,
      "A support taken before the field grew must still describe the same "
      "body");

  // Moving a body by hand is the one thing a solver cannot account for, so
  // the world counts it and a game can assert the count is zero on the
  // frames it did not mean to teleport. Hover Racer needed exactly this
  // check, and had to watch positions itself to get it.
  TEST_CHECK_(world.TeleportsSinceStep() == 2,
      "Both teleports must be counted, got %d", world.TeleportsSinceStep());
  world.Step(1.0f / 60.0f);
  TEST_CHECK_(world.TeleportsSinceStep() == 0,
      "A step must clear the teleport count, got %d",
      world.TeleportsSinceStep());
  Vec3F before_step = world.Position(id);
  world.SetWishVelocity(id, Vec3F(1.0f, 0.0f, 0.0f));
  world.Step(1.0f / 60.0f);
  TEST_CHECK_(world.TeleportsSinceStep() == 0,
      "Driving a body must not look like a teleport, got %d",
      world.TeleportsSinceStep());
  TEST_CHECK_(Length(world.Position(id) - before_step) > 1.0e-4f,
      "The driven body must have moved for that check to mean anything");
}

TEST_LIST = {
//  {"Tga oom", test_tga_oom},
  {"Rgba", test_rgba},
  {"Radix sort", test_radix_sort},
  {"Radix sort correctness", test_radix_sort_correctness},
  {"Rgb", test_rgb},
  {"File operations", test_file_operations},
  {"Random generation", test_random},
  {"Localization basic load", test_localization_basic_load},
  {"Localization simple substitution", test_localization_simple_substitution},
  {"Localization plural English", test_localization_plural_english},
  {"Localization plural Russian", test_localization_plural_russian},
  {"Localization select", test_localization_select},
  {"Localization complex pattern", test_localization_complex_pattern},
  {"Localization nested plural/select", test_localization_nested_plural_select},
  {"Localization multi-locale CSV", test_localization_multi_locale_csv},
  {"Localization fallback", test_localization_fallback},
  {"Localization merge files", test_localization_merge_files},
  {"Localization Loc() function", test_localization_loc_function},
  {"Localization FormatPattern direct", test_localization_format_pattern_direct},
  {"Localization ordinal English", test_localization_ordinal_english},
  {"TTF font loading", test_ttf_font_loading},
  {"Find system font", test_find_system_font},
  {"Load system font", test_load_system_font},
  {"JSON parse string", test_json_parse_string},
  {"JSON parse file", test_json_parse_file},
  {"JSON build and serialize", test_json_build_and_serialize},
  {"JSON type conversions", test_json_type_conversions},
  {"JSON iteration", test_json_iteration},
  {"JSON error handling", test_json_error_handling},
  {"JSON modification", test_json_modification},
  {"JSON comparison", test_json_comparison},
  {"DataWriter empty initial write", test_data_writer_empty_initial_write},
  {"DataWriter multiple writes no overlap", test_data_writer_multiple_writes_no_overlap},
  {"DataWriter Ui16", test_data_writer_uint16},
  {"DataWriter Ui32", test_data_writer_uint32},
  {"DataWriter Ui64", test_data_writer_uint64},
  {"DataWriter float", test_data_writer_float},
  {"DataWriter mixed sequence", test_data_writer_mixed_sequence},
  {"DataWriter Ui16 array", test_data_writer_uint16array},
  {"DataReader advances pointer", test_data_reader_advances_pointer},
  {"Data roundtrip all types", test_data_roundtrip_all_types},
  {"Data roundtrip arrays", test_data_roundtrip_arrays},
  {"DataReader past end", test_data_reader_past_end},
  {"DataWriter large sequence", test_data_writer_large_sequence},
  {"Sound resample returns nullptr", test_sound_resample_returns_nullptr},
  {"Sound 8-bit stereo wrong offset", test_sound_8bit_stereo_wrong_offset},
  {"Sound 8-bit signed vs unsigned", test_sound_8bit_signed_vs_unsigned},
  {"Quaternion ToMat33F sign error", test_quat_to_mat33f_sign},
  {"Quaternion ToPartialMatrix33F sign error", test_quat_to_partial_mat33f_sign},
  {"Quaternion slerp uses unnormalized inputs", test_quat_slerp_unnormalized},
  {"Quaternion slerp negative dot product", test_quat_slerp_negative_dot},
  {"Transform3F Inverse round-trip", test_transform3f_inverse},
  {"Transform3F Inverse composition is identity", test_transform3f_inverse_composition},
  {"Skeleton bone 0 cannot be parent", test_skeleton_bone0_cannot_be_parent},
  {"Skeleton root not orphaned after AddBone(0)", test_skeleton_root_not_orphaned},
  {"Utf16ToUtf8 spurious null byte", test_utf16_to_utf8_spurious_null},
  {"Utf32ToUtf8 ASCII", test_utf32_to_utf8_ascii},
  {"Utf32ToUtf8 multibyte", test_utf32_to_utf8_multibyte},
  {"Utf16ToUtf8 BMP codepoint", test_utf16_to_utf8_bmp},
  {"Utf16ToUtf8 surrogate pair", test_utf16_to_utf8_surrogate},
  {"Utf32Reader round-trip", test_utf32_reader_roundtrip},
  {"Utf8Codepoint sizes", test_utf8_codepoint_sizes},
  {"IsUtf8Continuation", test_is_utf8_continuation},
  {"Utf8NextCharPos", test_utf8_next_char_pos},
  {"Utf8PrevCharPos", test_utf8_prev_char_pos},
  {"Mesh PLY readline does not strip CRLF", test_mesh_ply_readline_crlf},
  {"Mesh vertex attrib write overflow", test_mesh_vertex_attrib_write_overflow},
  {"Mesh extrude face covers all edges", test_mesh_extrude_face_covers_all_edges},
  {"Mesh vertex elements keep attribute names", test_mesh_named_elements},
  {"Mesh capacity from Init is final", test_mesh_capacity_is_final},
  {"Mesh Expand grows every stream", test_mesh_expand_every_stream},
  {"SetRandomSeed makes the sequence repeat", test_random_seed_determinism},
  {"Random state continues the sequence", test_random_state_continues_the_sequence},
  {"Random state survives a trip through text", test_random_state_text_round_trip},
  {"Sprite saves png and tga", test_sprite_save_png},
  {"Sprite reads back a png it saved", test_sprite_load_png_round_trip},
  {"Sprite reads a palette png with transparency", test_sprite_load_png_palette},
  {"Sprite refuses a broken png", test_sprite_load_png_refuses_garbage},
  {"SetRotationX vs SetRotationAxisAngle4", test_rotation_x_vs_axis_angle},
  {"SetRotationY vs SetRotationAxisAngle4", test_rotation_y_vs_axis_angle},
  {"SetRotationZ vs SetRotationAxisAngle4 (control)", test_rotation_z_vs_axis_angle},
  {"Rotation XY composition vs AxisAngle4", test_rotation_xy_composition_vs_axis_angle},
  {"Colorize blend R/B vs G inconsistency", test_colorize_blend_rb_vs_g_inconsistency},
  {"Colorize blend full vs partial alpha discontinuity", test_colorize_blend_full_vs_partial_alpha_discontinuity},
  {"Quaternion ToAxisAngle acos out of range", test_quat_to_axis_angle_acos_out_of_range},
  {"Quaternion ToAxisAngle normalized round-trip", test_quat_to_axis_angle_normalized_roundtrip},
  {"Transform3F scale affects point", test_transform3f_scale_affects_point},
  {"Transform3F scale affects composition", test_transform3f_scale_affects_transform_composition},
  {"Transform3F Inverse respects scale", test_transform3f_inverse_respects_scale},
  {"Sprite Reference zero-size source", test_sprite_reference_zero_size},
  {"HW sprite sub-region draws correctly", test_hw_sprite_subregion_draws_correctly},
  {"Quat matrix vs AxisAngle consistency", test_quat_matrix_vs_axis_angle},
  {"Rotation XYZ vs AxisAngle consistency", test_rotation_xyz_vs_axis_angle_consistency},
  {"Euler4 equals Rz*Ry*Rx", test_euler4_equals_composition},
  {"Quat ToMat33F vs SetRotationQuat 4x4", test_quat_tomat33_vs_setrotationquat},
  {"ExtractEuler roundtrip Euler4", test_extract_euler_roundtrip_euler4},
  {"ExtractEuler pure X rotation", test_extract_euler_pure_x},
  {"ExtractEuler pure Y rotation", test_extract_euler_pure_y},
  {"ExtractEuler pure Z rotation", test_extract_euler_pure_z},
  {"ExtractEuler gimbal lock", test_extract_euler_gimbal_lock},
  {"ExtractEuler ignores translation", test_extract_euler_ignores_translation},
  {"ExtractEuler gimbal branch reads m11", test_extract_euler_gimbal_branch_reads_m11},
  {"All rotation matrices are orthogonal", test_rotation_matrices_are_orthogonal},
  {"Translation transforms point correctly", test_translation_transforms_point},
  {"SetLookat produces orthonormal basis", test_lookat_orthonormal},
  {"SetLookat degenerate returns identity", test_lookat_degenerate_returns_identity},
  {"CSV round-trip: separator in field", test_csv_roundtrip_separator_in_field},
  {"CSV round-trip: quotes in field", test_csv_roundtrip_quotes_in_field},
  {"Panel anchor: no negative size on parent shrink", test_panel_anchor_no_negative_size},
  {"SetPerspective y == cot(fovy/2)", test_perspective_y_equals_cot_half_fovy},
  {"SetPerspective matches SetFrustumPerspective", test_perspective_matches_frustum_perspective},
  {"SetPerspectiveTiled identity matches SetPerspective", test_perspective_tiled_matches_perspective},
  {"SetOrtho symmetric maps corners to NDC", test_ortho_symmetric_corners},
  {"SetOrtho asymmetric maps corners to NDC", test_ortho_asymmetric_corners},
  {"SetOrtho center maps to origin", test_ortho_center_maps_to_origin},
  {"SetOrtho consistent with Perspective at z=near", test_ortho_consistent_with_perspective},
  {"CanonicalizePath non-existent path", test_canonicalize_nonexistent_path},
  {"CanonicalizePath before and after file create", test_canonicalize_before_and_after_create},
  {"RelativePathFromTo ignores a trailing slash", test_relative_path_from_to},
  {"CanonicalizeArgvPath uses the startup directory", test_canonicalize_argv_path},
  {"DescribeFilePath explains a missing file", test_describe_file_path},
  {"DoesFileExist and ChangeCurrentDirectory", test_file_existence_and_current_directory},
  {"Font border survives colorize", test_font_border_survives_colorize},
  {"Font loaders apply the border", test_font_loads_with_border},
  {"Editbox reports text change and edit done", test_editbox_reports_text_change_and_edit_done},
  {"Editbox takes text from any keyboard layout", test_editbox_accepts_any_layout},
  {"Typed characters of a message", test_typed_characters_of_a_message},
  {"Typed text and generic modifiers", test_typed_text_and_generic_modifiers},
  {"Top left coordinate helpers", test_top_left_coordinate_helpers},
  {"IsPointInSprite follows the drawing", test_is_point_in_sprite_follows_the_drawing},
  {"Mouse arrives in backbuffer pixels", test_mouse_arrives_in_backbuffer_pixels},
  {"Hardware rectangle fill and composition order", test_hw_rectangle_fill_and_composition_order},
  {"Window title names the program", test_window_title_names_the_program},
  {"KeyDownSeconds measures the hold", test_key_down_seconds_measures_the_hold},
  {"Loopback sockets connect, talk and close",
    test_loopback_sockets_connect_talk_and_close},
  {"Panel IsInside answers whose click it is",
    test_panel_is_inside_answers_for_the_interface},
  {"Hidden editbox and progressbar are not there",
    test_hidden_editbox_and_progressbar_are_not_there},
  {"IsInside counts editbox and scrollbar",
    test_is_inside_counts_editbox_and_scrollbar},
  {"Panel anchor and dock follow the calls",
    test_panel_anchor_and_dock_follow_the_calls},
  {"Tab skips hidden panels", test_tab_skips_hidden_panels},
  {"Hidden and disabled panels never handle input",
    test_hidden_and_disabled_panels_never_handle_input},
  {"Startup mode decider is asked at startup", test_startup_mode_decider},
  {"Log file is findable, clearable and rotated",
      test_log_file_is_findable_clearable_and_rotated},
  {"Main window close handler decides the exit", test_main_window_close_handler},
  {"FBX rejects an unsupported version", test_fbx_rejects_unsupported_version},
  {"FBX rejects a buffer too short for a header", test_fbx_rejects_a_buffer_too_short_for_a_header},
  {"FBX rejects a record past the end of file", test_fbx_rejects_record_past_end_of_file},
  {"FBX loads a scene without objects", test_fbx_empty_scene_loads},
  {"FBX triangulates mesh geometry", test_fbx_mesh_geometry_is_triangulated},
  {"FBX reads the material diffuse color", test_fbx_material_diffuse_color},
  {"FBX reads the texture file names", test_fbx_texture_file_names},
  {"FBX fills the material texture slots", test_fbx_material_texture_slots},
  {"FBX accepts a qualified texture property", test_fbx_material_texture_qualified_property},
  {"FBX ignores an unknown texture property", test_fbx_material_ignores_unknown_property},
  {"FBX takes a missing texture path from the video", test_fbx_video_supplies_missing_texture_file_name},
  {"FBX reads a lowercase video Filename", test_fbx_video_lowercase_file_name_element},
  {"FBX keeps the texture path over the video one", test_fbx_video_does_not_override_texture_file_name},
  {"FBX resolves a layered texture for a material", test_fbx_layered_texture_reaches_material},
  {"FBX keeps the bottom layer of a layered texture", test_fbx_layered_texture_keeps_the_bottom_layer},
  {"FBX resolves video through a layered texture", test_fbx_video_behind_layered_texture},
  {"FBX tolerates an unusable connection", test_fbx_tolerates_unexpected_connection},
  {"FBX reports the object types it parsed", test_fbx_object_types_and_count},
  {"FBX maps the file up axis onto the enum", test_fbx_global_settings_up_axis},
  {"FBX passes the other global settings through", test_fbx_global_settings_pass_other_fields_through},
  {"FBX accepts a file without global settings", test_fbx_missing_global_settings_is_not_an_error},
  {"LoadFbx rejects an empty buffer", test_loadfbx_empty_buffer_fails},
  {"LoadFbx rejects a scene without meshes", test_loadfbx_scene_without_meshes_fails},
  {"LoadFbx turns a quad into two triangles", test_loadfbx_quad_is_two_triangles},
  {"LoadFbx maps Z-up onto Y-up", test_loadfbx_z_up_swaps_y_and_z},
  {"LoadFbx keeps two materials as two parts", test_loadfbx_two_meshes_are_two_parts},
  {"ResolveAssetPath finds a file by stem", test_resolve_asset_path_finds_basename_and_stem},
  {"GlTextureCache returns the same pointer", test_gl_texture_cache_identity_and_white},
  {"LoadFbx copies skin weights and inverse bind", test_loadfbx_skin_and_cluster},
  {"LoadFbx copies animation curve keys", test_loadfbx_animation_curve_keys},
  {"LoadFbx keeps an unresolved texture path", test_loadfbx_texture_path_unresolved},
  {"Sphere vs triangle degenerate and inside", test_sphere_vs_triangle_degenerate_and_inside},
  {"Sphere vs triangle static overlap", test_sphere_vs_triangle_static_overlap},
  {"Sphere vs triangle swept face", test_sphere_vs_triangle_swept_face},
  {"Sphere vs triangle swept edge vertex overlap", test_sphere_vs_triangle_swept_edge_vertex_and_overlap},
  {"Sphere vs triangles earliest and camera boom", test_sphere_vs_triangles_earliest_and_empty},
  {"Line segment pierces triangle", test_line_segment_pierces_triangle},
  {"Swept sphere resting face does not fall through", test_swept_sphere_resting_face_does_not_fall_through},
  {"Slope into-plane remainder slides along tangent", test_slope_into_plane_slides_along_tangent},
  {"Hover racer log drop through rock slope", test_hover_racer_log_drop_through_rock_slope},
  {"Hover racer log zero-vel drop through slope", test_hover_racer_log_zero_vel_drop_through_slope},
  {"Hover racer log stall on shallow slope", test_hover_racer_log_stall_on_shallow_slope},
  {"Hover racer log stall on slope crease", test_hover_racer_log_stall_on_slope_crease},
  {"Hover racer log stall on shared edge", test_hover_racer_log_stall_on_shared_edge},
  {"Hover racer log slope hover spring fights sit", test_hover_racer_log_slope_hover_spring_fights_sit},
  {"Hover racer log still Y jitter on slope", test_hover_racer_log_still_y_jitter_on_slope},
  {"Hover racer log tiny reverse Y drop", test_hover_racer_log_tiny_reverse_y_drop},
  {"Hover racer log tiny forward hover dip", test_hover_racer_log_tiny_forward_hover_dip},
  {"Hover racer log downhill stair shake", test_hover_racer_log_downhill_stair_shake},
  {"Hover racer log crest fall instead of level", test_hover_racer_log_crest_fall_instead_of_level},
  {"Hover racer log wall corner dead stop", test_hover_racer_log_wall_corner_dead_stop},
  {"Hover racer log wall ledge shake", test_hover_racer_log_wall_ledge_shake},
  {"Hover racer log wall edge side snag",
      test_hover_racer_log_wall_edge_side_snag},
  {"Swept sphere hits ceiling from below", test_swept_sphere_hits_ceiling_from_below},
  {"Classify sphere pass through", test_classify_sphere_pass_through},
  {"Physics collide soup broadphase completeness",
      test_physics_collide_soup_broadphase_completeness},
      {"Physics collide soup wide AABB not dropped",
          test_physics_collide_soup_wide_aabb_not_dropped},
  {"Physics manifold feature stability",
      test_physics_manifold_feature_stability},
  {"Physics solver corner order invariance and PGS",
      test_physics_solver_corner_order_invariance_and_pgs},
  {"Physics solver coplanar wedge stops cleanly",
      test_physics_solver_coplanar_wedge_stops_cleanly},
  {"Physics solver duplicate constraint",
      test_physics_solver_duplicate_constraint},
  {"Physics world fixture corner dead stop",
      test_physics_world_fixture_corner_dead_stop},
  {"Physics world fixture edge snag",
      test_physics_world_fixture_edge_snag},
  {"Physics world fixture ledge shake",
      test_physics_world_fixture_ledge_shake},
  {"Physics world fixture floor climb collapse",
      test_physics_world_fixture_floor_climb_collapse},
  {"Physics sphere body rolls over road seam skirt",
      test_physics_sphere_body_rolls_over_road_seam_skirt},
  {"Physics sphere body step above center still blocks",
      test_physics_sphere_body_step_above_center_still_blocks},
  {"Physics drop probe conventions at a seam",
      test_physics_drop_probe_conventions_at_a_seam},
  {"Physics sweep backoff is a distance not a fraction",
      test_physics_sweep_backoff_is_a_distance_not_a_fraction},
  {"Physics step report merges substeps",
      test_physics_step_report_merges_substeps},
  {"Physics body separation stays out of the mesh",
      test_physics_body_separation_stays_out_of_the_mesh},
  {"Physics fixture builder catches bad geometry",
      test_physics_fixture_builder_catches_bad_geometry},
  {"Physics world query conventions",
      test_physics_world_query_conventions},
  {"Physics broad phase offers every overlapping triangle",
      test_physics_broad_phase_offers_every_overlapping_triangle},
  {"Physics support measures height above floor",
      test_physics_support_measures_height_above_floor},
  {"Physics sphere body reacquires steep floor gap",
      test_physics_sphere_body_reacquires_steep_floor_gap},
  {"Physics sphere body hovering over floor is not sitting",
      test_physics_sphere_body_hovering_over_floor_is_not_sitting},
  {0}
};

