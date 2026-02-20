#include "engine/easy.h"

#include <iostream>
#include <sstream>

using namespace arctic;  // NOLINT

void EasyMain() {
  std::stringstream str;
  std::vector<DirectoryEntry> list;
  std::string base = GetEngine()->GetInitialPath();

  std::string canonic = CanonicalizePath((base + "/..").c_str());
  if (canonic.empty()) {
    str << "empty canonic\n";
  } else {
    str << "Canonic: \"" << canonic << "\"\n";
  }

  std::string from_path = base + "/../../../..";
  std::string to_path = base + "/../../../../../piLibs";
  std::string relative = RelativePathFromTo(from_path.c_str(),
                                            to_path.c_str());
  str << "relative path: \"" << relative << "\"\n";
  str << "from " << CanonicalizePath(from_path.c_str()) << "\n";
  str << "to " << CanonicalizePath(to_path.c_str()) << "\n";

  std::string near_path = base + "/../piLibs";
  std::string far_path = base + "/../../piLibs";
  bool isok = GetDirectoryEntries(near_path.c_str(), &list);
  bool isnear = isok;
  if (!isok) {
    isok = GetDirectoryEntries(far_path.c_str(), &list);
  }
  Check(isok, "Can't list directory");
  for (const auto &entry: list) {
    str << entry.title << "\n";
  }
  std::string res = str.str();
  std::cout << res << std::endl;
  WriteFile(isnear ? "result.txt" : "../../../result.txt", (const Ui8*)(const void*)res.data(), res.size());
}
