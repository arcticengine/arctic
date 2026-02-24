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
DecoratedFrame g_button_disabled;
std::shared_ptr<GuiThemeScrollbar> g_v_scrollbar_theme;
std::vector<Rgba> g_text_palete;


Sound g_sound_chime;
Sound g_sound_button_down;
Sound g_sound_button_up;
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
  kProjectKindConquest = 4,
  kProjectKindDiscreteEventSimButton = 5
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
  Sprite button_disabled = g_button_disabled.DrawExternalSize(button_size);

  std::shared_ptr<Button> button(new Button(tag, pos,
    button_normal, button_down, button_hover,
    g_sound_button_down, g_sound_button_up, hotkey, tab_order,
    button_disabled));
  std::shared_ptr<Text> button_textbox(new Text(
    0, Vec2Si32(2, 8), Vec2Si32(button_size.x - 4, button_text_size.y),
    0, g_font, kTextOriginBottom, g_palete, text, kTextAlignmentCenter));
  button->AddChild(button_textbox);
  if (out_text) {
    *out_text = button_textbox;
  }
  return button;
}

std::string WordWrap(const std::string &text, Si32 max_width) {
  std::string result;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!result.empty()) {
      result += '\n';
    }
    if (line.empty()) {
      continue;
    }
    std::string current_line;
    std::istringstream words(line);
    std::string word;
    while (words >> word) {
      std::string candidate = current_line.empty()
        ? word : current_line + " " + word;
      Si32 width = g_font.EvaluateSize(candidate.c_str(), false).x;
      if (width > max_width && !current_line.empty()) {
        result += current_line + '\n';
        current_line = word;
      } else {
        current_line = candidate;
      }
    }
    result += current_line;
  }
  return result;
}

