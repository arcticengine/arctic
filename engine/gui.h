// The MIT License (MIT)
//
// Copyright (c) 2018 - 2024 Huldra
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
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/arctic_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/font.h"
#include "engine/decorated_frame.h"

namespace arctic {

/// @addtogroup global_gui
/// @{
enum GuiMessageKind {
  kGuiButtonClick,
  kGuiButtonDown,
  kGuiScrollChange,
  kGuiPanelLeftDown,
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

class Panel;

void inline DoNothing() {
  return;
}

class GuiThemeButton {
 public:
  DecoratedFrame normal_;
  DecoratedFrame down_;
  DecoratedFrame hovered_;
  DecoratedFrame disabled_;
  Sound down_sound_;
  Sound up_sound_;

  Font text_font_;
  std::vector<Rgba> text_palete_;
  TextAlignment text_alignment_;
};

class GuiTheme {
 public:
  DecoratedFrame panel_background_;

  std::shared_ptr<GuiThemeButton> button_;

  Font text_font_;
  TextOrigin text_origin_;
  std::vector<Rgba> text_palete_;
  TextAlignment text_alignment_;
  TextSelectionMode text_selection_mode_;
  Rgba text_selection_color_1_;
  Rgba text_selection_color_2_;

  DecoratedFrame progressbar_incomplete_;
  DecoratedFrame progressbar_complete_;

  Font editbox_font_;
  TextOrigin editbox_origin_;
  Rgba editbox_color_;
  TextAlignment editbox_alignment_;
  DecoratedFrame editbox_normal_;
  DecoratedFrame editbox_focused_;
  TextSelectionMode editbox_selection_mode_;
  Rgba editbox_selection_color_1_;
  Rgba editbox_selection_color_2_;

  DecoratedFrame h_scrollbar_normal_background_;
  DecoratedFrame h_scrollbar_focused_background_;
  DecoratedFrame h_scrollbar_disabled_background_;
  Sprite h_scrollbar_normal_button_dec_;
  Sprite h_scrollbar_focused_button_dec_;
  Sprite h_scrollbar_down_button_dec_;
  Sprite h_scrollbar_disabled_button_dec_;
  Sprite h_scrollbar_normal_button_inc_;
  Sprite h_scrollbar_focused_button_inc_;
  Sprite h_scrollbar_down_button_inc_;
  Sprite h_scrollbar_disabled_button_inc_;
  Sprite h_scrollbar_normal_button_cur_;
  Sprite h_scrollbar_focused_button_cur_;
  Sprite h_scrollbar_down_button_cur_;
  Sprite h_scrollbar_disabled_button_cur_;

  DecoratedFrame v_scrollbar_normal_background_;
  DecoratedFrame v_scrollbar_focused_background_;
  DecoratedFrame v_scrollbar_disabled_background_;
  Sprite v_scrollbar_normal_button_dec_;
  Sprite v_scrollbar_focused_button_dec_;
  Sprite v_scrollbar_down_button_dec_;
  Sprite v_scrollbar_disabled_button_dec_;
  Sprite v_scrollbar_normal_button_inc_;
  Sprite v_scrollbar_focused_button_inc_;
  Sprite v_scrollbar_down_button_inc_;
  Sprite v_scrollbar_disabled_button_inc_;
  Sprite v_scrollbar_normal_button_cur_;
  Sprite v_scrollbar_focused_button_cur_;
  Sprite v_scrollbar_down_button_cur_;
  Sprite v_scrollbar_disabled_button_cur_;

  Sprite checkbox_clear_normal_;
  Sprite checkbox_checked_normal_;
  Sprite checkbox_clear_down_;
  Sprite checkbox_checked_down_;
  Sprite checkbox_clear_hovered_;
  Sprite checkbox_checked_hovered_;
  Sprite checkbox_clear_disabled_;
  Sprite checkbox_checked_disabled_;
  Sound checkbox_down_sound_;
  Sound checkbox_up_sound_;

  void Load(const char *xml_file_path);
};



class GuiMessage {
 public:
  std::shared_ptr<Panel> panel;
  GuiMessageKind kind;

