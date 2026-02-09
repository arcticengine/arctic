// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/localization.h"

#include <cstdlib>
#include <sstream>

#include "engine/csv.h"

namespace arctic {

// ============================================================================
// LocValue implementation
// ============================================================================

Si64 LocValue::AsInt() const {
  switch (type) {
    case kInt: return int_value;
    case kDouble: return static_cast<Si64>(double_value);
    case kString: return std::atoll(string_value.c_str());
  }
  return 0;
}

std::string LocValue::AsString() const {
  switch (type) {
    case kInt: {
      std::ostringstream ss;
      ss << int_value;
      return ss.str();
    }
    case kDouble: {
      std::ostringstream ss;
      ss << double_value;
      return ss.str();
    }
    case kString: return string_value;
  }
  return "";
}

// ============================================================================
// ICU MessageFormat Parser - Recursive Descent
// ============================================================================

// Parser state
struct ParserState {
  const std::string& input;
  const LocArgs& args;
  const std::string& locale;
  size_t pos;
  
  ParserState(const std::string& in, const LocArgs& a, const std::string& loc)
      : input(in), args(a), locale(loc), pos(0) {}
  
  bool AtEnd() const { return pos >= input.size(); }
  char Peek() const { return AtEnd() ? '\0' : input[pos]; }
  char Advance() { return AtEnd() ? '\0' : input[pos++]; }
  
  void SkipWhitespace() {
    while (!AtEnd() && (Peek() == ' ' || Peek() == '\t' || 
                        Peek() == '\n' || Peek() == '\r')) {
      Advance();
    }
  }
  
  const LocValue* FindArg(const std::string& name) const {
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i].first == name) {
        return &args[i].second;
      }
    }
    return nullptr;
  }
};

// Forward declarations
static std::string ParseMessage(ParserState& state);
static std::string ParseArgument(ParserState& state);

// Get plural category for a number based on locale
static std::string GetPluralCategory(Si64 n, const std::string& locale) {
  Si64 abs_n = n < 0 ? -n : n;
  Si64 mod10 = abs_n % 10;
  Si64 mod100 = abs_n % 100;
  
  // Russian, Ukrainian, Belarusian
  if (locale == "ru" || locale == "uk" || locale == "be") {
    if (mod10 == 1 && mod100 != 11) return "one";
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return "few";
    return "many";
  }
  
  // Polish
  if (locale == "pl") {
    if (n == 1) return "one";
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return "few";
    return "many";
  }
  
  // Czech, Slovak
  if (locale == "cs" || locale == "sk") {
    if (n == 1) return "one";
    if (n >= 2 && n <= 4) return "few";
    return "other";
  }
  
  // German, English, Spanish, Italian, Portuguese, etc. (simple plural)
  if (abs_n == 1) return "one";
  return "other";
}

// Get ordinal category for a number based on locale
static std::string GetOrdinalCategory(Si64 n, const std::string& locale) {
  Si64 abs_n = n < 0 ? -n : n;
  Si64 mod10 = abs_n % 10;
  Si64 mod100 = abs_n % 100;
  
  // English ordinals
  if (locale == "en") {
    if (mod10 == 1 && mod100 != 11) return "one";
    if (mod10 == 2 && mod100 != 12) return "two";
    if (mod10 == 3 && mod100 != 13) return "few";
    return "other";
  }
  
  // Most languages don't have special ordinal forms
  return "other";
}

// Parse identifier (variable name or keyword)
static std::string ParseIdentifier(ParserState& state) {
  std::string result;
  while (!state.AtEnd()) {
    char c = state.Peek();
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_') {
      result += state.Advance();
    } else {
      break;
    }
  }
  return result;
}

// Parse a number (for exact match like =0, =1)
static bool ParseNumber(ParserState& state, Si64& out_value) {
  std::string num_str;
  bool negative = false;
  
  if (state.Peek() == '-') {
    negative = true;
    state.Advance();
  }
  
  while (!state.AtEnd() && state.Peek() >= '0' && state.Peek() <= '9') {
    num_str += state.Advance();
  }
  
  if (num_str.empty()) return false;
  
  out_value = std::atoll(num_str.c_str());
  if (negative) out_value = -out_value;
  return true;
}

