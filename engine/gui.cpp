// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/gui.h"
#include "engine/pugixml.h"

#include <sstream>

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
, do_regenerate_background_(false)
, background_(std::move(background))
, is_clickable_(is_clickable)
, is_visible_(true) {
}


Panel::Panel(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: tag_(tag)
, pos_(Vec2Si32(0, 0))
, size_(Vec2Si32(64, 64))
, tab_order_((Ui32)tag)
, is_current_tab_(0)
, do_regenerate_background_(true)
, is_clickable_(false)
, is_visible_(true)
, theme_(theme) {
  background_ = theme_->panel_background_.DrawExternalSize(size_);
}

std::shared_ptr<Panel> Panel::invalid_panel_(new Panel(0, Vec2Si32(0, 0), Vec2Si32(0, 0)));

Vec2Si32 Panel::GetSize() const {
  return size_;
}

void Panel::SetSize(Vec2Si32 size) {
  if (size_ != size) {
    Vec2Si32 prev_size = size_;
    size_ = size;
    RegenerateSprites();
    for (std::shared_ptr<Panel>& child : children_) {
      child->ParentSizeChanged(prev_size, size);
    }
    SetAnchor(anchor_);
  }
}

void Panel::ParentSizeChanged(Vec2Si32 prev_size, Vec2Si32 cur_size) {
  if (anchor_) {
    Vec2Si32 new_size = size_;
    if ((anchor_ & kAnchorBottom) && (anchor_ & kAnchorTop)) {
      pos_.y = anchor_bottom_d_;
      new_size.y = cur_size.y - pos_.y - anchor_top_d_;
    } else if (anchor_ & kAnchorBottom) {
      pos_.y = anchor_bottom_d_;
    } else if (anchor_ & kAnchorTop) {
      pos_.y = cur_size.y - anchor_top_d_ - size_.y;
    }

    if ((anchor_ & kAnchorLeft) && (anchor_ & kAnchorRight)) {
      pos_.x = anchor_left_d_;
      new_size.x = cur_size.x - pos_.x - anchor_right_d_;
    } else if (anchor_ & kAnchorLeft) {
      pos_.x = anchor_left_d_;
    } else if (anchor_ & kAnchorRight) {
      pos_.x = cur_size.x - anchor_right_d_ - size_.x;
    }

    SetSize(new_size);
  } else if (dock_) {
    Vec2Si32 new_size = size_;
    if ((dock_ & kDockTop) && (dock_ & kDockBottom)) {
      pos_.y = 0;
      new_size.y = cur_size.y;
    } else if (dock_ & kDockTop) {
      pos_.y = cur_size.y - size_.y;
    } else if (dock_ & kDockBottom) {
      pos_.y = 0;
    }

    if ((dock_ & kDockRight) && (dock_ & kDockLeft)) {
      pos_.x = 0;
      new_size.x = cur_size.x;
    } else if (dock_ & kDockRight) {
      pos_.x = cur_size.x - size_.x;
    } else if (dock_ & kDockLeft) {
      pos_.x = 0;
    }

    SetSize(new_size);
  }
}

void Panel::SetSize(Si32 width, Si32 height) {
  SetSize(Vec2Si32(width, height));
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

void Panel::SetPos(Si32 x, Si32 y) {
  pos_ = Vec2Si32(x, y);
}

void Panel::SetWidth(Si32 width) {
  SetSize(width, size_.y);
}

void Panel::SetHeight(Si32 height) {
  SetSize(size_.x, height);
}

void Panel::RegenerateSprites() {
  if (theme_ && do_regenerate_background_) {
    background_ = theme_->panel_background_.DrawExternalSize(size_);
  }
}

void Panel::SetBackground(const Sprite &background) {
  background_ = background;
  do_regenerate_background_ = false;
}

Panel::~Panel() {
  while (!children_.empty()) {
    Check(children_.front()->parent_ != nullptr, "Panel contains a child that does not have a parent");
    Check(children_.front()->parent_ == this, "Panel contains a child of a different parent");
    children_.front()->parent_ = nullptr;
    children_.pop_front();
  }
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
        if (out_gui_messages) {
          out_gui_messages->emplace_back(shared_from_this(), kGuiPanelLeftDown);
        }
        OnPanelLeftDown();
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
  Check(child->parent_ == nullptr, "AddChild called with child that already has a parent");
  children_.push_back(child);
  child->parent_ = this;
}

void Panel::RemoveChild(std::shared_ptr<Panel> child) {
  Check(!!child, "RemoveChild called with child == nullptr");
  Check(child.get() != this, "RemoveChild called with child == this");
  Check(child->parent_ != nullptr, "RemoveChild called with child that does not have a parent");
  Check(child->parent_ == this, "RemoveChild called with some other parents child");
  child->parent_ = nullptr;
  size_t size = children_.size();
  for (size_t to = 0; to < size; ++to) {
    if (children_[to] == child) {
      size_t moving_to = to;
      for (size_t from = moving_to + 1; from < size; ++from) {
        children_[moving_to] = children_[from];
        ++moving_to;
      }
      children_.pop_back();
      return;
    }
  }
  Fatal("RemoveChild could not remove the child");
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

void Panel::SetEnabled(bool) {
}

void Panel::SetEnabledByTag(Ui64 tag, bool is_enabled) {
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    (*it)->SetEnabledByTag(tag, is_enabled);
  }
  if (tag == tag_) {
    SetEnabled(is_enabled);
  }
}

void Panel::BecomeChild(Panel *parent) {
  Check(parent_ == nullptr, "Not null parent_ in BecomeChild");
  parent_ = parent;
  if (anchor_) {
    SetAnchor(anchor_);
  }
}

void Panel::SetAnchor(AnchorKind anchor) {
  anchor_ = anchor;
  if (anchor_) {
    dock_ = kDockNone;
  }
  if (parent_) {
    anchor_bottom_d_ = pos_.y;
    anchor_top_d_ = parent_->GetSize().y - size_.y - pos_.y;
    anchor_left_d_ = pos_.x;
    anchor_right_d_ = parent_->GetSize().x - size_.x - pos_.x;
  }
}

void Panel::SetDock(DockKind dock) {
  dock_ = dock;
  if (dock_) {
    anchor_ = kAnchorNone;
  }
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

Button::Button(Ui64 tag, std::shared_ptr<GuiThemeButton> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(150, 54), (Ui32)tag)
, theme_(theme)
, down_sound_(theme->down_sound_)
, up_sound_(theme->up_sound_)
, hotkey_(kKeyNone) {
  text_ = std::make_shared<Text>(0, theme_->text_);
  text_->SetPos(theme->normal_.BorderSize());
  text_->SetSize(size_ - theme->normal_.BorderSize()*2);
  text_->SetOrigin(kTextOriginCenter);
  Panel::AddChild(text_);
  RegenerateSprites();
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
  if (text_) {
    text_->SetEnabled(is_enabled);
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
        if (out_gui_messages) {
          out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
        }
        OnButtonClick();
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
        if (out_gui_messages) {
          out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
        }
        OnButtonDown();
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
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
            }
            OnButtonDown();
          }
        } else {
          if (prev_state == kDown) {
            *in_out_is_applied = true;
            up_sound_.Play();
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
            }
            OnButtonClick();
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

void Button::SetTextColor(Rgba rgba) {
  if (text_) {
    text_->SetColor(rgba);
  }
}

void Button::SetText(std::string text) {
  if (text_) {
    text_->SetText(text);
  }
}

void Button::SetFont(Font font) {
  if (text_) {
    text_->SetFont(font);
  }
}

void Button::RegenerateSprites() {
  if (theme_) {
    normal_ = theme_->normal_.DrawExternalSize(size_);
    down_ = theme_->down_.DrawExternalSize(size_);
    hovered_ = theme_->hovered_.DrawExternalSize(size_);
    disabled_ = theme_->disabled_.DrawExternalSize(size_);

    if (text_) {
      Vec2Si32 text_size = size_ - theme_->normal_.BorderSize()*2;
      text_->SetSize(text_size);
    }
  }
}

void Button::SetHotkey(KeyCode hotkey) {
  hotkey_ = hotkey;
}

void Button::SetTextPos(Vec2Si32 pos) {
  if (text_) {
    text_->SetPos(pos);
  }
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

Text::Text(Ui64 tag, std::shared_ptr<GuiThemeText> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(0, 0), Ui32(tag))
, font_(theme->font_)
, origin_(theme->origin_)
, palete_(theme->palete_)
, text_("Text")
, alignment_(theme->alignment_)
, selection_begin_(0)
, selection_end_(0)
, selection_mode_(kTextSelectionModeInvert)
, selection_color_1_(Rgba(0, 0, 0))
, selection_color_2_(Rgba(255, 255, 255))
, theme_(theme) {
  Check(!palete_.empty(), "Error! Palete is empty!");
  color_ = palete_[0];
}

void Text::SetColor(Rgba rgba) {
  color_ = rgba;
  if (!palete_.empty()) {
    palete_[0] = rgba;
  }
}

void Text::SetText(std::string text) {
  text_ = std::move(text);
  selection_begin_ = 0;
  selection_end_ = 0;
}

void Text::SetFont(Font font) {
  font_ = font;
}

void DrawSelection(Si32 x1, Si32 y1, Si32 x2, Si32 y2,
                   TextSelectionMode selection_mode,
                   Rgba c1, Rgba c2, Sprite backbuffer) {
  Si32 bw = backbuffer.Width();
  Si32 bh = backbuffer.Height();
  if (x1 < 0) {
    x1 = 0;
  }
  if (y1 < 0) {
    y1 = 0;
  }
  if (x2 > bw) {
    x2 = bw;
  }
  if (y2 > bh) {
    y2 = bh;
  }
  if (x1 >= x2 || y1 >= y2) {
    return;
  }
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

Vec2Si32 Text::EvaluateSize() {
  return font_.EvaluateSize(text_.c_str(), false);
}

void Text::SetEnabled(bool is_enabled) {
  is_enabled_ = is_enabled;
}

void Text::SetOrigin(TextOrigin origin) {
  origin_ = origin;
}

void Text::SetAlignment(TextAlignment alignment) {
  alignment_ = alignment;
}

void Text::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  if (!font_.FontInstance()) {
    return;
  }
  Vec2Si32 size = font_.EvaluateSize(text_.c_str(), false);
  Vec2Si32 offset;

  switch (origin_) {
    case kTextOriginTop:
      offset.y = size_.y;
      break;
    case kTextOriginBottom:
      offset.y = 0;
      break;
    case kTextOriginCenter:
      offset.y = size_.y / 2;
      break;
    case kTextOriginLastBase:
      offset.y = 0;
      break;
    case kTextOriginFirstBase:
      offset.y = 0;
      break;
  }
  switch (alignment_) {
    case kTextAlignmentLeft:
      offset.x = 0;
      break;
    case kTextAlignmentRight:
      offset.x = size_.x;
      break;
    case kTextAlignmentCenter:
      offset.x = size_.x / 2;
      break;
  }

  Vec2Si32 absolute_pos = parent_absolute_pos + pos_ + offset;
  if (!palete_.empty()) {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
               origin_, alignment_, kDrawBlendingModeColorize, kFilterNearest,
               (is_enabled_ || !theme_ || theme_->disabled_palete_.empty()) ? palete_ : theme_->disabled_palete_);
  } else {
    font_.Draw(text_.c_str(), absolute_pos.x, absolute_pos.y,
               origin_, alignment_, kDrawBlendingModeColorize, kFilterNearest, color_);
  }

  if (selection_begin_ != selection_end_) {
    Vec2Si32 size1;
    Vec2Si32 pos1 = font_.EvaluateCharacterPos(text_.c_str(), text_.c_str()+selection_begin_, origin_, alignment_, &size1);
    Vec2Si32 size2;
    const char *plast = text_.c_str()+selection_end_-1;
    while (plast > text_.c_str()+selection_begin_) {
      if (*plast <= 127 && *plast > 0 && *plast != '\n' && *plast != '\r') {
        break;
      }
      --plast;
    }
    Vec2Si32 pos2 = font_.EvaluateCharacterPos(text_.c_str(), plast, origin_, alignment_, &size2);

    Si32 x1 = absolute_pos.x + pos1.x;
    Si32 x2 = absolute_pos.x + pos2.x + size2.x;
    Si32 y1 = absolute_pos.y;
    Si32 y2 = absolute_pos.y + font_.FontInstance()->line_height_;
    Sprite backbuffer = GetEngine()->GetBackbuffer();

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
  text_ = std::make_shared<Text>(Ui64(0), Vec2Si32(0, 0), GetSize(), 0,
                                 font, kTextOriginBottom, palete, "0% Done", kTextAlignmentCenter);
  Panel::AddChild(text_);
  UpdateText();
}

Progressbar::Progressbar(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(150, 54), 0)
, total_value_(1.0)
, current_value_(0.0)
, theme_(theme) {
  incomplete_ = theme_->progressbar_incomplete_.DrawExternalSize(size_);
  complete_ = theme_->progressbar_complete_.DrawExternalSize(size_);
  text_ = std::make_shared<Text>(Ui64(0), theme->button_->text_);
  text_->SetPos(theme->progressbar_incomplete_.BorderSize());
  text_->SetSize(size_-theme->progressbar_incomplete_.BorderSize()*2);
  text_->SetText("0%");
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
    p = Si32(current_value_ / total_value_ * 100.0f);
  }
  char str[32];
  snprintf(str, sizeof(str), "%d%%", p);
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

void Progressbar::RegenerateSprites() {
  if (theme_) {
    incomplete_ = theme_->progressbar_incomplete_.DrawExternalSize(size_);
    complete_ = theme_->progressbar_complete_.DrawExternalSize(size_);
    if (text_) {
      Vec2Si32 text_size = size_ - theme_->progressbar_incomplete_.BorderSize()*2;
      text_->SetSize(text_size);
    }
  }
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

Editbox::Editbox(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(48, 48), Ui32(tag))
, font_(theme->editbox_text_->font_)
, origin_(theme->editbox_text_->origin_)
, color_(theme->editbox_text_->palete_[0])
, text_("")
, alignment_(theme->editbox_text_->alignment_)
, cursor_pos_((Si32)0)
, display_pos_(0)
, selection_begin_(0)
, selection_end_(0)
, selection_mode_(kTextSelectionModeInvert)
, selection_color_1_(Rgba(0, 0, 0))
, selection_color_2_(Rgba(255, 255, 255))
, is_digits_(false) 
, theme_(theme) {
  normal_ = theme->editbox_normal_.DrawExternalSize(size_);
  focused_ = theme->editbox_focused_.DrawExternalSize(size_);
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
  cursor_pos_ = std::min(std::max(0, cursor_pos_), (Si32)text.length());
  selection_begin_ = 0;
  selection_end_ = 0;
  display_pos_ = 0;
  text_ = text;
}

void Editbox::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 pos = parent_absolute_pos + pos_;
  if (is_current_tab_) {
    focused_.Draw(pos);
  } else {
    normal_.Draw(pos);
  }
  if (font_.FontInstance()) {
    Si32 border = std::max(0,
      (normal_.Height() - font_.FontInstance()->line_height_) / 2);
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
      if (available_width > 0) {
        while (w > available_width && display_pos_ < cursor_pos_) {
          display_pos_++;
          part = text_.substr(static_cast<size_t>(display_pos_),
                              static_cast<size_t>(cursor_pos_ - display_pos_));
          w = font_.EvaluateSize(part.c_str(), true).x;
          end_pos = std::min(cursor_pos_ + 1, (Si32)text_.length());
        }
      }
    }
    // Display part of text with start at the display_pos_.

    std::string display_text = text_.substr(static_cast<size_t>(display_pos_),
                                            static_cast<size_t>(end_pos - display_pos_));
    Si32 visible_width = font_.EvaluateSize(display_text.c_str(), false).x;
    if (available_width) {
      while (visible_width > displayable_width) {
        Si32 visible_len = (Si32)display_text.length();
        Si32 desired_len = Si32(Si64(visible_len) * Si64(displayable_width) / Si64(visible_width));
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
               origin_, alignment_, kDrawBlendingModeColorize, kFilterNearest, color_);

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
      Vec2Si32 size1;
      Vec2Si32 pos1 = font_.EvaluateCharacterPos(text_.c_str(), text_.c_str()+selection_begin_, origin_, alignment_, &size1);
      Vec2Si32 size2;
      const char *plast = text_.c_str()+selection_end_-1;
      while (plast > text_.c_str()+selection_begin_) {
        if (*plast <= 127 && *plast > 0 && *plast != '\n' && *plast != '\r') {
          break;
        }
        --plast;
      }
      Vec2Si32 pos2 = font_.EvaluateCharacterPos(text_.c_str(), plast, origin_, alignment_, &size2);

      Si32 x1 = pos.x + border + pos1.x;
      Si32 x2 = pos.x + border + pos2.x + size2.x;
      Si32 y1 = pos.y + border;
      Si32 y2 = pos.y + border + font_.FontInstance()->line_height_;
      Sprite backbuffer = GetEngine()->GetBackbuffer();

      x1 = std::min(std::max(pos.x + border, x1 - skip_x),
                    pos.x + border + displayable_width);
      x2 = std::min(std::max(pos.x + border, x2 - skip_x),
                    pos.x + border + displayable_width);

      DrawSelection(x1, y1, x2, y2, selection_mode_,
                    selection_color_1_, selection_color_2_, backbuffer);
    }
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

void Editbox::RegenerateSprites() {
  if (theme_) {
    normal_ = theme_->editbox_normal_.DrawExternalSize(size_);
    focused_ = theme_->editbox_focused_.DrawExternalSize(size_);
  }
}

void Editbox::SetIsDigits(bool is_digits) {
  is_digits_ = is_digits;
}



Scrollbar::Scrollbar(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
                     Sprite normal_background,
                     Sprite focused_background, Sprite normal_button_dec,
                     Sprite focused_button_dec, Sprite down_button_dec,
                     Sprite normal_button_inc, Sprite focused_button_inc,
                     Sprite down_button_inc, Sprite normal_button_cur,
                     Sprite focused_button_cur, Sprite down_button_cur,
                     Si32 min_value, Si32 max_value, Si32 value,
                     ScrollKind kind)
: Panel(tag, pos,
        Max(normal_background.Size(),
            focused_background.Size()),
        tab_order)
, normal_background_(normal_background)
, focused_background_(focused_background)
, normal_button_dec_(std::move(normal_button_dec))
, focused_button_dec_(std::move(focused_button_dec))
, down_button_dec_(std::move(down_button_dec))
, normal_button_inc_(std::move(normal_button_inc))
, focused_button_inc_(std::move(focused_button_inc))
, down_button_inc_(std::move(down_button_inc))
, normal_button_cur_(std::move(normal_button_cur))
, focused_button_cur_(std::move(focused_button_cur))
, down_button_cur_(std::move(down_button_cur))
, step_(5)
, min_value_(min_value)
, max_value_(max_value)
, value_(value)
, dir_(kind) {
}

Scrollbar::Scrollbar(Ui64 tag, std::shared_ptr<GuiThemeScrollbar> theme)
: Panel(tag, Vec2Si32(0, 0), (theme->is_horizontal_ ? Vec2Si32(150, 29) : Vec2Si32(29, 150)), Ui32(tag))
, theme_(theme) {
  normal_background_ = theme_->normal_background_.DrawExternalSize(size_);
  focused_background_ = theme_->focused_background_.DrawExternalSize(size_);
  normal_button_dec_ = theme_->normal_button_dec_;
  focused_button_dec_ = theme_->focused_button_dec_;
  down_button_dec_ = theme_->down_button_dec_;
  normal_button_inc_ = theme_->normal_button_inc_;
  focused_button_inc_ = theme_->focused_button_inc_;
  down_button_inc_ = theme_->down_button_inc_;
  normal_button_cur_ = theme_->normal_button_cur_;
  focused_button_cur_ = theme_->focused_button_cur_;
  down_button_cur_ = theme_->down_button_cur_;
  step_ = 5;
  min_value_ = 0;
  max_value_ = 100;
  value_ = 0;
  dir_ = (theme_->is_horizontal_ ? kScrollHorizontal : kScrollVertical);
}

void Scrollbar::ApplyInput(Vec2Si32 parent_pos,
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
  Panel::ApplyInput(parent_pos, message, is_top_level, in_out_is_applied,
                    out_gui_messages, out_current_tab);
  //    s1    s2  s3    s4
  // |--|-----|---|-----|--|
  // |<<|     | @ |     |>>|
  // |--|-----|---|-----|--|
  Si32 s1 = 1 + normal_button_dec_.Size()[dir_];
  Si32 s4 = size_[dir_] - 1 - normal_button_inc_.Size()[dir_];
  Si32 w = std::max(1, s4 - s1 - normal_button_cur_.Size()[dir_]);
  Si64 value_range = Si64(max_value_) - Si64(min_value_);
  Si32 s2 = Si32(Si64(s1) +
                 (value_range ? Si64(w) * (Si64(value_) - Si64(min_value_)) / value_range : 0));
  Si32 s3 = s2 + normal_button_cur_.Size()[dir_];

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
      Si32 drag_s = relative_pos[dir_] - start_relative_s_;
      Si32 value_diff = Si32(Si64(drag_s) * (Si64(max_value_) - Si64(min_value_)) / Si64(w));
      value_ = Clamp(start_value_ + value_diff, min_value_, max_value_);
      if (out_gui_messages) {
        out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
      }
      OnScrollChange();
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
          Si32 drag_s = relative_pos[dir_] - start_relative_s_;
          Si32 value_diff = Si32(Si64(drag_s) * (Si64(max_value_) - Si64(min_value_)) / Si64(w));
          value_ = Clamp(start_value_ + value_diff, min_value_, max_value_);
          if (out_gui_messages) {
            out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
          }
          OnScrollChange();
        } else if (relative_pos[dir_] < s1) {
          state_ = kDecDown;
          if (prev_state != state_) {
            value_ = std::max(min_value_, value_ - 1);
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
            }
            OnScrollChange();
          }
        } else if (relative_pos[dir_] < s2) {
          state_ = kDecFast;
          if (prev_state != state_) {
            value_ = std::max(min_value_, value_ - step_);
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
            }
            OnScrollChange();
          }
        } else if (relative_pos[dir_] < s3) {
          state_ = kMiddleDragged;
          if (prev_state != state_) {
            start_relative_s_ = relative_pos[dir_];
            start_value_ = value_;
          }
        } else if (relative_pos[dir_] < s4) {
          state_ = kIncFast;
          if (prev_state != state_) {
            value_ = std::min(max_value_, value_ + step_);
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
            }
            OnScrollChange();
          }
        } else {
          state_ = kIncDown;
          if (prev_state != state_) {
            value_ = std::min(max_value_, value_ + 1);
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
            }
            OnScrollChange();
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
          if (out_gui_messages) {
            out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
          }
          OnScrollChange();
        } else if (message.keyboard.key == kKeyRight) {
          value_ = Clamp(value_ + 1, min_value_, max_value_);
          if (out_gui_messages) {
            out_gui_messages->emplace_back(shared_from_this(), kGuiScrollChange);
          }
          OnScrollChange();
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

void Scrollbar::Draw(Vec2Si32 parent_absolute_pos) {
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
  if (state_ == kDecDown) {
    down_button_dec_.Draw(absolute_pos + button_offset);
  } else {
    normal_button_dec_.Draw(absolute_pos + button_offset);
  }
  Vec2Si32 inc_pos = absolute_pos +
  (dir_
   ? size_.oy() - normal_button_inc_.Size().oy() + Vec2Si32(0, -2)
   : size_.xo() - normal_button_inc_.Size().xo() + Vec2Si32(-2, 0));
  if (state_ == kIncDown) {
    down_button_inc_.Draw(inc_pos + button_offset);
  } else {
    normal_button_inc_.Draw(inc_pos + button_offset);
  }
  Vec2Si32 after_dec = absolute_pos +
  (dir_
   ? button_offset.oy() + normal_button_dec_.Size().oy()
   : button_offset.xo() + normal_button_dec_.Size().xo());
  Si32 length = inc_pos[dir_] - after_dec[dir_] - normal_button_cur_.Size()[dir_] + 1;
  Si32 offset = 0;
  if (length > 0 && max_value_ != min_value_) {
    offset = Si32(Si64(length) * (Si64(value_) - Si64(min_value_)) / (Si64(max_value_) - Si64(min_value_)));
  }
  Vec2Si32 cur_pos = after_dec + (dir_ ? Vec2Si32(1, offset) : Vec2Si32(offset, 1));
  if (state_ == kMiddleDragged) {
    down_button_cur_.Draw(cur_pos);
  } else {
    normal_button_cur_.Draw(cur_pos);
  }
  Panel::Draw(parent_absolute_pos);
}

void Scrollbar::SetStep(Si32 step) {
  step_ = std::max(1, step);
}

void Scrollbar::SetValue(Si32 value) {
  value_ = std::min(max_value_, std::max(min_value_, value));
}

Si32 Scrollbar::GetValue() const {
  return value_;
}

void Scrollbar::SetMinValue(Si32 min_value) {
  min_value_ = min_value;
  max_value_ = std::max(min_value_, max_value_);
  value_ = std::min(max_value_, std::max(min_value_, value_));
}

Si32 Scrollbar::GetMinValue() const {
  return min_value_;
}

void Scrollbar::SetMaxValue(Si32 max_value) {
  max_value_ = max_value;
  min_value_ = std::min(min_value_, max_value_);
  value_ = std::min(max_value_, std::max(min_value_, value_));
}

Si32 Scrollbar::GetMaxValue() const {
  return max_value_;
}

void Scrollbar::RegenerateSprites() {
  if (theme_) {
    normal_background_ = theme_->normal_background_.DrawExternalSize(size_);
    focused_background_ = theme_->focused_background_.DrawExternalSize(size_);
  }
}



Checkbox::Checkbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
                   Sprite clear_normal,
                   Sprite checked_normal,
                   Sprite clear_down,
                   Sprite checked_down,
                   Sprite clear_hovered,
                   Sprite checked_hovered,
                   Sprite clear_disabled,
                   Sprite checked_disabled,
                   Sound down_sound,
                   Sound up_sound,
                   KeyCode hotkey,
                   CheckboxValue value)
: Panel(tag,
        pos,
        Max(clear_normal.Size(), Max(clear_hovered.Size(), clear_down.Size())),
        tab_order)
, down_sound_(std::move(down_sound))
, up_sound_(std::move(up_sound))
, hotkey_(hotkey)
, value_(value) {
  normal_[0] = clear_normal;
  normal_[1] = checked_normal;
  down_[0] = clear_down;
  down_[1] = checked_down;
  hovered_[0] = clear_hovered;
  hovered_[1] = checked_hovered;
  disabled_[0] = clear_disabled;
  disabled_[1] = checked_disabled;
}

Checkbox::Checkbox(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(54, 54), (Ui32)tag)
, theme_(theme)
, down_sound_(theme->checkbox_down_sound_)
, up_sound_(theme->checkbox_up_sound_)
, hotkey_(kKeyNone)
, value_(Checkbox::kValueClear) {
  normal_[0] = theme_->checkbox_clear_normal_;
  normal_[1] = theme_->checkbox_checked_normal_;
  down_[0] = theme_->checkbox_clear_down_;
  down_[1] = theme_->checkbox_checked_down_;
  hovered_[0] = theme_->checkbox_clear_hovered_;
  hovered_[1] = theme_->checkbox_checked_hovered_;
  disabled_[0] = theme_->checkbox_clear_disabled_;
  disabled_[1] = theme_->checkbox_checked_disabled_;
  text_ = std::make_shared<Text>(0, theme->text_);
  text_->SetPos(Vec2Si32(normal_[0].Size().x, 0));
  text_->SetText("");
  Panel::AddChild(text_);
}

void Checkbox::Draw(Vec2Si32 parent_absolute_pos) {
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  switch (state_) {
  case kHidden:
    break;
  case kNormal:
    normal_[value_].Draw(absolute_pos);
    break;
  case kHovered:
    hovered_[value_].Draw(absolute_pos);
    break;
  case kDown:
    down_[value_].Draw(absolute_pos);
    break;
  case kDisabled:
    disabled_[value_].Draw(absolute_pos);
    break;
  }
  Panel::Draw(parent_absolute_pos);
}

void Checkbox::SetEnabled(bool is_enabled) {
  if (state_ == Checkbox::kHidden) {
    return;
  }
  if (text_) {
    text_->SetEnabled(is_enabled);
  }
  if (is_enabled) {
    if (state_ == Checkbox::kDisabled) {
      state_ = Checkbox::kNormal;
      return;
    }
  } else {
    if (state_ != Checkbox::kDisabled) {
      state_ = Checkbox::kDisabled;
      return;
    }
  }
}

void Checkbox::ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  if (state_ == kHidden || state_ == kDisabled) {
    return;
  }
  Check(in_out_is_applied,
    "ApplyInput must not be called with in_out_is_applied == nullptr");
  Panel::ApplyInput(parent_pos, message, is_top_level, in_out_is_applied,
    out_gui_messages, out_current_tab);
  CheckboxState prev_state = state_;
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
        value_ = (value_ == kValueClear ? kValueChecked : kValueClear);
        if (out_gui_messages) {
          out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
        }
        OnButtonClick();
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
        if (out_gui_messages) {
          out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
        }
        OnButtonDown();
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
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiButtonDown);
            }
            OnButtonDown();
          }
        } else {
          if (prev_state == kDown) {
            *in_out_is_applied = true;
            up_sound_.Play();
            value_ = (value_ == kValueClear ? kValueChecked : kValueClear);
            if (out_gui_messages) {
              out_gui_messages->emplace_back(shared_from_this(), kGuiButtonClick);
            }
            if (GetTabOrder() == 0 || !is_current_tab_) {
              state_ = kNormal;
            } else {
              state_ = kHovered;
            }
            OnButtonClick();
          }
        }
      }
    }
  }
}