  GuiMessage(std::shared_ptr<Panel> panel, GuiMessageKind kind);
};

class Panel : public std::enable_shared_from_this<Panel> {
 protected:
  static std::shared_ptr<Panel> invalid_panel_;
  Ui64 tag_;
  Vec2Si32 pos_;
  Vec2Si32 size_;
  Ui32 tab_order_;
  bool is_current_tab_;
  Sprite background_;
  std::deque<std::shared_ptr<Panel>> children_;
  bool is_clickable_;
  bool is_visible_;

 public:
  Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order = 0,
    Sprite background = Sprite(), bool is_clickable = false);
  static std::shared_ptr<Panel> Invalid() {
    return invalid_panel_;
  }
  Vec2Si32 GetSize() const;
  Ui32 GetTabOrder() const;
  void SetTabOrder(Ui32 tab_order);
  Ui64 GetTag() const;
  void SetTag(Ui64 tag);
  Vec2Si32 GetPos() const;
  void SetPos(Vec2Si32 pos);
  void SetWidth(Si32 width);
  void SetHeight(Si32 height);
  void SetBackground(const Sprite &background);
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
  virtual void SetVisible(bool is_visible);
  virtual void SetEnabled(bool is_enabled);
  virtual bool IsVisible();
  virtual bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos);
  virtual void SetEnabledByTag(Ui64 tag, bool is_enabled);
  virtual void RegenerateSprites();

  std::function<void(void)> OnPanelLeftDown = DoNothing;
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
  void SetSelectionMode(
      TextSelectionMode selection_mode = kTextSelectionModeInvert,
      Rgba selection_color_1 = Rgba(0, 0, 0),
      Rgba selection_color_2 = Rgba(255, 255, 255));
};

class Button : public Panel {
 public:
  enum ButtonState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kDown = 3,
    kDisabled = 4
  };

 protected:
  Sprite normal_;
  Sprite down_;
  Sprite hovered_;
  Sprite disabled_;
  Sound down_sound_;
  Sound up_sound_;
  KeyCode hotkey_;
  ButtonState state_ = kNormal;
  std::shared_ptr<Text> text_;
  std::shared_ptr<GuiThemeButton> theme_;

 public:
  Button(Ui64 tag, Vec2Si32 pos,
    Sprite normal,
    Sprite down = Sprite(),
    Sprite hovered = Sprite(),
    Sound down_sound = Sound(),
    Sound up_sound = Sound(),
    KeyCode hotkey = kKeyNone,
    Ui32 tab_order = 0,
    Sprite disabled = Sprite());
  Button(Ui64 tag, std::shared_ptr<GuiThemeButton> theme);
  void Draw(Vec2Si32 parent_absolute_pos)
    override;
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;
  void SetCurrentTab(bool is_current_tab) override;
  void SetVisible(bool is_visible) override;
  void SetEnabled(bool is_enabled) override;
  bool IsVisible() override;
  bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) override;
  void SetText(std::string text);
  void RegenerateSprites() override;

  std::function<void(void)> OnButtonClick = DoNothing;
  std::function<void(void)> OnButtonDown = DoNothing;
};

class Progressbar: public Panel {
 protected:
  Sprite incomplete_;
  Sprite complete_;
  float total_value_;
  float current_value_;
  std::shared_ptr<Text> text_;

 public:
  Progressbar(Ui64 tag, Vec2Si32 pos,
    Sprite incomplete, Sprite complete,
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
  Sprite normal_;
  Sprite focused_;
  Si32 cursor_pos_;
  Si32 display_pos_;
  Si32 selection_begin_;
  Si32 selection_end_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;
  bool is_digits_;
  std::unordered_set<Ui32> allow_list_;

 public:
  Editbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite normal, Sprite focused,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment = kAlignLeft, bool is_digits = false,
    std::unordered_set<Ui32> allow_list = std::unordered_set<Ui32>());
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;
  void SetText(std::string text);
  void Draw(Vec2Si32 parent_absolute_pos) override;
  std::string GetText();
  void SelectAll();
  void SetSelectionMode(
      TextSelectionMode selection_mode = kTextSelectionModeInvert,
      Rgba selection_color_1 = Rgba(0, 0, 0),
      Rgba selection_color_2 = Rgba(255, 255, 255));
};