void ShowResultDialog(bool is_success, const std::string &message) {
  UpdateResolution();
  const char *title = is_success
    ? "Success"
    : "Error";

  Vec2Si32 box_size(640, 480);
  std::shared_ptr<Panel> box(new Panel(0, Vec2Si32(0, 0),
    box_size, 0, g_border.DrawExternalSize(box_size)));

  Si32 text_area_width = box_size.x - 48;
  std::string wrapped = WordWrap(message, text_area_width);
  std::string full_text = std::string("The Snow Wizard\n\n")
    + title + "\n\n" + wrapped;

  Si32 y = box->GetSize().y - 32;
  std::shared_ptr<Text> textbox(new Text(
    0, Vec2Si32(24, y), Vec2Si32(box->GetSize().x - 48, 0),
    0, g_font, kTextOriginTop, g_palete, full_text, kTextAlignmentLeft));
  box->AddChild(textbox);

  const Ui64 kOkButton = 1;
  std::shared_ptr<Button> ok_button = MakeButton(
    kOkButton, Vec2Si32(32, 16), kKeyEnter,
    1, "OK", Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(ok_button);

  if (is_success) {
    g_sound_jingle.Play();
  } else {
    g_sound_error.Play();
  }
  ShowModalDialogue(box);
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
    0, g_font, kTextOriginTop, g_palete, welcome, kTextAlignmentLeft));
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
    Vec2Si32(640, 480+64*2), 0, g_border.DrawExternalSize(Vec2Si32(640, 480+64*2))));
  const Ui64 kTetraminoButton = 1;
  const Ui64 kHelloButton = 2;
  const Ui64 kSnakeButton = 3;
  const Ui64 kCodingForKidsButton = 4;
  const Ui64 kConquestButton = 5;
  const Ui64 kDiscreteEventSimButton = 6;
  const Ui64 kExitButton = 100;

  const char *welcome = (const char *)u8"The Snow Wizard\n\n"
  "Please select the flavour of the new project.";

  Si32 y = box->GetSize().y-32;

  std::shared_ptr<Text> textbox(new Text(
    0, Vec2Si32(24, y), Vec2Si32(box->GetSize().x, 0),
    0, g_font, kTextOriginTop, g_palete, welcome, kTextAlignmentLeft));
  box->AddChild(textbox);
  y = 8 + 16 + 64 + 64 + 64 + 64 + 64 + 64;
  std::shared_ptr<Button> tetramino_button = MakeButton(
    kTetraminoButton, Vec2Si32(32, y), kKeyR,
    1, "Tet\001r\002amino game project", Vec2Si32(box->GetSize().x - 64, 48));
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
  std::shared_ptr<Button> discrete_event_sim_button = MakeButton(
      kDiscreteEventSimButton, Vec2Si32(32, y), kKeyD,
      4, "\001D\002iscrete-event simulator project",
      Vec2Si32(box->GetSize().x - 64, 48));
    box->AddChild(discrete_event_sim_button);
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
  } else if (action == kDiscreteEventSimButton) {
    g_project_kind = kProjectKindDiscreteEventSimButton;
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
    0, g_font, kTextOriginTop, g_palete, welcome, kTextAlignmentLeft));
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
  Si32 editbox_y = y;
  std::shared_ptr<Editbox> editbox(new Editbox(
    kEditBox,
    Vec2Si32(32, editbox_y),
    1,
    button_normal, 
    button_hover,
    g_font,
    kTextOriginBottom,
    Rgba(255, 255, 255, 255),
    "",
    kTextAlignmentLeft,
    false,
    allow_list));
  box->AddChild(editbox);

  std::shared_ptr<Text> error_text(new Text(
    0, Vec2Si32(32, editbox_y - 24), Vec2Si32(box->GetSize().x - 64, 0),
    0, g_font, kTextOriginTop, Rgba(255, 80, 80),
    "Enter a project name", kTextAlignmentLeft));
  box->AddChild(error_text);

  y = 8 + 16 + 64 + 64;
  y -= 64;
  std::shared_ptr<Button> done_button = MakeButton(
    kDoneButton, Vec2Si32(32, y), kKeyNone,
    2, "Done",
    Vec2Si32(box->GetSize().x - 64, 48));
  done_button->SetEnabled(false);
  box->AddChild(done_button);
  box->SwitchCurrentTab(true);

  y -= 64;
  std::shared_ptr<Button> exit_button = MakeButton(
    kExitButton, Vec2Si32(32, y), kKeyNone,
    100, "Exit",
    Vec2Si32(box->GetSize().x - 64, 48));
  box->AddChild(exit_button);

  std::string base_dir;
  {
    std::string search_dir = g_current_directory;
    for (Si32 i = 0; i < 10; ++i) {
      std::string marker = search_dir + "/arctic.engine";
      std::vector<Ui8> data = ReadFile(marker.c_str(), true);
      const char *expected = "arctic.engine";
      if (data.size() >= std::strlen(expected)
          && memcmp(data.data(), expected, std::strlen(expected)) == 0) {
        base_dir = CanonicalizePath((search_dir + "/..").c_str());
        break;
      }
      search_dir += "/..";
    }
    if (base_dir.empty()) {
      base_dir = CanonicalizePath(
        (g_current_directory + "/..").c_str());
    }
  }

  std::string prev_name;
  Ui64 clicked_button = Ui64(-1);
  while (true) {
    std::string name = editbox->GetText();
    if (name != prev_name) {
      prev_name = name;
      if (name.empty()) {
        done_button->SetEnabled(false);
        error_text->SetText("Enter a project name");
      } else {
        std::string target_dir = base_dir + "/" + name;
        if (DoesDirectoryExist(target_dir.c_str()) == kTrivalentTrue) {
          done_button->SetEnabled(false);
          error_text->SetText("Directory already exists: " + target_dir);
        } else {
          done_button->SetEnabled(true);
          error_text->SetText("");
        }
      }
    }

    UpdateResolution();
    Clear();
    box->SetPos((ScreenSize() - box->GetSize()) / 2);
    box->Draw(Vec2Si32(0, 0));
    ShowFrame();
    if (clicked_button != Ui64(-1)) {
      if (clicked_button == kDoneButton) {
        g_project_name = editbox->GetText();
        return true;
      }
      return false;
    }
    std::deque<GuiMessage> messages;
    for (Si32 idx = 0; idx < InputMessageCount(); ++idx) {
      box->ApplyInput(GetInputMessage(idx), &messages);
    }
    for (auto it = messages.begin(); it != messages.end(); ++it) {
      if (it->kind == kGuiButtonClick) {
        clicked_button = it->panel->GetTag();
      }
    }
  }
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
  std::string search_dir = g_current_directory;
  for (Si32 i = 0; i < 10; ++i) {
    std::string marker = search_dir + "/arctic.engine";
    std::vector<Ui8> data = ReadFile(marker.c_str(), true);
    const char *expected = "arctic.engine";
    if (data.size() >= std::strlen(expected)
        && memcmp(data.data(), expected, std::strlen(expected)) == 0) {
      std::string parent = CanonicalizePath(
        (search_dir + "/..").c_str());
      g_project_directory = parent;
      break;
    }
    search_dir += "/..";
  }
  if (g_project_directory.empty()) {
    g_project_directory = g_current_directory;
  }
  GatherEntries(&entries);

  const Si32 kVisibleRows = 10;
  const Si32 kRowHeight = 36;
  const Si32 kScrollbarWidth = 29;
  const Si32 kBoxWidth = 720;
  const Si32 kListWidth = kBoxWidth - 64 - kScrollbarWidth - 4;
  const Si32 kHeaderHeight = 140;
  const Si32 kBoxHeight = kHeaderHeight + kRowHeight * kVisibleRows + 64 + 8;

  const Ui64 kTagSelectButton = 1;
  const Ui64 kTagExitButton = 2;
  const Ui64 kTagScrollbar = 3;
  const Ui64 kTagPathText = 4;
  const Ui64 kTagFilter = 5;
  const Ui64 kTagEntryBase = 1000;

  bool need_rebuild = true;
  bool filter_dirty = true;
  std::string filter_text;
  std::vector<DirectoryEntry> filtered;

  std::shared_ptr<Panel> box;
  std::shared_ptr<Text> path_text;
  std::shared_ptr<Scrollbar> scrollbar;
  std::shared_ptr<Button> select_button;
  std::shared_ptr<Editbox> filter_editbox;
  std::vector<std::shared_ptr<Button>> row_buttons;
  std::vector<std::shared_ptr<Text>> row_texts;
  Si32 wheel_accum = 0;

  while (true) {
    if (need_rebuild) {
      need_rebuild = false;
      row_buttons.clear();
      row_texts.clear();

      Vec2Si32 box_size(kBoxWidth, kBoxHeight);
      box.reset(new Panel(0, Vec2Si32(0, 0),
        box_size, 0, g_border.DrawExternalSize(box_size)));

      Si32 y = box_size.y - 28;

      std::shared_ptr<Text> title(new Text(
        0, Vec2Si32(24, y), Vec2Si32(box_size.x - 48, 0),
        0, g_font, kTextOriginTop, g_palete,
        "The Snow Wizard",
        kTextAlignmentLeft));
      box->AddChild(title);

      y -= 36;
      std::string display_path = "Path: " + g_project_directory;
      path_text.reset(new Text(
        kTagPathText, Vec2Si32(24, y), Vec2Si32(box_size.x - 48, 0),
        0, g_font, kTextOriginTop, g_palete,
        display_path, kTextAlignmentLeft));
      box->AddChild(path_text);

      y -= 30;
      Si32 filter_label_w = g_font.EvaluateSize("Filter:", false).x;
      Si32 filter_x = 24 + filter_label_w + 8;
      Si32 filter_w = 24 + kListWidth + kScrollbarWidth + 4 - filter_x;

      const Si32 kFilterEditH = 36;
      Si32 editbox_border = std::max(0,
        (kFilterEditH - g_font.FontInstance()->line_height_) / 2);
      std::shared_ptr<Text> filter_label(new Text(
        0, Vec2Si32(24, y - kFilterEditH + editbox_border),
        Vec2Si32(filter_label_w, 0),
        0, g_font, kTextOriginBottom, g_palete,
        "Filter:", kTextAlignmentLeft));
      box->AddChild(filter_label);

      Vec2Si32 filter_size(filter_w, kFilterEditH);
      Sprite filter_n = g_button_normal.DrawExternalSize(filter_size);
      Sprite filter_f = g_button_hover.DrawExternalSize(filter_size);
      filter_editbox.reset(new Editbox(
        kTagFilter, Vec2Si32(filter_x, y - filter_size.y), 1,
        filter_n, filter_f,
        g_font, kTextOriginBottom, Rgba(255, 255, 255, 255),
        "", kTextAlignmentLeft));
      box->AddChild(filter_editbox);

      y -= 46;
      Si32 list_top = y;

      for (Si32 i = 0; i < kVisibleRows; ++i) {
        Si32 row_y = list_top - i * kRowHeight;
        Vec2Si32 btn_size(kListWidth, kRowHeight - 2);
        Sprite btn_n = g_button_normal.DrawExternalSize(btn_size);
        Sprite btn_h = g_button_hover.DrawExternalSize(btn_size);
        Sprite btn_d = g_button_down.DrawExternalSize(btn_size);
        std::shared_ptr<Text> btn_text;
        std::shared_ptr<Button> btn(new Button(
          kTagEntryBase + static_cast<Ui64>(i),
          Vec2Si32(24, row_y - btn_size.y),
          btn_n, btn_d, btn_h,
          g_sound_button_down, g_sound_button_up,
          kKeyNone, 0));
        btn_text.reset(new Text(
          0, Vec2Si32(8, 2),
          Vec2Si32(btn_size.x - 16, btn_size.y - 4),
          0, g_font, kTextOriginCenter, g_palete, "",
          kTextAlignmentLeft));
        btn->AddChild(btn_text);
        box->AddChild(btn);
        row_buttons.push_back(btn);
        row_texts.push_back(btn_text);
      }

      scrollbar.reset(new Scrollbar(kTagScrollbar, g_v_scrollbar_theme));
      Si32 scroll_h = kRowHeight * kVisibleRows;
      scrollbar->SetPos(Vec2Si32(24 + kListWidth + 4,
        list_top - scroll_h));
      scrollbar->SetSize(Vec2Si32(kScrollbarWidth, scroll_h));
      scrollbar->RegenerateSprites();
      Si32 init_max = std::max(0,
        static_cast<Si32>(entries.size()) - kVisibleRows);
      scrollbar->SetMinValue(0);
      scrollbar->SetMaxValue(init_max);
      scrollbar->SetValue(init_max);
      scrollbar->SetStep(1);
      box->AddChild(scrollbar);

      Si32 bottom_y = 16;
      bool is_project_dir =
        (GetDirectoryProjects(g_project_directory).size() == 1);
      select_button = MakeButton(
        kTagSelectButton, Vec2Si32(24, bottom_y), kKeyNone, 2,
        "Select This Directory",
        Vec2Si32((box_size.x - 64) / 2, 48));
      select_button->SetEnabled(is_project_dir);
      box->AddChild(select_button);

      std::shared_ptr<Button> exit_button = MakeButton(
        kTagExitButton,
        Vec2Si32(24 + (box_size.x - 64) / 2 + 16, bottom_y),
        kKeyEscape, 3, "Exit",
        Vec2Si32((box_size.x - 64) / 2 - 16, 48));
      box->AddChild(exit_button);

      box->SwitchCurrentTab(true);
      filter_dirty = true;
    }

    std::string current_filter = filter_editbox->GetText();
    if (filter_dirty || current_filter != filter_text) {
      filter_text = current_filter;
      filtered.clear();
      for (size_t j = 0; j < entries.size(); ++j) {
        if (filter_text.empty() || entries[j].title == "..") {
          filtered.push_back(entries[j]);
        } else {
          std::string lower_title = entries[j].title;
          std::string lower_filter = filter_text;
          for (auto &c : lower_title) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
          }
          for (auto &c : lower_filter) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
          }
          if (lower_title.find(lower_filter) != std::string::npos) {
            filtered.push_back(entries[j]);
          }
        }
      }
      Si32 new_max = std::max(0,
        static_cast<Si32>(filtered.size()) - kVisibleRows);
      scrollbar->SetMaxValue(new_max);
      scrollbar->SetValue(new_max);
      filter_dirty = false;
    }

    Si32 max_scroll = std::max(0,
      static_cast<Si32>(filtered.size()) - kVisibleRows);
    Si32 scroll_offset = max_scroll - scrollbar->GetValue();
    for (Si32 i = 0; i < kVisibleRows; ++i) {
      Si32 entry_idx = scroll_offset + i;
      if (entry_idx < static_cast<Si32>(filtered.size())) {
        row_buttons[static_cast<size_t>(i)]->SetVisible(true);
        std::string label;
        if (filtered[static_cast<size_t>(entry_idx)].is_directory
            == kTrivalentTrue) {
          label = "\001" + filtered[static_cast<size_t>(entry_idx)].title
            + "/\002";
        } else {
          label = filtered[static_cast<size_t>(entry_idx)].title;
        }
        row_texts[static_cast<size_t>(i)]->SetText(label);
      } else {
        row_buttons[static_cast<size_t>(i)]->SetVisible(false);
      }
    }

    UpdateResolution();
    Clear();
    box->SetPos((ScreenSize() - box->GetSize()) / 2);
    box->Draw(Vec2Si32(0, 0));
    ShowFrame();

    std::deque<GuiMessage> messages;
    for (Si32 idx = 0; idx < InputMessageCount(); ++idx) {
      box->ApplyInput(GetInputMessage(idx), &messages);
    }
    for (auto it = messages.begin(); it != messages.end(); ++it) {
      if (it->kind == kGuiButtonClick) {
        Ui64 tag = it->panel->GetTag();
        if (tag == kTagExitButton) {
          return false;
        }
        if (tag == kTagSelectButton) {
          return true;
        }
        if (tag >= kTagEntryBase
            && tag < kTagEntryBase + kVisibleRows) {
          Si32 row = static_cast<Si32>(tag - kTagEntryBase);
          Si32 entry_idx = scroll_offset + row;
          if (entry_idx < static_cast<Si32>(filtered.size())) {
            if (filtered[static_cast<size_t>(entry_idx)].is_directory
                == kTrivalentTrue) {
              std::string new_dir = g_project_directory + "/"
                + filtered[static_cast<size_t>(entry_idx)].title;
              g_project_directory = CanonicalizePath(new_dir.c_str());
              GatherEntries(&entries);
              need_rebuild = true;
            }
          }
        }
      }
    }
    wheel_accum += MouseWheelDelta();
    const Si32 kWheelThreshold = 10;
    if (std::abs(wheel_accum) >= kWheelThreshold) {
      Si32 steps = wheel_accum / kWheelThreshold;
      wheel_accum -= steps * kWheelThreshold;
      Si32 cur = scrollbar->GetValue();
      Si32 new_val = cur + steps;
      if (new_val < scrollbar->GetMinValue()) {
        new_val = scrollbar->GetMinValue();
      }
      if (new_val > scrollbar->GetMaxValue()) {
        new_val = scrollbar->GetMaxValue();
      }
      scrollbar->SetValue(new_val);
    }
  }
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
  bool has_error = false;
  bool is_done = false;
  std::string error_message;
  char text[1 << 20];
  while (!has_error && !is_done) {
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
        g_path = CanonicalizePath(
          (g_current_directory + "/..").c_str()) + "/" + g_project_name;
      } else {
        error_message = "Can't detect Arctic Engine.";
        has_error = true;
      }
    }
    break;
    case 2:
      if (DoesDirectoryExist(g_path.c_str()) == 0) {
      } else {
        error_message = "A directory named\n\""
          + g_path + "\"\nalready exists. Use another name.";
        has_error = true;
      }
      break;
    case 3:
      if (MakeDirectory(g_path.c_str())) {
        g_project_directory = g_path;
      } else {
        error_message = "Can't create directory\n\""
          + g_path + "\".";
        has_error = true;
      }
      break;
    case 4:
      g_template = g_current_directory + "/template_project_name";
      if (DoesDirectoryExist(g_template.c_str()) == 1) {
      } else {
        error_message = "Can't find template directory\n\""
          + g_template + "\".";
        has_error = true;
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

        if (!is_ok) {
          error_message = "Can't create directory\n\""
            + g_project_directory + "/" + name + "\".";
          has_error = true;
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
        } else if (g_project_kind == kProjectKindDiscreteEventSimButton) {
          files.push_back("data/button_down.wav");
          files.push_back("data/button_up.wav");
          files.push_back("data/gui_atlas.tga");
          files.push_back("data/gui_atlas.xml");
          files.push_back("data/gui_theme.xml");
        }
        for (size_t idx = 0; idx < files.size(); ++idx) {
          auto data = ReadFile((g_template + "/" + files[idx]).c_str());
          std::string name = files[idx];
          ReplaceAll("template_project_name", g_project_name, &name);
          WriteFile((g_project_directory + "/" + name).c_str(),
              data.data(), data.size());
        }
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
          , "webserver.py"
          , "index.html"
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
          case kProjectKindDiscreteEventSimButton:
            PatchAndCopyTemplateFile("main_discrete_event_sim.cpp", "main.cpp");
            break;
          default:
          case kProjectKindHello:
            PatchAndCopyTemplateFile("main_hello.cpp", "main.cpp");
            break;
        }
        is_done = true;
      }
        break;
      default:
        break;
    }
    step++;

    UpdateResolution();
    Clear();
    const char *welcome = (const char *)u8"The Snow Wizard\n\n"
    "Creating project \"%s\"\n\n"
    "Current directory: %s\n"
    "%s";

    snprintf(text, sizeof(text), welcome,
        g_project_name.c_str(), g_current_directory.c_str(),
        g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop,
                kTextAlignmentLeft, kDrawBlendingModeColorize,
                kFilterNearest, g_palete);
    ShowFrame();
  }
  if (!g_pause_when_done) {
    return !has_error;
  }
  if (has_error) {
    ShowResultDialog(false, error_message);
    return false;
  }
  return true;
}

