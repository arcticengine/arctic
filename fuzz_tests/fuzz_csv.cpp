#include <engine/csv.h>
#include <engine/arctic_types.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>

#include <iostream>

jmp_buf arctic_jmp_env;
using namespace arctic;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  std::string fake_csv(reinterpret_cast<const char*>(data), size);
  int val = setjmp(arctic_jmp_env);
  if(val == 1337) {
    return 0;
  }
  arctic::CsvTable table;
  std::cout << "fake_csv: " << fake_csv << std::endl;
  bool isOk = table.LoadString(fake_csv, ',');
  if (isOk) {
    table.GetFileName();
    Ui64 rows = table.RowCount();
    Ui64 cols = table.ColumnCount();
    for (Ui64 i = 0; i < cols; ++i) {
      table.GetHeaderElement(i);
    }
    for (Ui64 i = 0; i < rows; ++i) {
      arctic::CsvRow *row = table.GetRow(i);
      for (Ui64 j = 0; j < rows; ++j) {
        Ui64 value1 = row->GetValue<Ui64>(j, 0);
        float value2 = row->GetValue<float>(j, 0);
        bool value3 = row->GetValue<bool>(j, 0);
        std::string value4 = row->GetValue<std::string>(j, 0);
      }
    }
  } else {
    table.GetErrorDescription();
  }

  return 0;
}


