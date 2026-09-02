// The file formats: JSON, CSV, the binary DataWriter/DataReader and the
// localization tables.
#define TEST_NO_MAIN
#include "test_helpers.h"

// ============================================================================
// Localization tests
// ============================================================================

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

// The document is written by the test itself rather than kept in tests/data:
// a *.json file there is hidden from a sandboxed run, and what is under test
// is parsing from a stream, not finding a file.
void test_json_parse_file() {
  const char *path = "/tmp/arctic_json_test_config.json";
  {
    std::ofstream out(path);
    out << R"({
  "window": {
    "title": "Arctic Test",
    "width": 1280,
    "height": 720,
    "fullscreen": false
  },
  "audio": {
    "master_volume": 0.8,
    "music_volume": 0.6,
    "sfx_volume": 1.0
  },
  "player": {
    "name": "Arctic Fox",
    "level": 42,
    "inventory": ["sword", "shield", "potion"],
    "position": {"x": 10.5, "y": -3.25, "z": 0.0}
  },
  "enemies": [
    {"type": "goblin", "hp": 30, "aggressive": true},
    {"type": "dragon", "hp": 500, "aggressive": false}
  ],
  "empty_object": {},
  "empty_array": []
})";
  }

  std::ifstream file(path);
  if (!TEST_CHECK_(file.is_open(), "Failed to open %s",
      arctic::DescribeFilePath(path).c_str())) {
    return;
  }

  json j = json::parse(file);
  file.close();
  std::remove(path);

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
