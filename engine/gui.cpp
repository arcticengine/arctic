// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 - 2020 Huldra
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

#include "engine/gui.h"

#include <cstring>
#include <memory>
#include <utility>
#include "engine/easy_advanced.h"
#include "engine/easy_drawing.h"
#include "engine/easy_util.h"
#include "engine/unicode.h"
#include "engine/arctic_platform.h"
#include "engine/scalar_math.h"

namespace arctic {

GuiMessage::GuiMessage(std::shared_ptr<Panel> in_panel, GuiMessageKind in_kind)
    : panel(in_panel)
    , kind(in_kind) {
}

Panel::Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Sprite background, bool is_clickable)
    : tag_(tag)
    , pos_(pos)
    , size_(size)
    , tab_order_(tab_order)
    , is_current_tab_(0)
    , background_(std::move(background))
    , is_clickable_(is_clickable)
    , is_visible_(true) {
}

std::shared_ptr<Panel> Panel::invalid_panel_(new Panel(0, Vec2Si32(0, 0), Vec2Si32(0, 0)));

Vec2Si32 Panel::GetSize() const {
  return size_;
}

Ui32 Panel::GetTabOrder() const {
  return tab_order_;
}

void Panel::SetTabOrder(Ui32 tab_order) {
  tab_order_ = tab_order;
}

Ui64 Panel::GetTag() const {
  return tag_;
}

void Panel::SetTag(Ui64 tag) {
  tag_ = tag;
}

Vec2Si32 Panel::GetPos() const {
  return pos_;
}

void Panel::SetPos(Vec2Si32 pos) {
  pos_ = pos;
}

void Panel::SetBackground(const Sprite &background) {
  background_ = background;
}

Panel::~Panel() {
}

void Panel::Draw(Vec2Si32 parent_absolute_pos) {
  if (!is_visible_) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  background_.Draw(absolute_pos, size_);
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    (**it).Draw(absolute_pos);
  }
}

bool Panel::ApplyInput(const InputMessage &message,
    std::deque<GuiMessage> *out_gui_messages) {
  bool is_applied = false;
  std::shared_ptr<Panel> current_tab;
  ApplyInput(Vec2Si32(0, 0), message, true, &is_applied, out_gui_messages,
     &current_tab);
  return is_applied;
}

void Panel::ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  Check(in_out_is_applied,
    "ApplyInput must not be called with in_out_is_applied == nullptr");
  Check(out_gui_messages,
    "ApplyInput must not be called with out_gui_messages == nullptr");
  if (!is_visible_) {
    return;
  }
  Vec2Si32 pos = parent_pos + pos_;
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    (**it).ApplyInput(pos, message, false, in_out_is_applied,
        out_gui_messages, out_current_tab);
  }
  if (!*in_out_is_applied &&
      is_clickable_ &&
      message.kind == InputMessage::kMouse) {
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;
    if (is_inside) {
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1) {
        out_gui_messages->emplace_back(shared_from_this(), kGuiPanelLeftDown);
        *in_out_is_applied = true;
      }
    }
  }
  if (is_top_level) {
    if (*in_out_is_applied == false) {
      if (message.kind == InputMessage::kKeyboard &&
          message.keyboard.key == kKeyTab &&
          (message.keyboard.key_state & 1u) == 1u) {
        bool is_forward = !(message.keyboard.state[kKeyShift] & 1u);
        bool is_switched = SwitchCurrentTab(is_forward);
        if (is_switched) {
          *in_out_is_applied = true;
        }
      }
    }
    if (*out_current_tab) {
      MakeCurrentTab((*out_current_tab).get());
      if (!(*out_current_tab)->is_current_tab_) {
        (*out_current_tab)->is_current_tab_ = true;
      }
    }
  }
}

void Panel::MakeCurrentTab(const Panel *target) {
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    if ((*it)->is_current_tab_ && (*it).get() != target) {
      (*it)->SetCurrentTab(false);
    }
    (*it)->MakeCurrentTab(target);
  }
}

bool Panel::SwitchCurrentTab(bool is_forward) {
  Panel *cur = FindCurrentTab();
  Ui32 current_tab_order = (cur ? cur->GetTabOrder() : 0);
  Panel *prev = nullptr;
  Panel *next = nullptr;
  FindNeighbors(current_tab_order, &prev, &next);
  Panel *target = is_forward ? next : prev;
  if (cur) {
    cur->SetCurrentTab(false);
  }
  if (target) {
    target->SetCurrentTab(true);
  }
  return !!target;
}

