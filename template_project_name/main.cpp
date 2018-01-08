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

// fjortris.cpp : Defines the entry point for the application.
#include <sys/types.h>
#include <sys/stat.h>

#include "engine/easy.h"

using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

Font g_font;
std::string g_project_name;
std::string g_progress;

/*void ReplaceAll(std::string &str, const std::string &from,
    const std::string &to) {
  if (from.empty()) {
    return;
  }
  size_t start_pos = 0;
  while (true) {
    start_pos = str.find(from, start_pos));
    if (start_pos == std::string::npos) {
      return;
    }
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}*/

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

bool IsProjectNameOk() {
  struct stat info;
  if (stat(g_project_name.c_str(), &info) != 0) {
    return true;
  } else if( info.st_mode & S_IFDIR ) {
    return false;
  } else {
    return false;
  }
}

bool ShowProgress() {
  Si32 step = 0;
  char text[1 << 20];
  while (!IsKeyDownward(kKeyEscape)) {
    switch (step) {
      case 1:
        if (IsProjectNameOk()) {
          g_progress.append(u8"Directory name is OK\n");
        } else {
          g_progress.append(u8"A directory named \"");
          g_progress.append(g_project_name);
          g_progress.append(u8"\" already exists. Use another name.\n");
          step = 100500;
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
    "%s\n\n"
    "Press ESC to leave the Snow Wizard";
    
    snprintf(text, sizeof(text), welcome,
        g_project_name.c_str(), g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop);
    ShowFrame();
  }
  return false;
}

void EasyMain() {
  g_font.Load("data/arctic_one_bmf.fnt");
  if (!GetProjectName()) {
    return;
  }
  if (!ShowProgress()) {
    return;
  }
}
