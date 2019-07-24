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
  std::deque<DirectoryEntry> list;
  std::string canonic = arctic::CanonicalizePath("./..");
  
  std::string arctic_engine_dir = "../engine";

  Si32 i = 0;
  while (i < 10) {
    if (arctic::DoesDirectoryExist(arctic_engine_dir.c_str())) {
      break;
    }
    arctic_engine_dir = std::string("../") + arctic_engine_dir;
  }
  TEST_CHECK(i != 10);

  std::string arctic_root_dir = arctic_engine_dir + std::string("/..");

  std::string relative = RelativePathFromTo(arctic_root_dir.c_str(),
      arctic_engine_dir.c_str());
  TEST_CHECK(relative == std::string("./engine"));

  std::string relative2 = RelativePathFromTo(arctic_engine_dir.c_str(),
                                             arctic_root_dir.c_str());
  TEST_CHECK(relative2 == std::string("../"));
  
  bool isok = GetDirectoryEntries(arctic_engine_dir.c_str(), &list);
  TEST_CHECK(isok);
  TEST_CHECK(list.size() > 0);
}

TEST_LIST = {
  {"Rgb", test_rgb},
  {"File operations", test_file_operations},
  {0}
};

