// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 - 2020 Huldra
// Copyright (c) 2017 Romain Sylvian
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

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

#include "engine/csv.h"
#include "engine/arctic_types.h"
#include "engine/arctic_platform_fatal.h"

namespace arctic {

CsvRow CsvRow::invalid_row_(std::vector<std::string>());

CsvTable::CsvTable() {
}

bool CsvTable::LoadFile(const std::string &filename, char sep) {
  type_ = kCsvSourceFile;
  sep_ = sep;
  file_ = filename;
  std::string line;
  std::ifstream ifile(file_.c_str());
  if (ifile.is_open()) {
    while (ifile.good()) {
      getline(ifile, line);
      if (!line.empty()) {
        original_file_.push_back(line);
      }
    }
    ifile.close();

    if (original_file_.empty()) {
      error_description = std::string("No Data in ").append(file_);
      return false;
    }
    // Remove the BOM (Byte Order Mark) for UTF-8
    if (original_file_[0].length() >= 3 &&
        Ui8(original_file_[0][0]) == 0xEF &&
        Ui8(original_file_[0][1]) == 0xBB &&
        Ui8(original_file_[0][2]) == 0xBF) {
      original_file_[0] = original_file_[0].erase(0, 3);
      // Check if header becomes empty after BOM removal
      if (original_file_[0].empty()) {
        error_description = "Header line is empty after BOM removal";
        return false;
      }
    }
    bool is_ok = true;
    is_ok = is_ok && ParseHeader();
    is_ok = is_ok && ParseContent();
    return is_ok;
  } else {
    error_description = std::string("Failed to open ").append(file_);
    return false;
  }
}

bool CsvTable::LoadString(const std::string &data, char sep) {
  type_ = kCsvSourcePure;
  sep_ = sep;
  std::string line;
  std::istringstream stream(data);
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      original_file_.push_back(line);
    }
  }
  if (original_file_.empty()) {
    error_description = std::string("No Data in pure content");
    return false;
  }

  bool is_ok = true;
  is_ok = is_ok && ParseHeader();
  is_ok = is_ok && ParseContent();
  return is_ok;
}

CsvTable::~CsvTable() {
  std::vector<CsvRow *>::iterator it;
  for (it = content_.begin(); it != content_.end(); ++it) {
    delete *it;
  }
}

bool CsvTable::ParseHeader() {
  const std::string& header_line = original_file_[0];
  bool quoted = false;
  std::string current_field;
  current_field.reserve(header_line.length()); // Reserve memory to avoid reallocations
  
  for (size_t i = 0; i < header_line.length(); ++i) {
    char c = header_line.at(i);
    
    if (c == '"') {
      // Check for escaped quote ("")
      if (i + 1 < header_line.length() && header_line.at(i + 1) == '"' && quoted) {
        // Escaped quote - add one quote to field and skip the second
        current_field += '"';
        ++i; // Skip the second quote
      } else {
        // Toggle quoted state, but don't add the quote to the field
        quoted = !quoted;
      }
    } else if (c == sep_ && !quoted) {
      // End of field - remove any trailing \r and add to header
      while (!current_field.empty() && current_field.back() == '\r') {
        current_field.pop_back();
      }
      header_.push_back(current_field);
      current_field.clear();
    } else {
      // Regular character - add to current field
      current_field += c;
    }
  }
  
  // Handle the last field
  while (!current_field.empty() && current_field.back() == '\r') {
    current_field.pop_back();
  }
  header_.push_back(current_field);
  
  // Check for unmatched quotes in header
  if (quoted) {
    error_description = "Unmatched quote in CSV header";
    return false;
  }
  
  return true;
}

bool CsvTable::ParseContent() {
  if (original_file_.size() <= 1) {
    // No content rows to process, only header
    return true;
  }
  
  std::deque<std::string>::iterator it = original_file_.begin();
  ++it;  // skip header
  Ui64 line_idx = 1;
  for (; it != original_file_.end(); ++it) {
    bool quoted = false;
    std::string current_field;
    current_field.reserve(it->length()); // Reserve memory to avoid reallocations
    CsvRow *row = new CsvRow(header_);

    for (size_t i = 0; i < it->length(); ++i) {
      char c = it->at(i);
      
      if (c == '"') {
        // Check for escaped quote ("")
        if (i + 1 < it->length() && it->at(i + 1) == '"' && quoted) {
          // Escaped quote - add one quote to field and skip the second
          current_field += '"';
          ++i; // Skip the second quote
        } else {
          // Toggle quoted state, but don't add the quote to the field
          quoted = !quoted;
        }
      } else if (c == sep_ && !quoted) {
        // End of field - remove any trailing \r and add to row
        while (!current_field.empty() && current_field.back() == '\r') {
          current_field.pop_back();
        }
        row->Push(current_field);
        current_field.clear();
      } else {
        // Regular character - add to current field
        current_field += c;
      }
    }
    
    // Handle the last field
    while (!current_field.empty() && current_field.back() == '\r') {
      current_field.pop_back();
    }
    row->Push(current_field);

    // Check for unmatched quotes in this row
    if (quoted) {
      std::stringstream str;
      str << "Unmatched quote at line " << line_idx
        << " of file \"" << file_ << "\"";
      error_description = str.str();
      delete row;
      return false;
    }

    // if value(s) missing
    if (row->Size() != header_.size()) {
      std::stringstream str;
      str << "Сorrupted data at line " << line_idx
        << " of file \"" << file_ << "\""
        << " header items: " << header_.size()
        << " row items: " << row->Size();
      error_description = str.str();
      delete row;
      return false;
    }
    content_.push_back(row);

    line_idx++;
  }
  return true;
}