void Panel::FindNeighbors(Ui32 current_tab_order,
    Panel **in_out_prev, Panel **in_out_next) {
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    Ui32 order = (*it)->GetTabOrder();
    if (order != 0 && current_tab_order != order) {
      if (*in_out_prev) {
        Ui32 prev = (*in_out_prev)->GetTabOrder();
        if (current_tab_order > prev) {  // prev .?. current
          if (order > prev && order < current_tab_order) {
            *in_out_prev = (*it).get();
          }
        } else if (current_tab_order < prev) {  // .?. current ... prev .?.
          if (order > prev || order < current_tab_order) {
            *in_out_prev = (*it).get();
          }
        }
      } else {
        *in_out_prev = (*it).get();
      }

      if (*in_out_next) {
        Ui32 next = (*in_out_next)->GetTabOrder();
        if (current_tab_order < next) {  // current .?. next
          if (order < next && order > current_tab_order) {
            *in_out_next = (*it).get();
          }
        } else if (current_tab_order > next) {  // .?. next ... current .?.
          if (order < next || order > current_tab_order) {
            *in_out_next = (*it).get();
          }
        }
      } else {
        *in_out_next = (*it).get();
      }
    }
    (*it)->FindNeighbors(current_tab_order, in_out_prev, in_out_next);
  }
}

Panel *Panel::FindCurrentTab() {
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    if ((*it)->is_current_tab_) {
      return it->get();
    }
    Panel *current_tab = (*it)->FindCurrentTab();
    if (current_tab) {
      return current_tab;
    }
  }
  return nullptr;
}

void Panel::SetCurrentTab(bool is_current_tab) {
  is_current_tab_ = is_current_tab;
}

void Panel::AddChild(std::shared_ptr<Panel> child) {
  Check(!!child, "AddChild called with child == nullptr");
  Check(child.get() != this, "AddChild called with child == this");
  children_.push_back(child);
}

void Panel::SetVisible(bool is_visible) {
  is_visible_ = is_visible;
}

bool Panel::IsVisible() {
  return is_visible_;
}

bool Panel::IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) {
  if (!is_visible_) {
    return true;
  }
  Vec2Si32 pos = parent_pos + pos_;
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (!(**it).IsMouseTransparentAt(pos, mouse_pos)) {
      return false;
    }
  }
  if (is_clickable_) {
    Vec2Si32 relative_pos = mouse_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;
    if (is_inside) {
      return false;
    }
  }
  return true;
}

Button::Button(Ui64 tag, Vec2Si32 pos,
  Sprite normal, Sprite down, Sprite hovered,
  Sound down_sound, Sound up_sound,
  KeyCode hotkey, Ui32 tab_order, Sprite disabled)
    : Panel(tag,
        pos,
        Max(normal.Size(), Max(hovered.Size(), down.Size())),
        tab_order)
    , normal_(normal)
    , down_(down)
    , hovered_(hovered)
    , disabled_(disabled)
    , down_sound_(std::move(down_sound))
    , up_sound_(std::move(up_sound))
    , hotkey_(hotkey) {
}

void Button::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  switch (state_) {
  case kHidden:
    break;
  case kNormal:
    normal_.Draw(absolute_pos);
    break;
  case kHovered:
    hovered_.Draw(absolute_pos);
    break;
  case kDown:
    down_.Draw(absolute_pos);
    break;
  case kDisabled:
    disabled_.Draw(absolute_pos);
    break;
  }
  Panel::Draw(parent_absolute_pos);
}

void Button::SetEnabled(bool is_enabled) {
  if (state_ == Button::kHidden) {
    return;
  }
  if (is_enabled) {
    if (state_ == Button::kDisabled) {
      state_ = Button::kNormal;
      return;
    }
  } else {
    if (state_ != Button::kDisabled) {
      state_ = Button::kDisabled;
      return;
    }
  }
}