void Checkbox::SetCurrentTab(bool is_current_tab) {
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

void Checkbox::SetVisible(bool is_visible) {
  if (Panel::IsVisible() != is_visible) {
    Panel::SetVisible(is_visible);
    if (Panel::IsVisible()) {
      state_ = kNormal;
    } else {
      state_ = kHidden;
    }
  }
}

bool Checkbox::IsVisible() {
  bool is_visible = Panel::IsVisible();
  bool should_be_visible = state_ != kHidden;
  Check(is_visible == should_be_visible,
      "Button visibility state inconsitency detected!");
  return is_visible;
}

bool Checkbox::IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) {
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

void Checkbox::SetChecked(bool is_checked) {
  value_ = (is_checked ? kValueChecked : kValueClear);
}

bool Checkbox::IsChecked() {
  return value_ == kValueChecked;
}

void Checkbox::SetText(std::string text) {
  if (text_) {
    text_->SetText(text);
    Vec2Si32 new_size = normal_[0].Size();
    new_size.x += text_->EvaluateSize().x;
    SetSize(new_size);
  }
}

void Checkbox::SetHotkey(KeyCode hotkey) {
  hotkey_ = hotkey;
}

struct LoaderContext {
  std::unordered_map<std::string, Sprite> atlas;
  std::string parent_path;
  pugi::XmlDocument doc;

  Sprite LoadSprite(std::string path) {
    auto it = atlas.find(path);
    if (it == atlas.end()) {
      Sprite s;
      s.Load(GluePath(parent_path.c_str(), path.c_str()));
      return s;
    }
    return it->second;
  }
};

void LoadDecoratedFrame(LoaderContext &ctx, const char* child_name, DecoratedFrame *out_decorated_frame) {
  pugi::XmlNode child = ctx.doc.child(child_name);
  if (!child.empty()) {
    out_decorated_frame->Split(ctx.LoadSprite(child.attribute("path").as_string("")),
                               child.attribute("border").as_int(0),
                               child.attribute("is_x_scaleable").as_bool(false),
                               child.attribute("is_y_scaleable").as_bool(false));
  }
}

namespace {

void LoadThemeFont(Font *font, const pugi::XmlNode &node,
                   const std::string &parent_path) {
  const char *system_name = node.attribute("system").as_string(nullptr);
  if (system_name) {
    float size = node.attribute("size").as_float(24.0f);
    const char *chars = node.attribute("chars").as_string(nullptr);
    Si32 index = node.attribute("index").as_int(0);
    font->LoadSystemFont(system_name, size, chars, index);
    return;
  }
  const char *path_str = node.attribute("path").as_string(nullptr);
  if (!path_str) {
    return;
  }
  std::string full_path = GluePath(parent_path.c_str(), path_str);
  const char *ext = strrchr(path_str, '.');
  if (ext && (StrCaseCmp(ext, ".ttf") == 0 || StrCaseCmp(ext, ".ttc") == 0)) {
    float size = node.attribute("size").as_float(24.0f);
    const char *chars = node.attribute("chars").as_string(nullptr);
    Si32 index = node.attribute("index").as_int(0);
    font->LoadTtf(full_path.c_str(), size, chars, index);
  } else {
    font->Load(full_path.c_str());
  }
}

}  // namespace

void GuiTheme::Load(const char *xml_file_path) {
  LoaderContext ctx;
  pugi::XmlParseResult parse_result = ctx.doc.load_file(xml_file_path);
  if (parse_result.status != pugi::status_ok) {
    std::stringstream str;
    str << "Error loading " << xml_file_path << " gui theme, at offset " << parse_result.offset << " (line " << parse_result.line;
    str << " column " << parse_result.column << ": " << parse_result.description();
    Fatal(str.str().c_str());
  }

  ctx.parent_path = ParentPath(xml_file_path);

  const char* atlas_name = ctx.doc.child("gui_texture_atlas").attribute("path").as_string(nullptr);
  if (atlas_name) {
    std::string atlas_path = GluePath(ctx.parent_path.c_str(), atlas_name);
    std::string atlas_parent_path = ParentPath(atlas_path.c_str());
    pugi::XmlDocument atlas_doc;
    pugi::XmlParseResult parse_result = atlas_doc.load_file(atlas_path.c_str());
    if (parse_result.status != pugi::status_ok) {
      std::stringstream str;
      str << "Error loading " << atlas_name << " sprite atlas, at offset " << parse_result.offset << " (line " << parse_result.line;
      str << " column " << parse_result.column << ": " << parse_result.description();
      Fatal(str.str().c_str());
    }
    Sprite atlas_sprite;
    atlas_sprite.Load(GluePath(atlas_parent_path.c_str(),
      atlas_doc.child("TextureAtlas").attribute("imagePath").as_string("TextureAtlas")));
    pugi::XmlObjectRange<pugi::XmlNodeIterator> items = atlas_doc.child("TextureAtlas").children();
    for (auto it = items.begin(); it != items.end(); ++it) {
      std::string name = it->attribute("n").as_string();
      Si32 x = it->attribute("x").as_int(0);
      Si32 y = it->attribute("y").as_int(0);
      Si32 w = it->attribute("w").as_int(0);
      Si32 h = it->attribute("h").as_int(0);
      if (name.length()) {
        ctx.atlas[name].Reference(atlas_sprite, x, atlas_sprite.Height() - y - h, w, h);
      }
    }
  }

  LoadDecoratedFrame(ctx, "panel_background", &panel_background_);

  text_ = std::make_shared<GuiThemeText>();
  LoadThemeFont(&text_->font_, ctx.doc.child("text_font"), ctx.parent_path);
  text_->origin_ = kTextOriginBottom;
  text_->alignment_ = kTextAlignmentLeft;
  text_->selection_mode_ = kTextSelectionModeInvert;
  text_->selection_color_1_ = Rgba(0, 0, 0);
  text_->selection_color_2_ = Rgba(255, 255, 255);

  button_ = std::make_shared<GuiThemeButton>();
  LoadDecoratedFrame(ctx, "button_normal", &button_->normal_);
  LoadDecoratedFrame(ctx, "button_down", &button_->down_);
  LoadDecoratedFrame(ctx, "button_hovered", &button_->hovered_);
  LoadDecoratedFrame(ctx, "button_disabled", &button_->disabled_);
  button_->down_sound_.Load(GluePath(ctx.parent_path.c_str(), ctx.doc.child("button_down_sound").attribute("path").as_string("button_down_sound")), true);
  button_->up_sound_.Load(GluePath(ctx.parent_path.c_str(), ctx.doc.child("button_up_sound").attribute("path").as_string("button_up_sound")), true);

  button_->text_ = std::make_shared<GuiThemeText>();
  button_->text_->font_ = text_->font_;
  button_->text_->origin_ = kTextOriginBottom;

  for (auto it = ctx.doc.child("text_palete").children().begin(); it != ctx.doc.child("text_palete").children().end(); ++it) {
    Rgba rgb(it->attribute("r").as_uint(255),
      it->attribute("g").as_uint(255),
      it->attribute("b").as_uint(255));
    button_->text_->palete_.push_back(rgb);
  }
  if (button_->text_->palete_.empty()) {
    button_->text_->palete_ = {Rgba(255, 255, 255), Rgba(128, 255, 128)};
  }
  button_->text_->disabled_palete_ = {Rgba(128, 128, 128), Rgba(64, 128, 64)};
  button_->text_->alignment_ = kTextAlignmentCenter;
  button_->text_->selection_mode_ = kTextSelectionModeInvert;
  button_->text_->selection_color_1_ = Rgba(0, 0, 0);
  button_->text_->selection_color_2_ = Rgba(255, 255, 255);

  LoadDecoratedFrame(ctx, "progressbar_incomplete", &progressbar_incomplete_);
  LoadDecoratedFrame(ctx, "progressbar_complete", &progressbar_complete_);

  editbox_text_ = std::make_shared<GuiThemeText>();
  LoadThemeFont(&editbox_text_->font_, ctx.doc.child("editbox_font"),
    ctx.parent_path);
  editbox_text_->origin_ = kTextOriginBottom;
  for (auto it = ctx.doc.child("text_palete").children().begin(); it != ctx.doc.child("text_palete").children().end(); ++it) {
    Rgba rgb(it->attribute("r").as_uint(255),
      it->attribute("g").as_uint(255),
      it->attribute("b").as_uint(255));
    editbox_text_->palete_.push_back(rgb);
  }
  if (editbox_text_->palete_.empty()) {
    editbox_text_->palete_ = {Rgba(255, 255, 255), Rgba(128, 255, 128)};
  }
  editbox_text_->disabled_palete_ = {Rgba(128, 128, 128), Rgba(64, 128, 64)};
  editbox_text_->alignment_ = kTextAlignmentLeft;
  editbox_text_->selection_mode_ = kTextSelectionModeInvert;
  editbox_text_->selection_color_1_ = Rgba(0, 0, 0);
  editbox_text_->selection_color_2_ = Rgba(255, 255, 255);

  LoadDecoratedFrame(ctx, "editbox_normal", &editbox_normal_);
  LoadDecoratedFrame(ctx, "editbox_focused", &editbox_focused_);


  
  text_->origin_ = kTextOriginBottom;
  for (auto it = ctx.doc.child("text_palete").children().begin(); it != ctx.doc.child("text_palete").children().end(); ++it) {
    Rgba rgb(it->attribute("r").as_uint(255),
      it->attribute("g").as_uint(255),
      it->attribute("b").as_uint(255));
    text_->palete_.push_back(rgb);
  }
  if (text_->palete_.empty()) {
    text_->palete_ = {Rgba(255, 255, 255), Rgba(128, 128, 128)};

  }
  for (auto it = ctx.doc.child("disabled_palete").children().begin(); it != ctx.doc.child("disabled_palete").children().end(); ++it) {
    Rgba rgb(it->attribute("r").as_uint(255),
      it->attribute("g").as_uint(255),
      it->attribute("b").as_uint(255));
    text_->disabled_palete_.push_back(rgb);
  }
  if (text_->disabled_palete_.empty()) {
    text_->disabled_palete_ = {Rgba(128, 128, 128), Rgba(64, 128, 64)};
  }
  text_->alignment_ = kTextAlignmentLeft;
  text_->selection_mode_ = kTextSelectionModeInvert;
  text_->selection_color_1_ = Rgba(0, 0, 0);
  text_->selection_color_2_ = Rgba(255, 255, 255);

  h_scrollbar_ = std::make_shared<GuiThemeScrollbar>();
  LoadDecoratedFrame(ctx, "h_scrollbar_normal_background", &h_scrollbar_->normal_background_);
  LoadDecoratedFrame(ctx, "h_scrollbar_focused_background", &h_scrollbar_->focused_background_);
  LoadDecoratedFrame(ctx, "h_scrollbar_disabled_background", &h_scrollbar_->disabled_background_);
  h_scrollbar_->normal_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_normal_button_dec").attribute("path").as_string("h_scrollbar_normal_button_dec"));
  h_scrollbar_->focused_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_focused_button_dec").attribute("path").as_string("h_scrollbar_focused_button_dec"));
  h_scrollbar_->down_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_down_button_dec").attribute("path").as_string("h_scrollbar_down_button_dec"));
  h_scrollbar_->disabled_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_disabled_button_dec").attribute("path").as_string("h_scrollbar_disabled_button_dec"));
  h_scrollbar_->normal_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_normal_button_inc").attribute("path").as_string("h_scrollbar_normal_button_inc"));
  h_scrollbar_->focused_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_focused_button_inc").attribute("path").as_string("h_scrollbar_focused_button_inc"));
  h_scrollbar_->down_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_down_button_inc").attribute("path").as_string("h_scrollbar_down_button_inc"));
  h_scrollbar_->disabled_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_disabled_button_inc").attribute("path").as_string("h_scrollbar_disabled_button_inc"));
  h_scrollbar_->normal_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_normal_button_cur").attribute("path").as_string("h_scrollbar_normal_button_cur"));
  h_scrollbar_->focused_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_focused_button_cur").attribute("path").as_string("h_scrollbar_focused_button_cur"));
  h_scrollbar_->down_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_down_button_cur").attribute("path").as_string("h_scrollbar_down_button_cur"));
  h_scrollbar_->disabled_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("h_scrollbar_disabled_button_cur").attribute("path").as_string("h_scrollbar_disabled_button_cur"));
  h_scrollbar_->is_horizontal_ = true;

  v_scrollbar_ = std::make_shared<GuiThemeScrollbar>();
  LoadDecoratedFrame(ctx, "v_scrollbar_normal_background", &v_scrollbar_->normal_background_);
  LoadDecoratedFrame(ctx, "v_scrollbar_focused_background", &v_scrollbar_->focused_background_);
  LoadDecoratedFrame(ctx, "v_scrollbar_disabled_background", &v_scrollbar_->disabled_background_);
  v_scrollbar_->normal_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_normal_button_dec").attribute("path").as_string("v_scrollbar_normal_button_dec"));
  v_scrollbar_->focused_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_focused_button_dec").attribute("path").as_string("v_scrollbar_focused_button_dec"));
  v_scrollbar_->down_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_down_button_dec").attribute("path").as_string("v_scrollbar_down_button_dec"));
  v_scrollbar_->disabled_button_dec_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_disabled_button_dec").attribute("path").as_string("v_scrollbar_disabled_button_dec"));
  v_scrollbar_->normal_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_normal_button_inc").attribute("path").as_string("v_scrollbar_normal_button_inc"));
  v_scrollbar_->focused_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_focused_button_inc").attribute("path").as_string("v_scrollbar_focused_button_inc"));
  v_scrollbar_->down_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_down_button_inc").attribute("path").as_string("v_scrollbar_down_button_inc"));
  v_scrollbar_->disabled_button_inc_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_disabled_button_inc").attribute("path").as_string("v_scrollbar_disabled_button_inc"));
  v_scrollbar_->normal_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_normal_button_cur").attribute("path").as_string("v_scrollbar_normal_button_cur"));
  v_scrollbar_->focused_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_focused_button_cur").attribute("path").as_string("v_scrollbar_focused_button_cur"));
  v_scrollbar_->down_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_down_button_cur").attribute("path").as_string("v_scrollbar_down_button_cur"));
  v_scrollbar_->disabled_button_cur_ = ctx.LoadSprite(
    ctx.doc.child("v_scrollbar_disabled_button_cur").attribute("path").as_string("v_scrollbar_disabled_button_cur"));
  v_scrollbar_->is_horizontal_ = false;

  checkbox_clear_normal_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_clear_normal").attribute("path").as_string("checkbox_clear_normal"));
  checkbox_checked_normal_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_checked_normal").attribute("path").as_string("checkbox_checked_normal"));
  checkbox_clear_down_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_clear_down").attribute("path").as_string("checkbox_clear_down"));
  checkbox_checked_down_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_checked_down").attribute("path").as_string("checkbox_checked_down"));
  checkbox_clear_hovered_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_clear_hovered").attribute("path").as_string("checkbox_clear_hovered"));
  checkbox_checked_hovered_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_checked_hovered").attribute("path").as_string("checkbox_checked_hovered"));
  checkbox_clear_disabled_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_clear_disabled").attribute("path").as_string("checkbox_clear_disabled"));
  checkbox_checked_disabled_ = ctx.LoadSprite(
    ctx.doc.child("checkbox_checked_disabled").attribute("path").as_string("checkbox_checked_disabled"));
  checkbox_down_sound_.Load(GluePath(ctx.parent_path.c_str(),
    ctx.doc.child("checkbox_down_sound").attribute("path").as_string("checkbox_down_sound")), true);
  checkbox_up_sound_.Load(GluePath(ctx.parent_path.c_str(),
    ctx.doc.child("checkbox_up_sound").attribute("path").as_string("checkbox_up_sound")), true);
}

