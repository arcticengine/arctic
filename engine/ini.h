// The MIT License (MIT)
//
// Copyright (c) 2025 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef ENGINE_INI_H_
#define ENGINE_INI_H_

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_utility
/// @{

/// @brief Represents a section in an INI file.
class IniSection {
 private:
  std::string name_;
  std::map<std::string, std::string> values_;

 public:
  /// @brief Constructs an IniSection with the given name.
  /// @param name The name of the section.
  explicit IniSection(const std::string &name);

  /// @brief Destructor for IniSection.
  ~IniSection();

  /// @brief Gets the name of the section.
  /// @return The section name.
  const std::string &GetName() const;

  /// @brief Returns the number of key-value pairs in the section.
  /// @return The size of the section.
  Ui64 Size() const;

  /// @brief Sets a value in the section.
  /// @param key The key name.
  /// @param value The value to be set.
  void SetValue(const std::string &key, const std::string &value);

  /// @brief Gets a value from the section by key and converts it to the specified type.
  /// @tparam T The type to convert the value to.
  /// @param key The key name.
  /// @param default_value The default value to return if key is not found or conversion fails.
  /// @return The converted value or the default value.
  template<typename T>
  const T GetValue(const std::string &key, T default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
      return default_value;
    }
    
    T res;
    std::stringstream ss(it->second);
    ss >> res;
    if (ss.fail()) {
      return default_value;
    }
    if (ss.peek() == std::iostream::traits_type::eof()) {
      return res;
    }
    return default_value;
  }

  /// @brief Gets a string value from the section by key.
  /// @param key The key name.
  /// @param default_value The default value to return if key is not found.
  /// @return The string value or the default value.
  const std::string GetString(const std::string &key, const std::string &default_value = "") const;

  /// @brief Gets a boolean value from the section by key.
  /// @param key The key name.
  /// @param default_value The default value to return if key is not found or conversion fails.
  /// @return The boolean value or the default value.
  bool GetBool(const std::string &key, bool default_value = false) const;

  /// @brief Gets an integer value from the section by key.
  /// @param key The key name.
  /// @param default_value The default value to return if key is not found or conversion fails.
  /// @return The integer value or the default value.
  Si32 GetInt(const std::string &key, Si32 default_value = 0) const;

  /// @brief Gets a float value from the section by key.
  /// @param key The key name.
  /// @param default_value The default value to return if key is not found or conversion fails.
  /// @return The float value or the default value.
  float GetFloat(const std::string &key, float default_value = 0.0f) const;

  /// @brief Checks if a key exists in the section.
  /// @param key The key name to check.
  /// @return True if the key exists, false otherwise.
  bool HasKey(const std::string &key) const;

  /// @brief Gets all keys in the section.
  /// @return A vector of all key names.
  std::vector<std::string> GetKeys() const;

  /// @brief Accesses a value in the section by key.
  /// @param key The key name.
  /// @return The value associated with the key, or empty string if not found.
  const std::string operator[](const std::string &key) const;

  /// @brief Outputs the section to an output stream.
  friend std::ostream& operator<<(std::ostream& os, const IniSection &section);

  /// @brief Outputs the section to an output file stream.
  friend std::ofstream& operator<<(std::ofstream& os, const IniSection &section);
};

/// @brief Enumerates the types of INI sources.
enum IniSourceType {
  kIniSourceFile = 0,  ///< INI source is a file.
  kIniSourcePure = 1   ///< INI source is a string.
};

/// @brief Represents an INI configuration file.
class IniFile {
 public:
  /// @brief Default constructor for IniFile.
  IniFile();

  /// @brief Loads an INI file.
  /// @param filename The name of the file to load.
  /// @return True if the file was loaded successfully, false otherwise.
  bool LoadFile(const std::string &filename);

  /// @brief Loads an INI from a string.
  /// @param input The input string containing INI data.
  /// @return True if the string was parsed successfully, false otherwise.
  bool LoadString(const std::string &input);

  /// @brief Destructor for IniFile.
  ~IniFile();

  /// @brief Gets a section from the file.
  /// @param section_name The name of the section.
  /// @return A pointer to the IniSection, or nullptr if not found.
  IniSection *GetSection(const std::string &section_name) const;

  /// @brief Returns the number of sections in the file.
  /// @return The section count.
  Ui64 SectionCount() const;