void Button::ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  if (state_ == kHidden || state_ == kDisabled) {
    return;
  }
  Check(in_out_is_applied,
    "ApplyInput must not be called with in_out_is_applied == nullptr");
  Check(out_gui_messages,
    "ApplyInput must not be called with out_gui_messages == nullptr");
  Panel::ApplyInput(parent_pos, message, is_top_level, in_out_is_applied,
    out_gui_messages, out_current_tab);
  ButtonState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;
    if (is_inside && !*in_out_is_applied) {
      *out_current_tab = Panel::Invalid();
      is_current_tab_ = false;
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        state_ = kDown;
      } else {
        state_ = kHovered;
        *in_out_is_applied = true;
      }
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 2 &&
          prev_state == kDown) {
        *in_out_is_applied = true;
        up_sound_.Play();
        out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
      }
    } else {
      if (is_current_tab_) {
        state_ = kHovered;
      } else {
        state_ = kNormal;
      }
    }
    if (state_ != prev_state) {
      if (state_ == kDown) {
        down_sound_.Play();
        *in_out_is_applied = true;
        out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
      }
      if (prev_state == kDown) {
        up_sound_.Play();
      }
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (!*in_out_is_applied) {
      bool is_hotkey = (message.keyboard.key == hotkey_);
      bool is_tab_order_enter = (is_current_tab_ &&
        (message.keyboard.key == kKeyEnter ||
          message.keyboard.key == kKeySpace));
      if (is_hotkey || is_tab_order_enter) {
        if (message.keyboard.key_state & 1u) {
          if ((prev_state != kDown && is_hotkey) ||
              (prev_state == kHovered && is_tab_order_enter)) {
            *in_out_is_applied = true;
            down_sound_.Play();
            state_ = kDown;
            if (is_hotkey && GetTabOrder() != 0) {
              *out_current_tab = shared_from_this();
            }
            out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
          }
        } else {
          if (prev_state == kDown) {
            *in_out_is_applied = true;
            up_sound_.Play();
            out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
            if (GetTabOrder() == 0 || !is_current_tab_) {
              state_ = kNormal;
            } else {
              state_ = kHovered;
            }
          }
        }
      }
    }
  }
}

void Button::SetCurrentTab(bool is_current_tab) {
  if (!is_current_tab) {
    if (state_ == kHovered) {
      state_ = kNormal;
    }
  } else {
    if (state_ == kNormal) {
      state_ = kHovered;
    }
  }
  is_current_tab_ = is_current_tab;
}

void Button::SetVisible(bool is_visible) {
  if (Panel::IsVisible() != is_visible) {
    Panel::SetVisible(is_visible);
    if (Panel::IsVisible()) {
      state_ = kNormal;
    } else {
      state_ = kHidden;
    }
  }
}

bool Button::IsVisible() {
  bool is_visible = Panel::IsVisible();
  bool should_be_visible = state_ != kHidden;
  Check(is_visible == should_be_visible,
      "Button visibility state inconsitency detected!");
  return is_visible;
}

bool Button::IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) {
  if (!is_visible_) {
    return true;
  }
  Vec2Si32 pos = parent_pos + pos_;
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (!(**it).IsMouseTransparentAt(pos, mouse_pos)) {
      return false;
    }
  }

  Vec2Si32 relative_pos = mouse_pos - pos;
  bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
    relative_pos.x < size_.x && relative_pos.y < size_.y;
  if (is_inside) {
    return false;
  }
  return true;
}

Text::Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, Rgba color, std::string text,
      TextAlignment alignment)
    : Panel(tag, pos, size, tab_order)
    , font_(std::move(font))
    , origin_(origin)
    , color_(color)
    , text_(std::move(text))
    , alignment_(alignment)
    , selection_begin_(0)
    , selection_end_(0)
    , selection_mode_(kTextSelectionModeInvert)
    , selection_color_1_(Rgba(0, 0, 0))
    , selection_color_2_(Rgba(255, 255, 255)) {
}

Text::Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, std::vector<Rgba> palete, std::string text,
      TextAlignment alignment)
    : Panel(tag, pos, size, tab_order)
    , font_(std::move(font))
    , origin_(origin)
    , palete_(palete)
    , text_(std::move(text))
    , alignment_(alignment)
    , selection_begin_(0)
    , selection_end_(0)
    , selection_mode_(kTextSelectionModeInvert)
    , selection_color_1_(Rgba(0, 0, 0))
    , selection_color_2_(Rgba(255, 255, 255)) {
  Check(!palete.empty(), "Error! Palete is empty!");
  color_ = palete[0];
}

void Text::SetText(std::string text) {
  text_ = std::move(text);
  selection_begin_ = 0;
  selection_end_ = 0;
}

void DrawSelection(Si32 x1, Si32 y1, Si32 x2, Si32 y2,
    TextSelectionMode selection_mode,
    Rgba c1, Rgba c2, Sprite backbuffer) {
  switch (selection_mode) {
    case kTextSelectionModeInvert:
      for (Si32 y = y1; y < y2; ++y) {
        Rgba *p = backbuffer.RgbaData() + backbuffer.StridePixels() * y;
        for (Si32 x = x1; x < x2; ++x) {
          Rgba &c = p[x];
          c.r = 255 - c.r;
          c.g = 255 - c.g;
          c.b = 255 - c.b;
        }
      }
      break;
    case kTextSelectionModeSwapColors:
      for (Si32 y = y1; y < y2; ++y) {
        Rgba *p = backbuffer.RgbaData() + backbuffer.StridePixels() * y;
        for (Si32 x = x1; x < x2; ++x) {
          Rgba &c = p[x];
          if (c.rgba == c1.rgba) {
            c.rgba = c2.rgba;
          } else if (c.rgba == c2.rgba) {
            c.rgba = c1.rgba;
          }
        }
      }
      break;
  }
}

