// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#include "engine/arctic_platform_def.h"
#include "engine/arctic_platform.h"

#ifdef ARCTIC_PLATFORM_PI

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <deque>
#include <string>

#include "engine/easy.h"

extern void EasyMain();

namespace arctic {

Trivalent DoesDirectoryExist(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return kTrivalentFalse;
  } else if (info.st_mode & S_IFDIR) {
    return kTrivalentTrue;
  } else {
    return kTrivalentUnknown;
  }
}

bool MakeDirectory(const char *path) {
  Si32 result = mkdir(path,
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
  return (result == 0);
}

bool GetCurrentPath(std::string *out_dir) {
  char cwd[1 << 20];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    out_dir->assign(cwd);
    return true;
  }
  return false;
}

bool GetDirectoryEntries(const char *path,
     std::deque<DirectoryEntry> *out_entries) {
  Check(out_entries,
    "GetDirectoryEntries Error. Unexpected nullptr in out_entries!");
  out_entries->clear();
  DIR *dir = opendir(path);
  if (dir == nullptr) {
    std::stringstream info;
    info << "Error errno: " << errno
      << " while opening path: \"" << path << "\"" << std::endl;
    Log(info.str().c_str());
    return false;
  }
  char full_path[1 << 20];
  while (true) {
    struct dirent *dir_entry = readdir(dir);
    if (dir_entry == nullptr) {
      break;
    }
    DirectoryEntry entry;
    entry.title = dir_entry->d_name;
    snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);
    struct stat info;
    if (stat(full_path, &info) != 0) {
      return false;
    }
    if (info.st_mode & S_IFDIR) {
      entry.is_directory = kTrivalentTrue;
    }
    if (info.st_mode & S_IFREG) {
      entry.is_file = kTrivalentTrue;
    }
    out_entries->push_back(entry);
  }
  closedir(dir);
  return true;
}


std::string CanonicalizePath(const char *path) {
  Check(path, "CanonicalizePath error, path can't be nullptr");
  char *canonic_path = realpath(path, nullptr);
  std::string result;
  if (canonic_path) {
    result.assign(canonic_path);
    free(canonic_path);
  }
  return result;
}

std::string RelativePathFromTo(const char *from, const char *to) {
  std::string from_abs = CanonicalizePath(from);
  if (from && from[strlen(from) - 1] == '/' &&
      from_abs.size() && from_abs[from_abs.size() - 1] != '/') {
    from_abs = from_abs + '/';
  }
  std::string to_abs = CanonicalizePath(to);
  if (to && to[strlen(to) - 1] == '/' &&
      to_abs.size() && to_abs[to_abs.size() - 1] != '/') {
    to_abs = to_abs + '/';
  }
  Ui32 matching = 0;
  while (matching < from_abs.size() && matching < to_abs.size()) {
    if (from_abs[matching] == to_abs[matching]) {
      ++matching;
    } else {
      break;
    }
  }
  if (matching == from_abs.size() && matching == to_abs.size()) {
    return "./";
  }
  while (matching && from_abs[matching - 1] != '/') {
    --matching;
  }
  const char *from_part = from_abs.c_str() + matching;
  std::stringstream res;
  while (*from_part != 0) {
    res << "../";
    ++from_part;
    while (*from_part != 0 && *from_part != '/') {
      ++from_part;
    }
  }
  const char *to_part = to_abs.c_str() + matching;
  res << to_part;
  return res.str();
}


}  // namespace arctic

#endif  // ARCTIC_PLATFORM_PI