bool ShowUpdateProgress() {
  std::vector<DirectoryEntry> engine_entries;
  Si32 step = 0;
  bool has_error = false;
  bool is_done = false;
  std::string error_message;
  char text[1 << 20];
  while (!has_error && !is_done) {
    switch (step) {
      case 1: {
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
        if (!is_ok) {
          error_message = "Can't detect Arctic Engine.";
          has_error = true;
        }
      }
        break;
      case 2: {
        g_engine = CanonicalizePath((g_current_directory + "/engine").c_str());
        if (DoesDirectoryExist(g_engine.c_str()) == 1) {
          GetDirectoryEntries(g_engine.c_str(), &engine_entries);
          g_progress.append("Engine found: \"");
          g_progress.append(g_engine);
          g_progress.append("\"\n");
        } else {
          error_message = "Can't find engine directory\n\""
            + g_engine + "\".";
          has_error = true;
        }
      }
        break;
      case 3: {
        std::deque<std::string> candidates =
          GetDirectoryProjects(g_project_directory);
        Si32 candidate_count = (Si32)candidates.size();
        for (Si32 i = 0; i < candidate_count; ++i) {
          g_progress.append("Project name candidate \"");
          g_progress.append(candidates[static_cast<size_t>(i)]);
          g_progress.append("\"\n");
        }

        if (candidate_count == 0) {
          error_message = "Can't find project files in\n\""
            + g_project_directory + "\".";
          has_error = true;
        } else if (candidate_count > 1) {
          error_message = "Multiple project files in\n\""
            + g_project_directory + "\".";
          has_error = true;
        } else {
          g_project_name = candidates[0];
        }
      }
        break;
      case 4: {
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
        std::stringstream new_buildfiles;
        std::stringstream new_files;
        std::stringstream new_engine_children;
        std::stringstream new_project_children;
        std::stringstream new_buildphase;
        std::unordered_set<std::string> new_hashes;

        std::deque<FileToAdd> files_to_add;
        if (g_project_kind == kProjectKindCodingForKids) {
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
            std::string uid_file = MakeUid();
            std::string uid_buildfile = MakeUid();

            if (full_content.find(uid_file) == std::string::npos &&
                new_hashes.find(uid_file) == new_hashes.end() &&
                full_content.find(uid_buildfile) == std::string::npos &&
                new_hashes.find(uid_buildfile) == new_hashes.end() &&
                uid_file != uid_buildfile) {
              is_inserted = true;
              new_hashes.insert(uid_file);
              new_hashes.insert(uid_buildfile);

              if (EndsWith(entry.title, std::string(".cpp")) ||
                  EndsWith(entry.title, std::string(".c")) ||
                  EndsWith(entry.title, std::string(".mm"))) {
                new_buildfiles << "\t\t" << uid_buildfile
                  << " /* " << entry.title << " in Sources */ = {"
                  << "isa = PBXBuildFile; fileRef = " << uid_file
                  << " /* " << entry.title << " */; };\n";
              }

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
                  new_engine_children << "\n\t\t\t\t" << uid_file <<
                    " /* " << entry.title << " */,";
                  break;
                case kFileToAddProject:
                  new_project_children << "\n\t\t\t\t" << uid_file <<
                    " /* " << entry.title << " */,";
                  break;
              }

              if (EndsWith(entry.title, std::string(".cpp")) ||
                  EndsWith(entry.title, std::string(".c")) ||
                  EndsWith(entry.title, std::string(".mm"))) {
                new_buildphase << "\n\t\t\t\t" << uid_buildfile
                  << " /* " << entry.title << " in Sources */,";
              }
            }
          }
        }

        std::stringstream resulting_file;
        std::size_t cursor = 0;
        std::size_t next_item =
          full_content.find("/* End PBXBuildFile section */");
        if (next_item == std::string::npos) {
          error_message = "No PBXBuildFile section in Xcode project.";
          has_error = true;
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_buildfiles.str();

        cursor = next_item;
        next_item = full_content.find("/* End PBXFileReference section */");
        if (next_item == std::string::npos) {
          error_message = "No PBXFileReference section in Xcode project.";
          has_error = true;
          break;
        }
        if (next_item < cursor) {
          error_message = "Out of order PBXFileReference in Xcode project.";
          has_error = true;
          break;
        }
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_files.str();

        std::string main_group_entry("/* main.cpp */,");
        cursor = next_item;
        next_item = full_content.find(main_group_entry);
        if (next_item == std::string::npos) {
          error_message = "No main_group_entry in Xcode project.";
          has_error = true;
          break;
        }
        if (next_item < cursor) {
          error_message = "Out of order main_group_entry in Xcode project.";
          has_error = true;
          break;
        }
        next_item += main_group_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_project_children.str();

        std::string engine_group_entry("/* engine.cpp */,");
        cursor = next_item;
        next_item = full_content.find(engine_group_entry);
        if (next_item == std::string::npos) {
          error_message = "No engine_group_entry in Xcode project.";
          has_error = true;
          break;
        }
        if (next_item < cursor) {
          error_message = "Out of order engine_group_entry in Xcode project.";
          has_error = true;
          break;
        }
        next_item += engine_group_entry.size();
        resulting_file << full_content.substr(cursor, next_item - cursor);
        resulting_file << new_engine_children.str();

        std::string buildphase_entry("/* engine.cpp in Sources */,");
        cursor = next_item;
        next_item = full_content.find(buildphase_entry);
        if (next_item == std::string::npos) {
          error_message = "No buildphase_entry in Xcode project.";
          has_error = true;
          break;
        }
        if (next_item < cursor) {
          error_message = "Out of order buildphase_entry in Xcode project.";
          has_error = true;
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
      }
        break;
      case 5: {
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
          }
        }
        if (g_project_kind == kProjectKindCodingForKids) {
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
            error_message = "No engine.h in VS project.";
            has_error = true;
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
            error_message = "No engine.cpp in VS project.";
            has_error = true;
            break;
          }
          if (next_item < cursor) {
            error_message = "Out of order engine.cpp in VS project.";
            has_error = true;
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
            error_message = "No engine.cpp in VS project filters.";
            has_error = true;
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
            error_message = "No engine.h in VS project filters.";
            has_error = true;
            break;
          }
          if (next_item < cursor) {
            error_message = "Out of order engine.h in VS project filters.";
            has_error = true;
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
      }
        break;
      case 6:
        g_template = g_current_directory + "/template_project_name";
        if (DoesDirectoryExist(g_template.c_str()) == 1) {
        } else {
          error_message = "Can't find template directory\n\""
            + g_template + "\".";
          has_error = true;
        }
        break;
      case 7: {
          PatchAndCopyTemplateFile("CMakeLists.txt");
          is_done = true;
        }
        break;
      default:
        break;
    }
    step++;

    UpdateResolution();
    Clear();
    const char *welcome = (const char *)u8"The Snow Wizard\n\n"
    "Updating project \"%s\"\n"
    "Current directory: %s\n"
    "%s";

    snprintf(text, sizeof(text), welcome,
             g_project_name.c_str(), g_current_directory.c_str(),
             g_progress.c_str());
    g_font.Draw(text, 32, ScreenSize().y - 32, kTextOriginTop,
                kTextAlignmentLeft, kDrawBlendingModeColorize,
                kFilterNearest, g_palete);
    ShowFrame();
  }
  if (!g_pause_when_done) {
    return !has_error;
  }
  if (has_error) {
    ShowResultDialog(false, error_message);
    return false;
  }
  const char *action = (g_mode_of_operation == kModeCreate)
    ? "created" : "updated";
  ShowResultDialog(true,
    "Project \"" + g_project_name + "\" " + action + " successfully.");
  return true;
}