void Text::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 size = font_.EvaluateSize(text_.c_str(), false);
  Vec2Si32 offset = (size_ - size) / 2;
  if (offset.x < 0) {
    offset.x = 0;
  }
  if (offset.y < 0) {
    offset.y = 0;
  }
  if (alignment_ == kAlignLeft) {
    offset.x = 0;
  } else if (alignment_ == kAlignRight) {
    offset.x = (size_.x - size.x);
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_ + offset;
  if (!palete_.empty()) {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
      origin_, kDrawBlendingModeColorize, kFilterNearest, palete_);
  } else {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
      origin_, kDrawBlendingModeColorize, kFilterNearest, color_);
  }

  if (selection_begin_ != selection_end_) {
    Si32 x1 = absolute_pos.x + font_.EvaluateSize(
        text_.substr(0, static_cast<size_t>(selection_begin_)).c_str(),
          false).x;
    Si32 x2 = absolute_pos.x  + font_.EvaluateSize(
        text_.substr(0, static_cast<size_t>(selection_end_)).c_str(),
          true).x;
    Si32 y1 = absolute_pos.y;
    Si32 y2 = absolute_pos.y + font_.line_height_;
    Sprite backbuffer = GetEngine()->GetBackbuffer();

    x1 = std::max(absolute_pos.x, x1);
    x2 = std::max(absolute_pos.x, x2);

    DrawSelection(x1, y1, x2, y2, selection_mode_,
        selection_color_1_, selection_color_2_, backbuffer);
  }
}

void Text::Select(Si32 selection_begin, Si32 selection_end) {
  selection_begin_ = Clamp(selection_begin, 0, (Si32)text_.length());
  selection_end_ = Clamp(selection_end, 0, (Si32)text_.length());
  if (selection_begin_ > selection_end_) {
    selection_begin_ = selection_end_;
  }
}

void Text::SetSelectionMode(TextSelectionMode selection_mode,
    Rgba selection_color_1, Rgba selection_color_2) {
  selection_mode_ = selection_mode;
  selection_color_1_ = selection_color_1;
  selection_color_2_ = selection_color_2;
}

Progressbar::Progressbar(Ui64 tag, Vec2Si32 pos,
      Sprite incomplete, Sprite complete,
      std::vector<Rgba> palete, Font font,
      float total_value, float current_value)
    : Panel(tag, pos, Max(incomplete.Size(), complete.Size()), 0)
    , incomplete_(incomplete)
    , complete_(complete)
    , total_value_(total_value)
    , current_value_(current_value) {
  text_ = std::make_shared<Text>(Ui64(-1), Vec2Si32(0, 0), GetSize(), 0,
    font, kTextOriginBottom, palete, "0% Done", kAlignCenter);
  Panel::AddChild(text_);
  UpdateText();
}

void Progressbar::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  Si32 w1 = GetSize().x;
  if (current_value_ >= 0.0f
      && total_value_ > 0.0f
      && current_value_ <= total_value_) {
    w1 = Si32(current_value_ / total_value_ * GetSize().x);
  }
  Si32 w2 = GetSize().x - w1;
  complete_.Draw(absolute_pos.x, absolute_pos.y, w1, complete_.Size().y,
    0, 0, w1, complete_.Size().y);
  incomplete_.Draw(absolute_pos.x + w1, absolute_pos.y,
      w2, incomplete_.Size().y,
    w1, 0, w2, incomplete_.Size().y);
  Panel::Draw(parent_absolute_pos);
}

void Progressbar::UpdateText() {
  Si32 p = 100;
  if (current_value_ >= 0.0f
      && total_value_ > 0.0f
      && current_value_ <= total_value_) {
    p = Si32(current_value_ / total_value_ * 100);
  }
  char str[32];
  snprintf(str, sizeof(str), "%d%% Done", p);
  text_->SetText(str);
}

void Progressbar::SetTotalValue(float total_value) {
  total_value_ = total_value;
  UpdateText();
}

void Progressbar::SetCurrentValue(float current_value) {
  current_value_ = current_value;
  UpdateText();
}


