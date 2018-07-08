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

#include "engine/gui.h"

#include "engine/arctic_platform.h"

namespace arctic {

GuiMessage::GuiMessage(std::shared_ptr<Panel> in_panel, GuiMessageKind in_kind)
    : panel(in_panel)
    , kind(in_kind) {
}

Panel::Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      easy::Sprite background)
    : tag_(tag)
    , pos_(pos)
    , size_(size)
    , tab_order_(tab_order)
    , is_current_tab_(0)
    , background_(background) {
}

Vec2Si32 Panel::GetSize() {
  return size_;
}
  
Ui32 Panel::GetTabOrder() {
  return tab_order_;
}

void Panel::SetTabOrder(Ui32 tab_order) {
  tab_order_ = tab_order;
}

Ui64 Panel::GetTag() {
  return tag_;
}

void Panel::SetTag(Ui64 tag) {
  tag_ = tag;
}
  
void Panel::SetPos(Vec2Si32 pos) {
  pos_ = pos;
}

void Panel::SetBackground(easy::Sprite background) {
  background_ = background;
}

Panel::~Panel() {
}

void Panel::Draw(Vec2Si32 parent_absolute_pos) {
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
  Vec2Si32 pos = parent_pos + pos_;
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    (**it).ApplyInput(pos, message, false, in_out_is_applied,
        out_gui_messages, out_current_tab);
  }
  if (is_top_level) {
    if (*in_out_is_applied == false) {
      if (message.kind == InputMessage::kKeyboard &&
          message.keyboard.key == kKeyTab &&
          (message.keyboard.key_state & 1) == 1) {
        bool is_forward = !(message.keyboard.state[kKeyShift] & 1);
        bool is_switched = SwitchCurrentTab(is_forward);
        if (is_switched) {
          *in_out_is_applied = true;
        }
      }
    }
    if ((*out_current_tab).get()) {
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
        if (current_tab_order > prev) { // prev .?. current
          if (order > prev && order < current_tab_order) {
            *in_out_prev = (*it).get();
          }
        } else if (current_tab_order < prev) { // .?. current ... prev .?.
          if (order > prev || order < current_tab_order) {
            *in_out_prev = (*it).get();
          }
        }
      } else {
        *in_out_prev = (*it).get();
      }
      
      if (*in_out_next) {
        Ui32 next = (*in_out_next)->GetTabOrder();
        if (current_tab_order < next) { // current .?. next
          if (order < next && order > current_tab_order) {
            *in_out_next = (*it).get();
          }
        } else if (current_tab_order > next) { // .?. next ... current .?.
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
  children_.push_back(child);
}

Button::Button(Ui64 tag, Vec2Si32 pos,
  easy::Sprite normal, easy::Sprite down, easy::Sprite hovered,
  easy::Sound down_sound, easy::Sound up_sound,
  KeyCode hotkey, Ui32 tab_order)
    : Panel(tag,
        pos,
        Max(normal.Size(), Max(hovered.Size(), down.Size())),
        tab_order)
    , normal_(normal)
    , down_(down)
    , hovered_(hovered)
    , down_sound_(down_sound)
    , up_sound_(up_sound)
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
  }
  Panel::Draw(parent_absolute_pos);
}

void Button::ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
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
  Vec2Si32 pos = parent_pos + pos_;
  ButtonState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = relative_pos.x >= 0 && relative_pos.y >= 0 &&
      relative_pos.x < size_.x && relative_pos.y < size_.y;
    if (is_inside && !*in_out_is_applied) {
      *out_current_tab = shared_from_this();
      is_current_tab_ = true;
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        state_ = kDown;
      } else {
        state_ = kHovered;
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
        if (message.keyboard.key_state & 1) {
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
  

Text::Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, Rgba color, std::string text)
    : Panel(tag, pos, size, tab_order)
    , font_(font)
    , origin_(origin)
    , color_(color)
    , text_(text) {
}

Text::Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, std::vector<Rgba> palete, std::string text)
    : Panel(tag, pos, size, tab_order)
    , font_(font)
    , origin_(origin)
    , palete_(palete)
    , text_(text) {
  Check(palete.size(), "Error! Palete is empty!");
  color_ = palete[0];
}

void Text::SetText(std::string text) {
  text_ = text;
}
  
void Text::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 size = font_.EvaluateSize(text_.c_str(), false);
  Vec2Si32 offset = (size_ - size) / 2;
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_ + offset;
  if (palete_.size()) {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
      origin_, easy::kColorize, palete_);
  } else {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
      origin_, easy::kColorize, color_);
  }
}
  
}  // namespace arctic
