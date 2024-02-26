// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2016 - 2022 Huldra
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

#include <algorithm>
#include <cstring>
#include <deque>
#include <memory>
#include <regex>  // NOLINT
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <iostream>

#include "engine/easy.h"
#include "engine/decorated_frame.h"

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif


using namespace arctic;  // NOLINT

enum FileToAddLocation {
  kFileToAddEngine = 0,
  kFileToAddProject = 1,
};

struct FileToAdd {
  std::string title;
  FileToAddLocation location;
};

Font g_font;
DecoratedFrame g_border;
DecoratedFrame g_button_normal;
DecoratedFrame g_button_hover;
DecoratedFrame g_button_down;
std::vector<Rgba> g_text_palete;


Sound g_sound_chime;
Sound g_sound_click;
Sound g_sound_error;
Sound g_sound_jingle;



std::string g_project_name;  // NOLINT
std::string g_progress;  // NOLINT
std::string g_path;  // NOLINT
std::string g_current_directory;  // NOLINT
std::string g_template;  // NOLINT
std::string g_project_directory;  // NOLINT
std::string g_engine;  // NOLINT
std::vector<std::string> g_deprecated_files;  // NOLINT

std::vector<Rgba> g_palete;

enum ProjectKind {
  kProjectKindTetramino = 0,
  kProjectKindHello = 1,
  kProjectKindSnake = 2,
  kProjectKindCodingForKids = 3,
  kProjectKindConquest = 4
};

enum MainMode {
  kModeCreate = 0,
  kModeUpdate = 1
};

ProjectKind g_project_kind = kProjectKindTetramino;
MainMode g_mode_of_operation = kModeCreate;
bool g_pause_when_done = true;



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
    out_suffix->clear();
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

void AppendDeprecated(std::unordered_set<std::string> *in_out_data) {
  Check(in_out_data, "Unexpected in_out_data = nullptr");
  for (size_t i = 0; i < g_deprecated_files.size(); ++i) {
    std::string path = g_deprecated_files[i];
    ReplaceAll("\\", "/", &path);
    std::string full_path =
      CanonicalizePath((g_engine + "/" + path).c_str());
    std::string rel_path = RelativePathFromTo(
      (g_engine + "/").c_str(), full_path.c_str());
    if (rel_path.size() && rel_path[0] != '.') {
      in_out_data->insert(rel_path);
    }
  }
}

void UpdateResolution() {
  Vec2Si32 size = WindowSize();
  while (size.x >= 1280*2 && size.y >= 800*2) {
    size = size / 2;
  }
  if (ScreenSize() != size) {
    ResizeScreen(size);
  }
}

Ui64 ShowModalDialogue(std::shared_ptr<Panel> gui) {
  Ui64 clicked_button = Ui64(-1);
  while (true) {
    UpdateResolution();
    Clear();
    gui->SetPos((ScreenSize() - gui->GetSize()) / 2);
    gui->Draw(Vec2Si32(0, 0));
    ShowFrame();
    if (clicked_button != Ui64(-1)) {
      return clicked_button;
    }
    std::deque<GuiMessage> messages;
    for (Si32 idx = 0; idx < InputMessageCount(); ++idx) {
      gui->ApplyInput(GetInputMessage(idx), &messages);
    }
    for (auto it = messages.begin(); it != messages.end(); ++it) {
      if (it->kind == kGuiButtonClick) {
        clicked_button = it->panel->GetTag();
      }
    }
  }
}

std::shared_ptr<Button> MakeButton(Ui64 tag, Vec2Si32 pos,
    KeyCode hotkey, Ui32 tab_order, std::string text,
    Vec2Si32 size = Vec2Si32(0, 0),
    std::shared_ptr<Text> *out_text = nullptr) {
  Vec2Si32 button_text_size = g_font.EvaluateSize(text.c_str(), false);
  Vec2Si32 button_size = button_text_size + Vec2Si32(13 * 2 + 4, 4);
  if (size != Vec2Si32(0, 0)) {
    button_size = size;
  }
  Sprite button_normal = g_button_normal.DrawExternalSize(button_size);
  Sprite button_hover = g_button_hover.DrawExternalSize(button_size);
  Sprite button_down = g_button_down.DrawExternalSize(button_size);


  Sound silent;
  std::shared_ptr<Button> button(new Button(tag, pos,
    button_normal, button_down, button_hover,
    silent, g_sound_click, hotkey, tab_order));
  std::shared_ptr<Text> button_textbox(new Text(
    0, Vec2Si32(2, 8), Vec2Si32(button_size.x - 4, button_text_size.y),
    0, g_font, kTextOriginBottom, g_palete, text, kAlignCenter));
  button->AddChild(button_textbox);
  if (out_text) {
    *out_text = button_textbox;
  }
  return button;
}