Editbox::Editbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite normal, Sprite focused,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment, bool is_digits,
    std::unordered_set<Ui32> allow_list)
  : Panel(tag,
        pos,
        Max(normal.Size(), focused.Size()),
        tab_order)
  , font_(std::move(font))
  , origin_(origin)
  , color_(color)
  , text_(text)
  , alignment_(alignment)
  , normal_(normal)
  , focused_(focused)
  , cursor_pos_((Si32)text.length())
  , display_pos_(0)
  , selection_begin_(0)
  , selection_end_((Si32)text.length())
  , selection_mode_(kTextSelectionModeInvert)
  , selection_color_1_(Rgba(0, 0, 0))
  , selection_color_2_(Rgba(255, 255, 255))
  , is_digits_(is_digits)
  , allow_list_(std::move(allow_list)) {
}

void Editbox::ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level, bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  Panel::ApplyInput(parent_pos, message, is_top_level, in_out_is_applied,
    out_gui_messages, out_current_tab);
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;
    if (is_inside && !*in_out_is_applied) {
      *out_current_tab = shared_from_this();
      is_current_tab_ = true;
      *in_out_is_applied = true;
    }
  }
  if (!*in_out_is_applied && is_current_tab_) {
    // Edit the text
    if (message.kind == InputMessage::kKeyboard) {
      if (message.keyboard.key == kKeyTab &&
          ((!allow_list_.empty() && allow_list_.find('\t') == allow_list_.end())
            || is_digits_)) { 
        return Panel::ApplyInput(parent_pos, message,
            is_top_level, in_out_is_applied,
            out_gui_messages, out_current_tab);
      }
      if (message.keyboard.key_state == 1) {
        Ui32 key = message.keyboard.key;
        if (key == kKeyBackspace) {
          *in_out_is_applied = true;
          if (text_.length()) {
            if (selection_begin_ != selection_end_) {
              text_.erase(static_cast<size_t>(selection_begin_),
                static_cast<size_t>(selection_end_ - selection_begin_));
              selection_end_ = selection_begin_;
              cursor_pos_ = selection_begin_;
            } else if (cursor_pos_) {
              text_.erase(static_cast<size_t>(cursor_pos_ - 1), 1);
              cursor_pos_--;
            }
          }
        } else if (key == kKeyDelete) {
          *in_out_is_applied = true;
          if (text_.length()) {
            if (selection_begin_ != selection_end_) {
              text_.erase(static_cast<size_t>(selection_begin_),
                static_cast<size_t>(selection_end_ - selection_begin_));
              selection_end_ = selection_begin_;
              cursor_pos_ = selection_begin_;
            } else if (cursor_pos_ >= 0 && cursor_pos_ < (Si32)text_.length()) {
              text_.erase(static_cast<size_t>(cursor_pos_), 1);
              // cursor_pos_;
            }
          }
        } else if (key == kKeyLeft) {
          *in_out_is_applied = true;
          if (message.keyboard.state[kKeyShift]) {
            if (cursor_pos_) {
              if (selection_begin_ == selection_end_) {
                selection_end_ = cursor_pos_;
                cursor_pos_--;
                selection_begin_ = cursor_pos_;
              } else if (selection_begin_ == cursor_pos_) {
                cursor_pos_--;
                selection_begin_ = cursor_pos_;
              } else if (selection_end_ == cursor_pos_) {
                cursor_pos_--;
                selection_end_ = cursor_pos_;
              }
            }
          } else {
            if (cursor_pos_) {
              cursor_pos_--;
              selection_begin_ = cursor_pos_;
              selection_end_ = cursor_pos_;
            }
          }
        } else if (key == kKeyRight) {
          *in_out_is_applied = true;
          if (message.keyboard.state[kKeyShift]) {
            if (cursor_pos_ < (Si32)text_.length()) {
              if (selection_begin_ == selection_end_) {
                selection_begin_ = cursor_pos_;
                cursor_pos_++;
                selection_end_ = cursor_pos_;
              } else if (selection_end_ == cursor_pos_) {
                cursor_pos_++;
                selection_end_ = cursor_pos_;
              } else if (selection_begin_ == cursor_pos_) {
                cursor_pos_++;
                selection_begin_ = cursor_pos_;
              }
            }
          } else {
            if (cursor_pos_ < (Si32)text_.length()) {
              cursor_pos_++;
              selection_begin_ = cursor_pos_;
              selection_end_ = cursor_pos_;
            }
          }
        } else if (key == kKeyEnter) {
          // skip
        } else if (is_digits_ ? (key >= kKey0 && key <= kKey9) : true) {
            // (key >= kKeySpace && key <= kKeyGraveAccent)) {
          *in_out_is_applied = true;
          if (!message.keyboard.characters[0]) {
            if (key >= kKeyA && key <= kKeyZ) {
              if (!message.keyboard.state[kKeyShift]) {
                key = key - kKeyA + Ui32('a');
              }
            }
          }
          if (selection_begin_ != selection_end_) {
            text_.erase(static_cast<size_t>(selection_begin_),
              static_cast<size_t>(selection_end_ - selection_begin_));
            cursor_pos_ = selection_begin_;
            selection_end_ = selection_begin_;
          }
          if (message.keyboard.characters[0]) {
            bool do_insert = true;
            if (!allow_list_.empty()) {
              Utf32Reader reader;
              reader.Reset(reinterpret_cast<const Ui8*>(
                message.keyboard.characters));
              Ui32 codepoint = reader.ReadOne();
              auto found_it = allow_list_.find(codepoint);
              if (found_it == allow_list_.end()) {
                do_insert = false;
              }
            }
            if (do_insert) {
              text_.insert(static_cast<size_t>(cursor_pos_),
                message.keyboard.characters);
              cursor_pos_ += static_cast<Si32>(strlen(
                message.keyboard.characters));
            }
          } else {
            // text_.insert(cursor_pos_, 1, static_cast<char>(key));
            // cursor_pos_++;
          }
        }
      }
    }
  }
  cursor_pos_ = std::min(std::max(0, cursor_pos_), (Si32)text_.length());
  selection_begin_ = std::min(std::max(0, selection_begin_),
      (Si32)text_.length());
  selection_end_ = std::min(std::max(0, selection_end_), (Si32)text_.length());
}

