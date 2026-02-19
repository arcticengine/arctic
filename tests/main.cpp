#include "engine/test_main.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"
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
#include "engine/mesh.h"
#include "engine/mesh_gen_mod_complex.h"


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

  Si32 i = 0;
  while (i < 10) {
    if (arctic::DoesDirectoryExist(arctic_engine_dir.c_str())) {
      break;
    }
    arctic_engine_dir = std::string("../") + arctic_engine_dir;
  }
  TEST_CHECK(i != 10);

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
  if (!TEST_CHECK_(file.is_open(), "Failed to open %s", path.c_str())) {
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
  float angle = static_cast<float>(M_PI / 2.0);  // 90 degrees
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
  float angle = static_cast<float>(M_PI / 2.0);
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

void test_hw_sprite_subregion_draws_correctly() {
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
  {"Mesh PLY readline does not strip CRLF", test_mesh_ply_readline_crlf},
  {"Mesh vertex attrib write overflow", test_mesh_vertex_attrib_write_overflow},
  {"Mesh extrude face covers all edges", test_mesh_extrude_face_covers_all_edges},
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
  {"HW sprite sub-region draws correctly", test_hw_sprite_subregion_draws_correctly},
  {0}
};