// Parse content inside braces, handling nesting
static std::string ParseBraceContent(ParserState& state) {
  if (state.Peek() != '{') return "";
  state.Advance(); // skip '{'
  
  std::string content;
  int depth = 1;
  
  while (!state.AtEnd() && depth > 0) {
    char c = state.Peek();
    if (c == '{') {
      depth++;
      content += state.Advance();
    } else if (c == '}') {
      depth--;
      if (depth > 0) {
        content += state.Advance();
      } else {
        state.Advance(); // skip closing '}'
      }
    } else if (c == '\'' && depth == 1) {
      // Handle quoted literals in ICU format
      state.Advance(); // skip opening quote
      if (state.Peek() == '\'') {
        // Escaped single quote
        content += '\'';
        state.Advance();
      } else {
        // Quoted section
        while (!state.AtEnd() && state.Peek() != '\'') {
          content += state.Advance();
        }
        if (!state.AtEnd()) state.Advance(); // skip closing quote
      }
    } else {
      content += state.Advance();
    }
  }
  
  return content;
}

// Parse plural/select/selectordinal options
struct PluralOption {
  std::string key;      // "one", "few", "many", "other", "=0", "=1", etc.
  std::string content;  // The message content for this option
  bool is_exact;        // True if key is exact match (=N)
  Si64 exact_value;     // Value for exact match
};

static std::vector<PluralOption> ParsePluralOptions(ParserState& state) {
  std::vector<PluralOption> options;
  
  while (!state.AtEnd()) {
    state.SkipWhitespace();
    if (state.AtEnd() || state.Peek() == '}') break;
    
    PluralOption opt;
    opt.is_exact = false;
    opt.exact_value = 0;
    
    // Check for exact match (=N)
    if (state.Peek() == '=') {
      state.Advance(); // skip '='
      opt.is_exact = true;
      if (!ParseNumber(state, opt.exact_value)) {
        break; // Parse error
      }
      std::ostringstream ss;
      ss << "=" << opt.exact_value;
      opt.key = ss.str();
    } else {
      // Parse category name (one, few, many, other, etc.)
      opt.key = ParseIdentifier(state);
      if (opt.key.empty()) break;
    }
    
    state.SkipWhitespace();
    
    // Parse the content in braces
    if (state.Peek() == '{') {
      opt.content = ParseBraceContent(state);
      options.push_back(opt);
    } else {
      break; // Parse error
    }
  }
  
  return options;
}

// Process plural argument
static std::string ProcessPlural(ParserState& state, const std::string& var_name,
                                  const std::string& options_str, bool is_ordinal) {
  const LocValue* value = state.FindArg(var_name);
  if (!value) return "";
  
  Si64 n = value->AsInt();
  
  // Parse options from the options string
  ParserState opt_state(options_str, state.args, state.locale);
  std::vector<PluralOption> options = ParsePluralOptions(opt_state);
  
  // First, check for exact match
  for (size_t i = 0; i < options.size(); ++i) {
    if (options[i].is_exact && options[i].exact_value == n) {
      // Process the content, replacing # with the number
      std::string content = options[i].content;
      ParserState content_state(content, state.args, state.locale);
      std::string result = ParseMessage(content_state);
      
      // Replace # with the number
      std::ostringstream num_ss;
      num_ss << n;
      std::string num_str = num_ss.str();
      
      size_t hash_pos = 0;
      while ((hash_pos = result.find('#', hash_pos)) != std::string::npos) {
        result.replace(hash_pos, 1, num_str);
        hash_pos += num_str.size();
      }
      return result;
    }
  }
  
  // Get the plural/ordinal category
  std::string category = is_ordinal 
      ? GetOrdinalCategory(n, state.locale)
      : GetPluralCategory(n, state.locale);
  
  // Find matching category
  std::string selected_content;
  std::string other_content;
  
  for (size_t i = 0; i < options.size(); ++i) {
    if (!options[i].is_exact) {
      if (options[i].key == category) {
        selected_content = options[i].content;
      }
      if (options[i].key == "other") {
        other_content = options[i].content;
      }
    }
  }
  
  // Use category match or fall back to "other"
  std::string content = selected_content.empty() ? other_content : selected_content;
  
  // Process the content
  ParserState content_state(content, state.args, state.locale);
  std::string result = ParseMessage(content_state);
  
  // Replace # with the number
  std::ostringstream num_ss;
  num_ss << n;
  std::string num_str = num_ss.str();
  
  size_t hash_pos = 0;
  while ((hash_pos = result.find('#', hash_pos)) != std::string::npos) {
    result.replace(hash_pos, 1, num_str);
    hash_pos += num_str.size();
  }
  
  return result;
}