void Editbox::SetText(std::string text) {
  cursor_pos_ = std::min(std::max(0, cursor_pos_), (Si32)text_.length());
  selection_begin_ = 0;
  selection_end_ = 0;
}

void Editbox::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 pos = parent_absolute_pos + pos_;
  if (is_current_tab_) {
    focused_.Draw(pos);
  } else {
    normal_.Draw(pos);
  }
  Si32 border = (normal_.Height() - font_.line_height_) / 2;
  Si32 space_width = font_.EvaluateSize(" ", false).x;

  Si32 available_width = size_.x - border * 2 - space_width;
  Si32 displayable_width = size_.x - border * 2;

  // Update display_pos_ so that both display_pos_
  // and cursor_pos_ are both visible.
  Si32 end_pos = (Si32)text_.length();
  if (cursor_pos_ <= display_pos_) {
    // Move display pos to the left when cursor is at the left border.
    display_pos_ = std::max(0, cursor_pos_ - 1);
  } else {
    // Move display pos to the right when cursor is at the right border.
    std::string part = text_.substr(static_cast<size_t>(display_pos_),
      static_cast<size_t>(cursor_pos_ - display_pos_));
    Si32 w = font_.EvaluateSize(part.c_str(), true).x;
    if (available_width) {
      while (w > available_width) {
        display_pos_++;
        part = text_.substr(static_cast<size_t>(display_pos_),
          static_cast<size_t>(cursor_pos_ - display_pos_));
        w = font_.EvaluateSize(part.c_str(), true).x;
        end_pos = cursor_pos_ + 1;
      }
    }
  }
  // Display part of text with start at the display_pos_.

  // const char *visible = text_.c_str();

  std::string display_text = text_.substr(static_cast<size_t>(display_pos_),
   static_cast<size_t>(end_pos - display_pos_));
  Si32 visible_width = font_.EvaluateSize(display_text.c_str(), false).x;
  if (available_width) {
    while (visible_width > displayable_width) {
      Si32 visible_len = (Si32)display_text.length();
      Si32 desired_len = visible_len * displayable_width / visible_width;
      if (desired_len >= visible_len) {
        desired_len = visible_len - 1;
      }
      end_pos = display_pos_ + desired_len;
      display_text = text_.substr(static_cast<size_t>(display_pos_),
        static_cast<size_t>(end_pos - display_pos_));
      visible_width = font_.EvaluateSize(display_text.c_str(), false).x;
    }
  }

  font_.Draw(display_text.c_str(), pos.x + border, pos.y + border,
    origin_, kDrawBlendingModeColorize, kFilterNearest, color_);

  Si32 cursor_pos = std::max(0, std::min(cursor_pos_, (Si32)text_.length()));
  std::string left_part = text_.substr(0, static_cast<size_t>(cursor_pos));
  Si32 cursor_x = font_.EvaluateSize(left_part.c_str(), false).x;

  Si32 skip_x = font_.EvaluateSize(
      text_.substr(0, static_cast<size_t>(display_pos_)).c_str(), false).x;

  Vec2Si32 a(pos.x + border + cursor_x + 1, pos.y + border);
  a.x = std::max(pos.x + border, a.x - skip_x);
  Vec2Si32 b(a.x + space_width - 1, a.y);
  if (fmod(Time(), 0.6) < 0.3 && is_current_tab_) {
    for (Si32 y = 0; y < 3; ++y) {
      DrawLine(a, b, color_);
      a.y++;
      b.y++;
    }
  }

  if (selection_begin_ != selection_end_ && is_current_tab_) {
    Si32 x1 = pos.x + border + font_.EvaluateSize(
      text_.substr(0, static_cast<size_t>(selection_begin_)).c_str(), false).x;
    Si32 x2 = pos.x + border + font_.EvaluateSize(
      text_.substr(0, static_cast<size_t>(selection_end_)).c_str(), true).x;
    Si32 y1 = pos.y + border;
    Si32 y2 = pos.y + border + font_.line_height_;
    Sprite backbuffer = GetEngine()->GetBackbuffer();

    x1 = std::min(std::max(pos.x + border, x1 - skip_x),
        pos.x + border + displayable_width);
    x2 = std::min(std::max(pos.x + border, x2 - skip_x),
        pos.x + border + displayable_width);

    DrawSelection(x1, y1, x2, y2, selection_mode_,
        selection_color_1_, selection_color_2_, backbuffer);
  }

  Panel::Draw(parent_absolute_pos);
}

