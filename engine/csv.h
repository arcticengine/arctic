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

#ifndef ENGINE_CSV_H_
#define ENGINE_CSV_H_

#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <sstream>

#include "engine/arctic_types.h"

namespace arctic {

class CsvError : public std::runtime_error {
 public:
  CsvError(const std::string &msg):
      std::runtime_error(std::string("CsvParser : ").append(msg))
  {
  }
};

class CsvRow {
 private:
  const std::vector<std::string> header_;
  std::vector<std::string> values_;

 public:
  CsvRow(const std::vector<std::string> &);
  ~CsvRow();

  unsigned int Size() const;
  void Push(const std::string &);
  bool Set(const std::string &, const std::string &);

  template<typename T>
  const T GetValue(unsigned int pos) const {
    if (pos < values_.size()) {
      T res;
      std::stringstream ss;
      ss << values_[pos];
      ss >> res;
      return res;
    }
    throw CsvError("can't return this value (doesn't exist)");
  }
  const std::string operator[](unsigned int) const;
  const std::string operator[](const std::string &valueName) const;
  friend std::ostream& operator<<(std::ostream& os, const CsvRow &row);
  friend std::ofstream& operator<<(std::ofstream& os, const CsvRow &row);
};

enum CsvSourceType {
  kCsvSourceFile = 0,
  kCsvSourcePure = 1
};

class CsvTable {
 public:
  CsvTable(const std::string &, const CsvSourceType &type = kCsvSourceFile, char sep = ',');
  ~CsvTable();

  CsvRow &GetRow(unsigned int row) const;
  unsigned int RowCount() const;
  unsigned int ColumnCount() const;
  std::vector<std::string> GetHeader() const;
  const std::string GetHeaderElement(unsigned int pos) const;
  const std::string &GetFileName() const;

  bool DeleteRow(unsigned int row);
  bool AddRow(unsigned int pos, const std::vector<std::string> &);
  void Sync() const;

  CsvRow &operator[](unsigned int row) const;
 protected:
  void ParseHeader();
  void ParseContent();

 private:
  std::string file_;
  const CsvSourceType type_;
  const char sep_;
  std::vector<std::string> original_file_;
  std::vector<std::string> header_;
  std::vector<CsvRow *> content_;
};

}  // namespace arctic

#endif  // ENGINE_CSV_H_
