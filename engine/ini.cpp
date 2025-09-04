// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "ini.h"
#include <algorithm>
#include <cctype>

namespace arctic {

// IniSection implementation

IniSection::IniSection(const std::string &name) : name_(name) {
}

IniSection::~IniSection() {
}

const std::string &IniSection::GetName() const {
  return name_;
}

Ui64 IniSection::Size() const {
  return static_cast<Ui64>(values_.size());
}

void IniSection::SetValue(const std::string &key, const std::string &value) {
  values_[key] = value;
}

const std::string IniSection::GetString(const std::string &key, const std::string &default_value) const {
  auto it = values_.find(key);
  if (it == values_.end()) {
    return default_value;
  }
  return it->second;
}

bool IniSection::GetBool(const std::string &key, bool default_value) const {
  auto it = values_.find(key);
  if (it == values_.end()) {
    return default_value;
  }
  
  std::string value = it->second;
  std::transform(value.begin(), value.end(), value.begin(), ::tolower);
  
  if (value == "true" || value == "yes" || value == "on" || value == "1") {
    return true;
  } else if (value == "false" || value == "no" || value == "off" || value == "0") {
    return false;
  }
  
  return default_value;
}

Si32 IniSection::GetInt(const std::string &key, Si32 default_value) const {
  return GetValue(key, default_value);
}

float IniSection::GetFloat(const std::string &key, float default_value) const {
  return GetValue(key, default_value);
}

bool IniSection::HasKey(const std::string &key) const {
  return values_.find(key) != values_.end();
}

std::vector<std::string> IniSection::GetKeys() const {
  std::vector<std::string> keys;
  keys.reserve(values_.size());
  for (const auto &pair : values_) {
    keys.push_back(pair.first);
  }
  return keys;
}

const std::string IniSection::operator[](const std::string &key) const {
  return GetString(key, "");
}

std::ostream& operator<<(std::ostream& os, const IniSection &section) {
  if (!section.name_.empty()) {
    os << "[" << section.name_ << "]" << std::endl;
  }
  for (const auto &pair : section.values_) {
    os << pair.first << "=" << pair.second << std::endl;
  }
  return os;
}

std::ofstream& operator<<(std::ofstream& os, const IniSection &section) {
  if (!section.name_.empty()) {
    os << "[" << section.name_ << "]" << std::endl;
  }
  for (const auto &pair : section.values_) {
    os << pair.first << "=" << pair.second << std::endl;
  }
  return os;
}

// IniFile implementation

IniFile::IniFile() : global_section_(nullptr) {
}

IniFile::~IniFile() {
  for (auto &pair : sections_) {
    delete pair.second;
  }
  if (global_section_) {
    delete global_section_;
  }
}

bool IniFile::LoadFile(const std::string &filename) {
  filename_ = filename;
  type_ = kIniSourceFile;
  
  std::ifstream file(filename);
  if (!file.is_open()) {
    error_description_ = "Cannot open file: " + filename;
    return false;
  }
  
  lines_.clear();
  std::string line;
  while (std::getline(file, line)) {
    lines_.push_back(line);
  }
  file.close();
  
  return ParseContent();
}

bool IniFile::LoadString(const std::string &input) {
  filename_.clear();
  type_ = kIniSourcePure;
  
  lines_.clear();
  std::stringstream ss(input);
  std::string line;
  while (std::getline(ss, line)) {
    lines_.push_back(line);
  }
  
  return ParseContent();
}

IniSection *IniFile::GetSection(const std::string &section_name) const {
  if (section_name.empty() && global_section_) {
    return global_section_;
  }
  
  auto it = sections_.find(section_name);
  if (it == sections_.end()) {
    return nullptr;
  }
  return it->second;
}

Ui64 IniFile::SectionCount() const {
  Ui64 count = static_cast<Ui64>(sections_.size());
  if (global_section_ && global_section_->Size() > 0) {
    count++;
  }
  return count;
}

std::vector<std::string> IniFile::GetSectionNames() const {
  std::vector<std::string> names;
  names.reserve(sections_.size());
  
  if (global_section_ && global_section_->Size() > 0) {
    names.push_back("");  // Empty name for global section
  }
  
  for (const auto &pair : sections_) {
    names.push_back(pair.first);
  }
  return names;
}

const std::string &IniFile::GetFileName() const {
  return filename_;
}

bool IniFile::HasSection(const std::string &section_name) const {
  if (section_name.empty()) {
    return global_section_ && global_section_->Size() > 0;
  }
  return sections_.find(section_name) != sections_.end();
}

IniSection *IniFile::AddSection(const std::string &section_name) {
  if (section_name.empty()) {
    if (!global_section_) {
      global_section_ = new IniSection("");
    }
    return global_section_;
  }
  
  if (sections_.find(section_name) != sections_.end()) {
    return nullptr;  // Section already exists
  }
  
  IniSection *section = new IniSection(section_name);
  sections_[section_name] = section;
  return section;
}

bool IniFile::RemoveSection(const std::string &section_name) {
  if (section_name.empty()) {
    if (global_section_) {
      delete global_section_;
      global_section_ = nullptr;
      return true;
    }
    return false;
  }
  
  auto it = sections_.find(section_name);
  if (it == sections_.end()) {
    return false;
  }
  
  delete it->second;
  sections_.erase(it);
  return true;
}

bool IniFile::SaveFile(const std::string &filename) const {
  std::string file_to_save = filename.empty() ? filename_ : filename;
  if (file_to_save.empty()) {
    return false;
  }
  
  std::ofstream file(file_to_save);
  if (!file.is_open()) {
    return false;
  }
  
  // Write global section first
  if (global_section_ && global_section_->Size() > 0) {
    file << *global_section_ << std::endl;
  }
  
  // Write named sections
  for (const auto &pair : sections_) {
    file << *pair.second << std::endl;
  }
  
  file.close();
  return true;
}

const std::string IniFile::GetString(const std::string &section_name, const std::string &key, const std::string &default_value) const {
  IniSection *section = GetSection(section_name);
  if (section == nullptr) {
    return default_value;
  }
  return section->GetString(key, default_value);
}

bool IniFile::GetBool(const std::string &section_name, const std::string &key, bool default_value) const {
  IniSection *section = GetSection(section_name);
  if (section == nullptr) {
    return default_value;
  }
  return section->GetBool(key, default_value);
}

Si32 IniFile::GetInt(const std::string &section_name, const std::string &key, Si32 default_value) const {
  IniSection *section = GetSection(section_name);
  if (section == nullptr) {
    return default_value;
  }
  return section->GetInt(key, default_value);
}

float IniFile::GetFloat(const std::string &section_name, const std::string &key, float default_value) const {
  IniSection *section = GetSection(section_name);
  if (section == nullptr) {
    return default_value;
  }
  return section->GetFloat(key, default_value);
}

IniSection *IniFile::operator[](const std::string &section_name) const {
  return GetSection(section_name);
}

std::string IniFile::GetErrorDescription() const {
  return error_description_;
}

bool IniFile::ParseContent() {
  // Clear existing data
  for (auto &pair : sections_) {
    delete pair.second;
  }
  sections_.clear();
  
  if (global_section_) {
    delete global_section_;
    global_section_ = nullptr;
  }
  
  IniSection *current_section = nullptr;
  
  for (const std::string &line : lines_) {
    std::string trimmed = Trim(line);
    
    // Skip empty lines and comments
    if (trimmed.empty() || IsComment(trimmed)) {
      continue;
    }
    
    // Check if it's a section header
    if (trimmed.front() == '[' && trimmed.back() == ']') {
      std::string section_name = ExtractSectionName(trimmed);
      if (section_name.empty() && trimmed.size() > 2) {
        error_description_ = "Invalid section header: " + trimmed;
        return false;
      }
      
      current_section = AddSection(section_name);
      if (!current_section) {
        current_section = GetSection(section_name);  // Section already exists, use it
      }
      continue;
    }
    
    // Parse key-value pair
    std::string key, value;
    if (ParseKeyValue(trimmed, key, value)) {
      if (!current_section) {
        // Create global section if we don't have one
        current_section = AddSection("");
      }
      current_section->SetValue(key, value);
    } else {
      error_description_ = "Invalid key-value pair: " + trimmed;
      return false;
    }
  }
  
  return true;
}

std::string IniFile::Trim(const std::string &str) const {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

bool IniFile::IsComment(const std::string &line) const {
  return !line.empty() && (line[0] == ';' || line[0] == '#');
}

std::string IniFile::ExtractSectionName(const std::string &line) const {
  if (line.size() < 2 || line.front() != '[' || line.back() != ']') {
    return "";
  }
  
  std::string section_name = line.substr(1, line.size() - 2);
  return Trim(section_name);
}

bool IniFile::ParseKeyValue(const std::string &line, std::string &key, std::string &value) const {
  size_t pos = line.find('=');
  if (pos == std::string::npos) {
    pos = line.find(':');
  }
  
  if (pos == std::string::npos) {
    return false;
  }
  
  key = Trim(line.substr(0, pos));
  value = Trim(line.substr(pos + 1));
  
  // Remove quotes if present
  if (value.size() >= 2) {
    if ((value.front() == '"' && value.back() == '"') ||
        (value.front() == '\'' && value.back() == '\'')) {
      value = value.substr(1, value.size() - 2);
    }
  }
  
  return !key.empty();
}

}  // namespace arctic