std::string Editbox::GetText() {
  return text_;
}

void Editbox::SelectAll() {
  selection_begin_ = 0;
  selection_end_ = (Si32)text_.length();
}

void Editbox::SetSelectionMode(TextSelectionMode selection_mode,
    Rgba selection_color_1, Rgba selection_color_2) {
  selection_mode_ = selection_mode;
  selection_color_1_ = selection_color_1;
  selection_color_2_ = selection_color_2;
}


HorizontalScroll::HorizontalScroll(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
  Sprite normal_background,
  Sprite focused_background, Sprite normal_button_left,
  Sprite focused_button_left, Sprite down_button_left,
  Sprite normal_button_right, Sprite focused_button_right,
  Sprite down_button_right, Sprite normal_button_cur,
  Sprite focused_button_cur, Sprite down_button_cur,
  Si32 min_value, Si32 max_value, Si32 value)
  : Panel(tag, pos,
      Max(normal_background.Size(),
        focused_background.Size()),
      tab_order)
  , normal_background_(normal_background)
  , focused_background_(focused_background)
  , normal_button_left_(std::move(normal_button_left))
  , focused_button_left_(std::move(focused_button_left))
  , down_button_left_(std::move(down_button_left))
  , normal_button_right_(std::move(normal_button_right))
  , focused_button_right_(std::move(focused_button_right))
  , down_button_right_(std::move(down_button_right))
  , normal_button_cur_(std::move(normal_button_cur))
  , focused_button_cur_(std::move(focused_button_cur))
  , down_button_cur_(std::move(down_button_cur))
  , min_value_(min_value)
  , max_value_(max_value)
  , value_(value) {
}