std::shared_ptr<Panel> GuiFactory::MakePanel() {
  ++last_tag_;
  return std::make_shared<Panel>(last_tag_, theme_);
}

std::shared_ptr<Panel> GuiFactory::MakeTransparentPanel() {
  ++last_tag_;
  Vec2Si32 size(64, 64);
  Sprite background;
  return std::make_shared<Panel>(last_tag_, Vec2Si32(0, 0), size, Ui32(last_tag_), background, false);
}

std::shared_ptr<Button> GuiFactory::MakeButton() {
  ++last_tag_;
  return std::make_shared<Button>(last_tag_, theme_->button_);
}

std::shared_ptr<Text> GuiFactory::MakeText() {
  ++last_tag_;
  return std::make_shared<Text>(last_tag_, theme_->text_);
}

std::shared_ptr<Progressbar> GuiFactory::MakeProgressbar() {
  ++last_tag_;
  return std::make_shared<Progressbar>(last_tag_, theme_);
}

std::shared_ptr<Scrollbar> GuiFactory::MakeHorizontalScrollbar() {
  ++last_tag_;
  return std::make_shared<Scrollbar>(last_tag_, theme_->h_scrollbar_);
}

std::shared_ptr<Scrollbar> GuiFactory::MakeVerticalScrollbar() {
  ++last_tag_;
  return std::make_shared<Scrollbar>(last_tag_, theme_->v_scrollbar_);
}

std::shared_ptr<Checkbox> GuiFactory::MakeCheckbox() {
  ++last_tag_;
  return std::make_shared<Checkbox>(last_tag_, theme_);
}

std::shared_ptr<Editbox> GuiFactory::MakeEditbox() {
  ++last_tag_;
  return std::make_shared<Editbox>(last_tag_, theme_);
}



}  // namespace arctic


