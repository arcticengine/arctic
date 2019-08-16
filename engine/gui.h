// The MIT License (MIT)
//
// Copyright (c) 2018 Huldra
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

#ifndef ENGINE_GUI_H_
#define ENGINE_GUI_H_

#include <deque>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/arctic_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/font.h"

namespace arctic {

enum GuiMessageKind {
  kGuiButtonClick,
  kGuiButtonDown,
  kGuiScrollChange,
  kGuiPanelLeftDown,
};

class Panel;

class GuiMessage {
 public:
  std::shared_ptr<Panel> panel;
  GuiMessageKind kind;

  GuiMessage(std::shared_ptr<Panel> panel, GuiMessageKind kind);
};

class Panel : public std::enable_shared_from_this<Panel> {
 protected:
  Ui64 tag_;
  Vec2Si32 pos_;
  Vec2Si32 size_;
  Ui32 tab_order_;
  bool is_current_tab_;
  easy::Sprite background_;
  std::deque<std::shared_ptr<Panel>> children_;
  bool is_clickable_;

 public:
  Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order = 0,
    easy::Sprite background = easy::Sprite(), bool is_clickable = false);
  Vec2Si32 GetSize();
  Ui32 GetTabOrder();
  void SetTabOrder(Ui32 tab_order);
  Ui64 GetTag() const;
  void SetTag(Ui64 tag);
  Vec2Si32 GetPos() const;
  void SetPos(Vec2Si32 pos);
  void SetBackground(const easy::Sprite &background);
  virtual ~Panel();
  virtual void Draw(Vec2Si32 parent_absolute_pos);
  virtual bool ApplyInput(const InputMessage &message,
      std::deque<GuiMessage> *out_gui_messages);
  virtual void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab);
  void MakeCurrentTab(const Panel *target);
  bool SwitchCurrentTab(bool is_forward);
  void FindNeighbors(Ui32 current_tab_order,
      Panel **in_out_prev, Panel **in_out_next);
  Panel *FindCurrentTab();
  virtual void SetCurrentTab(bool is_current_tab);
  virtual void AddChild(std::shared_ptr<Panel> child);
};

class Button : public Panel {
 public:
  enum ButtonState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kDown = 3
  };

 protected:
  easy::Sprite normal_;
  easy::Sprite down_;
  easy::Sprite hovered_;
  easy::Sound down_sound_;
  easy::Sound up_sound_;
  KeyCode hotkey_;
  ButtonState state_ = kNormal;

 public:
  Button(Ui64 tag, Vec2Si32 pos,
    easy::Sprite normal,
    easy::Sprite down = easy::Sprite(),
    easy::Sprite hovered = easy::Sprite(),
    easy::Sound down_sound = easy::Sound(),
    easy::Sound up_sound = easy::Sound(),
    KeyCode hotkey = kKeyNone, Ui32 tab_order = 0);
  void Draw(Vec2Si32 parent_absolute_pos)
    override;
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;
  void SetCurrentTab(bool is_current_tab) override;
};

enum TextAlignment {
  kAlignLeft,
  kAlignCenter,
  kAlignRight
};

enum TextSelectionMode {
  kTextSelectionModeInvert,
  kTextSelectionModeSwapColors
};

class Text : public Panel {
 protected:
  Font font_;
  TextOrigin origin_;
  Rgba color_;
  std::vector<Rgba> palete_;
  std::string text_;
  TextAlignment alignment_;
  Si32 selection_begin_;
  Si32 selection_end_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;

 public:
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment = kAlignLeft);
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
    Font font, TextOrigin origin, std::vector<Rgba> palete, std::string text,
    TextAlignment alignment = kAlignLeft);
  void SetText(std::string text);
  void Draw(Vec2Si32 parent_absolute_pos) override;
  void Select(Si32 selection_begin, Si32 selection_end);
  void SetSelectionMode(TextSelectionMode selection_mode = kTextSelectionModeInvert,
    Rgba selection_color_1 = Rgba(0, 0, 0),
    Rgba selection_color_2 = Rgba(255, 255, 255));
};

class Progressbar: public Panel {
 protected:
  easy::Sprite incomplete_;
  easy::Sprite complete_;
  float total_value_;
  float current_value_;
  std::shared_ptr<Text> text_;

 public:
  Progressbar(Ui64 tag, Vec2Si32 pos,
    easy::Sprite incomplete, easy::Sprite complete,
    std::vector<Rgba> palete, Font font,
    float total_value = 1.0f, float current_value = 0.0f);
  void Draw(Vec2Si32 parent_absolute_pos) override;
  void UpdateText();
  void SetTotalValue(float total_value);
  void SetCurrentValue(float current_value);
};

class Editbox: public Panel {
 protected:
  Font font_;
  TextOrigin origin_;
  Rgba color_;
  std::string text_;
  TextAlignment alignment_;
  easy::Sprite normal_;
  easy::Sprite focused_;
  Si32 cursor_pos_;
  Si32 display_pos_;
  Si32 selection_begin_;
  Si32 selection_end_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;
  bool is_digits_;
  std::unordered_set<Ui32> white_list_;

 public:
  Editbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    easy::Sprite normal, easy::Sprite focused,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment = kAlignLeft, bool is_digits = false,
    std::unordered_set<Ui32> white_list = std::unordered_set<Ui32>());
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;
  void SetText(std::string text);
  void Draw(Vec2Si32 parent_absolute_pos) override;
  std::string GetText();
  void SelectAll();
  void SetSelectionMode(TextSelectionMode selection_mode = kTextSelectionModeInvert,
    Rgba selection_color_1 = Rgba(0, 0, 0),
    Rgba selection_color_2 = Rgba(255, 255, 255));
};

class HorizontalScroll : public Panel {
 public:
  enum ScrollState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kLeftDown = 3,
    kRightDown = 4,
    kMiddleDragged = 5,
    kLeftFast = 6,
    kRightFast = 7
  };

 protected:
  easy::Sprite normal_background_;
  easy::Sprite focused_background_;
  easy::Sprite normal_button_left_;
  easy::Sprite focused_button_left_;
  easy::Sprite down_button_left_;
  easy::Sprite normal_button_right_;
  easy::Sprite focused_button_right_;
  easy::Sprite down_button_right_;
  easy::Sprite normal_button_cur_;
  easy::Sprite focused_button_cur_;
  easy::Sprite down_button_cur_;
  Si32 min_value_;
  Si32 max_value_;
  Si32 value_;
  ScrollState state_ = kNormal;
  Si32 start_x_;
  Si32 start_value_;

 public:
  HorizontalScroll(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    easy::Sprite normal_background,
    easy::Sprite focused_background, easy::Sprite normal_button_left,
    easy::Sprite focused_button_left, easy::Sprite down_button_left,
    easy::Sprite normal_button_right, easy::Sprite focused_button_right,
    easy::Sprite down_button_right, easy::Sprite normal_button_cur,
    easy::Sprite focused_button_cur, easy::Sprite down_button_cur,
    Si32 min_value, Si32 max_value, Si32 value);
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;
  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetValue(Si32 value);
  Si32 GetValue();
};

}  // namespace arctic

#endif  // ENGINE_GUI_H_
