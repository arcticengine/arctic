#include "engine/easy.h"

#include <iostream>
#include <sstream>

using namespace arctic;  // NOLINT

void EasyMain() {
  std::stringstream str;
  std::deque<DirectoryEntry> list;
  std::string canonic = CanonicalizePath("./..");
  if (canonic.empty()) {
    str << "empty canonic\n";
  } else {
    str << "Canonic: \"" << canonic << "\"\n";
  }
  
  std::string relative = RelativePathFromTo("../../../../../",
                                            "../../../../../../piLibs");
  str << "relative path: \"" << relative << "\"\n";
  str << "from " << CanonicalizePath("../../../../../") << "\n";
  str << "to " << CanonicalizePath("../../../../../../piLibs") << "\n";
  
  bool isok = GetDirectoryEntries("../../../../../../piLibs", &list);
  Check(isok, "Can't list directory");
  for (const auto &entry: list) {
    str << entry.title << "\n";
  }
  std::string res = str.str();
  std::cout << res << std::endl;
  
  WriteFile("../../../result.txt", (const Ui8*)(const void*)res.data(), res.size());
}
