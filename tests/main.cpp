#include "engine/test_main.h"

#include <deque>
#include <string>
#include <iostream>
#include <sstream>

#include "engine/arctic_types.h"
#include "engine/arctic_platform.h"
#include "engine/easy.h"
#include "engine/rgb.h"


using namespace arctic;

void test_rgb() {
  Rgb x(1, 2, 3);
  Rgb y(Ui8(4));
  Rgb z(Ui32(0xaabbcc));
  Rgb u = z;
  Ui8 rx = x[0];
  Ui8 gx = x[1];
  Ui8 bx = x[2];
  y.r = 5;
  Rgb yt(5, 4, 4);
  TEST_CHECK(yt == y);
  TEST_CHECK(yt != u);
  TEST_CHECK(rx == 1);
  TEST_CHECK(gx == 2);
  TEST_CHECK(bx == 3);
  TEST_CHECK(u.r == 0xcc);
  TEST_CHECK(u.g == 0xbb);
  TEST_CHECK(u.b == 0xaa);
}

void test_file_operations() {
  std::stringstream str;
  std::deque<DirectoryEntry> list;
  std::string canonic = CanonicalizePath("./..");
  /*
  if (canonic.empty()) {
    str << "empty canonic\n";
  } else {
    str << "Canonic: \"" << canonic << "\"\n";
  }
  
  std::string relative = RelativePathFromTo("../../../../../",
                                            "../../../../../../piLibs");
  str << "relative path: \"" << relative.c_str() << "\"\n";
  str << "from " << CanonicalizePath("../../../../../").c_str() << "\n";
  str << "to " << CanonicalizePath("../../../../../../piLibs").c_str() << "\n";
  
  bool isok = GetDirectoryEntries("../../../../../../piLibs", &list);
  TEST_CHECK(isok);
  for (const auto &entry: list) {
    str << entry.title << "\n";
  }
  std::string res = str.str();
  std::cout << res << std::endl;
  
  arctic::easy::WriteFile("../../../result.txt",
      (const Ui8*)(const void*)res.data(), res.size());
      */
}

TEST_LIST = {
  {"Rgb", test_rgb},
  {"File operations", test_file_operations},
  {0}
};

