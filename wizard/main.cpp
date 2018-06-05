// The MIT License (MIT)
//
// Copyright (c) 2016 - 2018 Huldra
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

#include <cstring>
#include <regex>
#include <unordered_set>
#include <string>
#include <sstream>
#include <vector>

#include <iostream>

#include "engine/easy.h"


using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

Font g_font;
std::string g_project_name;  // NOLINT
std::string g_progress;  // NOLINT
std::string g_path;  // NOLINT
std::string g_current_directory;  // NOLINT
std::string g_template;  // NOLINT
std::string g_project_directory;  // NOLINT
std::string g_engine;  //NOLINT

enum MainMode {
  kModeCreate = 0,
  kModeUpdate = 1
};

MainMode g_mode_of_operation = kModeCreate;

std::string MakeUid() {
  char uid[25];
  static const char *src_letter = "0123456789ABCDEF";
  for (Si32 letter = 0; letter < 25; ++letter) {
    uid[letter] = src_letter[Random(0, 15)];
  }
  uid[24] = 0;
  std::string uid_string(uid);
  return  uid_string;
}

void ReplaceAll(const std::string &from,
    const std::string &to, std::string *in_out_str) {
  if (from.empty()) {
    return;
  }
  Check(in_out_str, "ReplaceAll called with in_out_str == nullptr");
  size_t start_pos = 0;
  while (true) {
    start_pos = in_out_str->find(from, start_pos);
    if (start_pos == std::string::npos) {
      return;
    }
    in_out_str->replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

bool EndsWith(std::string const &longer_string, std::string const &ending) {
  if (longer_string.length() >= ending.length()) {
    return (longer_string.compare(
      longer_string.length() - ending.length(), ending.length(), ending) == 0);
  } else {
    return false;
  }
}

void SplitString(std::string const &full_string, std::string const &splitter,
    std::string *out_prefix, std::string *out_suffix) {
  std::size_t found = full_string.find(splitter);
  if (found == std::string::npos) {
    *out_prefix = full_string;
    *out_suffix = "";
    return;
  }
  *out_prefix = full_string.substr(0, found);
  *out_suffix = full_string.substr(found + splitter.size(), std::string::npos);
  return;
}

std::string CutStringBetween(std::string const &full_string,
    std::string const &begin_after, std::string const &end_before) {
  std::size_t begin = full_string.find(begin_after);
  if (begin == std::string::npos) {
    return "";
  }
  std::size_t end = full_string.find(end_before);
  if (end == std::string::npos) {
    return "";
  }
  std::size_t size = end - begin_after.size() - begin;
  return full_string.substr(begin + begin_after.size(), size);
}

void UpdateResolution() {
  Vec2Si32 size = WindowSize();
  while (size.x > 1600 && size.y > 1000) {
    size = size / 2;
  }
  if (ScreenSize() != size) {
    ResizeScreen(size);
  }
}

bool GetOperationMode() {
  bool is_done = false;
  while (!IsKeyDownward(kKeyEscape)) {
    UpdateResolution();
    Clear();
    if (IsKeyUpward("C")) {
      g_mode_of_operation = kModeCreate;
      is_done = true;
    } else if (IsKeyUpward("U")) {
      g_mode_of_operation = kModeUpdate;
      is_done = true;
    }
    
    const char *welcome = u8"The Snow Wizard\n\n"
    "This wizard can create a new Arctic Engine project for you or update an existing one.\n\n"
    "Press C to Create a new project.\n"
    "Press U to Update an existing project.\n"
    "Press ESC to leave the Snow Wizard.";

    g_font.Draw(welcome, 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
    if (is_done) {
      return true;
    }
  }
  return false;
}

bool GetProjectName() {
  while (!IsKeyDownward(kKeyEscape)) {
    UpdateResolution();
    Clear();
    for (Si32 idx = 0; idx < InputMessageCount(); ++idx) {
      const InputMessage &m = GetInputMessage(idx);
      const char *ch = "";
      bool is_backspace = false;
      bool is_enter = false;
      if (m.kind == InputMessage::kKeyboard && m.keyboard.key_state == 1) {
        switch (m.keyboard.key) {
          case kKey0:
          case kKeyNumpad0:
            ch = "0";
            break;
          case kKey1:
          case kKeyNumpad1:
            ch = "1";
            break;
          case kKey2:
          case kKeyNumpad2:
            ch = "2";
            break;
          case kKey3:
          case kKeyNumpad3:
            ch = "3";
            break;
          case kKey4:
          case kKeyNumpad4:
            ch = "4";
            break;
          case kKey5:
          case kKeyNumpad5:
            ch = "5";
            break;
          case kKey6:
          case kKeyNumpad6:
            ch = "6";
            break;
          case kKey7:
          case kKeyNumpad7:
            ch = "7";
            break;
          case kKey8:
          case kKeyNumpad8:
            ch = "8";
            break;
          case kKey9:
          case kKeyNumpad9:
            ch = "9";
            break;
          case kKeyA:
            ch = "a";
            break;
          case kKeyB:
            ch = "b";
            break;
          case kKeyC:
            ch = "c";
            break;
          case kKeyD:
            ch = "d";
            break;
          case kKeyE:
            ch = "e";
            break;
          case kKeyF:
            ch = "f";
            break;
          case kKeyG:
            ch = "g";
            break;
          case kKeyH:
            ch = "h";
            break;
          case kKeyI:
            ch = "i";
            break;
          case kKeyJ:
            ch = "j";
            break;
          case kKeyK:
            ch = "k";
            break;
          case kKeyL:
            ch = "l";
            break;
          case kKeyM:
            ch = "m";
            break;
          case kKeyN:
            ch = "n";
            break;
          case kKeyO:
            ch = "o";
            break;
          case kKeyP:
            ch = "p";
            break;
          case kKeyQ:
            ch = "q";
            break;
          case kKeyR:
            ch = "r";
            break;
          case kKeyS:
            ch = "s";
            break;
          case kKeyT:
            ch = "t";
            break;
          case kKeyU:
            ch = "u";
            break;
          case kKeyV:
            ch = "v";
            break;
          case kKeyW:
            ch = "w";
            break;
          case kKeyX:
            ch = "x";
            break;
          case kKeyY:
            ch = "y";
            break;
          case kKeyZ:
            ch = "z";
            break;
          case kKeyMinus:
            ch = "_";
            break;
          case kKeyEnter:
            is_enter = true;
            break;
          case kKeyBackspace:
            is_backspace = true;
            break;
          default:
            break;
        }
        if (is_backspace) {
          Si32 len = static_cast<Si32>(g_project_name.size());
          if (len) {
            g_project_name.replace(len - 1, 1, "");
          }
        }
        if (is_enter) {
          if (g_project_name.size()) {
            return true;
          }
        }
        if (ch[0] != '\0') {
          g_project_name.append(ch);
        }
      }
    }
    
    const char *welcome = u8"The Snow Wizard\n\n"
    "This wizard will create a new Arctic Engine project for you.\n\n"
    "Enter the project name:  \"%s\"\n\n"
    "You may use only latin letters, numbers and underscores\n"
    "Press ESC to leave the Snow Wizard";
    char text[1024];
    snprintf(text, sizeof(text), welcome, g_project_name.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
  }
  return false;
}

bool SelectProject() {
  std::deque<DirectoryEntry> entries;
  Ui32 selected_idx = 0;
  g_project_directory = g_current_directory;
  arctic::GetDirectoryEntries(g_project_directory.c_str(), &entries);
  if (entries[0].title == ".") {
    entries.erase(entries.begin());
  }
  bool is_done = false;
  while (!IsKeyDownward(kKeyEscape)) {
    UpdateResolution();
    Clear();
    if (IsKeyUpward(kKeyEnter)) {
      if (selected_idx < entries.size()) {
        if (entries[selected_idx].is_directory == kTrivalentTrue) {
          std::stringstream new_dir;
          new_dir << g_project_directory << "/" << entries[selected_idx].title;
          g_project_directory = CanonicalizePath(new_dir.str().c_str());
          arctic::GetDirectoryEntries(g_project_directory.c_str(), &entries);
          if (entries[0].title == ".") {
            entries.erase(entries.begin());
          }
          selected_idx = 0;
        }
      }
    } else if (IsKeyUpward(kKeyUp)) {
      if (selected_idx > 0) {
        selected_idx--;
      }
    } else if (IsKeyUpward(kKeyDown)) {
      if (selected_idx + 1 < entries.size()) {
        selected_idx++;
      }
    } else if (IsKeyUpward("S")) {
      is_done = true;
    }
    
    std::stringstream str;
    str << u8"The Snow Wizard\n\n"
    "Select an existing Arctic Engine project to update.\n\n"
    "Press arrow keys and ENTER to navigate.\n"
    "Press S while in a project directory to select it.\n"
    "Press ESC to leave the Snow Wizard.\n\n";
    str << "Path: " << g_project_directory << "\n\n";
    
    Si32 begin_i = std::max(0, Si32(selected_idx) - 5);
    for (Ui32 i = (Ui32)begin_i; i < entries.size(); ++i) {
      if (selected_idx == i) {
        str << "--> ";
      } else {
        str << "    ";
      }
      str << entries[i].title;
      str << "\n";
    }
    
    g_font.Draw(str.str().c_str(), 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
    if (is_done) {
      return true;
    }
  }
  return false;
}

bool ShowProgress() {
  Si32 step = 0;
  char text[1 << 20];
  while (!IsKeyDownward(kKeyEscape)) {
    switch (step) {
      case 1:
      {
        bool is_ok = false;
        for (Si32 i = 0; i < 10; ++i) {
          std::string file;
          file.clear();
          file.append(g_current_directory);
          file.append("/arctic.engine");
          std::vector<Ui8> data = ReadFile(file.c_str(), true);
          const char *expected = "arctic.engine";
          if (data.size() >= std::strlen(expected)) {
            if (memcmp(data.data(), expected, std::strlen(expected)) == 0) {
              is_ok = true;
              break;
            }
          }
          g_current_directory.append("/..");
        }
        if (is_ok) {
          g_path = g_current_directory;
          g_path.append("/");
          g_path.append(g_project_name);
          g_progress.append(u8"Arctic Engine is detected.\n");
        } else {
          g_progress.append(u8"Can't detect Arctic Engine. ERROR.\n");
          step = 100500;
        }
      }
        break;
      case 2:
        if (DoesDirectoryExist(g_path.c_str()) == 0) {
          g_progress.append(u8"Directory name is OK\n");
        } else {
          g_progress.append(u8"A directory named \"");
          g_progress.append(g_path);
          g_progress.append(u8"\" already exists. ERROR. Use another name.\n");
          step = 100500;
        }
        break;
      case 3:
        if (MakeDirectory(g_path.c_str())) {
          g_progress.append(u8"Directory is created OK\n");
          g_project_directory = g_path;
        } else {
          g_progress.append(u8"Can't create directory \"");
          g_progress.append(g_path);
          g_progress.append(u8"\". ERROR.\n");
          step = 100500;
        }
        break;
      case 4:
        g_template = g_current_directory + "/template_project_name";
        if (DoesDirectoryExist(g_template.c_str()) == 1) {
          g_progress.append(u8"Template found OK\n");
        } else {
          g_progress.append(u8"Can't find template directory \"");
          g_progress.append(g_template);
          g_progress.append(u8"\". ERROR.\n");
          step = 100500;
        }
        break;
      case 5:
      {
        std::vector<std::string> files = {
          "Assets.xcassets"
          , "Assets.xcassets/AppIcon.appiconset"
          , "data"
          , "template_project_name.xcodeproj"
          , "template_project_name.xcodeproj/project.xcworkspace"
          , "template_project_name.xcodeproj/xcshareddata"
          , "template_project_name.xcodeproj/xcshareddata/xcschemes/"
        };
        for (Si32 idx = 0; idx < static_cast<Si32>(files.size()); ++idx) {
          std::string name = files[idx];
          ReplaceAll("template_project_name", g_project_name, &name);
          MakeDirectory((g_path + "/" + name).c_str());
        }
        g_progress.append(u8"Project structure created OK\n");
      }
        break;
      case 6:
      {
        std::vector<std::string> files = {
          "app_icon.ico"
          , "data/arctic_one_bmf_0.tga"
          , "data/arctic_one_bmf.fnt"
          , "data/block_1.tga"
          , "data/block_2.tga"
        };
        for (Si32 idx = 0; idx < static_cast<Si32>(files.size()); ++idx) {
          auto data = ReadFile((g_template + "/" + files[idx]).c_str());
          std::string name = files[idx];
          ReplaceAll("template_project_name", g_project_name, &name);
          WriteFile((g_path + "/" + name).c_str(), data.data(), data.size());
        }
        g_progress.append(u8"Data files copied OK\n");
      }
        break;
      case 7:
      {
        std::vector<std::string> files = {
          "Assets.xcassets/AppIcon.appiconset/Contents.json"
          , "CMakeLists.txt"
          , "Info.plist"
          , "main.cpp"
          , "resource.h"
          , "resource.rc"
          , "targetver.h"
          , "template_project_name.sln"
          , "template_project_name.vcxproj"
          , "template_project_name.vcxproj.filters"
          , "template_project_name.xcodeproj/project.pbxproj"
          , "template_project_name.xcodeproj/"
                "project.xcworkspace/contents.xcworkspacedata"
          , "template_project_name.xcodeproj/"
                "xcshareddata/xcschemes/Debug.xcscheme"
          , "template_project_name.xcodeproj/"
                "xcshareddata/xcschemes/Release.xcscheme"
        };
        for (Si32 idx = 0; idx < static_cast<Si32>(files.size()); ++idx) {
          std::vector<Ui8> data = ReadFile(
              (g_template + "/" + files[idx]).c_str());
          std::string name = files[idx];
          ReplaceAll("template_project_name", g_project_name, &name);
          data.push_back('\0');
          std::string content = reinterpret_cast<char*>(data.data());
          ReplaceAll("template_project_name", g_project_name, &content);
          WriteFile((g_path + "/" + name).c_str(),
              reinterpret_cast<const Ui8*>(content.data()), content.size());
        }
        g_progress.append(u8"Project created OK\n");
      }
        break;
      case 9:
        return true;
      default:
        break;
    }
    step++;

    UpdateResolution();
    Clear();
    const char *welcome = u8"The Snow Wizard\n\n"
    "Creating project \"%s\"\n\n"
    "Current directory: %s\n"
    "%s\n\n"
    "Press ESC to leave the Snow Wizard";

    snprintf(text, sizeof(text), welcome,
        g_project_name.c_str(), g_current_directory.c_str(),
        g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
  }
  return false;
}

bool ShowUpdateProgress() {
  std::deque<DirectoryEntry> engine_entries;
  Si32 step = 0;
  char text[1 << 20];
  while (!IsKeyDownward(kKeyEscape)) {
    switch (step) {
      case 1: {
        // Find Arctic Engine directory
        bool is_ok = false;
        for (Si32 i = 0; i < 10; ++i) {
          std::string file;
          file.clear();
          file.append(g_current_directory);
          file.append("/arctic.engine");
          file = CanonicalizePath(file.c_str());
          std::vector<Ui8> data = ReadFile(file.c_str(), true);
          const char *expected = "arctic.engine";
          if (data.size() >= std::strlen(expected)) {
            if (memcmp(data.data(), expected, std::strlen(expected)) == 0) {
              is_ok = true;
              break;
            }
          }
          g_current_directory.append("/..");
          g_current_directory = CanonicalizePath(g_current_directory.c_str());
        }
        if (is_ok) {
          g_progress.append(u8"Arctic Engine is detected.\n");
        } else {
          g_progress.append(u8"Can't detect Arctic Engine. ERROR.\n");
          step = 100500;
        }
      }
        break;
      case 2: {
        // Find /engine directory
        g_engine = CanonicalizePath((g_current_directory + "/engine").c_str());
        if (DoesDirectoryExist(g_engine.c_str()) == 1) {
          // List /engine files
          GetDirectoryEntries(g_engine.c_str(), &engine_entries);
          g_progress.append(u8"Enigne found OK: \"");
          g_progress.append(g_engine);
          g_progress.append(u8"\"\n");
        } else {
          g_progress.append(u8"Can't find engine directory \"");
          g_progress.append(g_engine);
          g_progress.append(u8"\". ERROR.\n");
          step = 100500;
        }
      }
        break;
      case 3: {
        // find *.xcodeproj folder and *.vcxproj files, make sure that
        // names match and there are no other xcodeproj and vcxproj pairs
        
        // List selected folder content
        std::deque<DirectoryEntry> entries;
        GetDirectoryEntries(g_project_directory.c_str(), &entries);
        bool is_ok = false;
        Si32 candidate_count = 0;
        // find *.xcodeproj folder and *.vcxproj files
        std::string xcode_ending = ".xcodeproj";
        std::string vcx_ending = ".vcxproj";
        for (Ui32 idx = 0; idx < entries.size(); ++idx) {
          auto &file = entries[idx];
          if (file.is_directory == kTrivalentTrue) {
            if (EndsWith(file.title, xcode_ending)) {
              std::string project_name =
                file.title.substr(0, file.title.size() - xcode_ending.size());
              for (Ui32 i = 0; i < entries.size(); ++i) {
                std::string vcx = project_name + vcx_ending;
                // make sure that names match
                if (entries[i].title == vcx
                    && entries[i].is_file == kTrivalentTrue) {
                  candidate_count++;
                  is_ok = true;
                  g_project_name = project_name;
                  g_progress.append(u8"Project name candidate \"");
                  g_progress.append(project_name);
                  g_progress.append(u8"\"\n");
                }
              }
            }
          }
        }
        
        // make sure that there are no other xcodeproj and vcxproj pairs
        if (candidate_count == 0) {
          g_progress.append(u8"Can't find project files in\"");
          g_progress.append(g_project_directory);
          g_progress.append(u8"\". ERROR.\n");
          step = 100500;
        } else if (candidate_count > 1) {
          g_progress.append(u8"Multiple project files in\"");
          g_progress.append(g_project_directory);
          g_progress.append(u8"\". ERROR.\n");
          step = 100500;
        }
      }
        break;
      case 4: {
        // update xcodeproj
        
        // parse xcodeproj extracting the list of engine files
        std::unordered_set<std::string> existing_files;
        std::string name =
          "template_project_name.xcodeproj/project.pbxproj";
        ReplaceAll("template_project_name", g_project_name, &name);
        std::string xcode_project_full_name = g_project_directory + "/" + name;
        std::vector<Ui8> data = ReadFile(xcode_project_full_name.c_str());
        data.push_back(0);
        std::string pbx_files_begin = "/* Begin PBXFileReference section */";
        std::string pbx_files_end = "/* End PBXFileReference section */";
        std::string full_content(reinterpret_cast<char*>(data.data()));
        std::string files = CutStringBetween(full_content,
          pbx_files_begin, pbx_files_end);
        std::regex path_regex("path = (.*?);");
        std::sregex_iterator next(files.begin(), files.end(), path_regex);
        std::sregex_iterator end;
        while (next != end) {
          std::string path = next->str(1).c_str();
          std::string full_path =
            CanonicalizePath((g_project_directory + "/" + path).c_str());
          std::string rel_path = RelativePathFromTo(
            (g_engine + "/").c_str(), full_path.c_str());
          if (rel_path.size() && rel_path[0] != '.') {
            existing_files.insert(rel_path);
          }
          next++;
        }
        // find missing engine files
        std::stringstream new_buildfiles;  // PBXBuildFile
        std::stringstream new_files;  // PBXFileReference
        std::stringstream new_engine_children;  // PBXGroup engine children
        std::stringstream new_buildphase;  // PBXSourcesBuildPhase
        std::unordered_set<std::string> new_hashes;
        for (Ui32 idx = 0; idx < engine_entries.size(); ++idx) {
          auto &entry = engine_entries[idx];
          if (entry.is_file == kTrivalentTrue
                && existing_files.find(entry.title) == existing_files.end()) {
            bool is_inserted = false;
            while (!is_inserted) {
              // generate 2 random uids
              std::string uid_file = MakeUid();
              std::string uid_buildfile = MakeUid();
              
              // make sure the uid is not used in the xcodeproj
              if (full_content.find(uid_file) == std::string::npos &&
                  new_hashes.find(uid_file) == new_hashes.end() &&
                  full_content.find(uid_buildfile) == std::string::npos &&
                  new_hashes.find(uid_buildfile) == new_hashes.end() &&
                  uid_file != uid_buildfile) {
                is_inserted = true;
                // save hashes
                new_hashes.insert(uid_file);
                new_hashes.insert(uid_buildfile);
                
                // add to PBXBuildFile
                if (EndsWith(entry.title, std::string(".cpp"))) {
                  new_buildfiles << "\t\t" << uid_buildfile
                    << " /* " << entry.title << " in Sources */ = {"
                    << "isa = PBXBuildFile; fileref = " << uid_file
                    << " /* " << entry.title << " */; };\n";
                }

                // add to PBXFileReference
                new_files << "\t\t" << uid_file
                  << "/* " << entry.title << " */ = {"
                  << "isa = PBXFileReference; fileEncoding = 4;";
                if (EndsWith(entry.title, std::string(".h"))) {
                  new_files << " lastKnownFileType = sourcecode.c.h;";
                } else {
                  new_files << " lastKnownFileType = sourcecode.cpp.cpp;";
                }
                std::string rel_path = RelativePathFromTo(
                  g_project_directory.c_str(),
                  (g_engine + "/" + entry.title).c_str());
                ReplaceAll("\\", "/", &rel_path);
                new_files << " name = " << entry.title << ";"
                  << " path = " << rel_path << ";";
                new_files << " sourceTree = SOURCE_ROOT;";
                new_files << " };\n";
                
                // add to PBXGroup engine children
                new_engine_children << "\n\t\t\t\t" << uid_file <<
                  " /* " << entry.title << " */,";
                
                // add to PBXSourcesBuildPhase
                if (EndsWith(entry.title, std::string(".cpp"))) {
                  new_buildphase << "\n\t\t\t\t" << uid_buildfile
                    << " /* " << entry.title << " in Sources */,";
                }
              }  // if ... uids are unique
            }  // while (!is_inserted)
          }  // if .. entry is a file AND is missing from references
        }  // for ... entries
        
        // save the resulting file
        std::stringstream resulting_file;
        std::size_t cursor = 0;
        std::size_t next_item =
          full_content.find("/* End PBXBuildFile section */");
        if (next_item == std::string::npos) {
          g_progress.append(
            u8"No PBXBuildFile section in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_buildfiles.str();
        
        cursor = next_item;
        next_item = full_content.find("/* End PBXFileReference section */");
        if (next_item == std::string::npos) {
          g_progress.append(
            u8"No PBXFileReference section in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        if (next_item < cursor) {
          g_progress.append(
            u8"Out of order PBXFileReference in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_files.str();
        
        std::string engine_group_entry("/* engine.cpp */,");
        cursor = next_item;
        next_item = full_content.find(engine_group_entry);
        if (next_item == std::string::npos) {
          g_progress.append(
            u8"No engine_group_entry in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        if (next_item < cursor) {
          g_progress.append(
            u8"Out of order engine_group_entry in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        next_item += engine_group_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_engine_children.str();
        
        
        std::string buildphase_entry("/* engine.cpp in Sources */,");
        cursor = next_item;
        next_item = full_content.find(buildphase_entry);
        if (next_item == std::string::npos) {
          g_progress.append(
            u8"No buildphase_entry in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        if (next_item < cursor) {
          g_progress.append(
            u8"Out of order buildphase_entry in xcode project!\nERROR.\n");
          step = 100500;
          break;
        }
        next_item += buildphase_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_buildphase.str();
        
        cursor = next_item;
        resulting_file << full_content.substr(cursor, std::string::npos);
        
        WriteFile(xcode_project_full_name.c_str(),
          reinterpret_cast<const Ui8 *>(resulting_file.str().c_str()),
          resulting_file.str().size());
        g_progress.append(u8"XCode project updated OK\n");
      }
        break;
      case 5: {
        // update vcxproj
        
        // parse xcodeproj extracting the list of engine files
        std::unordered_set<std::string> existing_files;
        std::string name = "template_project_name.vcxproj";
        ReplaceAll("template_project_name", g_project_name, &name);
        std::string vs_project_full_name = g_project_directory + "/" + name;
        std::vector<Ui8> data = ReadFile(vs_project_full_name.c_str());
        data.push_back(0);
        std::string full_content(reinterpret_cast<char*>(data.data()));
        
        {
          std::regex include_path_regex("<ClInclude Include=\"(.*)\" />");
          std::sregex_iterator next(full_content.begin(),
            full_content.end(), include_path_regex);
          std::sregex_iterator end;
          while (next != end) {
            std::string path = next->str(1).c_str();
            ReplaceAll("\\", "/", &path);
            std::string full_path =
              CanonicalizePath((g_project_directory + "/" + path).c_str());
            std::string rel_path = RelativePathFromTo(
              (g_engine + "/").c_str(), full_path.c_str());
            if (rel_path.size() && rel_path[0] != '.') {
              existing_files.insert(rel_path);
            }
            next++;
          }
        }
        {
          std::regex compile_path_regex("<ClCompile Include=\"(.*)\" />");
          std::sregex_iterator next(full_content.begin(),
            full_content.end(), compile_path_regex);
          std::sregex_iterator end;
          while (next != end) {
            std::string path = next->str(1).c_str();
            ReplaceAll("\\", "/", &path);
            std::string full_path =
            CanonicalizePath((g_project_directory + "/" + path).c_str());
            std::string rel_path = RelativePathFromTo(
              (g_engine + "/").c_str(), full_path.c_str());
            if (rel_path.size() && rel_path[0] != '.') {
              existing_files.insert(rel_path);
            }
            next++;
          }
        }
        // find missing engine files
        std::stringstream new_h;
        std::stringstream new_cpp;
        for (Ui32 idx = 0; idx < engine_entries.size(); ++idx) {
          auto &entry = engine_entries[idx];
          if (entry.is_file == kTrivalentTrue
              && existing_files.find(entry.title) == existing_files.end()) {
            std::string rel_path = RelativePathFromTo(
              (g_project_directory).c_str(),
              (g_engine + "/" + entry.title).c_str());
            ReplaceAll("/", "\\", &rel_path);
            if (EndsWith(entry.title, std::string(".cpp"))) {
              new_cpp << "\n    <ClCompile Include=\"" << rel_path << "\" />";
            } else {
              new_h << "\n    <ClInclude Include=\"" << rel_path << "\" />";
            }
          }  // if .. entry is a file AND is missing from references
        }  // for ... entries
        
        
        // save the resulting file
        std::stringstream resulting_file;
        std::size_t cursor = 0;
        std::string engine_h_pattern =
          "<ClInclude Include=\"..\\engine\\engine.h\" />";
        std::size_t next_item =
          full_content.find(engine_h_pattern);
        if (next_item == std::string::npos) {
          g_progress.append(u8"No engine.h in VS project!\nERROR.\n");
          step = 100500;
          break;
        }
        next_item += engine_h_pattern.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_h.str();
        
        cursor = next_item;
        std::string engine_cpp_pattern =
          "<ClCompile Include=\"..\\engine\\engine.cpp\" />";
        next_item = full_content.find(engine_cpp_pattern);
        if (next_item == std::string::npos) {
          g_progress.append(u8"No engine.cpp in VS project!\nERROR.\n");
          step = 100500;
          break;
        }
        if (next_item < cursor) {
          g_progress.append(
            u8"Out of order engine.cpp in VS project!\nERROR.\n");
          step = 100500;
          break;
        }
        next_item += engine_cpp_pattern.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_cpp.str();
        
        cursor = next_item;
        resulting_file << full_content.substr(cursor, std::string::npos);

        WriteFile(vs_project_full_name.c_str(),
          reinterpret_cast<const Ui8 *>(resulting_file.str().c_str()),
          resulting_file.str().size());

        g_progress.append(u8"Visual Studio project updated OK\n");
        g_progress.append(u8"All Done\n");
      }
        break;
      default:
        break;
    }
    step++;
    
    UpdateResolution();
    Clear();
    const char *welcome = u8"The Snow Wizard\n\n"
    "Creating project \"%s\"\n\n"
    "Current directory: %s\n"
    "%s\n\n"
    "Press ESC to leave the Snow Wizard";
    
    snprintf(text, sizeof(text), welcome,
             g_project_name.c_str(), g_current_directory.c_str(),
             g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
  }
  return false;
}

void EasyMain() {
  g_font.Load("data/arctic_one_bmf.fnt");
  if (!GetCurrentPath(&g_current_directory)) {
    //
  }

  if (!GetOperationMode()) {
    return;
  }
  if (g_mode_of_operation == kModeCreate) {
    if (!GetProjectName()) {
      return;
    }
    if (!ShowProgress()) {
      return;
    }
  } else if (g_mode_of_operation == kModeUpdate) {
    if (!SelectProject()) {
      return;
    }
  }
  if (!ShowUpdateProgress()) {
    return;
  }
}
