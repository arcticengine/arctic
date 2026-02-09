// The MIT License (MIT)
//
// Copyright (c) 2018 - 2021 Huldra
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

#include <deque>
#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <sstream>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_utility
/// @{

/// @brief Represents a row in a CSV file.
class CsvRow {
 private:
  static CsvRow invalid_row_;
  const std::vector<std::string> header_;
  std::vector<std::string> values_;
  char sep_ = ',';

 public:
  /// @brief Constructs a CsvRow with the given header.
  /// @param header The header for the CSV row.
  explicit CsvRow(const std::vector<std::string> &header, char sep = ',');

  /// @brief Returns a reference to a static invalid row sentinel.
  /// Mutating methods on the invalid row will trigger a fatal error.
  /// @return Reference to the invalid row.
  static CsvRow &Invalid() {
    return invalid_row_;
  }

  /// @brief Returns true if this is a valid row (not the invalid sentinel).
  bool IsValid() const {
    return this != &invalid_row_;
  }

  /// @brief Destructor for CsvRow.
  ~CsvRow();

  /// @brief Returns the number of elements in the row.
  /// @return The size of the row.
  Ui64 Size() const;

  /// @brief Adds a value to the row.
  /// @param value The value to be added.
  void Push(const std::string &value);

  /// @brief Sets a value in the row by column name.
  /// @param key The column name.
  /// @param value The value to be set.
  /// @return True if the value was set successfully, false otherwise.
  bool Set(const std::string &key, const std::string &value);

  /// @brief Gets a value from the row by position and converts it to the specified type.
  /// @tparam T The type to convert the value to.
  /// @param pos The position of the value in the row.
  /// @param default_value The default value to return if conversion fails.
  /// @return The converted value or the default value.
  template<typename T>
  const T GetValue(Ui64 pos, T default_value) const {
    if (pos < values_.size()) {
      T res;
      std::stringstream ss;
      ss << values_[static_cast<size_t>(pos)];
      ss >> res;
      if (ss.fail()) {
        return default_value;
      }
      if (ss.peek() == std::iostream::traits_type::eof()) {
        return res;
      }
    }
    return default_value;
  }

  /// @brief Gets a value from the row by column name and converts it to the specified type.
  /// @tparam T The type to convert the value to.
  /// @param value_name The name of the column.
  /// @param default_value The default value to return if conversion fails.
  /// @return The converted value or the default value.
  template<typename T>
  const T GetValue(const std::string &value_name, T default_value) const {
    std::stringstream ss((*this)[value_name]);
    T res;
    ss >> res;
    if (ss.fail()) {
      return default_value;
    }
    if (ss.peek() == std::iostream::traits_type::eof()) {
      return res;
    }
    return default_value;
  }

  /// @brief Accesses a value in the row by position.
  /// @param pos The position of the value.
  /// @return The value at the specified position.
  const std::string operator[](Ui64 pos) const;

  /// @brief Accesses a value in the row by column name.
  /// @param value_name The name of the column.
  /// @return The value in the specified column.
  const std::string operator[](const std::string &value_name) const;

  /// @brief Outputs the row to an output stream.
  friend std::ostream& operator<<(std::ostream& os, const CsvRow &row);

  /// @brief Outputs the row to an output file stream.
  friend std::ofstream& operator<<(std::ofstream& os, const CsvRow &row);
};

/// @brief Enumerates the types of CSV sources.
enum CsvSourceType {
  kCsvSourceFile = 0,  ///< CSV source is a file.
  kCsvSourcePure = 1   ///< CSV source is a string.
};

/// @brief Represents a CSV table.
class CsvTable {
 public:
  /// @brief Default constructor for CsvTable.
  CsvTable();

  /// @brief Loads a CSV file.
  /// @param filename The name of the file to load.
  /// @param sep The separator character (default is comma).
  /// @return True if the file was loaded successfully, false otherwise.
  bool LoadFile(const std::string &filename, char sep = ',');

  /// @brief Loads a CSV from a string.
  /// @param input The input string containing CSV data.
  /// @param sep The separator character (default is comma).
  /// @return True if the string was parsed successfully, false otherwise.
  bool LoadString(const std::string &input, char sep = ',');

  /// @brief Destructor for CsvTable.
  ~CsvTable();

  /// @brief Gets a row from the table.
  /// @param row The index of the row.
  /// @return A pointer to the CsvRow at the specified index.
  CsvRow *GetRow(Ui64 row) const;

  /// @brief Returns the number of rows in the table.
  /// @return The row count.
  Ui64 RowCount() const;

  /// @brief Returns the number of columns in the table.
  /// @return The column count.
  Ui64 ColumnCount() const;

  /// @brief Gets the header of the CSV table.
  /// @return A vector of strings representing the header.
  std::vector<std::string> GetHeader() const;

  /// @brief Gets a header element by position.
  /// @param pos The position of the header element.
  /// @return The header element at the specified position.
  const std::string GetHeaderElement(Ui64 pos) const;

  /// @brief Gets the filename of the CSV file.
  /// @return The filename.
  const std::string &GetFileName() const;

  /// @brief Deletes a row from the table.
  /// @param row The index of the row to delete.
  /// @return True if the row was deleted successfully, false otherwise.
  bool DeleteRow(Ui64 row);

  /// @brief Adds a row to the table at a specified position.
  /// @param pos The position to add the row.
  /// @param row_data The data for the new row.
  /// @return True if the row was added successfully, false otherwise.
  bool AddRow(Ui64 pos, const std::vector<std::string> &row_data);

  /// @brief Saves the CSV table to a file.
  void SaveFile() const;

  /// @brief Accesses a row in the table by index.
  /// @param row The index of the row.
  /// @return A reference to the CsvRow at the specified index.
  CsvRow &operator[](Ui64 row) const;

  /// @brief Gets the error description if an operation fails.
  /// @return The error description string.
  std::string GetErrorDescription() const;

 protected:
  /// @brief Parses the header of the CSV.
  /// @return True if the header was parsed successfully, false otherwise.
  bool ParseHeader();

  /// @brief Parses the content of the CSV.
  /// @return True if the content was parsed successfully, false otherwise.
  bool ParseContent();

 private:
  std::string file_;
  CsvSourceType type_ = kCsvSourcePure;
  char sep_ = ',';
  std::deque<std::string> original_file_;
  std::vector<std::string> header_;
  std::vector<CsvRow *> content_;
  std::string error_description;
};
/// @}

}  // namespace arctic

#endif  // ENGINE_CSV_H_