  /// @brief Gets all section names.
  /// @return A vector of section names.
  std::vector<std::string> GetSectionNames() const;

  /// @brief Gets the filename of the INI file.
  /// @return The filename.
  const std::string &GetFileName() const;

  /// @brief Checks if a section exists.
  /// @param section_name The name of the section to check.
  /// @return True if the section exists, false otherwise.
  bool HasSection(const std::string &section_name) const;

  /// @brief Adds a new section to the file.
  /// @param section_name The name of the new section.
  /// @return A pointer to the created section, or nullptr if section already exists.
  IniSection *AddSection(const std::string &section_name);

  /// @brief Removes a section from the file.
  /// @param section_name The name of the section to remove.
  /// @return True if the section was removed successfully, false otherwise.
  bool RemoveSection(const std::string &section_name);

  /// @brief Saves the INI file.
  /// @param filename Optional filename to save to (uses original filename if empty).
  /// @return True if saved successfully, false otherwise.
  bool SaveFile(const std::string &filename = "") const;

  /// @brief Gets a value directly from a section without getting the section first.
  /// @tparam T The type to convert the value to.
  /// @param section_name The name of the section.
  /// @param key The key name.
  /// @param default_value The default value to return if not found or conversion fails.
  /// @return The converted value or the default value.
  template<typename T>
  const T GetValue(const std::string &section_name, const std::string &key, T default_value) const {
    IniSection *section = GetSection(section_name);
    if (section == nullptr) {
      return default_value;
    }
    return section->GetValue(key, default_value);
  }

  /// @brief Gets a string value directly from a section.
  /// @param section_name The name of the section.
  /// @param key The key name.
  /// @param default_value The default value to return if not found.
  /// @return The string value or the default value.
  const std::string GetString(const std::string &section_name, const std::string &key, const std::string &default_value = "") const;

  /// @brief Gets a boolean value directly from a section.
  /// @param section_name The name of the section.
  /// @param key The key name.
  /// @param default_value The default value to return if not found or conversion fails.
  /// @return The boolean value or the default value.
  bool GetBool(const std::string &section_name, const std::string &key, bool default_value = false) const;

  /// @brief Gets an integer value directly from a section.
  /// @param section_name The name of the section.
  /// @param key The key name.
  /// @param default_value The default value to return if not found or conversion fails.
  /// @return The integer value or the default value.
  Si32 GetInt(const std::string &section_name, const std::string &key, Si32 default_value = 0) const;

  /// @brief Gets a float value directly from a section.
  /// @param section_name The name of the section.
  /// @param key The key name.
  /// @param default_value The default value to return if not found or conversion fails.
  /// @return The float value or the default value.
  float GetFloat(const std::string &section_name, const std::string &key, float default_value = 0.0f) const;

  /// @brief Accesses a section in the file by name.
  /// @param section_name The name of the section.
  /// @return A pointer to the IniSection, or nullptr if not found.
  IniSection *operator[](const std::string &section_name) const;

  /// @brief Gets the error description if an operation fails.
  /// @return The error description string.
  std::string GetErrorDescription() const;

 protected:
  /// @brief Parses the content of the INI.
  /// @return True if the content was parsed successfully, false otherwise.
  bool ParseContent();

  /// @brief Trims whitespace from both ends of a string.
  /// @param str The string to trim.
  /// @return The trimmed string.
  std::string Trim(const std::string &str) const;

  /// @brief Checks if a line is a comment.
  /// @param line The line to check.
  /// @return True if the line is a comment, false otherwise.
  bool IsComment(const std::string &line) const;

  /// @brief Extracts section name from a section header line.
  /// @param line The line containing the section header.
  /// @return The section name, or empty string if invalid.
  std::string ExtractSectionName(const std::string &line) const;

  /// @brief Parses a key-value pair from a line.
  /// @param line The line to parse.
  /// @param key Output parameter for the key.
  /// @param value Output parameter for the value.
  /// @return True if parsing was successful, false otherwise.
  bool ParseKeyValue(const std::string &line, std::string &key, std::string &value) const;

 private:
  std::string filename_;
  IniSourceType type_ = kIniSourcePure;
  std::vector<std::string> lines_;
  std::map<std::string, IniSection*> sections_;
  IniSection *global_section_;  // For key-value pairs before any section
  std::string error_description_;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_INI_H_
