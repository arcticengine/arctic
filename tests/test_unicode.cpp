// UTF-8, UTF-16 and UTF-32 conversions and the codepoint walkers of
// engine/unicode.h.
#define TEST_NO_MAIN
#include "test_helpers.h"

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
