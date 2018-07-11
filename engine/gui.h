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

#include "engine/arctic_types.h"
#include "engine/arctic_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/font.h"

namespace arctic {

enum GuiMessageKind {
  kGuiButtonClick,
  kGuiButtonDown,
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
public:

  Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order = 0,
    easy::Sprite background = easy::Sprite());
  Vec2Si32 GetSize();
  Ui32 GetTabOrder();
  void SetTabOrder(Ui32 tab_order);
  Ui64 GetTag();
  void SetTag(Ui64 tag);
  void SetPos(Vec2Si32 pos);
  void SetBackground(easy::Sprite background);
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
    easy::Sprite normal, easy::Sprite down = easy::Sprite(), easy::Sprite hovered = easy::Sprite(),
    easy::Sound down_sound = easy::Sound(), easy::Sound up_sound = easy::Sound(),
    KeyCode hotkey = kKeyNone, Ui32 tab_order = 0);
  void Draw(Vec2Si32 parent_absolute_pos) override;
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

class Text : public Panel {
protected:
  Font font_;
  TextOrigin origin_;
  Rgba color_;
  std::vector<Rgba> palete_;
  std::string text_;
  TextAlignment alignment_;
public:
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment = kAlignLeft);
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
    Font font, TextOrigin origin, std::vector<Rgba> palete, std::string text,
    TextAlignment alignment = kAlignLeft);
  void SetText(std::string text);
  void Draw(Vec2Si32 parent_absolute_pos) override;
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

}  // namespace arctic

#endif  // ENGINE_GUI_H_