bool GetOperationMode() {
  UpdateResolution();

  std::shared_ptr<Panel> box(new Panel(0, Vec2Si32(0, 0),
    Vec2Si32(640, 480), 0, g_border.DrawExternalSize(Vec2Si32(640, 480))));
  const Ui64 kCreateButton = 1;
  const Ui64 kUpdateButton = 2;
  const Ui64 kExitButton = 100;

  const char *welcome = (const char*)u8"The Snow Wizard\n\n"
  "This wizard can create a new Arctic Engine project\n"
    "for you or update an existing one.";

  Si32 y = box->GetSize().y-32;

  std::shared_ptr<Text> textbox(new Text(
    0, Vec2Si32(24, y), Vec2Si32(box->GetSize().x, 0),
    0, g_font, kTextOriginTop, g_palete, welcome, kAlignLeft));
  box->AddChild(textbox);
  y = 8 + 16 + 64 + 64;
  std::shared_ptr<Button> create_button = MakeButton(
    kCreateButton, Vec2Si32(32, y), kKeyC,
    1, "\001C\002reate a new project", Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(create_button);
  y -= 64;
  std::shared_ptr<Button> update_button = MakeButton(
    kUpdateButton, Vec2Si32(32, y), kKeyU,
    2, "\001U\002pdate an existing project",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(update_button);
  y -= 64;
  std::shared_ptr<Button> exit_button = MakeButton(
    kExitButton, Vec2Si32(32, y), kKeyE,
    100, "\001E\002xit",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(exit_button);

  Ui64 action = ShowModalDialogue(box);
  if (action == kCreateButton) {
    g_mode_of_operation = kModeCreate;
    return true;
  } else if (action == kUpdateButton) {
    g_mode_of_operation = kModeUpdate;
    return true;
  }
  return false;
}

bool GetProjectKind() {
  UpdateResolution();

  std::shared_ptr<Panel> box(new Panel(0, Vec2Si32(0, 0),
    Vec2Si32(640, 480+64), 0, g_border.DrawExternalSize(Vec2Si32(640, 480+64))));
  const Ui64 kTetraminoButton = 1;
  const Ui64 kHelloButton = 2;
  const Ui64 kSnakeButton = 3;
  const Ui64 kCodingForKidsButton = 4;
  const Ui64 kConquestButton = 5;
  const Ui64 kExitButton = 100;

  const char *welcome = (const char *)u8"The Snow Wizard\n\n"
  "Please select the flavour of the new project.";

  Si32 y = box->GetSize().y-32;

  std::shared_ptr<Text> textbox(new Text(
    0, Vec2Si32(24, y), Vec2Si32(box->GetSize().x, 0),
    0, g_font, kTextOriginTop, g_palete, welcome, kAlignLeft));
  box->AddChild(textbox);
  y = 8 + 16 + 64 + 64 + 64 + 64 + 64;
  std::shared_ptr<Button> tetramino_button = MakeButton(
    kTetraminoButton, Vec2Si32(32, y), kKeyT,
    1, "\001T\002etramino game project", Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(tetramino_button);
  y -= 64;
  std::shared_ptr<Button> hello_button = MakeButton(
    kHelloButton, Vec2Si32(32, y), kKeyH,
    2, "\001H\002ello World project",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(hello_button);
  y -= 64;
  std::shared_ptr<Button> snake_button = MakeButton(
      kSnakeButton, Vec2Si32(32, y), kKeyS,
      3, "\001S\002nake project",
      Vec2Si32(box->GetSize().x - 64, 48));
    box->AddChild(snake_button);
  y -= 64;
  std::shared_ptr<Button> coding_for_kids_button = MakeButton(
      kCodingForKidsButton, Vec2Si32(32, y), kKeyC,
      4, "\001C\002oding For Kids project",
      Vec2Si32(box->GetSize().x - 64, 48));
    box->AddChild(coding_for_kids_button);
  y -= 64;
  std::shared_ptr<Button> conquest_button = MakeButton(
      kConquestButton, Vec2Si32(32, y), kKeyT,
      4, "\001T\002urn-based strategy project",
      Vec2Si32(box->GetSize().x - 64, 48));
    box->AddChild(conquest_button);
  y -= 64;
  std::shared_ptr<Button> exit_button = MakeButton(
    kExitButton, Vec2Si32(32, y), kKeyE,
    100, "\001E\002xit",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(exit_button);


  Ui64 action = ShowModalDialogue(box);
  if (action == kTetraminoButton) {
    g_project_kind = kProjectKindTetramino;
    return true;
  } else if (action == kHelloButton) {
    g_project_kind = kProjectKindHello;
    return true;
  } else if (action == kSnakeButton) {
    g_project_kind = kProjectKindSnake;
    return true;
  } else if (action == kCodingForKidsButton) {
    g_project_kind = kProjectKindCodingForKids;
    return true;
  } else if (action == kConquestButton) {
    g_project_kind = kProjectKindConquest;
    return true;
  }
  return false;
}

bool GetProjectName() {
  UpdateResolution();

  std::shared_ptr<Panel> box(new Panel(0, Vec2Si32(0, 0),
    Vec2Si32(640, 480), 0, g_border.DrawExternalSize(Vec2Si32(640, 480))));
  const Ui64 kEditBox = 1;
  const Ui64 kDoneButton = 2;
  const Ui64 kExitButton = 100;

  const char *welcome = (const char *)u8"The Snow Wizard\n\n"
  "Name the project. You may use only latin\n"
  "letters, numbers and underscores.\n\n"
  "Project name:";

  Si32 y = box->GetSize().y-32;

  std::shared_ptr<Text> textbox(new Text(
    0, Vec2Si32(24, y), Vec2Si32(box->GetSize().x, 0),
    0, g_font, kTextOriginTop, g_palete, welcome, kAlignLeft));
  box->AddChild(textbox);
  y = 8 + 16 + 64 + 64 + 48;

  Vec2Si32 item_size(box->GetSize().x - 64, 48);
  Sprite button_normal = g_button_normal.DrawExternalSize(item_size);
  Sprite button_hover = g_button_hover.DrawExternalSize(item_size);
  std::unordered_set<Ui32> allow_list;
  for (char ch = '0'; ch <= '9'; ++ch) {
    allow_list.insert(ch);
  }
  for (char ch = 'a'; ch <= 'z'; ++ch) {
    allow_list.insert(ch);
  }
  allow_list.insert('_');
  std::shared_ptr<Editbox> editbox(new Editbox(
    kEditBox,
    Vec2Si32(32, y),
    1,
    button_normal, 
    button_hover,
    g_font,
    kTextOriginBottom,
    Rgba(255, 255, 255, 255),
    "",
    kAlignLeft,
    false,
    allow_list));
  box->AddChild(editbox);

  y = 8 + 16 + 64 + 64;
  y -= 64;
  std::shared_ptr<Button> done_button = MakeButton(
    kDoneButton, Vec2Si32(32, y), kKeyNone,
    2, "Done",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(done_button);
  box->SwitchCurrentTab(true);

  y -= 64;
  std::shared_ptr<Button> exit_button = MakeButton(
    kExitButton, Vec2Si32(32, y), kKeyNone,
    100, "Exit",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(exit_button);


  Ui64 action = ShowModalDialogue(box);
  if (action == kDoneButton) {
    g_project_name = editbox->GetText();
    return true;
  }
  return false;
}

std::deque<std::string> GetDirectoryProjects(std::string project_directory) {
  std::deque<std::string> candidates;
  // List selected folder content
  std::vector<DirectoryEntry> entries;
  GetDirectoryEntries(project_directory.c_str(), &entries);

  // find *.xcodeproj folder and *.vcxproj files
  std::string xcode_ending = ".xcodeproj";
  std::string vcx_ending = ".vcxproj";
  for (size_t idx = 0; idx < entries.size(); ++idx) {
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
            candidates.push_back(project_name);
          }
        }
      }
    }
  }
  return candidates;
}

void GatherEntries(std::vector<DirectoryEntry> *in_out_entries) {
  arctic::GetDirectoryEntries(g_project_directory.c_str(), in_out_entries);
  if ((*in_out_entries)[0].title == ".") {
    in_out_entries->erase(in_out_entries->begin());
  }
  std::sort(in_out_entries->begin(), in_out_entries->end(),
        [](const DirectoryEntry &a, const DirectoryEntry &b) -> bool {
          return strcasecmp(a.title.c_str(), b.title.c_str()) < 0;
        });
}

bool SelectProject() {
  std::vector<DirectoryEntry> entries;
  Ui32 selected_idx = 0;
  g_project_directory = g_current_directory;
  GatherEntries(&entries);
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
          GatherEntries(&entries);
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


    bool is_project_dir =
      (GetDirectoryProjects(g_project_directory).size() == 1);
    std::stringstream str;
    str << "The Snow Wizard\n\n"
    "Select an existing Arctic Engine project to update.\n\n"
    "Press arrow keys and ENTER to navigate.\n"
    "Press " << (is_project_dir ? "\001S\002" : "S")
      << " while in a project directory to select it.\n"
    "Press ESC to leave the Snow Wizard.\n\n";
    str << "Path: " << g_project_directory << "\n\n";

    Si32 begin_i = std::max(0, Si32(selected_idx) - 5);
    for (Ui32 i = (Ui32)begin_i; i < entries.size(); ++i) {
      if (selected_idx == i) {
        str << "--> ";
      } else {
        str << "    ";
      }
      if (entries[i].is_directory == kTrivalentTrue) {
        str << "\x01";
      }
      str << entries[i].title;
      str << "\x02";
      str << "\n";
    }

    g_font.Draw(str.str().c_str(), 32, ScreenSize().y - 32, kTextOriginTop,
      kDrawBlendingModeColorize, kFilterNearest, g_palete);
    ShowFrame();
    if (is_done) {
      return true;
    }
  }
  return false;
}

void PatchAndCopyTemplateFile(std::string file_name, std::string target_name) {
  std::vector<Ui8> data = ReadFile(
      (g_template + "/" + file_name).c_str());
  std::string name = target_name;
  ReplaceAll("template_project_name", g_project_name, &name);
  data.push_back('\0');
  std::string content = reinterpret_cast<char*>(data.data());
  ReplaceAll("template_project_name", g_project_name, &content);
  WriteFile((g_project_directory + "/" + name).c_str(),
      reinterpret_cast<const Ui8*>(content.data()), content.size());
}

void PatchAndCopyTemplateFile(std::string file_name) {
  PatchAndCopyTemplateFile(file_name, file_name);
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
        const char* expected = "arctic.engine";
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
        g_path.append("/..");
        g_path.append("/");
        g_path.append(g_project_name);
        //g_progress.append((const char *)u8"Arctic Engine is detected.\n");
      }
      else {
        g_progress.append((const char*)u8"\003Can't detect Arctic Engine. ERROR.\n");
        step = 100500;
        g_sound_error.Play();
      }
    }
    break;
    case 2:
      if (DoesDirectoryExist(g_path.c_str()) == 0) {
        //g_progress.append((const char *)u8"Directory name is OK\n");
      }
      else {
        g_progress.append((const char*)u8"\003A directory named \"");
        g_progress.append(g_path);
        g_progress.append((const char*)u8"\" already exists. ERROR. Use another name.\n");
        step = 100500;
        g_sound_error.Play();
      }
      break;
    case 3:
      if (MakeDirectory(g_path.c_str())) {
        // g_progress.append((const char *)u8"Directory is created OK\n");
        g_project_directory = g_path;
      }
      else {
        g_progress.append((const char*)u8"\003Can't create directory \"");
        g_progress.append(g_path);
        g_progress.append((const char*)u8"\". ERROR.\n");
        step = 100500;
        g_sound_error.Play();
      }
      break;
    case 4:
      g_template = g_current_directory + "/template_project_name";
      if (DoesDirectoryExist(g_template.c_str()) == 1) {
        // g_progress.append((const char *)u8"Template found OK\n");
      }
      else {
        g_progress.append((const char*)u8"\003Can't find template directory \"");
        g_progress.append(g_template);
        g_progress.append((const char*)u8"\". ERROR.\n");
        step = 100500;
        g_sound_error.Play();
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
      for (size_t idx = 0; idx < files.size(); ++idx) {
        std::string name = files[idx];
        ReplaceAll("template_project_name", g_project_name, &name);
        bool is_ok = MakeDirectory((g_project_directory + "/" + name).c_str());

        if (is_ok) {
          //g_progress.append((const char*)u8"Project structure created OK\n");
        } else {
          g_progress.append((const char*)u8"\003Can't create directory \"");
          g_progress.append((g_project_directory + "/" + name).c_str());
          g_progress.append((const char*)u8"\". ERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
      }
    }
        break;
      case 6:
      {
        std::vector<std::string> files = {
          "app_icon.ico"
          , "data/arctic_one_bmf_0.tga"
          , "data/arctic_one_bmf.fnt"
        };
        if (g_project_kind == kProjectKindConquest) {
          files.push_back("data/blue_bow.tga");
          files.push_back("data/blue_city.tga");
          files.push_back("data/blue_sword.tga");
          files.push_back("data/frame.tga");
          files.push_back("data/grass.tga");
          files.push_back("data/grey_city.tga");
          files.push_back("data/hills.tga");
          files.push_back("data/red_bow.tga");
          files.push_back("data/red_city.tga");
          files.push_back("data/red_sword.tga");
          files.push_back("data/water.tga");
        } else if (g_project_kind == kProjectKindCodingForKids) {
          files.push_back("data/font_8x8.tga");
          files.push_back("data/font_8x8.txt");
        } else if (g_project_kind == kProjectKindTetramino) {
          files.push_back("data/block_1.tga");
          files.push_back("data/block_2.tga");
        }
        for (size_t idx = 0; idx < files.size(); ++idx) {
          auto data = ReadFile((g_template + "/" + files[idx]).c_str());
          std::string name = files[idx];
          ReplaceAll("template_project_name", g_project_name, &name);
          WriteFile((g_project_directory + "/" + name).c_str(),
              data.data(), data.size());
        }
        //g_progress.append((const char *)u8"Data files copied OK\n");
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
        for (size_t idx = 0; idx < files.size(); ++idx) {
          PatchAndCopyTemplateFile(files[idx]);
        }

        switch (g_project_kind) {
          case kProjectKindTetramino:
            PatchAndCopyTemplateFile("main.cpp", "main.cpp");
            break;
          case kProjectKindSnake:
            PatchAndCopyTemplateFile("main_snake.cpp", "main.cpp");
            break;
          case kProjectKindCodingForKids:
            PatchAndCopyTemplateFile("main_coding_for_kids.cpp", "main.cpp");
            PatchAndCopyTemplateFile("code.inc.h", "code.inc.h");
            break;
          case kProjectKindConquest:
            PatchAndCopyTemplateFile("main_conquest.cpp", "main.cpp");
            break;
          default:
          case kProjectKindHello:
            PatchAndCopyTemplateFile("main_hello.cpp", "main.cpp");
            break;
        }
        // g_progress.append((const char *)u8"Project created OK\n");

        if (!g_pause_when_done) {
          return false;
        }
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
    const char *welcome = (const char *)u8"The Snow Wizard\n\n"
    "Creating project \"%s\"\n\n"
    "Current directory: %s\n"
    "%s\n\n"
    "Press ESC to leave the Snow Wizard";

    snprintf(text, sizeof(text), welcome,
        g_project_name.c_str(), g_current_directory.c_str(),
        g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32,  kTextOriginTop,
                kDrawBlendingModeColorize, kFilterNearest, g_palete);
    ShowFrame();
  }
  return false;
}

bool ShowUpdateProgress() {
  std::vector<DirectoryEntry> engine_entries;
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
          // g_progress.append((const char *)u8"Arctic Engine is detected.\n");
        } else {
          g_progress.append((const char *)u8"\003Can't detect Arctic Engine. ERROR.\n");
          step = 100500;
          g_sound_error.Play();
        }
      }
        break;
      case 2: {
        // Find /engine directory
        g_engine = CanonicalizePath((g_current_directory + "/engine").c_str());
        if (DoesDirectoryExist(g_engine.c_str()) == 1) {
          // List /engine files
          GetDirectoryEntries(g_engine.c_str(), &engine_entries);
          g_progress.append((const char *)u8"Enigne found OK: \"");
          g_progress.append(g_engine);
          g_progress.append((const char *)u8"\"\n");
        } else {
          g_progress.append((const char *)u8"\003Can't find engine directory \"");
          g_progress.append(g_engine);
          g_progress.append((const char *)u8"\". ERROR.\n");
          step = 100500;
          g_sound_error.Play();
        }
      }
        break;
      case 3: {
        // find *.xcodeproj folder and *.vcxproj files, make sure that
        // names match and there are no other xcodeproj and vcxproj pairs
        std::deque<std::string> candidates =
          GetDirectoryProjects(g_project_directory);
        Si32 candidate_count = (Si32)candidates.size();
        for (Si32 i = 0; i < candidate_count; ++i) {
          g_progress.append((const char *)u8"Project name candidate \"");
          g_progress.append(candidates[static_cast<size_t>(i)]);
          g_progress.append((const char *)u8"\"\n");
        }

        // make sure that there are no other xcodeproj and vcxproj pairs
        if (candidate_count == 0) {
          g_progress.append((const char *)u8"\003Can't find project files in\"");
          g_progress.append(g_project_directory);
          g_progress.append((const char *)u8"\". ERROR.\n");
          step = 100500;
          g_sound_error.Play();
        } else if (candidate_count > 1) {
          g_progress.append((const char *)u8"\003Multiple project files in\"");
          g_progress.append(g_project_directory);
          g_progress.append((const char *)u8"\". ERROR.\n");
          step = 100500;
          g_sound_error.Play();
        } else {
          g_project_name = candidates[0];
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
        AppendDeprecated(&existing_files);
        // find missing engine files
        std::stringstream new_buildfiles;  // PBXBuildFile
        std::stringstream new_files;  // PBXFileReference
        std::stringstream new_engine_children;  // PBXGroup engine children
        std::stringstream new_project_children;  // PBXGroup project children
        std::stringstream new_buildphase;  // PBXSourcesBuildPhase
        std::unordered_set<std::string> new_hashes;


        std::deque<FileToAdd> files_to_add;
        // patch-in code.inc.h if needed
        if (g_project_kind == kProjectKindCodingForKids) {
          //engine_entries.emplace_back(
          if (existing_files.find("code.inc.h") == existing_files.end()) {
            files_to_add.push_back({"code.inc.h", kFileToAddProject});
          }
        }

        for (Ui32 idx = 0; idx < engine_entries.size(); ++idx) {
          auto &entry = engine_entries[idx];
          if (entry.is_file == kTrivalentTrue
                && existing_files.find(entry.title) == existing_files.end()) {
            files_to_add.push_back({entry.title, kFileToAddEngine});
          }
        }


        for (Ui32 idx = 0; idx < files_to_add.size(); ++idx) {
          auto entry = files_to_add[idx];

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
              if (EndsWith(entry.title, std::string(".cpp")) ||
                  EndsWith(entry.title, std::string(".c")) ||
                  EndsWith(entry.title, std::string(".mm"))) {
                new_buildfiles << "\t\t" << uid_buildfile
                  << " /* " << entry.title << " in Sources */ = {"
                  << "isa = PBXBuildFile; fileRef = " << uid_file
                  << " /* " << entry.title << " */; };\n";
              }

              // add to PBXFileReference
              new_files << "\t\t" << uid_file
                << " /* " << entry.title << " */ = {"
                << "isa = PBXFileReference; fileEncoding = 4;";
              if (EndsWith(entry.title, std::string(".h"))) {
                new_files << " lastKnownFileType = sourcecode.c.h;";
              } else if (EndsWith(entry.title, std::string(".mm"))) {
                new_files << " lastKnownFileType = sourcecode.cpp.objcpp;";
              } else {
                new_files << " lastKnownFileType = sourcecode.cpp.cpp;";
              }
              std::string rel_path;
              switch (entry.location) {
                case kFileToAddEngine:
                  rel_path = RelativePathFromTo(
                      g_project_directory.c_str(),
                      (g_engine + "/" + entry.title).c_str());
                  break;
                case kFileToAddProject:
                  rel_path = RelativePathFromTo(
                      g_project_directory.c_str(),
                      (g_project_directory + "/" + entry.title).c_str());
                  break;
              }

              ReplaceAll("\\", "/", &rel_path);
              new_files << " name = " << entry.title << ";"
                << " path = " << rel_path << ";";
              new_files << " sourceTree = SOURCE_ROOT;";
              new_files << " };\n";

              switch (entry.location) {
                case kFileToAddEngine:
                  // add to PBXGroup engine children
                  new_engine_children << "\n\t\t\t\t" << uid_file <<
                    " /* " << entry.title << " */,";
                  break;
                case kFileToAddProject:
                  // add to PBXGroup engine children
                  new_project_children << "\n\t\t\t\t" << uid_file <<
                    " /* " << entry.title << " */,";
                  break;
              }

              // add to PBXSourcesBuildPhase
              if (EndsWith(entry.title, std::string(".cpp")) ||
                  EndsWith(entry.title, std::string(".c")) ||
                  EndsWith(entry.title, std::string(".mm"))) {
                new_buildphase << "\n\t\t\t\t" << uid_buildfile
                  << " /* " << entry.title << " in Sources */,";
              }
            }  // if ... uids are unique
          }  // while (!is_inserted)
        }  // for ... entries

        // save the resulting file
        std::stringstream resulting_file;
        std::size_t cursor = 0;
        std::size_t next_item =
          full_content.find("/* End PBXBuildFile section */");
        if (next_item == std::string::npos) {
          g_progress.append((const char *)
            u8"\003No PBXBuildFile section in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_buildfiles.str();

        cursor = next_item;
        next_item = full_content.find("/* End PBXFileReference section */");
        if (next_item == std::string::npos) {
          g_progress.append((const char *)
            u8"\003No PBXFileReference section in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        if (next_item < cursor) {
          g_progress.append((const char *)
            u8"\003Out of order PBXFileReference in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_files.str();


        std::string main_group_entry("/* main.cpp */,");
        cursor = next_item;
        next_item = full_content.find(main_group_entry);
        if (next_item == std::string::npos) {
          g_progress.append((const char *)
            u8"\003No main_group_entry in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        if (next_item < cursor) {
          g_progress.append((const char *)
            u8"\003Out of order main_group_entry in xcode project!\n"
            u8"ERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        next_item += main_group_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_project_children.str();


        std::string engine_group_entry("/* engine.cpp */,");
        cursor = next_item;
        next_item = full_content.find(engine_group_entry);
        if (next_item == std::string::npos) {
          g_progress.append((const char *)
            u8"\003No engine_group_entry in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        if (next_item < cursor) {
          g_progress.append((const char *)
            u8"\003Out of order engine_group_entry in xcode project!\n"
            u8"ERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        next_item += engine_group_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_engine_children.str();


        std::string buildphase_entry("/* engine.cpp in Sources */,");
        cursor = next_item;
        next_item = full_content.find(buildphase_entry);
        if (next_item == std::string::npos) {
          g_progress.append((const char *)
            u8"\003No buildphase_entry in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
          break;
        }
        if (next_item < cursor) {
          g_progress.append((const char *)
            u8"\003Out of order buildphase_entry in xcode project!\nERROR.\n");
          step = 100500;
          g_sound_error.Play();
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
        // g_progress.append((const char *)u8"XCode project updated OK\n");
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

        std::string filter_name = "template_project_name.vcxproj.filters";
        ReplaceAll("template_project_name", g_project_name, &filter_name);
        std::string vs_filter_full_name =
          g_project_directory + "/" + filter_name;
        std::vector<Ui8> filter_data = ReadFile(vs_filter_full_name.c_str());
        filter_data.push_back(0);
        std::string full_filter_content(
          reinterpret_cast<char*>(filter_data.data()));

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
        AppendDeprecated(&existing_files);
        // find missing engine files
        std::stringstream new_h;
        std::stringstream new_cpp;
        std::stringstream new_filter_h;
        std::stringstream new_filter_cpp;
        for (Ui32 idx = 0; idx < engine_entries.size(); ++idx) {
          auto &entry = engine_entries[idx];
          if (entry.is_file != kTrivalentFalse
              && existing_files.find(entry.title) == existing_files.end()) {
            std::string rel_path = RelativePathFromTo(
              (g_project_directory).c_str(),
              (g_engine + "/" + entry.title).c_str());
            ReplaceAll("/", "\\", &rel_path);
            if (EndsWith(entry.title, std::string(".cpp")) ||
              EndsWith(entry.title, std::string(".c"))) {
              new_cpp << "\n    <ClCompile Include=\"" << rel_path << "\" />";
              new_filter_cpp << "\n      <Filter>engine</Filter>"
                << "\n    </ClCompile>"
                << "\n    <ClCompile Include=\"" << rel_path << "\">";
            } else if (EndsWith(entry.title, std::string(".h")) ||
                     EndsWith(entry.title, std::string(".inc")) ||
                     EndsWith(entry.title, std::string(".mm")) ||
                     EndsWith(entry.title, std::string(".hpp"))) {
              new_h << "\n    <ClInclude Include=\"" << rel_path << "\" />";
              new_filter_h << "\n      <Filter>engine</Filter>"
                << "\n    </ClInclude>"
                << "\n    <ClInclude Include=\"" << rel_path << "\">";
            }
          }  // if .. entry is a file AND is missing from references
        }  // for ... entries
        if (g_project_kind == kProjectKindCodingForKids) {
          //engine_entries.emplace_back(
          if (existing_files.find("code.inc.h") == existing_files.end()) {
            new_h << "\n    <ClInclude Include=\"code.inc.h\" />";
          }
        }

        std::string rel_engine_h_path = RelativePathFromTo(
          (g_project_directory).c_str(),
          (g_engine + "/engine.h").c_str());
        std::string rel_engine_cpp_path = RelativePathFromTo(
          (g_project_directory).c_str(),
          (g_engine + "/engine.cpp").c_str());

        // save the resulting file
        {
          std::string engine_h_pattern =
            "<ClInclude Include=\"" + rel_engine_h_path + "\" />";
          std::string engine_cpp_pattern =
            "<ClCompile Include=\"" + rel_engine_cpp_path + "\" />";

          std::stringstream resulting_file;
          std::size_t cursor = 0;
          std::size_t next_item =
            full_content.find(engine_h_pattern);


          if (next_item == std::string::npos) {
            ReplaceAll("/", "\\", &rel_engine_h_path);
            engine_h_pattern =
              "<ClInclude Include=\"" + rel_engine_h_path + "\" />";
            next_item = full_content.find(engine_h_pattern);
          }

          if (next_item == std::string::npos) {
            g_progress.append((const char *)u8"\003No engine.h in VS project!\nERROR.\n");
            step = 100500;
            g_sound_error.Play();
            break;
          }
          next_item += engine_h_pattern.size();
          resulting_file << full_content.substr(cursor, next_item - cursor);
          resulting_file << new_h.str();

          cursor = next_item;
          next_item = full_content.find(engine_cpp_pattern);
          if (next_item == std::string::npos) {
            ReplaceAll("/", "\\", &rel_engine_cpp_path);
            std::string engine_cpp_pattern =
              "<ClCompile Include=\"" + rel_engine_cpp_path + "\" />";
            next_item = full_content.find(engine_cpp_pattern);
          }

          if (next_item == std::string::npos) {
            g_progress.append((const char *)u8"\003No engine.cpp in VS project!\nERROR.\n");
            step = 100500;
            g_sound_error.Play();
            break;
          }
          if (next_item < cursor) {
            g_progress.append((const char *)
              u8"\003Out of order engine.cpp in VS project!\nERROR.\n");
            step = 100500;
            g_sound_error.Play();
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
        }

        // save the resulting filter file
        {
          std::string engine_h_pattern =
            "<ClInclude Include=\"" + rel_engine_h_path + "\">";
          std::string engine_cpp_pattern =
            "<ClCompile Include=\"" + rel_engine_cpp_path + "\">";

          std::stringstream resulting_file;
          std::size_t cursor = 0;
          std::size_t next_item =
            full_filter_content.find(engine_cpp_pattern);
          if (next_item == std::string::npos) {
            g_progress.append((const char *)u8"\003No engine.cpp in VS project filters!"
                u8"\nERROR.\n");
            step = 100500;
            g_sound_error.Play();
            break;
          }
          next_item += engine_cpp_pattern.size();
          resulting_file << full_filter_content.substr(cursor,
              next_item - cursor);
          resulting_file << new_filter_cpp.str();

          cursor = next_item;
          next_item = full_filter_content.find(engine_h_pattern);
          if (next_item == std::string::npos) {
            ReplaceAll("/", "\\", &engine_h_pattern);
            next_item = full_content.find(engine_h_pattern);
          }

          if (next_item == std::string::npos) {
            g_progress.append((const char *)u8"\003No engine.h in VS project filters!\n"
                u8"ERROR.\n");
            step = 100500;
            g_sound_error.Play();
            break;
          }
          if (next_item < cursor) {
            g_progress.append((const char *)
              u8"\003Out of order engine.h in VS project filters!\nERROR.\n");
            step = 100500;
            g_sound_error.Play();
            break;
          }
          next_item += engine_h_pattern.size();
          resulting_file << full_filter_content.substr(cursor,
              next_item - cursor);
          resulting_file << new_filter_h.str();

          cursor = next_item;
          resulting_file << full_filter_content.substr(cursor,
              std::string::npos);

          WriteFile(vs_filter_full_name.c_str(),
            reinterpret_cast<const Ui8 *>(resulting_file.str().c_str()),
            resulting_file.str().size());
        }
        // g_progress.append((const char *)u8"Visual Studio project updated OK\n");
      }
        break;
      case 6:
        g_template = g_current_directory + "/template_project_name";
        if (DoesDirectoryExist(g_template.c_str()) == 1) {
          // g_progress.append((const char *)u8"Template found OK\n");
        } else {
          g_progress.append((const char *)u8"\003Can't find template directory \"");
          g_progress.append(g_template);
          g_progress.append((const char *)u8"\". ERROR.\n");
          step = 100500;
          g_sound_error.Play();
        }
        break;
      case 7: {
          PatchAndCopyTemplateFile("CMakeLists.txt");
          // g_progress.append((const char *)u8"Latest CMakeLists.txt applied OK\n");
          g_progress.append((const char *)u8"\nAll Done\n");
          g_sound_jingle.Play();
          if (!g_pause_when_done) {
            return false;
          }
        }

        break;
      default:
        break;
    }
    step++;

    UpdateResolution();
    Clear();
    const char *welcome = (const char *)u8"The Snow Wizard\n\n"
    "Creating project \"%s\"\n"
    "Current directory: %s\n"
    "%s\n"
    "Press ESC to leave the Snow Wizard";

    snprintf(text, sizeof(text), welcome,
             g_project_name.c_str(), g_current_directory.c_str(),
             g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop,
                kDrawBlendingModeColorize, kFilterNearest, g_palete);
    ShowFrame();
  }
  return false;
}

void EasyMain() {
  g_sound_chime.Load("data/chime.wav");
  g_sound_chime.Play();

  g_sound_click.Load("data/click.wav");
  g_sound_error.Load("data/error.wav");
  g_sound_jingle.Load("data/jingle.wav");

  g_palete.emplace_back((Ui32)0xffffffff);
  g_palete.emplace_back((Ui32)0xffff9999);
  g_palete.emplace_back((Ui32)0xffffffff);
  g_palete.emplace_back((Ui32)0xff6666ff);
  g_font.Load("data/arctic_one_bmf.fnt");

  Sprite border;
  border.Load("data/border.tga");
  g_border.Split(border, 32, true, true);
  Sprite button_normal;
  button_normal.Load("data/button_normal.tga");
  g_button_normal.Split(button_normal, 12, true, true);
  Sprite button_hover;
  button_hover.Load("data/button_hover.tga");
  g_button_hover.Split(button_hover, 12, true, true);
  Sprite button_down;
  button_down.Load("data/button_down.tga");
  g_button_down.Split(button_down, 12, true, true);

  g_text_palete = {Rgba(255, 255, 255)};


  CsvTable csv;
  csv.LoadFile("data/deprecations.csv");
  size_t row_count = static_cast<size_t>(csv.RowCount());
  for (size_t i = 0; i < row_count; ++i) {
    g_deprecated_files.push_back(csv.GetRow(i)->GetValue(0, std::string("")));
  }

  for (Si64 i = 0; i < GetEngine()->GetArgc(); ++i) {
    Log("Argument: ", GetEngine()->GetArgv()[i]);
  }

  bool is_mode_of_operation_set = false;
  bool is_project_name_set = false;
  bool is_project_directory_set = false;
  if (GetEngine()->GetArgc() == 3) {
    if (GetEngine()->GetArgv()[1] == std::string("create")) {
      g_mode_of_operation = kModeCreate;
      is_mode_of_operation_set = true;

      if (strlen(GetEngine()->GetArgv()[2]) > 0) {
        g_project_name.assign(GetEngine()->GetArgv()[2]);
        // TODO(Huldra): validate g_project_name
        is_project_name_set = true;
        g_pause_when_done = false;
      }
    }
    if (GetEngine()->GetArgv()[1] == std::string("update")) {
      g_mode_of_operation = kModeUpdate;
      is_mode_of_operation_set = true;

      if (strlen(GetEngine()->GetArgv()[2]) > 0) {
        g_project_directory.assign(GetEngine()->GetArgv()[2]);
        // TODO(Huldra): validate g_project_directory
        is_project_directory_set = true;
        g_pause_when_done = false;
      }
    }
  }

  g_current_directory = GetEngine()->GetInitialPath();

  if (!is_mode_of_operation_set) {
    if (!GetOperationMode()) {
      return;
    }
  }
  if (g_mode_of_operation == kModeCreate) {
    if (!is_project_name_set) {
      if (!GetProjectKind()) {
        return;
      }
      if (!GetProjectName()) {
        return;
      }
    }
    if (!ShowProgress()) {
      return;
    }
  } else if (g_mode_of_operation == kModeUpdate) {
    if (!is_project_directory_set) {
      if (!SelectProject()) {
        return;
      }
    }
  }
  if (!ShowUpdateProgress()) {
    return;
  }
}