// Process select argument
static std::string ProcessSelect(ParserState& state, const std::string& var_name,
                                  const std::string& options_str) {
  const LocValue* value = state.FindArg(var_name);
  if (!value) return "";
  
  std::string select_value = value->AsString();
  
  // Parse options
  ParserState opt_state(options_str, state.args, state.locale);
  std::vector<PluralOption> options = ParsePluralOptions(opt_state);
  
  // Find matching option
  std::string selected_content;
  std::string other_content;
  
  for (size_t i = 0; i < options.size(); ++i) {
    if (options[i].key == select_value) {
      selected_content = options[i].content;
    }
    if (options[i].key == "other") {
      other_content = options[i].content;
    }
  }
  
  std::string content = selected_content.empty() ? other_content : selected_content;
  
  // Process the content recursively
  ParserState content_state(content, state.args, state.locale);
  return ParseMessage(content_state);
}

// Parse an argument expression: {name} or {name, type, options}
static std::string ParseArgument(ParserState& state) {
  if (state.Peek() != '{') return "";
  state.Advance(); // skip '{'
  
  state.SkipWhitespace();
  
  // Parse variable name
  std::string var_name = ParseIdentifier(state);
  
  state.SkipWhitespace();
  
  // Check if this is a simple substitution or has type/options
  if (state.Peek() == '}') {
    state.Advance(); // skip '}'
    // Simple substitution
    const LocValue* value = state.FindArg(var_name);
    if (value) {
      return value->AsString();
    }
    return "{" + var_name + "}";
  }
  
  if (state.Peek() != ',') {
    // Skip to closing brace
    int depth = 1;
    while (!state.AtEnd() && depth > 0) {
      if (state.Peek() == '{') depth++;
      else if (state.Peek() == '}') depth--;
      state.Advance();
    }
    return "";
  }
  
  state.Advance(); // skip ','
  state.SkipWhitespace();
  
  // Parse type (plural, select, selectordinal, number, date, etc.)
  std::string type = ParseIdentifier(state);
  
  state.SkipWhitespace();
  
  // For simple types like number/date, we might have format specifier or just closing brace
  if (state.Peek() == '}') {
    state.Advance();
    const LocValue* value = state.FindArg(var_name);
    if (value) {
      return value->AsString();
    }
    return "";
  }
  
  if (state.Peek() != ',') {
    int depth = 1;
    while (!state.AtEnd() && depth > 0) {
      if (state.Peek() == '{') depth++;
      else if (state.Peek() == '}') depth--;
      state.Advance();
    }
    return "";
  }
  
  state.Advance(); // skip ','
  
  // Collect the rest as options (everything until the matching closing brace)
  std::string options;
  int depth = 1;
  while (!state.AtEnd() && depth > 0) {
    char c = state.Peek();
    if (c == '{') {
      depth++;
      options += state.Advance();
    } else if (c == '}') {
      depth--;
      if (depth > 0) {
        options += state.Advance();
      } else {
        state.Advance(); // skip final '}'
      }
    } else {
      options += state.Advance();
    }
  }
  
  // Process based on type
  if (type == "plural") {
    return ProcessPlural(state, var_name, options, false);
  } else if (type == "selectordinal") {
    return ProcessPlural(state, var_name, options, true);
  } else if (type == "select") {
    return ProcessSelect(state, var_name, options);
  } else if (type == "number") {
    const LocValue* value = state.FindArg(var_name);
    if (value) {
      return value->AsString();
    }
  }
  
  return "";
}

// Parse the full message, handling plain text and argument expressions
static std::string ParseMessage(ParserState& state) {
  std::string result;
  
  while (!state.AtEnd()) {
    char c = state.Peek();
    
    if (c == '{') {
      result += ParseArgument(state);
    } else if (c == '\'') {
      // ICU quoted literal handling
      state.Advance(); // skip opening quote
      if (state.Peek() == '\'') {
        // Escaped single quote ''
        result += '\'';
        state.Advance();
      } else if (state.Peek() == '{' || state.Peek() == '}' || state.Peek() == '#') {
        // Single character escape
        result += state.Advance();
      } else {
        // Quoted section - everything until closing quote
        while (!state.AtEnd() && state.Peek() != '\'') {
          result += state.Advance();
        }
        if (!state.AtEnd()) state.Advance(); // skip closing quote
      }
    } else if (c == '}') {
      // End of a nested message
      break;
    } else {
      result += state.Advance();
    }
  }
  
  return result;
}

// ============================================================================
// Localization class implementation
// ============================================================================