void EasyMain() {
  g_sound_chime.Load("data/chime.wav");
  g_sound_chime.Play();

  g_sound_button_down.Load("data/button_down.wav");
  g_sound_button_up.Load("data/button_up.wav");
  g_sound_error.Load("data/error.wav");
  g_sound_jingle.Load("data/jingle.wav");

  g_palete.emplace_back((Ui32)0xffffffff);
  g_palete.emplace_back((Ui32)0xffff9999);
  g_palete.emplace_back((Ui32)0xffffffff);
  g_palete.emplace_back((Ui32)0xff6666ff);
  g_font.Load("data/arctic_one_bmf.fnt");

  Sprite border;
  border.Load("data/panel_border.tga");
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
  Sprite button_disabled;
  button_disabled.Load("data/button_disabled.tga");
  g_button_disabled.Split(button_disabled, 12, true, true);

  g_text_palete = {Rgba(255, 255, 255)};

  {
    g_v_scrollbar_theme = std::make_shared<GuiThemeScrollbar>();
    g_v_scrollbar_theme->is_horizontal_ = false;
    Sprite s;
    s.Load("data/v_scroll_bg_normal.tga");
    g_v_scrollbar_theme->normal_background_.Split(s, 9, true, true);
    s.Load("data/v_scroll_bg_hover.tga");
    g_v_scrollbar_theme->focused_background_.Split(s, 9, true, true);
    s.Load("data/v_scroll_bg_disabled.tga");
    g_v_scrollbar_theme->disabled_background_.Split(s, 9, true, true);
    g_v_scrollbar_theme->normal_button_dec_.Load("data/v_scroll_dec_normal.tga");
    g_v_scrollbar_theme->focused_button_dec_.Load("data/v_scroll_dec_hover.tga");
    g_v_scrollbar_theme->down_button_dec_.Load("data/v_scroll_dec_down.tga");
    g_v_scrollbar_theme->disabled_button_dec_.Load("data/v_scroll_dec_disabled.tga");
    g_v_scrollbar_theme->normal_button_inc_.Load("data/v_scroll_inc_normal.tga");
    g_v_scrollbar_theme->focused_button_inc_.Load("data/v_scroll_inc_hover.tga");
    g_v_scrollbar_theme->down_button_inc_.Load("data/v_scroll_inc_down.tga");
    g_v_scrollbar_theme->disabled_button_inc_.Load("data/v_scroll_inc_disabled.tga");
    g_v_scrollbar_theme->normal_button_cur_.Load("data/v_scroll_cur_normal.tga");
    g_v_scrollbar_theme->focused_button_cur_.Load("data/v_scroll_cur_hover.tga");
    g_v_scrollbar_theme->down_button_cur_.Load("data/v_scroll_cur_down.tga");
    g_v_scrollbar_theme->disabled_button_cur_.Load("data/v_scroll_cur_disabled.tga");
  }

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