void HorizontalScroll::ApplyInput(Vec2Si32 parent_pos,
    const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  if (state_ == kHidden) {
    return;
  }
  Check(in_out_is_applied,
    "ApplyInput must not be called with in_out_is_applied == nullptr");
  Check(out_gui_messages,
    "ApplyInput must not be called with out_gui_messages == nullptr");
  Panel::ApplyInput(parent_pos, message, is_top_level, in_out_is_applied,
    out_gui_messages, out_current_tab);
  //    x1    x2  x3    x4
  // |--|-----|---|-----|--|
  // |<<|     | @ |     |>>|
  // |--|-----|---|-----|--|
  Si32 x1 = 1 + normal_button_left_.Size().x;
  Si32 x4 = size_.x - 1 - normal_button_right_.Size().x;
  Si32 w = x4 - x1 - normal_button_cur_.Size().x;
  Si32 x2 = x1 + w * (value_ - min_value_) / (max_value_ - min_value_);
  Si32 x3 = x2 + normal_button_cur_.Size().x;

  ScrollState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;

    if (!*in_out_is_applied &&
        message.keyboard.state[kKeyMouseLeft] == 1 &&
        state_ == kMiddleDragged &&
        is_current_tab_) {
      *out_current_tab = shared_from_this();
      *in_out_is_applied = true;
      Si32 drag_x = relative_pos.x - start_x_;
      Si32 value_diff = drag_x * (max_value_ - min_value_) / w;
      value_ = Clamp(start_value_ + value_diff, min_value_, max_value_);
      out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
    } else if (message.keyboard.state[kKeyMouseLeft] != 1) {
      if (is_inside && !*in_out_is_applied) {
        *out_current_tab = shared_from_this();
        is_current_tab_ = true;
      }
      if (is_current_tab_) {
        state_ = kHovered;
      } else {
        state_ = kNormal;
      }
    } else if (is_inside && !*in_out_is_applied) {
      *out_current_tab = shared_from_this();
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        is_current_tab_ = true;
        *in_out_is_applied = true;
        if (state_ == kMiddleDragged) {
          Si32 drag_x = relative_pos.x - start_x_;
          Si32 value_diff = drag_x * (max_value_ - min_value_) / w;
          value_ = Clamp(start_value_ + value_diff, min_value_, max_value_);
          out_gui_messages->emplace_back(
              shared_from_this(), kGuiScrollChange);
        } else if (relative_pos.x < x1) {
          state_ = kLeftDown;
          if (prev_state != state_) {
            value_ = std::max(min_value_, value_ - 1);
            out_gui_messages->emplace_back(
                shared_from_this(), kGuiScrollChange);
          }
        } else if (relative_pos.x < x2) {
          state_ = kLeftFast;
          if (prev_state != state_) {
            value_ = std::max(min_value_, value_ - 5);
            out_gui_messages->emplace_back(
                shared_from_this(), kGuiScrollChange);
          }
        } else if (relative_pos.x < x3) {
          state_ = kMiddleDragged;
          if (prev_state != state_) {
            start_x_ = relative_pos.x;
            start_value_ = value_;
          }
        } else if (relative_pos.x < x4) {
          state_ = kRightFast;
          if (prev_state != state_) {
            value_ = std::min(max_value_, value_ + 5);
            out_gui_messages->emplace_back(
                shared_from_this(), kGuiScrollChange);
          }
        } else {
          state_ = kRightDown;
          if (prev_state != state_) {
            value_ = std::min(max_value_, value_ + 1);
            out_gui_messages->emplace_back(
                shared_from_this(), kGuiScrollChange);
          }
        }
      } else {
        if (is_current_tab_) {
          state_ = kHovered;
        } else {
          state_ = kNormal;
        }
      }
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (is_current_tab_) {
      if (message.keyboard.key_state == 1) {
        if (message.keyboard.key == kKeyLeft) {
          value_ = Clamp(value_ - 1, min_value_, max_value_);
          out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
        } else if (message.keyboard.key == kKeyRight) {
          value_ = Clamp(value_ + 1, min_value_, max_value_);
          out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
        }
      }
    }
  }

  if (is_current_tab_ && state_ == kNormal) {
    state_ = kHovered;
  }
  if (!is_current_tab_) {
    state_ = kNormal;
  }
}

void HorizontalScroll::Draw(Vec2Si32 parent_absolute_pos) {
  if (state_ == kHidden) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  Vec2Si32 button_offset = Vec2Si32(1, 1);
  if (state_ == kNormal) {
    normal_background_.Draw(absolute_pos);
  } else {
    focused_background_.Draw(absolute_pos);
  }
  if (state_ == kLeftDown) {
    down_button_left_.Draw(absolute_pos + button_offset);
  } else {
    normal_button_left_.Draw(absolute_pos + button_offset);
  }
  Vec2Si32 right_pos = absolute_pos + size_.xo()
    - normal_button_right_.Size().xo() + Vec2Si32(-1, 0);
  if (state_ == kRightDown) {
    down_button_right_.Draw(right_pos + button_offset);
  } else {
    normal_button_right_.Draw(right_pos + button_offset);
  }
  Vec2Si32 after_left = absolute_pos + button_offset.xo()
    + normal_button_left_.Size().xo();
  Si32 width = right_pos.x - after_left.x - normal_button_cur_.Size().x + 1;
  Si32 offset = 0;
  if (width > 0 && max_value_ != min_value_) {
    offset = width * (value_ - min_value_) / (max_value_ - min_value_);
  }
  Vec2Si32 cur_pos = after_left + Vec2Si32(offset, 1);
  if (state_ == kMiddleDragged) {
    down_button_cur_.Draw(cur_pos);
  } else {
    normal_button_cur_.Draw(cur_pos);
  }
  Panel::Draw(parent_absolute_pos);
}

void HorizontalScroll::SetValue(Si32 value) {
  value_ = std::min(max_value_, std::max(min_value_, value));
}

Si32 HorizontalScroll::GetValue() const {
  return value_;
}



}  // namespace arctic


template class std::shared_ptr<arctic::GuiMessage>;
template class std::shared_ptr<arctic::Panel>;
template class std::shared_ptr<arctic::Button>;
template class std::shared_ptr<arctic::Text>;
template class std::shared_ptr<arctic::Progressbar>;
template class std::shared_ptr<arctic::Editbox>;
template class std::shared_ptr<arctic::HorizontalScroll>;
