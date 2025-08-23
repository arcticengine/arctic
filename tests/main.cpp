#include "engine/test_main.h"

#include <algorithm>
#include <array>
#include <deque>
#include <string>
#include <iostream>
#include <sstream>

#include "engine/arctic_types.h"
#include "engine/arctic_platform.h"
#include "engine/easy.h"
#include "engine/rgb.h"
#include <ctime>


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
  Ui32 *temp = (Ui32*)malloc(temp_size * sizeof(Ui32));
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


TEST_LIST = {
//  {"Tga oom", test_tga_oom},
  {"Rgba", test_rgba},
  {"Radix sort", test_radix_sort},
  {"Radix sort correctness", test_radix_sort_correctness},
  {"Rgb", test_rgb},
  {"File operations", test_file_operations},
  {"Random generation", test_random},
  {0}
};

