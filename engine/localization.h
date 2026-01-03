// The MIT License (MIT)
//
// Copyright (c) 2026 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef ENGINE_LOCALIZATION_H_
#define ENGINE_LOCALIZATION_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_localization
/// @{

/// @brief Represents a value that can be passed to localization formatting.
/// Supports integer, floating point, and string values.
struct LocValue {
  enum Type { kInt, kDouble, kString };
  
  Type type;
  Si64 int_value;
  double double_value;
  std::string string_value;
  
  LocValue() : type(kInt), int_value(0), double_value(0.0) {}
  LocValue(int v) : type(kInt), int_value(v), double_value(0.0) {}
  LocValue(Si64 v) : type(kInt), int_value(v), double_value(0.0) {}
  LocValue(Ui64 v) : type(kInt), int_value(static_cast<Si64>(v)), double_value(0.0) {}
  LocValue(double v) : type(kDouble), int_value(0), double_value(v) {}
  LocValue(const char* v) : type(kString), int_value(0), double_value(0.0), string_value(v) {}
  LocValue(const std::string& v) : type(kString), int_value(0), double_value(0.0), string_value(v) {}
  
  Si64 AsInt() const;
  std::string AsString() const;
};

/// @brief Named arguments for localization formatting.
using LocArgs = std::vector<std::pair<std::string, LocValue>>;

/// @brief Localization system with ICU MessageFormat support.
/// Loads strings from UTF-8 CSV files and supports plural, select, and selectordinal.
/// Supports loading multiple CSV files and merging them together.
class Localization {
 public:
  /// @brief Returns the singleton instance.
  static Localization& Instance();
  
  /// @brief Loads a CSV file with localized strings.
  /// Supports single-language (id, locale) or multi-language (id, en, ru, de, ...) format.
  /// Locale codes are determined from column headers.
  /// Merges with existing strings for all locales found in the file.
  /// @param path Path to the CSV file (UTF-8 encoded).
  /// @param id_column Name of the column containing string IDs.
  /// @return True if loaded successfully.
  bool Load(const std::string& path,
            const std::string& id_column = "id");
  
  /// @brief Sets the current locale code (e.g., "en", "ru", "de").
  /// Used for string lookup and plural rules selection.
  void SetLocale(const std::string& locale_code);
  
  /// @brief Gets the current locale code.
  const std::string& GetLocale() const { return locale_; }
  
  /// @brief Sets the fallback locale used when a string is not found in the current locale.
  void SetFallbackLocale(const std::string& locale_code);
  
  /// @brief Gets the fallback locale code.
  const std::string& GetFallbackLocale() const { return fallback_locale_; }
  
  /// @brief Gets a raw localized string by key.
  /// Looks up in current locale, then fallback locale, then returns missing marker.
  /// @param key The string identifier.
  /// @return The localized string or the key itself if not found.
  const std::string& Get(const std::string& key) const;
  
  /// @brief Gets a raw localized string by key for a specific locale.
  /// @param key The string identifier.
  /// @param locale_code The locale to look up in.
  /// @return The localized string or the key itself if not found.
  const std::string& Get(const std::string& key, const std::string& locale_code) const;
  
  /// @brief Gets a formatted localized string.
  /// @param key The string identifier.
  /// @param args Named arguments for formatting.
  /// @return The formatted localized string.
  std::string Format(const std::string& key, const LocArgs& args) const;
  
  /// @brief Formats a pattern string directly (without lookup).
  /// @param pattern The ICU MessageFormat pattern.
  /// @param args Named arguments for formatting.
  /// @return The formatted string.
  std::string FormatPattern(const std::string& pattern, const LocArgs& args) const;
  
  /// @brief Checks if a key exists for the current locale.
  bool HasKey(const std::string& key) const;
  
  /// @brief Checks if a key exists for a specific locale.
  bool HasKey(const std::string& key, const std::string& locale_code) const;
  
  /// @brief Returns all available locale codes.
  std::vector<std::string> GetAvailableLocales() const;
  
  /// @brief Clears all loaded strings for all locales.
  void Clear();
  
  /// @brief Clears all loaded strings for a specific locale.
  void Clear(const std::string& locale_code);
  
  /// @brief Returns the number of loaded strings for the current locale.
  Ui64 Count() const;
  
  /// @brief Returns the number of loaded strings for a specific locale.
  Ui64 Count(const std::string& locale_code) const;
  
 private:
  Localization() = default;
  Localization(const Localization&) = delete;
  Localization& operator=(const Localization&) = delete;
  
  const std::string* FindString(const std::string& key, const std::string& locale_code) const;
  
  // Map: locale_code -> (key -> text)
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> strings_;
  std::string locale_ = "en";
  std::string fallback_locale_ = "en";
  std::string missing_key_prefix_ = "[?";
  std::string missing_key_suffix_ = "?]";
  mutable std::string missing_string_buffer_;
};

/// @brief Gets a localized string by key.
/// @param key The string identifier.
/// @return The localized string.
const std::string& Loc(const std::string& key);

/// @brief Gets a formatted localized string.
/// @param key The string identifier.
/// @param args Named arguments for formatting.
/// @return The formatted localized string.
std::string Loc(const std::string& key, const LocArgs& args);

/// @}

}  // namespace arctic

#endif  // ENGINE_LOCALIZATION_H_