class Scrollbar : public Panel {
public:
 enum ScrollState {
   kHidden = 0,
   kNormal = 1,
   kHovered = 2,
   kDecDown = 3,
   kIncDown = 4,
   kMiddleDragged = 5,
   kDecFast = 6,
   kIncFast = 7
 };
 enum ScrollKind {
   kScrollHorizontal = 0,
   kScrollVertical = 1
 };
 protected:
  Sprite normal_background_;
  Sprite focused_background_;
  Sprite normal_button_dec_;
  Sprite focused_button_dec_;
  Sprite down_button_dec_;
  Sprite normal_button_inc_;
  Sprite focused_button_inc_;
  Sprite down_button_inc_;
  Sprite normal_button_cur_;
  Sprite focused_button_cur_;
  Sprite down_button_cur_;

  Si32 min_value_;
  Si32 max_value_;
  Si32 value_;
  ScrollState state_ = kNormal;
  Si32 start_relative_s_ = 0;
  Si32 start_value_ = 0;
  Si32 dir_ = 0; // 0 for horizontal, 1 for vertical

 public:
  Scrollbar(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite normal_background,
    Sprite focused_background, Sprite normal_button_left,
    Sprite focused_button_left, Sprite down_button_left,
    Sprite normal_button_right, Sprite focused_button_right,
    Sprite down_button_right, Sprite normal_button_cur,
    Sprite focused_button_cur, Sprite down_button_cur,
    Si32 min_value, Si32 max_value, Si32 value,
    ScrollKind kind);
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;
  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetValue(Si32 value);
  Si32 GetValue() const;

  std::function<void(void)> OnScrollChange = DoNothing;
};

class Checkbox : public Panel {
 public:
  enum CheckboxState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kDown = 3,
    kDisabled = 4
  };

  enum CheckboxValue {
    kValueClear = 0,
    kValueChecked = 1
  };

 protected:
  Sprite normal_[2];
  Sprite down_[2];
  Sprite hovered_[2];
  Sprite disabled_[2];
  Sound down_sound_;
  Sound up_sound_;
  KeyCode hotkey_;
  CheckboxState state_ = kNormal;
  CheckboxValue value_ = kValueClear;

 public:
  Checkbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite clear_normal,
    Sprite checked_normal,
    Sprite clear_down = Sprite(),
    Sprite checked_down = Sprite(),
    Sprite clear_hovered = Sprite(),
    Sprite checked_hovered = Sprite(),
    Sprite clear_disabled = Sprite(),
    Sprite checked_disabled = Sprite(),
    Sound down_sound = Sound(),
    Sound up_sound = Sound(),
    KeyCode hotkey = kKeyNone,
    CheckboxValue value = kValueClear);
  void Draw(Vec2Si32 parent_absolute_pos)
    override;
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;
  void SetCurrentTab(bool is_current_tab) override;
  void SetVisible(bool is_visible) override;
  void SetEnabled(bool is_enabled) override;
  bool IsVisible() override;
  bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) override;
  void SetChecked(bool is_checked);
  bool IsChecked();

  std::function<void(void)> OnButtonClick = DoNothing;
  std::function<void(void)> OnButtonDown = DoNothing;
};

class GuiFactory {
protected:
  Ui64 last_tag_ = 0;
 public:
  std::shared_ptr<GuiTheme> theme_;

  std::shared_ptr<Panel> MakePanel();
  std::shared_ptr<Panel> MakeTransparentPanel();
  std::shared_ptr<Button> MakeButton();
  std::shared_ptr<Text> MakeText();
  std::shared_ptr<Progressbar> MakeProgressbar();
  std::shared_ptr<Scrollbar> MakeHorizontalScrollbar();
  std::shared_ptr<Scrollbar> MakeVerticalScrollbar();
  std::shared_ptr<Checkbox> MakeCheckbox();
};

/// @}

}  // namespace arctic


#endif  // ENGINE_GUI_H_
