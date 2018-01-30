// The MIT License(MIT)
//
// Copyright 2016-2018 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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
#ifdef ARCTIC_PLATFORM_WINDOWS
#include <windows.h>
#elif ((defined ARCTIC_PLATFORM_PI) || (defined ARCTIC_PLATFORM_MACOSX))
#include <unistd.h>
#else
#endif  // ARCTIC_PLATFORM_WINDOWS

#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <string>
#include <vector>

#include "engine/easy.h"


using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

Font g_font;
std::string g_project_name;  // NOLINT
std::string g_progress;  // NOLINT
std::string g_path;  // NOLINT
std::string g_current_directory;  // NOLINT
std::string g_template;  // NOLINT

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

void UpdateResolution() {
  Vec2Si32 size = WindowSize();
  while (size.x > 1600 && size.y > 1000) {
    size = size / 2;
  }
  if (ScreenSize() != size) {
    ResizeScreen(size);
  }
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

int DoesDirectoryExist(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return 0;
  } else if ( info.st_mode & S_IFDIR ) {
    return 1;
  } else {
    return -1;
  }
}

bool MakeDirectory(const char *path) {
#ifdef ARCTIC_PLATFORM_WINDOWS
  BOOL is_ok = CreateDirectory(path, NULL);
  return is_ok;
#elif ((defined ARCTIC_PLATFORM_PI) || (defined ARCTIC_PLATFORM_MACOSX))
  Si32 result = mkdir(path,
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
  return (result == 0);
#else
  // Not implemented for this platform
  return false;
#endif
}

bool CurrentDir(std::string *out_dir) {
#ifdef ARCTIC_PLATFORM_WINDOWS
  char cwd[1 << 20];
  DWORD res = GetCurrentDirectoryA(sizeof(cwd), cwd);
  if (res > 0) {
    out_dir->assign(cwd);
    return true;
  }
  return false;
#elif ((defined ARCTIC_PLATFORM_PI) || (defined ARCTIC_PLATFORM_MACOSX))
  char cwd[1 << 20];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    out_dir->assign(cwd);
    return true;
  }
  return false;
#else
  // Not implemented for this platform
  return false;
#endif  // ARCTIC_PLATFORM_WINDOWS
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
  if (!CurrentDir(&g_current_directory)) {
    //
  }

  if (!GetProjectName()) {
    return;
  }
  if (!ShowProgress()) {
    return;
  }
}