Localization& Localization::Instance() {
  static Localization instance;
  return instance;
}

bool Localization::Load(const std::string& path,
                        const std::string& id_column) {
  CsvTable csv;
  if (!csv.LoadFile(path)) {
    return false;
  }
  
  // Get all column names (locales) except the id column
  std::vector<std::string> header = csv.GetHeader();
  std::vector<std::string> locale_columns;
  
  for (size_t i = 0; i < header.size(); ++i) {
    if (header[i] != id_column) {
      locale_columns.push_back(header[i]);
    }
  }
  
  // Load strings for each locale
  for (Ui64 i = 0; i < csv.RowCount(); ++i) {
    CsvRow* row = csv.GetRow(i);
    if (!row) continue;
    
    std::string id = (*row)[id_column];
    if (id.empty()) continue;
    
    for (size_t loc_idx = 0; loc_idx < locale_columns.size(); ++loc_idx) {
      const std::string& locale_code = locale_columns[loc_idx];
      std::string text = (*row)[locale_code];
      
      if (!text.empty()) {
        strings_[locale_code][id] = text;
      }
    }
  }
  
  return true;
}

void Localization::SetLocale(const std::string& locale_code) {
  locale_ = locale_code;
}

void Localization::SetFallbackLocale(const std::string& locale_code) {
  fallback_locale_ = locale_code;
}

const std::string* Localization::FindString(const std::string& key, 
                                             const std::string& locale_code) const {
  auto locale_it = strings_.find(locale_code);
  if (locale_it != strings_.end()) {
    auto key_it = locale_it->second.find(key);
    if (key_it != locale_it->second.end()) {
      return &key_it->second;
    }
  }
  return nullptr;
}

const std::string& Localization::Get(const std::string& key) const {
  // Try current locale
  const std::string* result = FindString(key, locale_);
  if (result) {
    return *result;
  }
  
  // Try fallback locale
  if (locale_ != fallback_locale_) {
    result = FindString(key, fallback_locale_);
    if (result) {
      return *result;
    }
  }
  
  // Return the key wrapped in markers
  missing_string_buffer_ = missing_key_prefix_ + key + missing_key_suffix_;
  return missing_string_buffer_;
}

const std::string& Localization::Get(const std::string& key, 
                                      const std::string& locale_code) const {
  const std::string* result = FindString(key, locale_code);
  if (result) {
    return *result;
  }
  
  // Try fallback locale
  if (locale_code != fallback_locale_) {
    result = FindString(key, fallback_locale_);
    if (result) {
      return *result;
    }
  }
  
  // Return the key wrapped in markers
  missing_string_buffer_ = missing_key_prefix_ + key + missing_key_suffix_;
  return missing_string_buffer_;
}

std::string Localization::Format(const std::string& key, const LocArgs& args) const {
  return FormatPattern(Get(key), args);
}

std::string Localization::FormatPattern(const std::string& pattern, 
                                         const LocArgs& args) const {
  ParserState state(pattern, args, locale_);
  return ParseMessage(state);
}

bool Localization::HasKey(const std::string& key) const {
  return FindString(key, locale_) != nullptr;
}

bool Localization::HasKey(const std::string& key, const std::string& locale_code) const {
  return FindString(key, locale_code) != nullptr;
}

std::vector<std::string> Localization::GetAvailableLocales() const {
  std::vector<std::string> locales;
  locales.reserve(strings_.size());
  for (auto it = strings_.begin(); it != strings_.end(); ++it) {
    locales.push_back(it->first);
  }
  return locales;
}

void Localization::Clear() {
  strings_.clear();
}

void Localization::Clear(const std::string& locale_code) {
  auto it = strings_.find(locale_code);
  if (it != strings_.end()) {
    it->second.clear();
  }
}

Ui64 Localization::Count() const {
  auto it = strings_.find(locale_);
  if (it != strings_.end()) {
    return static_cast<Ui64>(it->second.size());
  }
  return 0;
}

Ui64 Localization::Count(const std::string& locale_code) const {
  auto it = strings_.find(locale_code);
  if (it != strings_.end()) {
    return static_cast<Ui64>(it->second.size());
  }
  return 0;
}

// ============================================================================
// Global helper functions
// ============================================================================

const std::string& Loc(const std::string& key) {
  return Localization::Instance().Get(key);
}

std::string Loc(const std::string& key, const LocArgs& args) {
  return Localization::Instance().Format(key, args);
}

}  // namespace arctic