CsvRow *CsvTable::GetRow(Ui64 row_position) const {
  if (row_position < content_.size()) {
    return content_[static_cast<size_t>(row_position)];
  }
  return nullptr;
}

CsvRow &CsvTable::operator[](Ui64 row_position) const {
  CsvRow *row = CsvTable::GetRow(row_position);
  if (!row) {
    return CsvRow::Invalid();
  }
  return *row;
}

std::string CsvTable::GetErrorDescription() const {
  return error_description;
}

Ui64 CsvTable::RowCount() const {
  return static_cast<Ui64>(content_.size());
}

Ui64 CsvTable::ColumnCount() const {
  return static_cast<Ui64>(header_.size());
}

std::vector<std::string> CsvTable::GetHeader() const {
  return header_;
}

const std::string CsvTable::GetHeaderElement(Ui64 pos) const {
  if (pos >= header_.size()) {
    return std::string();
  }
  return header_[static_cast<size_t>(pos)];
}

bool CsvTable::DeleteRow(Ui64 pos) {
  if (static_cast<size_t>(pos) < content_.size()) {
    delete *(content_.begin() + static_cast<int>(pos));
    content_.erase(content_.begin() + static_cast<int>(pos));
    return true;
  }
  return false;
}

bool CsvTable::AddRow(Ui64 pos, const std::vector<std::string> &r) {
  CsvRow *row = new CsvRow(header_);

  for (auto it = r.begin(); it != r.end(); ++it) {
    row->Push(*it);
  }

  if (pos <= content_.size()) {
    content_.insert(content_.begin() + static_cast<int>(pos), row);
    return true;
  }
  delete row;
  return false;
}

void CsvTable::SaveFile() const {
  if (type_ == CsvSourceType::kCsvSourceFile) {
    std::ofstream f;
    f.open(file_, std::ios::out | std::ios::trunc);

    // header
    Ui64 i = 0;
    for (auto it = header_.begin(); it != header_.end(); ++it) {
      f << *it;
      if (i < header_.size() - 1) {
        f << ",";
      } else {
        f << std::endl;
      }
      i++;
    }

    for (auto it = content_.begin(); it != content_.end(); ++it) {
      f << **it << std::endl;
    }
    f.close();
  }
}

const std::string &CsvTable::GetFileName() const {
  return file_;
}

// ROW
CsvRow::CsvRow(const std::vector<std::string> &header)
    : header_(header)
{}

CsvRow::~CsvRow() {}

Ui64 CsvRow::Size() const {
  return static_cast<Ui64>(values_.size());
}

void CsvRow::Push(const std::string &value) {
  Check(this != &invalid_row_, "CsvRow::Push called on invalid row");
  values_.push_back(value);
}

bool CsvRow::Set(const std::string &key, const std::string &value) {
  Check(this != &invalid_row_, "CsvRow::Set called on invalid row");
  std::vector<std::string>::const_iterator it;
  size_t pos = 0;
  for (it = header_.begin(); it != header_.end(); ++it) {
    if (key == *it) {
      values_[pos] = value;
      return true;
    }
    pos++;
  }
  return false;
}

const std::string CsvRow::operator[](Ui64 value_position) const {
  if (value_position < values_.size()) {
    return values_[static_cast<size_t>(value_position)];
  }
  return std::string();
}

const std::string CsvRow::operator[](const std::string &key) const {
  std::vector<std::string>::const_iterator it;
  size_t pos = 0;
  for (it = header_.begin(); it != header_.end(); ++it) {
    if (key == *it) {
      return values_[pos];
    }
    pos++;
  }
  return std::string();
}

std::ostream &operator<<(std::ostream &os, const CsvRow &row) {
  for (size_t i = 0; i != row.values_.size(); ++i) {
    os << row.values_[i] << " | ";
  }
  return os;
}

std::ofstream &operator<<(std::ofstream &os, const CsvRow &row) {
  for (size_t i = 0; i != row.values_.size(); ++i) {
    os << row.values_[i];
    if (i + 1 < row.values_.size()) {
      os << ",";
    }
  }
  return os;
}

}  // namespace arctic

