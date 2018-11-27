// The MIT License (MIT)
//
// Copyright (c) 2018 Huldra
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

#include "engine/csv.h"
#include "engine/arctic_types.h"

namespace arctic {

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
      if (line != "") {
        original_file_.push_back(line);
      }
    }
    ifile.close();
    
    if (original_file_.size() == 0) {
      error_description = std::string("No Data in ").append(file_);
      return false;
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
    if (line != "") {
      original_file_.push_back(line);
    }
  }
  if (original_file_.size() == 0) {
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
  for (it = content_.begin(); it != content_.end(); it++) {
    delete *it;
  }
}

bool CsvTable::ParseHeader() {
  std::stringstream ss(original_file_[0]);
  std::string item;
  while (std::getline(ss, item, sep_)) {
    header_.push_back(item);
  }
  return true;
}

bool CsvTable::ParseContent() {
  std::deque<std::string>::iterator it = original_file_.begin();
  it++; // skip header
  Si32 line_idx = 1;
  for (; it != original_file_.end(); it++) {
    bool quoted = false;
    Si32 token_start = 0;
    Ui32 i = 0;
    
    CsvRow *row = new CsvRow(header_);
    
    for (; i != it->length(); i++) {
      if (it->at(i) == '"') {
        quoted = ((quoted) ? (false) : (true));
      } else if (it->at(i) == ',' && !quoted) {
        row->Push(it->substr(token_start, i - token_start));
        token_start = i + 1;
      }
    }
    //end
    row->Push(it->substr(token_start, it->length() - token_start));
    
    // if value(s) missing
    if (row->Size() != header_.size()) {
      std::stringstream str;
      str << "corrupted data at line " << line_idx;
      error_description = str.str();
      return false;
    }
    content_.push_back(row);
    
    line_idx++;
  }
  return true;
}

CsvRow *CsvTable::GetRow(Ui32 row_position) const {
  if (row_position < content_.size()) {
    return content_[row_position];
  }
  return nullptr;
}

CsvRow &CsvTable::operator[](Ui32 row_position) const {
  CsvRow *row = CsvTable::GetRow(row_position);
  // Check(row, "row_position out of bounds in CvsTable");
  return *row;
}

Ui32 CsvTable::RowCount() const {
  return static_cast<Ui32>(content_.size());
}

Ui32 CsvTable::ColumnCount() const {
  return static_cast<Ui32>(header_.size());
}

std::vector<std::string> CsvTable::GetHeader() const {
  return header_;
}

const std::string CsvTable::GetHeaderElement(Ui32 pos) const {
  if (pos >= header_.size()) {
    return nullptr;
  }
  return header_[pos];
}

bool CsvTable::DeleteRow(Ui32 pos) {
  if (pos < content_.size()) {
    delete *(content_.begin() + pos);
    content_.erase(content_.begin() + pos);
    return true;
  }
  return false;
}

bool CsvTable::AddRow(Ui32 pos, const std::vector<std::string> &r) {
  CsvRow *row = new CsvRow(header_);
  
  for (auto it = r.begin(); it != r.end(); it++) {
    row->Push(*it);
  }
  
  if (pos <= content_.size()) {
    content_.insert(content_.begin() + pos, row);
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
    Ui32 i = 0;
    for (auto it = header_.begin(); it != header_.end(); it++) {
      f << *it;
      if (i < header_.size() - 1) {
        f << ",";
      } else {
        f << std::endl;
      }
      i++;
    }
    
    for (auto it = content_.begin(); it != content_.end(); it++) {
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

Ui32 CsvRow::Size() const {
  return static_cast<Ui32>(values_.size());
}

void CsvRow::Push(const std::string &value) {
  values_.push_back(value);
}

bool CsvRow::Set(const std::string &key, const std::string &value) {
  std::vector<std::string>::const_iterator it;
  Si32 pos = 0;
  for (it = header_.begin(); it != header_.end(); it++) {
    if (key == *it) {
      values_[pos] = value;
      return true;
    }
    pos++;
  }
  return false;
}

const std::string CsvRow::operator[](Ui32 value_position) const {
  if (value_position < values_.size()) {
    return values_[value_position];
  }
  return nullptr;
}

const std::string CsvRow::operator[](const std::string &key) const {
  std::vector<std::string>::const_iterator it;
  Si32 pos = 0;
  for (it = header_.begin(); it != header_.end(); it++) {
    if (key == *it) {
      return values_[pos];
    }
    pos++;
  }
  return nullptr;
}

std::ostream &operator<<(std::ostream &os, const CsvRow &row) {
  for (Ui32 i = 0; i != row.values_.size(); i++) {
    os << row.values_[i] << " | ";
  }
  return os;
}

std::ofstream &operator<<(std::ofstream &os, const CsvRow &row) {
  for (Ui32 i = 0; i != row.values_.size(); i++) {
    os << row.values_[i];
    if (i < row.values_.size() - 1) {
      os << ",";
    }
  }
  return os;
}

}  // namespace arctic

