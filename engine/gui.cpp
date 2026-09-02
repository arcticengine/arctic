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
#include "engine/arctic_platform_sound.h"
#include "engine/scalar_math.h"

namespace arctic {

namespace {

// Everything drawn while a ClipScope lives lands inside one rectangle of the
// backbuffer only. The scope swaps the backbuffer of the engine for a view of
// that rectangle (a Sprite that shares the pixels), and restores it after.
// Positions drawn inside the scope must be shifted by -Origin(), because the
// view counts its pixels from its own bottom-left corner.
class ClipScope {
 public:
  ClipScope(Vec2Si32 pos, Vec2Si32 size) {
    Sprite &backbuffer = GetEngine()->GetBackbuffer();
    saved_ = backbuffer;
    Vec2Si32 low(std::max(pos.x, 0), std::max(pos.y, 0));
    Vec2Si32 high(std::min(pos.x + size.x, saved_.Width()),
                  std::min(pos.y + size.y, saved_.Height()));
    Vec2Si32 clip_size(std::max(0, high.x - low.x), std::max(0, high.y - low.y));
    origin_ = low;
    Sprite view;
    view.Reference(saved_, low, clip_size);
    backbuffer = view;
  }

  ~ClipScope() {
    GetEngine()->GetBackbuffer() = saved_;
  }

  // Absolute position of the bottom-left pixel of the view.
  Vec2Si32 Origin() const {
    return origin_;
  }

 private:
  Sprite saved_;
  Vec2Si32 origin_;
};

}  // namespace

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
  tooltip_theme_ = theme_->tooltip_;
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
      new_size.y = std::max(0, cur_size.y - pos_.y - anchor_top_d_);
    } else if (anchor_ & kAnchorBottom) {
      pos_.y = anchor_bottom_d_;
    } else if (anchor_ & kAnchorTop) {
      pos_.y = cur_size.y - anchor_top_d_ - size_.y;
    }

    if ((anchor_ & kAnchorLeft) && (anchor_ & kAnchorRight)) {
      pos_.x = anchor_left_d_;
      new_size.x = std::max(0, cur_size.x - pos_.x - anchor_right_d_);
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
  if (anchor_) {
    // The anchor keeps the distances to the parent edges, and the new place is
    // the one to keep from now on.
    SetAnchor(anchor_);
  }
}

void Panel::SetPos(Si32 x, Si32 y) {
  SetPos(Vec2Si32(x, y));
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
  if (is_clipping_children_) {
    ClipScope clip(absolute_pos, size_);
    Vec2Si32 shifted_pos = absolute_pos - clip.Origin();
    for (auto it = children_.begin(); it != children_.end(); ++it) {
      (**it).Draw(shifted_pos);
    }
  } else {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
      (**it).Draw(absolute_pos);
    }
  }
  if (parent_ == nullptr) {
    // The root draws last what must lie above everything: open popups and the
    // tooltip. A panel drawn on its own is a root in this sense.
    DrawOverlays(absolute_pos);
    DrawTooltip(absolute_pos);
  }
}

const std::deque<std::shared_ptr<Panel>> &Panel::GetChildren() const {
  return children_;
}

Panel *Panel::GetParent() const {
  return parent_;
}

void Panel::SetClipChildren(bool is_clipping) {
  is_clipping_children_ = is_clipping;
}

bool Panel::IsClippingChildren() const {
  return is_clipping_children_;
}

void Panel::SetTooltip(std::string tooltip) {
  tooltip_ = std::move(tooltip);
}

const std::string &Panel::GetTooltip() const {
  return tooltip_;
}

void Panel::SetTooltipTheme(std::shared_ptr<GuiThemeTooltip> theme) {
  tooltip_theme_ = std::move(theme);
}

void Panel::DrawOverlays(Vec2Si32 absolute_pos) {
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    if ((**it).IsVisible()) {
      (**it).DrawOverlays(absolute_pos + (**it).pos_);
    }
  }
}

void Panel::HandleOverlayInput(Vec2Si32 absolute_pos,
                               const InputMessage &message,
                               bool *in_out_is_applied,
                               std::deque<GuiMessage> *out_gui_messages,
                               std::shared_ptr<Panel> *out_current_tab) {
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if ((**it).IsVisible() && (**it).IsEnabled()) {
      (**it).HandleOverlayInput(absolute_pos + (**it).pos_, message,
                                in_out_is_applied, out_gui_messages,
                                out_current_tab);
    }
  }
}

bool Panel::IsOverlayTransparentAt(Vec2Si32 absolute_pos,
                                   Vec2Si32 mouse_pos) {
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if ((**it).IsVisible() &&
        !(**it).IsOverlayTransparentAt(absolute_pos + (**it).pos_,
                                       mouse_pos)) {
      return false;
    }
  }
  return true;
}

Panel *Panel::FindTooltipOwnerAt(Vec2Si32 absolute_pos, Vec2Si32 mouse_pos) {
  if (!IsVisible() || !IsWithin(mouse_pos - absolute_pos)) {
    return nullptr;
  }
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    Panel *owner = (**it).FindTooltipOwnerAt(absolute_pos + (**it).pos_,
                                             mouse_pos);
    if (owner) {
      return owner;
    }
  }
  return tooltip_.empty() ? nullptr : this;
}

void Panel::TrackTooltip(Vec2Si32 absolute_pos, const InputMessage &message) {
  if (message.kind != InputMessage::kMouse) {
    if (message.kind == InputMessage::kKeyboard &&
        (message.keyboard.key_state & 1u) == 1u) {
      // A key press hides the tooltip until the cursor moves again.
      tooltip_owner_.reset();
    }
    return;
  }
  Panel *owner = nullptr;
  if (!IsOverlayTransparentAt(absolute_pos, message.mouse.backbuffer_pos)) {
    // An open popup covers the panel under it, and popups carry no tooltips.
    owner = nullptr;
  } else {
    owner = FindTooltipOwnerAt(absolute_pos, message.mouse.backbuffer_pos);
  }
  if (owner == this) {
    // The root itself may live on the stack, and a weak_ptr to it would not
    // be safe; only the children are owners.
    owner = nullptr;
  }
  std::shared_ptr<Panel> current = tooltip_owner_.lock();
  bool is_button_press = message.keyboard.key != kKeyNone &&
      (message.keyboard.key_state & 1u) == 1u;
  if (owner == nullptr) {
    tooltip_owner_.reset();
    return;
  }
  if (current.get() != owner || is_button_press) {
    // A new panel or a click restarts the delay; a move over the same panel
    // only follows the cursor.
    tooltip_owner_ = owner->shared_from_this();
    tooltip_since_ = Time();
  }
  tooltip_pos_ = message.mouse.backbuffer_pos;
}

void Panel::DrawTooltip(Vec2Si32 absolute_pos) {
  if (!tooltip_theme_ || !tooltip_theme_->font_.FontInstance()) {
    return;
  }
  std::shared_ptr<Panel> owner = tooltip_owner_.lock();
  if (!owner || owner->tooltip_.empty() || !owner->IsReachableForInput()) {
    return;
  }
  if (Time() - tooltip_since_ < tooltip_theme_->delay_seconds_) {
    return;
  }
  GuiThemeTooltip &t = *tooltip_theme_;
  Vec2Si32 text_size = t.font_.EvaluateSize(owner->tooltip_.c_str(), false);
  Vec2Si32 client(text_size.x + t.padding_ * 2, text_size.y + t.padding_ * 2);
  Sprite frame = t.frame_.DrawClientSize(client);
  Vec2Si32 size = frame.Size();
  if (size.x <= 0 || size.y <= 0) {
    size = client;
  }
  // Above and to the right of the cursor, pushed back inside the root.
  Vec2Si32 pos = tooltip_pos_ + Vec2Si32(12, 16);
  Vec2Si32 root_low = absolute_pos;
  Vec2Si32 root_high = absolute_pos + size_;
  if (pos.x + size.x > root_high.x) {
    pos.x = std::max(root_low.x, root_high.x - size.x);
  }
  if (pos.y + size.y > root_high.y) {
    pos.y = std::max(root_low.y, tooltip_pos_.y - 8 - size.y);
  }
  frame.Draw(pos, size);
  Vec2Si32 border = (size - client) / 2;
  t.font_.Draw(owner->tooltip_.c_str(),
               pos.x + border.x + t.padding_,
               pos.y + border.y + t.padding_ + text_size.y,
               kTextOriginTop, kTextAlignmentLeft,
               kDrawBlendingModeColorize, kFilterNearest, t.color_);
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
  Check(out_current_tab,
        "ApplyInput must not be called with out_current_tab == nullptr");
  if (!IsVisible() || !IsEnabled()) {
    return;
  }
  HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
              out_gui_messages, out_current_tab);
}

bool Panel::IsWithin(Vec2Si32 relative_pos) const {
  return relative_pos.x >= 0 && relative_pos.y >= 0 &&
    relative_pos.x < size_.x && relative_pos.y < size_.y;
}

bool Panel::TakesMouse() const {
  return is_clickable_;
}

void Panel::Emit(GuiMessageKind kind,
                 std::deque<GuiMessage> *out_gui_messages) {
  if (out_gui_messages) {
    out_gui_messages->emplace_back(shared_from_this(), kind);
  }
}

void Panel::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                        bool is_top_level,
                        bool *in_out_is_applied,
                        std::deque<GuiMessage> *out_gui_messages,
                        std::shared_ptr<Panel> *out_current_tab) {
  Vec2Si32 pos = parent_pos + pos_;
  if (is_top_level) {
    // Open popups lie above the tree, so they are asked first.
    HandleOverlayInput(pos, message, in_out_is_applied, out_gui_messages,
                       out_current_tab);
    TrackTooltip(pos, message);
  }
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    (**it).ApplyInput(pos, message, false, in_out_is_applied,
                      out_gui_messages, out_current_tab);
  }
  if (!*in_out_is_applied &&
      is_clickable_ &&
      message.kind == InputMessage::kMouse &&
      IsWithin(message.mouse.backbuffer_pos - pos)) {
    if (message.keyboard.key == kKeyMouseLeft &&
        message.keyboard.key_state == 1) {
      Emit(kGuiPanelLeftDown, out_gui_messages);
      OnPanelLeftDown();
      *in_out_is_applied = true;
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
    if (!(*it)->IsVisible()) {
      // Nothing inside a hidden panel can be seen or typed into, so the focus
      // has no business there.
      continue;
    }
    Ui32 order = (*it)->GetTabOrder();
    if (order != 0 && current_tab_order != order && (*it)->IsEnabled()) {
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

bool Panel::IsFocused() const {
  return is_current_tab_;
}

bool Panel::IsKeyboardCapturing() const {
  return false;
}

bool Panel::IsReachableForInput() {
  for (Panel *panel = this; panel != nullptr; panel = panel->parent_) {
    if (!panel->IsVisible()) {
      return false;
    }
  }
  return true;
}

bool Panel::IsKeyboardCaptured() {
  if (is_current_tab_ && IsKeyboardCapturing() && IsReachableForInput()) {
    return true;
  }
  Panel *tab = FindCurrentTab();
  // A panel hidden while it had the focus keeps the flag, and it can not be
  // typed into, so it does not hold the keyboard either.
  return tab != nullptr && tab->IsKeyboardCapturing()
    && tab->IsReachableForInput();
}

void Panel::AddChild(std::shared_ptr<Panel> child) {
  Check(!!child, "AddChild called with child == nullptr");
  Check(child.get() != this, "AddChild called with child == this");
  Check(child->parent_ == nullptr, "AddChild called with child that already has a parent");
  children_.push_back(child);
  child->BecomeChild(this);
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

bool Panel::IsInside(Vec2Si32 backbuffer_pos) {
  return !IsMouseTransparentAt(Vec2Si32(0, 0), backbuffer_pos);
}

bool Panel::IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) {
  if (!is_visible_) {
    return true;
  }
  Vec2Si32 pos = parent_pos + pos_;
  if (parent_ == nullptr && !IsOverlayTransparentAt(pos, mouse_pos)) {
    return false;
  }
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (!(**it).IsMouseTransparentAt(pos, mouse_pos)) {
      return false;
    }
  }
  return !(TakesMouse() && IsWithin(mouse_pos - pos));
}

void Panel::SetEnabled(bool) {
}

bool Panel::IsEnabled() {
  return true;
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
    // An anchor set before the panel had a parent is measured now, from where
    // the panel stands at this moment.
    SetAnchor(anchor_);
  } else if (dock_) {
    ParentSizeChanged(parent_->GetSize(), parent_->GetSize());
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
    if (parent_) {
      // A dock is a place, not a rule for the next resize: take it now.
      ParentSizeChanged(parent_->GetSize(), parent_->GetSize());
    }
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
      if (disabled_.Width() > 0) {
        disabled_.Draw(absolute_pos);
      } else {
        normal_.Draw(absolute_pos);
      }
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

bool Button::IsEnabled() {
  return state_ != Button::kDisabled && state_ != Button::kHidden;
}

void Button::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                         bool is_top_level,
                         bool *in_out_is_applied,
                         std::deque<GuiMessage> *out_gui_messages,
                         std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  ButtonState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    bool is_inside = IsWithin(message.mouse.backbuffer_pos - pos);
    if (is_inside && !*in_out_is_applied) {
      *out_current_tab = Panel::Invalid();
      is_current_tab_ = false;
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        state_ = kDown;
        *in_out_is_applied = true;
      } else {
        state_ = kHovered;
        Emit(kGuiButtonHover, out_gui_messages);
        *in_out_is_applied = true;
      }
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 2 &&
          prev_state == kDown) {
        *in_out_is_applied = true;
        up_sound_.Play(GetGuiSoundVolume());
        Emit(kGuiButtonClick, out_gui_messages);
        OnButtonClick();
      }
    } else {
      if (is_current_tab_) {
        state_ = kHovered;
        Emit(kGuiButtonHover, out_gui_messages);
      } else {
        state_ = kNormal;
      }
    }
    if (state_ != prev_state) {
      if (state_ == kDown) {
        down_sound_.Play(GetGuiSoundVolume());
        Emit(kGuiButtonDown, out_gui_messages);
        OnButtonDown();
      }
      if (prev_state == kDown) {
        up_sound_.Play(GetGuiSoundVolume());
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
            down_sound_.Play(GetGuiSoundVolume());
            state_ = kDown;
            if (is_hotkey && GetTabOrder() != 0) {
              *out_current_tab = shared_from_this();
            }
            Emit(kGuiButtonDown, out_gui_messages);
            OnButtonDown();
          }
        } else {
          if (prev_state == kDown) {
            *in_out_is_applied = true;
            up_sound_.Play(GetGuiSoundVolume());
            Emit(kGuiButtonClick, out_gui_messages);
            OnButtonClick();
            if (GetTabOrder() == 0 || !is_current_tab_) {
              state_ = kNormal;
            } else {
              state_ = kHovered;
              Emit(kGuiButtonHover, out_gui_messages);
            }
          }
        }
      }
    }
  }
}

bool Button::TakesMouse() const {
  return true;
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
    case kTextSelectionModeBlend: {
      Si32 a = c1.a;
      Si32 inv = 255 - a;
      for (Si32 y = y1; y < y2; ++y) {
        Rgba *p = backbuffer.RgbaData() + backbuffer.StridePixels() * y;
        for (Si32 x = x1; x < x2; ++x) {
          Rgba &c = p[x];
          c.r = (Ui8)((c1.r * a + c.r * inv) / 255);
          c.g = (Ui8)((c1.g * a + c.g * inv) / 255);
          c.b = (Ui8)((c1.b * a + c.b * inv) / 255);
        }
      }
      break;
    }
  }
}

Vec2Si32 Text::EvaluateSize() {
  return font_.EvaluateSize(ShownText().c_str(), false);
}

void Text::SetWordWrap(bool word_wrap) {
  word_wrap_ = word_wrap;
}

bool Text::IsWordWrap() const {
  return word_wrap_;
}

std::string Text::ShownText() const {
  if (!word_wrap_ || !font_.FontInstance() || size_.x <= 0) {
    return text_;
  }
  std::vector<WrappedTextLine> lines = WrapText(font_, text_, size_.x);
  std::string shown;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) {
      shown += '\n';
    }
    shown += lines[i].text;
  }
  return shown;
}

Si32 Text::ToShownOffset(Si32 offset) const {
  if (!word_wrap_ || !font_.FontInstance() || size_.x <= 0) {
    return offset;
  }
  // The shown text is the source with a '\n' put in at every soft break, so
  // an offset moves right by the number of soft breaks before it.
  std::vector<WrappedTextLine> lines = WrapText(font_, text_, size_.x);
  Si32 shown = offset;
  for (size_t i = 0; i + 1 < lines.size(); ++i) {
    const WrappedTextLine &ln = lines[i];
    bool is_hard_break = ln.end > ln.start + (Si32)ln.text.size();
    if (!is_hard_break && ln.end <= offset) {
      ++shown;
    }
  }
  return shown;
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
  Vec2Si32 offset(0, 0);

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
  const std::string shown = ShownText();
  if (!palete_.empty()) {
    font_.Draw(shown.c_str(), absolute_pos.x, absolute_pos.y,
               origin_, alignment_, kDrawBlendingModeColorize, kFilterNearest,
               (is_enabled_ || !theme_ || theme_->disabled_palete_.empty()) ? palete_ : theme_->disabled_palete_);
  } else {
    font_.Draw(shown.c_str(), absolute_pos.x, absolute_pos.y,
               origin_, alignment_, kDrawBlendingModeColorize, kFilterNearest, color_);
  }

  if (selection_begin_ != selection_end_) {
    const Si32 shown_begin = ToShownOffset(selection_begin_);
    const Si32 shown_end = ToShownOffset(selection_end_);
    Vec2Si32 size1;
    Vec2Si32 pos1 = font_.EvaluateCharacterPos(shown.c_str(), shown.c_str()+shown_begin, origin_, alignment_, &size1);
    Vec2Si32 size2;
    const char *plast = shown.c_str()+shown_end-1;
    while (plast > shown.c_str()+shown_begin) {
      if (*plast <= 127 && *plast > 0 && *plast != '\n' && *plast != '\r') {
        break;
      }
      --plast;
    }
    Vec2Si32 pos2 = font_.EvaluateCharacterPos(shown.c_str(), plast, origin_, alignment_, &size2);

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
  if (!is_visible_) {
    return;
  }
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

void Editbox::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                          bool is_top_level, bool *in_out_is_applied,
                          std::deque<GuiMessage> *out_gui_messages,
                          std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  // Remembered so that any caret / text / selection change made below can
  // restart the "caret is solid" cooldown from a single place.
  const Si32 cursor_was = cursor_pos_;
  const Si32 selection_begin_was = selection_begin_;
  const Si32 selection_end_was = selection_end_;
  const size_t length_was = text_.length();
  // The text only changes when the keyboard talks to a focused box, and that is
  // the only case worth copying the text for. A paste can replace a selection
  // with a string of the same length, so the length alone is not an answer.
  const bool may_change_text =
    is_current_tab_ && message.kind == InputMessage::kKeyboard;
  std::string text_was;
  if (may_change_text) {
    text_was = text_;
  }
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    if (IsWithin(relative_pos) && !*in_out_is_applied) {
      *out_current_tab = shared_from_this();
      is_current_tab_ = true;
      *in_out_is_applied = true;
      // Left-button press positions the caret and starts a drag selection.
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1 && font_.FontInstance()) {
        Si32 caret = CaretFromPoint(relative_pos);
        MoveCursorTo(caret, message.keyboard.state[kKeyShift] != 0);
        is_dragging_ = true;
        edit_group_ = kEditGroupNone;
      }
    }
    // While the button stays down, extend the selection to the pointer.
    if (is_dragging_ && font_.FontInstance()) {
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        Si32 caret = CaretFromPoint(relative_pos);
        MoveCursorTo(caret, true);
        *in_out_is_applied = true;
      } else {
        is_dragging_ = false;
      }
    }
  }
  if (!*in_out_is_applied && is_current_tab_) {
    // Edit the text
    if (message.kind == InputMessage::kKeyboard) {
      if (message.keyboard.key == kKeyTab) {
        // Tab moves the focus and never becomes text. The top level of the panel
        // tree has already had its say about the focus by now, see
        // Panel::ApplyInput, and a box that is the only one to hold the focus
        // used to swallow the key and insert a tabulation instead, which is not
        // what a user pressing Tab in a form is asking for.
        return;
      }
      if (message.keyboard.key_state == 1) {
        Ui32 key = message.keyboard.key;
        bool ctrl = message.keyboard.state[kKeyControl] != 0;
        bool shift = message.keyboard.state[kKeyShift] != 0;
        // A read-only box keeps the caret, the selection and Ctrl+C; whatever
        // would change the text is not taken, so the host still sees the key.
        const bool can_edit = !is_read_only_;
        if (can_edit && ctrl && key == kKeyZ && !shift) {
          *in_out_is_applied = true;
          Undo();
        } else if (can_edit && ctrl && (key == kKeyY || (key == kKeyZ && shift))) {
          *in_out_is_applied = true;
          Redo();
        } else if (ctrl && key == kKeyA) {
          *in_out_is_applied = true;
          selection_begin_ = 0;
          selection_end_ = (Si32)text_.length();
          cursor_pos_ = (Si32)text_.length();
          edit_group_ = kEditGroupNone;
        } else if (ctrl && key == kKeyC) {
          *in_out_is_applied = true;
          if (selection_begin_ != selection_end_) {
            SetClipboardText(text_.substr(
                static_cast<size_t>(selection_begin_),
                static_cast<size_t>(selection_end_ - selection_begin_)));
          }
          edit_group_ = kEditGroupNone;
        } else if (can_edit && ctrl && key == kKeyX) {
          *in_out_is_applied = true;
          if (selection_begin_ != selection_end_) {
            SetClipboardText(text_.substr(
                static_cast<size_t>(selection_begin_),
                static_cast<size_t>(selection_end_ - selection_begin_)));
            SnapshotBefore(kEditGroupDiscrete);
            EraseSelection();
          }
        } else if (can_edit && ctrl && key == kKeyV) {
          *in_out_is_applied = true;
          std::string paste = GetClipboardText();
          std::string norm;
          norm.reserve(paste.size());
          for (size_t i = 0; i < paste.size(); ++i) {
            char c = paste[i];
            if (c == '\r' || c == '\n') {
              // A field that does not take newlines from the keyboard should not
              // take them from the clipboard either: turn them into spaces so
              // that words are not glued together.
              if (is_multiline_ && accepts_return_) {
                norm += '\n';
              } else {
                norm += ' ';
              }
              if (c == '\r' && i + 1 < paste.size() && paste[i + 1] == '\n') {
                ++i;
              }
            } else {
              norm += c;
            }
          }
          paste.swap(norm);
          FilterAllowedInPlace(&paste);
          if (max_length_ > 0) {
            Si32 room = max_length_ - ((Si32)text_.length() -
                (selection_end_ - selection_begin_));
            if (room < 0) {
              room = 0;
            }
            if ((Si32)paste.length() > room) {
              paste.resize(static_cast<size_t>(room));
            }
          }
          if (!paste.empty()) {
            SnapshotBefore(kEditGroupDiscrete);
            if (selection_begin_ != selection_end_) {
              EraseSelection();
            }
            text_.insert(static_cast<size_t>(cursor_pos_), paste);
            cursor_pos_ += (Si32)paste.length();
          }
          edit_group_ = kEditGroupNone;
        } else if (can_edit && key == kKeyBackspace) {
          *in_out_is_applied = true;
          if (text_.length()) {
            if (selection_begin_ != selection_end_) {
              SnapshotBefore(kEditGroupDelete);
              EraseSelection();
            } else if (cursor_pos_) {
              SnapshotBefore(kEditGroupDelete);
              Si32 prev = Utf8PrevCharPos(text_, cursor_pos_);
              text_.erase(static_cast<size_t>(prev),
                static_cast<size_t>(cursor_pos_ - prev));
              cursor_pos_ = prev;
            }
          }
        } else if (can_edit && key == kKeyDelete) {
          *in_out_is_applied = true;
          if (text_.length()) {
            if (selection_begin_ != selection_end_) {
              SnapshotBefore(kEditGroupDelete);
              EraseSelection();
            } else if (cursor_pos_ >= 0 && cursor_pos_ < (Si32)text_.length()) {
              SnapshotBefore(kEditGroupDelete);
              Si32 next = Utf8NextCharPos(text_, cursor_pos_);
              text_.erase(static_cast<size_t>(cursor_pos_),
                static_cast<size_t>(next - cursor_pos_));
            }
          }
        } else if (key == kKeyLeft) {
          *in_out_is_applied = true;
          edit_group_ = kEditGroupNone;
          if (ctrl) {
            MoveCursorTo(PrevWordPos(cursor_pos_), shift);
          } else if (message.keyboard.state[kKeyShift]) {
            if (cursor_pos_) {
              Si32 prev = Utf8PrevCharPos(text_, cursor_pos_);
              if (selection_begin_ == selection_end_) {
                selection_end_ = cursor_pos_;
                cursor_pos_ = prev;
                selection_begin_ = cursor_pos_;
              } else if (selection_begin_ == cursor_pos_) {
                cursor_pos_ = prev;
                selection_begin_ = cursor_pos_;
              } else if (selection_end_ == cursor_pos_) {
                cursor_pos_ = prev;
                selection_end_ = cursor_pos_;
              }
            }
          } else {
            if (cursor_pos_) {
              cursor_pos_ = Utf8PrevCharPos(text_, cursor_pos_);
              selection_begin_ = cursor_pos_;
              selection_end_ = cursor_pos_;
            }
          }
        } else if (key == kKeyRight) {
          *in_out_is_applied = true;
          edit_group_ = kEditGroupNone;
          if (ctrl) {
            MoveCursorTo(NextWordPos(cursor_pos_), shift);
          } else if (message.keyboard.state[kKeyShift]) {
            if (cursor_pos_ < (Si32)text_.length()) {
              Si32 next = Utf8NextCharPos(text_, cursor_pos_);
              if (selection_begin_ == selection_end_) {
                selection_begin_ = cursor_pos_;
                cursor_pos_ = next;
                selection_end_ = cursor_pos_;
              } else if (selection_end_ == cursor_pos_) {
                cursor_pos_ = next;
                selection_end_ = cursor_pos_;
              } else if (selection_begin_ == cursor_pos_) {
                cursor_pos_ = next;
                selection_begin_ = cursor_pos_;
              }
            }
          } else {
            if (cursor_pos_ < (Si32)text_.length()) {
              cursor_pos_ = Utf8NextCharPos(text_, cursor_pos_);
              selection_begin_ = cursor_pos_;
              selection_end_ = cursor_pos_;
            }
          }
        } else if (key == kKeyHome) {
          *in_out_is_applied = true;
          edit_group_ = kEditGroupNone;
          Si32 target = is_multiline_ ? LineStart(cursor_pos_) : 0;
          MoveCursorTo(target, shift);
        } else if (key == kKeyEnd) {
          *in_out_is_applied = true;
          edit_group_ = kEditGroupNone;
          Si32 target = is_multiline_ ?
            LineEnd(cursor_pos_) : (Si32)text_.length();
          MoveCursorTo(target, shift);
        } else if (key == kKeyUp || key == kKeyDown) {
          if (is_multiline_) {
            *in_out_is_applied = true;
            edit_group_ = kEditGroupNone;
            MoveCursorVertical(key == kKeyUp ? -1 : 1, shift);
          }
        } else if (key == kKeyEnter) {
          if (can_edit && is_multiline_ && accepts_return_ && !ctrl) {
            *in_out_is_applied = true;
            if (max_length_ <= 0 || (Si32)text_.length() -
                (selection_end_ - selection_begin_) < max_length_) {
              SnapshotBefore(kEditGroupDiscrete);
              if (selection_begin_ != selection_end_) {
                EraseSelection();
              }
              text_.insert(static_cast<size_t>(cursor_pos_), "\n");
              cursor_pos_ += 1;
              selection_begin_ = cursor_pos_;
              selection_end_ = cursor_pos_;
            }
          } else {
            // Single line, or AcceptsReturn off: Enter ends the editing and is
            // still left to the host, which may have a use of its own for it.
            Emit(kGuiEditboxEditDone, out_gui_messages);
            OnEditDone();
          }
        } else if (can_edit && !ctrl && (is_digits_ ? ((key >= kKey0 && key <= kKey9) ||
            (message.keyboard.characters[0] >= '0' &&
             message.keyboard.characters[0] <= '9')) : true)) {
          *in_out_is_applied = true;
          if (!message.keyboard.characters[0]) {
            if (key >= kKeyA && key <= kKeyZ) {
              if (!message.keyboard.state[kKeyShift]) {
                key = key - kKeyA + Ui32('a');
              }
            }
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
            Si32 add_len =
              static_cast<Si32>(strlen(message.keyboard.characters));
            if (do_insert && max_length_ > 0) {
              Si32 cur_len = (Si32)text_.length() -
                (selection_end_ - selection_begin_);
              if (cur_len + add_len > max_length_) {
                do_insert = false;
              }
            }
            if (do_insert) {
              SnapshotBefore(kEditGroupType);
              if (selection_begin_ != selection_end_) {
                EraseSelection();
              }
              text_.insert(static_cast<size_t>(cursor_pos_),
                           message.keyboard.characters);
              cursor_pos_ += add_len;
            }
          }
        }
      }
    }
  }
  cursor_pos_ = std::min(std::max(0, cursor_pos_), (Si32)text_.length());
  selection_begin_ = std::min(std::max(0, selection_begin_),
                              (Si32)text_.length());
  selection_end_ = std::min(std::max(0, selection_end_), (Si32)text_.length());
  if (cursor_pos_ != cursor_was || text_.length() != length_was ||
      selection_begin_ != selection_begin_was ||
      selection_end_ != selection_end_was) {
    TouchCaret();
  }
  if (may_change_text && text_ != text_was) {
    Emit(kGuiEditboxTextChange, out_gui_messages);
    OnTextChange();
  }
}

bool Editbox::IsKeyboardCapturing() const {
  return true;
}

bool Editbox::TakesMouse() const {
  return true;
}

void Editbox::SetCurrentTab(bool is_current_tab) {
  const bool was_current_tab = is_current_tab_;
  Panel::SetCurrentTab(is_current_tab);
  if (was_current_tab && !is_current_tab) {
    // Whatever the mouse was doing in the box is over, and the next edit starts
    // its own undo step rather than continuing the abandoned one.
    is_dragging_ = false;
    edit_group_ = kEditGroupNone;
    OnEditDone();
  }
}

void Editbox::SetText(std::string text) {
  cursor_pos_ = std::min(std::max(0, cursor_pos_), (Si32)text.length());
  selection_begin_ = 0;
  selection_end_ = 0;
  display_pos_ = 0;
  first_visible_line_ = 0;
  text_ = text;
  // A programmatic text change starts a fresh edit history.
  undo_.clear();
  redo_.clear();
  edit_group_ = kEditGroupNone;
  TouchCaret();
}

void Editbox::SetMultiline(bool is_multiline) {
  is_multiline_ = is_multiline;
}

bool Editbox::IsMultiline() const {
  return is_multiline_;
}

void Editbox::SetWordWrap(bool word_wrap) {
  word_wrap_ = word_wrap;
}

bool Editbox::IsWordWrap() const {
  return word_wrap_;
}

void Editbox::SetAcceptsReturn(bool accepts_return) {
  accepts_return_ = accepts_return;
}

bool Editbox::AcceptsReturn() const {
  return accepts_return_;
}

void Editbox::SetLineSpacing(Si32 line_spacing) {
  line_spacing_ = std::max(0, line_spacing);
}

void Editbox::SetMultilinePadding(Si32 padding) {
  multiline_padding_ = std::max(0, padding);
}

Si32 Editbox::LineStep() const {
  if (line_spacing_ > 0) {
    return line_spacing_;
  }
  if (!font_.FontInstance()) {
    return 1;
  }
  return std::max(1, font_.FontInstance()->line_height_);
}

void Editbox::SetMaxLength(Si32 max_length) {
  max_length_ = std::max(0, max_length);
}

// Seconds the caret stays solid after it moves, before it resumes blinking.
static const double kEditboxCaretSolidSeconds = 0.6;

void Editbox::TouchCaret() {
  caret_touch_time_ = Time();
}

bool Editbox::IsCaretVisible() const {
  double now = Time();
  if (now - caret_touch_time_ < kEditboxCaretSolidSeconds) {
    return true;
  }
  return fmod(now, 0.6) < 0.3;
}

void Editbox::EraseSelection() {
  if (selection_begin_ == selection_end_) {
    return;
  }
  text_.erase(static_cast<size_t>(selection_begin_),
              static_cast<size_t>(selection_end_ - selection_begin_));
  cursor_pos_ = selection_begin_;
  selection_end_ = selection_begin_;
}

void Editbox::FilterAllowedInPlace(std::string *s) const {
  if (is_digits_) {
    std::string out;
    for (char c : *s) {
      if (c >= '0' && c <= '9') {
        out += c;
      }
    }
    *s = out;
    return;
  }
  if (!allow_list_.empty()) {
    std::string out;
    Si32 i = 0;
    Si32 n = (Si32)s->size();
    while (i < n) {
      Si32 nxt = Utf8NextCharPos(*s, i);
      std::string ch = s->substr(static_cast<size_t>(i),
                                 static_cast<size_t>(nxt - i));
      Utf32Reader reader;
      reader.Reset(reinterpret_cast<const Ui8*>(ch.c_str()));
      Ui32 cp = reader.ReadOne();
      if (allow_list_.find(cp) != allow_list_.end()) {
        out += ch;
      }
      i = nxt;
    }
    *s = out;
  }
}

void Editbox::SnapshotBefore(Si32 group) {
  if (group != kEditGroupDiscrete && group == edit_group_) {
    return;
  }
  EditSnapshot snap;
  snap.text = text_;
  snap.cursor = cursor_pos_;
  snap.sel_begin = selection_begin_;
  snap.sel_end = selection_end_;
  undo_.push_back(snap);
  const size_t kMaxSteps = 128;
  if (undo_.size() > kMaxSteps) {
    undo_.erase(undo_.begin());
  }
  redo_.clear();
  edit_group_ = (group == kEditGroupDiscrete) ? kEditGroupNone : group;
}

void Editbox::Undo() {
  if (undo_.empty()) {
    return;
  }
  EditSnapshot cur;
  cur.text = text_;
  cur.cursor = cursor_pos_;
  cur.sel_begin = selection_begin_;
  cur.sel_end = selection_end_;
  redo_.push_back(cur);
  EditSnapshot snap = undo_.back();
  undo_.pop_back();
  text_ = snap.text;
  cursor_pos_ = std::min(std::max(0, snap.cursor), (Si32)text_.length());
  selection_begin_ = std::min(std::max(0, snap.sel_begin),
                              (Si32)text_.length());
  selection_end_ = std::min(std::max(0, snap.sel_end), (Si32)text_.length());
  edit_group_ = kEditGroupNone;
}

void Editbox::Redo() {
  if (redo_.empty()) {
    return;
  }
  EditSnapshot cur;
  cur.text = text_;
  cur.cursor = cursor_pos_;
  cur.sel_begin = selection_begin_;
  cur.sel_end = selection_end_;
  undo_.push_back(cur);
  EditSnapshot snap = redo_.back();
  redo_.pop_back();
  text_ = snap.text;
  cursor_pos_ = std::min(std::max(0, snap.cursor), (Si32)text_.length());
  selection_begin_ = std::min(std::max(0, snap.sel_begin),
                              (Si32)text_.length());
  selection_end_ = std::min(std::max(0, snap.sel_end), (Si32)text_.length());
  edit_group_ = kEditGroupNone;
}

void Editbox::MoveCursorTo(Si32 target, bool shift) {
  target = std::min(std::max(0, target), (Si32)text_.length());
  if (shift) {
    Si32 anchor;
    if (selection_begin_ == selection_end_) {
      anchor = cursor_pos_;
    } else if (cursor_pos_ == selection_end_) {
      anchor = selection_begin_;
    } else {
      anchor = selection_end_;
    }
    cursor_pos_ = target;
    selection_begin_ = std::min(anchor, target);
    selection_end_ = std::max(anchor, target);
  } else {
    cursor_pos_ = target;
    selection_begin_ = target;
    selection_end_ = target;
  }
}

Si32 Editbox::MultilineInnerWidth() const {
  return std::max(1, size_.x - multiline_padding_ * 2);
}

Si32 Editbox::PrefixWidth(const std::string &line, Si32 bytes) {
  if (bytes <= 0) {
    return 0;
  }
  if (bytes >= (Si32)line.size()) {
    return font_.EvaluateSize(line.c_str(), true).x;
  }
  return font_.EvaluateSize(
      line.substr(0, static_cast<size_t>(bytes)).c_str(), true).x;
}

std::vector<WrappedTextLine> WrapText(Font font, const std::string &text,
                                      Si32 max_width) {
  std::vector<WrappedTextLine> lines;
  const Si32 n = (Si32)text.length();
  if (n == 0) {
    lines.push_back(WrappedTextLine());
    return lines;
  }

  // Split the text into segments that each carry their trailing hyphen or
  // whitespace run, so breaking between two segments loses no bytes. A '\n' is
  // a segment of its own and always forces a break.
  std::vector<std::pair<Si32, Si32>> segs;
  std::vector<bool> seg_hard_break;
  Si32 i = 0;
  while (i < n) {
    if (text[static_cast<size_t>(i)] == '\n') {
      segs.push_back(std::make_pair(i, i + 1));
      seg_hard_break.push_back(true);
      ++i;
      continue;
    }
    Si32 seg_start = i;
    while (i < n && text[static_cast<size_t>(i)] != '-'
           && text[static_cast<size_t>(i)] != ' '
           && text[static_cast<size_t>(i)] != '\t'
           && text[static_cast<size_t>(i)] != '\n') {
      ++i;
    }
    if (i < n && text[static_cast<size_t>(i)] == '-') {
      ++i;  // keep the hyphen at the end of the segment
    } else {
      while (i < n && (text[static_cast<size_t>(i)] == ' '
                       || text[static_cast<size_t>(i)] == '\t')) {
        ++i;
      }
    }
    segs.push_back(std::make_pair(seg_start, i));
    seg_hard_break.push_back(false);
  }

  Si32 line_start = 0;
  std::string buf;
  for (size_t si = 0; si < segs.size(); ++si) {
    if (seg_hard_break[si]) {
      WrappedTextLine ln;
      ln.text = buf;
      ln.start = line_start;
      // The '\n' byte belongs to this line's range but not to its text, so a
      // caret right after the newline maps to the start of the next line.
      ln.end = segs[si].second;
      lines.push_back(ln);
      line_start = segs[si].second;
      buf.clear();
      continue;
    }
    std::string seg_text = text.substr(
        static_cast<size_t>(segs[si].first),
        static_cast<size_t>(segs[si].second - segs[si].first));
    std::string candidate = buf + seg_text;
    if (!buf.empty() && max_width > 0 &&
        font.EvaluateSize(candidate.c_str(), false).x > max_width) {
      WrappedTextLine ln;
      ln.text = buf;
      ln.start = line_start;
      ln.end = segs[si].first;
      lines.push_back(ln);
      line_start = segs[si].first;
      buf = seg_text;
    } else {
      buf = candidate;
    }
  }
  WrappedTextLine last;
  last.text = buf;
  last.start = line_start;
  last.end = n;
  lines.push_back(last);
  return lines;
}

std::vector<Editbox::VisualLine> Editbox::WrapVisualLines() {
  return WrapText(font_, text_, word_wrap_ ? MultilineInnerWidth() : 0);
}

Si32 Editbox::VisualLineIndex(const std::vector<VisualLine> &lines, Si32 pos) {
  for (size_t i = 0; i + 1 < lines.size(); ++i) {
    if (pos < lines[i].end) {
      return (Si32)i;
    }
  }
  return (Si32)lines.size() - 1;
}

Si32 Editbox::VisualLineCaretLimit(const std::vector<VisualLine> &lines,
                                   Si32 li) const {
  const VisualLine &ln = lines[static_cast<size_t>(li)];
  Si32 end = ln.start + (Si32)ln.text.size();
  if (li + 1 < (Si32)lines.size() && ln.end == end) {
    while (end > ln.start) {
      char c = text_[static_cast<size_t>(end - 1)];
      if (c != ' ' && c != '\t') {
        break;
      }
      --end;
    }
  }
  return end;
}

Si32 Editbox::LineStart(Si32 pos) {
  std::vector<VisualLine> lines = WrapVisualLines();
  Si32 li = VisualLineIndex(lines, std::min(std::max(0, pos),
                                            (Si32)text_.length()));
  return lines[static_cast<size_t>(li)].start;
}

Si32 Editbox::LineEnd(Si32 pos) {
  std::vector<VisualLine> lines = WrapVisualLines();
  Si32 li = VisualLineIndex(lines, std::min(std::max(0, pos),
                                            (Si32)text_.length()));
  return VisualLineCaretLimit(lines, li);
}

Si32 Editbox::PrevWordPos(Si32 pos) const {
  Si32 i = std::min(std::max(0, pos), (Si32)text_.length());
  auto is_space = [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  };
  while (i > 0 && is_space(text_[static_cast<size_t>(i - 1)])) {
    --i;
  }
  while (i > 0 && !is_space(text_[static_cast<size_t>(i - 1)])) {
    --i;
  }
  return i;
}

Si32 Editbox::NextWordPos(Si32 pos) const {
  Si32 n = (Si32)text_.length();
  Si32 i = std::min(std::max(0, pos), n);
  auto is_space = [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  };
  while (i < n && !is_space(text_[static_cast<size_t>(i)])) {
    ++i;
  }
  while (i < n && is_space(text_[static_cast<size_t>(i)])) {
    ++i;
  }
  return i;
}

void Editbox::MoveCursorVertical(Si32 dir, bool shift) {
  if (!font_.FontInstance()) {
    return;
  }
  std::vector<VisualLine> lines = WrapVisualLines();
  Si32 cur_line = VisualLineIndex(lines, cursor_pos_);
  Si32 target_line = cur_line + dir;
  if (target_line < 0 || target_line >= (Si32)lines.size()) {
    return;  // already on the first / last visual line
  }
  const VisualLine &cl = lines[static_cast<size_t>(cur_line)];
  Si32 byte_in_line = std::min(cursor_pos_ - cl.start, (Si32)cl.text.size());
  Si32 target_x = PrefixWidth(cl.text, byte_in_line);

  // Walk the target line and keep the offset whose left edge is closest to the
  // caret's current pixel column.
  const VisualLine &tl = lines[static_cast<size_t>(target_line)];
  Si32 best = 0;
  Si32 best_dist = std::abs(target_x);
  Si32 i = 0;
  while (i < (Si32)tl.text.size()) {
    Si32 nxt = Utf8NextCharPos(tl.text, i);
    Si32 px = PrefixWidth(tl.text, nxt);
    Si32 dist = std::abs(target_x - px);
    if (dist < best_dist) {
      best_dist = dist;
      best = nxt;
    }
    i = nxt;
  }
  MoveCursorTo(std::min(tl.start + best,
                        VisualLineCaretLimit(lines, target_line)), shift);
}

Si32 Editbox::CaretFromPoint(Vec2Si32 relative_pos) {
  if (!font_.FontInstance()) {
    return cursor_pos_;
  }
  if (is_multiline_) {
    Si32 line_step = LineStep();
    Si32 pad = multiline_padding_;
    // Lines are stacked from the top; line 0 shown is first_visible_line_.
    Si32 from_top = (size_.y - pad) - relative_pos.y;
    Si32 line_offset = from_top / line_step;
    if (line_offset < 0) {
      line_offset = 0;
    }
    std::vector<VisualLine> lines = WrapVisualLines();
    Si32 target_line = first_visible_line_ + line_offset;
    if (target_line >= (Si32)lines.size()) {
      target_line = (Si32)lines.size() - 1;
    }
    const VisualLine &ln = lines[static_cast<size_t>(target_line)];
    Si32 want_x = relative_pos.x - pad;
    Si32 best = 0;
    Si32 best_dist = std::abs(want_x);
    Si32 i = 0;
    while (i < (Si32)ln.text.size()) {
      Si32 nxt = Utf8NextCharPos(ln.text, i);
      Si32 px = PrefixWidth(ln.text, nxt);
      Si32 dist = std::abs(want_x - px);
      if (dist < best_dist) {
        best_dist = dist;
        best = nxt;
      }
      i = nxt;
    }
    return std::min(ln.start + best,
                    VisualLineCaretLimit(lines, target_line));
  }
  // Single line: the character at display_pos_ is drawn at the left border.
  // Measured on the shown text, which differs from text_ under a password char.
  Si32 border = std::max(0,
      (normal_.Height() - font_.FontInstance()->line_height_) / 2);
  Si32 want_x = relative_pos.x - border;
  const std::string shown = ShownText();
  const Si32 display_pos = ToShownOffset(display_pos_);
  Si32 best = display_pos;
  Si32 best_dist = std::abs(want_x);
  Si32 i = display_pos;
  while (i < (Si32)shown.length()) {
    Si32 nxt = Utf8NextCharPos(shown, i);
    std::string part = shown.substr(static_cast<size_t>(display_pos),
                                    static_cast<size_t>(nxt - display_pos));
    Si32 px = font_.EvaluateSize(part.c_str(), false).x;
    Si32 dist = std::abs(want_x - px);
    if (dist < best_dist) {
      best_dist = dist;
      best = nxt;
    }
    i = nxt;
  }
  return FromShownOffset(best);
}

void Editbox::SetReadOnly(bool is_read_only) {
  is_read_only_ = is_read_only;
}

bool Editbox::IsReadOnly() const {
  return is_read_only_;
}

void Editbox::SetPasswordChar(Ui32 password_char) {
  password_char_ = password_char;
}

Ui32 Editbox::GetPasswordChar() const {
  return password_char_;
}

void Editbox::SetPlaceholder(std::string placeholder) {
  placeholder_ = std::move(placeholder);
}

const std::string &Editbox::GetPlaceholder() const {
  return placeholder_;
}

void Editbox::SetPlaceholderColor(Rgba color) {
  placeholder_color_ = color;
}

namespace {

std::string Utf8FromCodepoint(Ui32 codepoint) {
  Utf8Codepoint utf8;
  utf8.WriteUtf32(codepoint);
  return std::string(reinterpret_cast<const char*>(utf8.buffer),
                     static_cast<size_t>(utf8.size));
}

}  // namespace

std::string Editbox::ShownText() const {
  if (password_char_ == 0 || is_multiline_) {
    return text_;
  }
  const std::string mask = Utf8FromCodepoint(password_char_);
  std::string shown;
  Si32 i = 0;
  const Si32 n = (Si32)text_.length();
  while (i < n) {
    shown += mask;
    i = Utf8NextCharPos(text_, i);
  }
  return shown;
}

Si32 Editbox::ToShownOffset(Si32 offset) const {
  if (password_char_ == 0 || is_multiline_) {
    return offset;
  }
  const Si32 mask_len = (Si32)Utf8FromCodepoint(password_char_).length();
  Si32 chars = 0;
  Si32 i = 0;
  const Si32 n = std::min(offset, (Si32)text_.length());
  while (i < n) {
    ++chars;
    i = Utf8NextCharPos(text_, i);
  }
  return chars * mask_len;
}

Si32 Editbox::FromShownOffset(Si32 shown_offset) const {
  if (password_char_ == 0 || is_multiline_) {
    return shown_offset;
  }
  const Si32 mask_len = (Si32)Utf8FromCodepoint(password_char_).length();
  Si32 chars = std::max(0, shown_offset) / std::max(1, mask_len);
  Si32 i = 0;
  const Si32 n = (Si32)text_.length();
  while (chars > 0 && i < n) {
    i = Utf8NextCharPos(text_, i);
    --chars;
  }
  return i;
}

void Editbox::DrawMultiline(Vec2Si32 pos) {
  // line_height is the glyph box used for the caret and selection rectangles;
  // line_step is the distance between two rows, which the host can override to
  // match its own text layout.
  Si32 line_height = std::max(1, font_.FontInstance()->line_height_);
  Si32 line_step = LineStep();
  Si32 pad = multiline_padding_;

  std::vector<VisualLine> lines = WrapVisualLines();
  Si32 total_lines = (Si32)lines.size();
  Si32 caret_line = VisualLineIndex(lines, cursor_pos_);

  Si32 visible_lines = std::max(1, (size_.y - pad * 2) / line_step);
  if (caret_line < first_visible_line_) {
    first_visible_line_ = caret_line;
  }
  if (caret_line >= first_visible_line_ + visible_lines) {
    first_visible_line_ = caret_line - visible_lines + 1;
  }
  if (first_visible_line_ > total_lines - visible_lines) {
    first_visible_line_ = total_lines - visible_lines;
  }
  if (first_visible_line_ < 0) {
    first_visible_line_ = 0;
  }

  Sprite backbuffer = GetEngine()->GetBackbuffer();
  Si32 last_line = std::min(total_lines, first_visible_line_ + visible_lines);
  if (text_.empty() && !placeholder_.empty()) {
    font_.Draw(placeholder_.c_str(), pos.x + pad, pos.y + size_.y - pad,
               kTextOriginTop, kTextAlignmentLeft,
               kDrawBlendingModeColorize, kFilterNearest, placeholder_color_);
  }
  for (Si32 li = first_visible_line_; li < last_line; ++li) {
    const VisualLine &ln = lines[static_cast<size_t>(li)];
    Si32 row = li - first_visible_line_;
    Si32 top_y = pos.y + size_.y - pad - row * line_step;

    if (selection_begin_ != selection_end_ && is_current_tab_) {
      Si32 seg_b = std::max(selection_begin_, ln.start);
      Si32 seg_e = std::min(selection_end_, ln.end);
      if (seg_b < seg_e) {
        Si32 ax1 = pos.x + pad + PrefixWidth(ln.text, seg_b - ln.start);
        Si32 ax2 = pos.x + pad + PrefixWidth(ln.text, seg_e - ln.start);
        if (ax2 > ax1) {
          DrawSelection(ax1, top_y - line_height, ax2, top_y, selection_mode_,
                        selection_color_1_, selection_color_2_, backbuffer);
        }
      }
    }

    font_.Draw(ln.text.c_str(), pos.x + pad, top_y,
               kTextOriginTop, kTextAlignmentLeft,
               kDrawBlendingModeColorize, kFilterNearest, color_);
  }

  if (is_current_tab_ && IsCaretVisible() &&
      caret_line >= first_visible_line_ && caret_line < last_line) {
    const VisualLine &ln = lines[static_cast<size_t>(caret_line)];
    Si32 byte_in_line = std::min(cursor_pos_ - ln.start, (Si32)ln.text.size());
    Si32 caret_x = PrefixWidth(ln.text, byte_in_line);
    Si32 row = caret_line - first_visible_line_;
    Si32 top_y = pos.y + size_.y - pad - row * line_step;
    Si32 cx = pos.x + pad + caret_x;
    Vec2Si32 a(cx, top_y - line_height + 1);
    Vec2Si32 b(cx, top_y);
    for (Si32 dx = 0; dx < 2; ++dx) {
      DrawLine(a, b, color_);
      a.x++;
      b.x++;
    }
  }
}

void Editbox::Draw(Vec2Si32 parent_absolute_pos) {
  if (!is_visible_) {
    return;
  }
  Vec2Si32 pos = parent_absolute_pos + pos_;
  if (is_current_tab_) {
    focused_.Draw(pos);
  } else {
    normal_.Draw(pos);
  }
  if (is_multiline_ && font_.FontInstance()) {
    DrawMultiline(pos);
    Panel::Draw(parent_absolute_pos);
    return;
  }
  if (font_.FontInstance()) {
    Si32 border = std::max(0,
      (normal_.Height() - font_.FontInstance()->line_height_) / 2);
    Si32 space_width = font_.EvaluateSize(" ", false).x;

    // With a password char every character is drawn as that char, so the
    // layout below works on the shown text and shown offsets; display_pos_ is
    // mapped back at the end.
    const std::string shown = ShownText();
    const Si32 cursor_pos_shown = ToShownOffset(cursor_pos_);
    const Si32 selection_begin_shown = ToShownOffset(selection_begin_);
    const Si32 selection_end_shown = ToShownOffset(selection_end_);
    Si32 display_pos = ToShownOffset(display_pos_);

    Si32 available_width = size_.x - border * 2 - space_width;
    Si32 displayable_width = size_.x - border * 2;

    // Update display_pos so that both display_pos
    // and cursor_pos_shown are both visible.
    Si32 end_pos = (Si32)shown.length();
    if (cursor_pos_shown <= display_pos) {
      // Move display pos to the left when cursor is at the left border.
      display_pos = std::max(0, cursor_pos_shown - 1);
    } else {
      // Move display pos to the right when cursor is at the right border.
      std::string part = shown.substr(static_cast<size_t>(display_pos),
                                      static_cast<size_t>(cursor_pos_shown - display_pos));
      Si32 w = font_.EvaluateSize(part.c_str(), true).x;
      if (available_width > 0) {
        while (w > available_width && display_pos < cursor_pos_shown) {
          display_pos++;
          part = shown.substr(static_cast<size_t>(display_pos),
                              static_cast<size_t>(cursor_pos_shown - display_pos));
          w = font_.EvaluateSize(part.c_str(), true).x;
          end_pos = std::min(cursor_pos_shown + 1, (Si32)shown.length());
        }
      }
    }
    // Display part of text with start at the display_pos.

    std::string display_text = shown.substr(static_cast<size_t>(display_pos),
                                            static_cast<size_t>(end_pos - display_pos));
    Si32 visible_width = font_.EvaluateSize(display_text.c_str(), false).x;
    if (available_width > 0) {
      while (visible_width > displayable_width) {
        Si32 visible_len = (Si32)display_text.length();
        Si32 desired_len = Si32(Si64(visible_len) * Si64(displayable_width) / Si64(visible_width));
        if (desired_len >= visible_len) {
          desired_len = visible_len - 1;
        }
        end_pos = display_pos + desired_len;
        display_text = shown.substr(static_cast<size_t>(display_pos),
                                    static_cast<size_t>(end_pos - display_pos));
        visible_width = font_.EvaluateSize(display_text.c_str(), false).x;
      }
    }

    Si32 skip_x = PrefixWidth(shown, display_pos);

    // Compute the selection rectangle up front so a blend highlight can be
    // drawn behind the text (like tentacle's name field), while invert/swap
    // modes keep their original behavior of drawing over the text.
    bool has_sel = (selection_begin_shown != selection_end_shown) && is_current_tab_;
    Si32 sel_x1 = 0, sel_x2 = 0, sel_y1 = 0, sel_y2 = 0;
    if (has_sel) {
      Vec2Si32 size1;
      Vec2Si32 pos1 = font_.EvaluateCharacterPos(shown.c_str(), shown.c_str()+selection_begin_shown, origin_, alignment_, &size1);
      Vec2Si32 size2;
      const char *plast = shown.c_str()+selection_end_shown-1;
      while (plast > shown.c_str()+selection_begin_shown) {
        if (*plast <= 127 && *plast > 0 && *plast != '\n' && *plast != '\r') {
          break;
        }
        --plast;
      }
      Vec2Si32 pos2 = font_.EvaluateCharacterPos(shown.c_str(), plast, origin_, alignment_, &size2);

      sel_x1 = pos.x + border + pos1.x;
      sel_x2 = pos.x + border + pos2.x + size2.x;
      sel_y1 = pos.y + border;
      sel_y2 = pos.y + border + font_.FontInstance()->line_height_;

      sel_x1 = std::min(std::max(pos.x + border, sel_x1 - skip_x),
                    pos.x + border + displayable_width);
      sel_x2 = std::min(std::max(pos.x + border, sel_x2 - skip_x),
                    pos.x + border + displayable_width);
    }

    if (has_sel && selection_mode_ == kTextSelectionModeBlend) {
      Sprite backbuffer = GetEngine()->GetBackbuffer();
      DrawSelection(sel_x1, sel_y1, sel_x2, sel_y2, selection_mode_,
                    selection_color_1_, selection_color_2_, backbuffer);
    }

    if (text_.empty() && !placeholder_.empty()) {
      font_.Draw(placeholder_.c_str(), pos.x + border, pos.y + border,
                 origin_, alignment_, kDrawBlendingModeColorize,
                 kFilterNearest, placeholder_color_);
    } else {
      font_.Draw(display_text.c_str(), pos.x + border, pos.y + border,
                 origin_, alignment_, kDrawBlendingModeColorize,
                 kFilterNearest, color_);
    }

    Si32 cursor_pos = std::max(0, std::min(cursor_pos_shown, (Si32)shown.length()));
    Si32 cursor_x = PrefixWidth(shown, cursor_pos);

    // A vertical bar spanning the text line, matching the multiline caret.
    if (is_current_tab_ && IsCaretVisible()) {
      Si32 cx = pos.x + border + cursor_x - skip_x;
      cx = std::min(cx, pos.x + border + displayable_width - 1);
      cx = std::max(cx, pos.x + border);
      Vec2Si32 a(cx, pos.y + border);
      Vec2Si32 b(cx, pos.y + border + font_.FontInstance()->line_height_ - 1);
      for (Si32 dx = 0; dx < 2; ++dx) {
        DrawLine(a, b, color_);
        a.x++;
        b.x++;
      }
    }

    if (has_sel && selection_mode_ != kTextSelectionModeBlend) {
      Sprite backbuffer = GetEngine()->GetBackbuffer();
      DrawSelection(sel_x1, sel_y1, sel_x2, sel_y2, selection_mode_,
                    selection_color_1_, selection_color_2_, backbuffer);
    }
    display_pos_ = FromShownOffset(display_pos);
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

void Editbox::SetCursorPos(Si32 pos) {
  cursor_pos_ = std::min(std::max(Si32(0), pos), (Si32)text_.length());
  TouchCaret();
}

Si32 Editbox::GetCursorPos() const {
  return cursor_pos_;
}

Si32 Editbox::GetSelectionBegin() const {
  return selection_begin_;
}

Si32 Editbox::GetSelectionEnd() const {
  return selection_end_;
}

void Editbox::SetSelection(Si32 begin, Si32 end) {
  Si32 length = (Si32)text_.length();
  Si32 a = std::min(std::max(0, begin), length);
  Si32 b = std::min(std::max(0, end), length);
  selection_begin_ = std::min(a, b);
  selection_end_ = std::max(a, b);
  edit_group_ = kEditGroupNone;
  TouchCaret();
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
, line_step_(1)
, min_value_(min_value)
, max_value_(max_value)
, value_(value)
, dir_(kind) {
  InitThumbDecoratedFrames();
}

Scrollbar::Scrollbar(Ui64 tag, std::shared_ptr<GuiThemeScrollbar> theme)
  : Panel(tag, Vec2Si32(0, 0), (theme->is_horizontal_ ? Vec2Si32(150, 29) : Vec2Si32(29, 150)), Ui32(tag))
, theme_(theme) {
  normal_background_ = theme_->normal_background_.DrawExternalSize(size_);
  focused_background_ = theme_->focused_background_.DrawExternalSize(size_);
  disabled_background_ = theme_->disabled_background_.DrawExternalSize(size_);
  normal_button_dec_ = theme_->normal_button_dec_;
  focused_button_dec_ = theme_->focused_button_dec_;
  down_button_dec_ = theme_->down_button_dec_;
  disabled_button_dec_ = theme_->disabled_button_dec_;
  normal_button_inc_ = theme_->normal_button_inc_;
  focused_button_inc_ = theme_->focused_button_inc_;
  down_button_inc_ = theme_->down_button_inc_;
  disabled_button_inc_ = theme_->disabled_button_inc_;
  normal_button_cur_ = theme_->normal_button_cur_;
  focused_button_cur_ = theme_->focused_button_cur_;
  down_button_cur_ = theme_->down_button_cur_;
  disabled_button_cur_ = theme_->disabled_button_cur_;
  step_ = 5;
  line_step_ = 1;
  min_value_ = 0;
  max_value_ = 100;
  value_ = 0;
  dir_ = (theme_->is_horizontal_ ? kScrollHorizontal : kScrollVertical);
  InitThumbDecoratedFrames();
}

void Scrollbar::ApplyTheme(std::shared_ptr<GuiThemeScrollbar> theme) {
  theme_ = theme;
  if (!theme_) {
    return;
  }
  dir_ = (theme_->is_horizontal_ ? kScrollHorizontal : kScrollVertical);
  normal_button_dec_ = theme_->normal_button_dec_;
  focused_button_dec_ = theme_->focused_button_dec_;
  down_button_dec_ = theme_->down_button_dec_;
  disabled_button_dec_ = theme_->disabled_button_dec_;
  normal_button_inc_ = theme_->normal_button_inc_;
  focused_button_inc_ = theme_->focused_button_inc_;
  down_button_inc_ = theme_->down_button_inc_;
  disabled_button_inc_ = theme_->disabled_button_inc_;
  normal_button_cur_ = theme_->normal_button_cur_;
  focused_button_cur_ = theme_->focused_button_cur_;
  down_button_cur_ = theme_->down_button_cur_;
  disabled_button_cur_ = theme_->disabled_button_cur_;
  InitThumbDecoratedFrames();
  RegenerateSprites();
}

void Scrollbar::InvalidateThumbCache() {
  for (Si32 i = 0; i < 3; ++i) {
    thumb_cache_[i].along = -1;
    thumb_cache_[i].cross = -1;
  }
  thumb_cache_disabled_.along = -1;
  thumb_cache_disabled_.cross = -1;
}

void Scrollbar::InitThumbDecoratedFrames() {
  for (Si32 i = 0; i < 3; ++i) {
    thumb_df_ok_[i] = false;
  }
  thumb_df_disabled_ok_ = false;
  cur_hover_is_normal_ = false;
  cur_down_is_normal_ = false;

  auto try_split = [](const Sprite& sp, DecoratedFrame* df, bool* ok) {
    *ok = false;
    Si32 w = sp.Width();
    Si32 h = sp.Height();
    Si32 mn = std::min(w, h);
    Si32 b = std::max(Si32(2), std::min(Si32(6), mn / 3));
    if (mn >= b * 2 + 1) {
      df->Split(sp, b, true, true);
      *ok = true;
    }
  };

  try_split(normal_button_cur_, &cur_frame_normal_, &thumb_df_ok_[0]);

  if (focused_button_cur_.Width() > 0 && focused_button_cur_.Height() > 0) {
    try_split(focused_button_cur_, &cur_frame_hover_, &thumb_df_ok_[1]);
    cur_hover_is_normal_ = !thumb_df_ok_[1];
  } else {
    cur_hover_is_normal_ = true;
    thumb_df_ok_[1] = false;
  }

  if (down_button_cur_.Width() > 0 && down_button_cur_.Height() > 0) {
    try_split(down_button_cur_, &cur_frame_down_, &thumb_df_ok_[2]);
    cur_down_is_normal_ = !thumb_df_ok_[2];
  } else {
    cur_down_is_normal_ = true;
    thumb_df_ok_[2] = false;
  }

  try_split(disabled_button_cur_, &cur_frame_disabled_, &thumb_df_disabled_ok_);
  InvalidateThumbCache();
}

Si32 Scrollbar::ThumbTrackInnerPx() const {
  Si32 dec = normal_button_dec_.Size()[dir_];
  Si32 inc = normal_button_inc_.Size()[dir_];
  Si32 inner = size_[dir_] - dec - inc - 2;
  if (inner < 1) {
    return 1;
  }
  return inner;
}

Si32 Scrollbar::EffectiveThumbPx() const {
  Si32 base = normal_button_cur_.Size()[dir_];
  if (base < 1) {
    base = 1;
  }
  Si32 raw;
  if (thumb_extent_ > 0) {
    raw = thumb_extent_;
  } else {
    raw = base;
  }
  Si32 cap = ThumbTrackInnerPx();
  Si32 clamped = std::min(raw, cap);
  if (clamped < 1) {
    return 1;
  }
  return clamped;
}

Vec2Si32 Scrollbar::ThumbOuterPixelSize() const {
  Si32 eth = EffectiveThumbPx();
  Si32 cross = std::max(Si32(1), size_[1 - dir_] - 2);
  if (dir_ == kScrollVertical) {
    return Vec2Si32(cross, eth);
  }
  return Vec2Si32(eth, cross);
}

DecoratedFrame& Scrollbar::ThumbFrameForLayer(Si32 layer_idx) {
  if (layer_idx == 1 && cur_hover_is_normal_) {
    return cur_frame_normal_;
  }
  if (layer_idx == 2 && cur_down_is_normal_) {
    return cur_frame_normal_;
  }
  if (layer_idx == 0) {
    return cur_frame_normal_;
  }
  if (layer_idx == 1) {
    return cur_frame_hover_;
  }
  return cur_frame_down_;
}

void Scrollbar::DrawThumbAt(Vec2Si32 cur_pos_abs, Si32 layer_idx) {
  Vec2Si32 outer = ThumbOuterPixelSize();
  Si32 along = outer[dir_];
  Si32 cross = outer[1 - dir_];
  bool df_ok = false;
  if (layer_idx == 0) {
    df_ok = thumb_df_ok_[0];
  } else if (layer_idx == 1) {
    df_ok = thumb_df_ok_[0] && (cur_hover_is_normal_ || thumb_df_ok_[1]);
  } else {
    df_ok = thumb_df_ok_[0] && (cur_down_is_normal_ || thumb_df_ok_[2]);
  }

  if (!df_ok) {
    if (layer_idx == 0) {
      normal_button_cur_.Draw(cur_pos_abs);
    } else if (layer_idx == 1) {
      if (cur_hover_is_normal_ || focused_button_cur_.Width() == 0) {
        normal_button_cur_.Draw(cur_pos_abs);
      } else {
        focused_button_cur_.Draw(cur_pos_abs);
      }
    } else {
      if (cur_down_is_normal_ || down_button_cur_.Width() == 0) {
        normal_button_cur_.Draw(cur_pos_abs);
      } else {
        down_button_cur_.Draw(cur_pos_abs);
      }
    }
    return;
  }

  ThumbRasterCache& c = thumb_cache_[layer_idx];
  if (c.along != along || c.cross != cross) {
    DecoratedFrame& df = ThumbFrameForLayer(layer_idx);
    c.sprite = df.DrawExternalSize(outer);
    c.along = along;
    c.cross = cross;
  }
  c.sprite.Draw(cur_pos_abs, kDrawBlendingModeAlphaBlend, kFilterNearest,
                Rgba(255, 255, 255, 255));
}

void Scrollbar::DrawDisabledThumbAt(Vec2Si32 cur_pos_abs) {
  Vec2Si32 outer = ThumbOuterPixelSize();
  Si32 along = outer[dir_];
  Si32 cross = outer[1 - dir_];
  if (!thumb_df_disabled_ok_) {
    disabled_button_cur_.Draw(cur_pos_abs);
    return;
  }
  ThumbRasterCache& c = thumb_cache_disabled_;
  if (c.along != along || c.cross != cross) {
    c.sprite = cur_frame_disabled_.DrawExternalSize(outer);
    c.along = along;
    c.cross = cross;
  }
  c.sprite.Draw(cur_pos_abs, kDrawBlendingModeAlphaBlend, kFilterNearest,
                Rgba(255, 255, 255, 255));
}

void Scrollbar::UpdateHoverZone(Vec2Si32 relative_pos, Si32 s1, Si32 s2,
                                Si32 s3, Si32 s4) {
  Si32 p = relative_pos[dir_];
  if (p < s1) {
    hover_zone_ = ScrollHoverZone::kDec;
  } else if (p < s2) {
    hover_zone_ = ScrollHoverZone::kTrackBefore;
  } else if (p < s3) {
    hover_zone_ = ScrollHoverZone::kThumb;
  } else if (p < s4) {
    hover_zone_ = ScrollHoverZone::kTrackAfter;
  } else {
    hover_zone_ = ScrollHoverZone::kInc;
  }
}

void Scrollbar::DrawDecButton(Vec2Si32 absolute_pos, Vec2Si32 button_offset) {
  Vec2Si32 at = absolute_pos + button_offset;
  if (state_ == kDecDown) {
    down_button_dec_.Draw(at);
  } else if (hover_zone_ == ScrollHoverZone::kDec &&
             focused_button_dec_.Width() > 0 && focused_button_dec_.Height() > 0) {
    focused_button_dec_.Draw(at);
  } else {
    normal_button_dec_.Draw(at);
  }
}

void Scrollbar::DrawIncButton(Vec2Si32 inc_pos, Vec2Si32 button_offset) {
  Vec2Si32 at = inc_pos + button_offset;
  if (state_ == kIncDown) {
    down_button_inc_.Draw(at);
  } else if (hover_zone_ == ScrollHoverZone::kInc &&
             focused_button_inc_.Width() > 0 && focused_button_inc_.Height() > 0) {
    focused_button_inc_.Draw(at);
  } else {
    normal_button_inc_.Draw(at);
  }
}

void Scrollbar::SetThumbExtent(Si32 extent) {
  if (extent < 0) {
    extent = 0;
  }
  if (thumb_extent_ == extent) {
    return;
  }
  thumb_extent_ = extent;
  InvalidateThumbCache();
}

Si32 Scrollbar::GetThumbExtent() const {
  return thumb_extent_;
}

bool Scrollbar::IsThumbDragging() const {
  return state_ == kMiddleDragged;
}

bool Scrollbar::TakesMouse() const {
  return true;
}

void Scrollbar::SetSize(Vec2Si32 size) {
  InvalidateThumbCache();
  Panel::SetSize(size);
}

void Scrollbar::SetSize(Si32 width, Si32 height) {
  InvalidateThumbCache();
  Panel::SetSize(width, height);
}

void Scrollbar::HandleInput(Vec2Si32 parent_pos,
                            const InputMessage &message,
                            bool is_top_level,
                            bool *in_out_is_applied,
                            std::deque<GuiMessage> *out_gui_messages,
                            std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  Si32 eth = EffectiveThumbPx();
  Si32 s1 = 1 + normal_button_dec_.Size()[dir_];
  Si32 s4 = size_[dir_] - 1 - normal_button_inc_.Size()[dir_];
  Si32 w = std::max(1, s4 - s1 - eth);
  Si64 value_range = Si64(max_value_) - Si64(min_value_);
  Si32 s2 = Si32(Si64(s1) +
                 (value_range ? Si64(w) * (Si64(value_) - Si64(min_value_)) / value_range : 0));
  Si32 s3 = s2 + eth;

  ScrollState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - pos;
    bool is_inside = IsWithin(relative_pos);

    if (is_inside) {
      UpdateHoverZone(relative_pos, s1, s2, s3, s4);
    } else {
      hover_zone_ = ScrollHoverZone::kNone;
    }

    if (is_inside && message.mouse.wheel_delta != 0) {
      *in_out_is_applied = true;
      Si64 dv = -Si64(message.mouse.wheel_delta) * Si64(step_) / 120;
      if (dv == 0) {
        if (message.mouse.wheel_delta > 0) {
          dv = -1;
        } else if (message.mouse.wheel_delta < 0) {
          dv = 1;
        }
      }
      SetValueAndNotify(value_ + Si32(dv), out_gui_messages);
    }

    if (!*in_out_is_applied &&
        message.keyboard.state[kKeyMouseLeft] == 1 &&
        state_ == kMiddleDragged &&
        is_current_tab_) {
      *out_current_tab = shared_from_this();
      *in_out_is_applied = true;
      Si32 drag_s = relative_pos[dir_] - start_relative_s_;
      Si32 value_diff = Si32(Si64(drag_s) * (Si64(max_value_) - Si64(min_value_)) / Si64(w));
      SetValueAndNotify(start_value_ + value_diff, out_gui_messages);
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
          SetValueAndNotify(start_value_ + value_diff, out_gui_messages);
        } else if (relative_pos[dir_] < s1) {
          state_ = kDecDown;
          if (prev_state != state_) {
            SetValueAndNotify(value_ - line_step_, out_gui_messages);
          }
        } else if (relative_pos[dir_] < s2) {
          state_ = kDecFast;
          if (prev_state != state_) {
            SetValueAndNotify(value_ - step_, out_gui_messages);
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
            SetValueAndNotify(value_ + step_, out_gui_messages);
          }
        } else {
          state_ = kIncDown;
          if (prev_state != state_) {
            SetValueAndNotify(value_ + line_step_, out_gui_messages);
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
        KeyCode key_dec = (dir_ == kScrollVertical) ? kKeyDown : kKeyLeft;
        KeyCode key_inc = (dir_ == kScrollVertical) ? kKeyUp : kKeyRight;
        if (message.keyboard.key == key_dec) {
          SetValueAndNotify(value_ - line_step_, out_gui_messages);
        } else if (message.keyboard.key == key_inc) {
          SetValueAndNotify(value_ + line_step_, out_gui_messages);
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

void Scrollbar::SetValueAndNotify(Si32 value,
                                  std::deque<GuiMessage> *out_gui_messages) {
  value_ = Clamp(value, min_value_, max_value_);
  Emit(kGuiScrollChange, out_gui_messages);
  OnScrollChange();
}

void Scrollbar::Draw(Vec2Si32 parent_absolute_pos) {
  if (state_ == kHidden) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  Vec2Si32 button_offset = Vec2Si32(1, 1);
  Si32 eth = EffectiveThumbPx();
  if (state_ == kDisabled) {
    disabled_background_.Draw(absolute_pos);
    disabled_button_dec_.Draw(absolute_pos + button_offset);
    Vec2Si32 inc_pos = absolute_pos +
    (dir_
     ? size_.oy() - disabled_button_inc_.Size().oy() + Vec2Si32(0, -2)
     : size_.xo() - disabled_button_inc_.Size().xo() + Vec2Si32(-2, 0));
    disabled_button_inc_.Draw(inc_pos + button_offset);
    Vec2Si32 after_dec = absolute_pos +
    (dir_
     ? button_offset.oy() + disabled_button_dec_.Size().oy()
     : button_offset.xo() + disabled_button_dec_.Size().xo());
    Si32 length = inc_pos[dir_] - after_dec[dir_] - eth + 1;
    Si32 offset = 0;
    if (length > 0 && max_value_ != min_value_) {
      offset = Si32(Si64(length) * (Si64(value_) - Si64(min_value_)) / (Si64(max_value_) - Si64(min_value_)));
    }
    Vec2Si32 cur_pos = after_dec + (dir_ ? Vec2Si32(1, offset) : Vec2Si32(offset, 1));
    DrawDisabledThumbAt(cur_pos);
    Panel::Draw(parent_absolute_pos);
    return;
  }
  if (state_ == kNormal) {
    normal_background_.Draw(absolute_pos);
  } else {
    focused_background_.Draw(absolute_pos);
  }
  DrawDecButton(absolute_pos, button_offset);
  Vec2Si32 inc_pos = absolute_pos +
  (dir_
   ? size_.oy() - normal_button_inc_.Size().oy() + Vec2Si32(0, -2)
   : size_.xo() - normal_button_inc_.Size().xo() + Vec2Si32(-2, 0));
  DrawIncButton(inc_pos, button_offset);
  Vec2Si32 after_dec = absolute_pos +
  (dir_
   ? button_offset.oy() + normal_button_dec_.Size().oy()
   : button_offset.xo() + normal_button_dec_.Size().xo());
  Si32 length = inc_pos[dir_] - after_dec[dir_] - eth + 1;
  Si32 offset = 0;
  if (length > 0 && max_value_ != min_value_) {
    offset = Si32(Si64(length) * (Si64(value_) - Si64(min_value_)) / (Si64(max_value_) - Si64(min_value_)));
  }
  Vec2Si32 cur_pos = after_dec + (dir_ ? Vec2Si32(1, offset) : Vec2Si32(offset, 1));
  Si32 thumb_layer = 0;
  if (state_ == kMiddleDragged) {
    thumb_layer = 2;
  } else if (hover_zone_ == ScrollHoverZone::kThumb) {
    thumb_layer = 1;
  }
  DrawThumbAt(cur_pos, thumb_layer);
  Panel::Draw(parent_absolute_pos);
}

void Scrollbar::SetStep(Si32 step) {
  step_ = std::max(1, step);
}

void Scrollbar::SetLineStep(Si32 line_step) {
  line_step_ = std::max(1, line_step);
}

Si32 Scrollbar::GetLineStep() const {
  return line_step_;
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
  InvalidateThumbCache();
  if (theme_) {
    normal_background_ = theme_->normal_background_.DrawExternalSize(size_);
    focused_background_ = theme_->focused_background_.DrawExternalSize(size_);
    disabled_background_ = theme_->disabled_background_.DrawExternalSize(size_);
  }
}

void Scrollbar::SetVisible(bool is_visible) {
  if (Panel::IsVisible() != is_visible) {
    Panel::SetVisible(is_visible);
    if (Panel::IsVisible()) {
      state_ = kNormal;
    } else {
      state_ = kHidden;
    }
  }
}

void Scrollbar::SetEnabled(bool is_enabled) {
  if (state_ == kHidden) {
    return;
  }
  if (is_enabled) {
    if (state_ == kDisabled) {
      state_ = kNormal;
    }
  } else {
    if (state_ != kDisabled) {
      state_ = kDisabled;
    }
  }
}

bool Scrollbar::IsEnabled() {
  return state_ != kDisabled && state_ != kHidden;
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
  text_->SetSize(Vec2Si32(0, normal_[0].Size().y));
  text_->SetOrigin(kTextOriginCenter);
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

void Checkbox::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
    out_gui_messages, out_current_tab);
  CheckboxState prev_state = state_;
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 pos = parent_pos + pos_;
    bool is_inside = IsWithin(message.mouse.backbuffer_pos - pos);
    if (is_inside && !*in_out_is_applied) {
      *out_current_tab = Panel::Invalid();
      is_current_tab_ = false;
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        *in_out_is_applied = true;
        state_ = kDown;
      } else {
        state_ = kHovered;
        *in_out_is_applied = true;
      }
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 2 &&
          prev_state == kDown) {
        up_sound_.Play(GetGuiSoundVolume());
        Toggle(out_gui_messages);
        Emit(kGuiButtonClick, out_gui_messages);
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
        down_sound_.Play(GetGuiSoundVolume());
        *in_out_is_applied = true;
        Emit(kGuiButtonDown, out_gui_messages);
        OnButtonDown();
      }
      if (prev_state == kDown) {
        up_sound_.Play(GetGuiSoundVolume());
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
            down_sound_.Play(GetGuiSoundVolume());
            state_ = kDown;
            if (is_hotkey && GetTabOrder() != 0) {
              *out_current_tab = shared_from_this();
            }
            Emit(kGuiButtonDown, out_gui_messages);
            OnButtonDown();
          }
        } else {
          if (prev_state == kDown) {
            *in_out_is_applied = true;
            up_sound_.Play(GetGuiSoundVolume());
            Toggle(out_gui_messages);
            Emit(kGuiButtonClick, out_gui_messages);
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

bool Checkbox::TakesMouse() const {
  return true;
}

bool Checkbox::IsEnabled() {
  return state_ != kDisabled && state_ != kHidden;
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
    Vec2Si32 text_eval = text_->EvaluateSize();
    text_->SetSize(Vec2Si32(text_eval.x, normal_[0].Size().y));
    Vec2Si32 new_size = normal_[0].Size();
    new_size.x += text_eval.x;
    SetSize(new_size);
  }
}

void Checkbox::SetHotkey(KeyCode hotkey) {
  hotkey_ = hotkey;
}

void Checkbox::Toggle(std::deque<GuiMessage> *out_gui_messages) {
  (void)out_gui_messages;
  value_ = (value_ == kValueClear ? kValueChecked : kValueClear);
}

RadioButton::RadioButton(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
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
                         Si32 group)
: Checkbox(tag, pos, tab_order, clear_normal, checked_normal, clear_down,
           checked_down, clear_hovered, checked_hovered, clear_disabled,
           checked_disabled, down_sound, up_sound, hotkey, kValueClear)
, group_(group) {
}

RadioButton::RadioButton(Ui64 tag, std::shared_ptr<GuiTheme> theme,
                         Si32 group)
: Checkbox(tag, theme)
, group_(group) {
  normal_[0] = theme->radio_clear_normal_;
  normal_[1] = theme->radio_checked_normal_;
  down_[0] = theme->radio_clear_down_;
  down_[1] = theme->radio_checked_down_;
  hovered_[0] = theme->radio_clear_hovered_;
  hovered_[1] = theme->radio_checked_hovered_;
  disabled_[0] = theme->radio_clear_disabled_;
  disabled_[1] = theme->radio_checked_disabled_;
  if (text_) {
    text_->SetPos(Vec2Si32(normal_[0].Size().x, 0));
    text_->SetSize(Vec2Si32(0, normal_[0].Size().y));
  }
  SetSize(normal_[0].Size());
}

void RadioButton::SetGroup(Si32 group) {
  group_ = group;
}

Si32 RadioButton::GetGroup() const {
  return group_;
}

void RadioButton::SelectQuietly() {
  value_ = kValueChecked;
  if (!parent_) {
    return;
  }
  const std::deque<std::shared_ptr<Panel>> &siblings = parent_->GetChildren();
  for (auto it = siblings.begin(); it != siblings.end(); ++it) {
    RadioButton *other = dynamic_cast<RadioButton*>(it->get());
    if (other && other != this && other->group_ == group_) {
      other->value_ = kValueClear;
    }
  }
}

void RadioButton::SetChecked(bool is_checked) {
  if (is_checked) {
    SelectQuietly();
  } else {
    value_ = kValueClear;
  }
}

void RadioButton::Toggle(std::deque<GuiMessage> *out_gui_messages) {
  if (value_ == kValueChecked) {
    // Clicking the selected one of a group changes nothing.
    return;
  }
  SelectQuietly();
  Emit(kGuiRadioSelect, out_gui_messages);
  OnSelect();
}

Image::Image(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Sprite sprite,
             ScaleMode scale_mode)
: Panel(tag, pos, size, 0)
, sprite_(std::move(sprite))
, scale_mode_(scale_mode) {
}

void Image::SetSprite(Sprite sprite) {
  sprite_ = std::move(sprite);
}

Sprite Image::GetSprite() const {
  return sprite_;
}

void Image::SetScaleMode(ScaleMode scale_mode) {
  scale_mode_ = scale_mode;
}

Image::ScaleMode Image::GetScaleMode() const {
  return scale_mode_;
}

void Image::SetDrawMode(DrawBlendingMode blending_mode,
                        DrawFilterMode filter_mode) {
  blending_mode_ = blending_mode;
  filter_mode_ = filter_mode;
}

void Image::SetColor(Rgba color) {
  color_ = color;
}

void Image::GetPictureRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const {
  Vec2Si32 own = sprite_.Size();
  switch (scale_mode_) {
    case kScaleStretch:
      *out_pos = Vec2Si32(0, 0);
      *out_size = size_;
      break;
    case kScaleFit: {
      if (own.x <= 0 || own.y <= 0) {
        *out_pos = Vec2Si32(0, 0);
        *out_size = Vec2Si32(0, 0);
        break;
      }
      // The larger of the two ratios is the one that does not fit.
      Vec2Si32 fitted;
      if (Si64(own.x) * Si64(size_.y) >= Si64(own.y) * Si64(size_.x)) {
        fitted.x = size_.x;
        fitted.y = Si32(Si64(own.y) * Si64(size_.x) / Si64(own.x));
      } else {
        fitted.y = size_.y;
        fitted.x = Si32(Si64(own.x) * Si64(size_.y) / Si64(own.y));
      }
      *out_size = fitted;
      *out_pos = (size_ - fitted) / 2;
      break;
    }
    case kScaleNone:
      *out_pos = Vec2Si32(0, 0);
      *out_size = own;
      break;
    case kScaleCenter:
      *out_pos = (size_ - own) / 2;
      *out_size = own;
      break;
  }
}

void Image::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  Vec2Si32 own = sprite_.Size();
  if (own.x > 0 && own.y > 0 && size_.x > 0 && size_.y > 0) {
    Vec2Si32 pic_pos;
    Vec2Si32 pic_size;
    GetPictureRect(&pic_pos, &pic_size);
    // A view without a pivot, so that the position is the bottom-left corner.
    Sprite plain;
    plain.Reference(sprite_, 0, 0, own.x, own.y);
    ClipScope clip(absolute_pos, size_);
    Vec2Si32 at = absolute_pos + pic_pos - clip.Origin();
    plain.Draw(at, pic_size, blending_mode_, filter_mode_, color_);
  }
  Panel::Draw(parent_absolute_pos);
}

Slider::Slider(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
               DecoratedFrame track, DecoratedFrame thumb_normal,
               DecoratedFrame thumb_hovered, DecoratedFrame thumb_down,
               DecoratedFrame thumb_disabled, Si32 thumb_length,
               bool is_horizontal)
: Panel(tag, pos, size, tab_order)
, track_frame_(track)
, thumb_length_(std::max(1, thumb_length))
, is_horizontal_(is_horizontal) {
  thumb_frame_[kNormal] = thumb_normal;
  thumb_frame_[kHovered] = thumb_hovered;
  thumb_frame_[kDown] = thumb_down;
  thumb_frame_[kDisabled] = thumb_disabled;
  RegenerateSprites();
}

Slider::Slider(Ui64 tag, std::shared_ptr<GuiThemeSlider> theme)
: Panel(tag, Vec2Si32(0, 0),
        theme->is_horizontal_ ? Vec2Si32(200, 24) : Vec2Si32(24, 200),
        (Ui32)tag)
, track_frame_(theme->track_)
, thumb_length_(std::max(1, theme->thumb_length_))
, is_horizontal_(theme->is_horizontal_)
, theme_(theme) {
  thumb_frame_[kNormal] = theme->thumb_normal_;
  thumb_frame_[kHovered] = theme->thumb_hovered_;
  thumb_frame_[kDown] = theme->thumb_down_;
  thumb_frame_[kDisabled] = theme->thumb_disabled_;
  RegenerateSprites();
}

Si32 Slider::Along(Vec2Si32 v) const {
  return is_horizontal_ ? v.x : v.y;
}

Si32 Slider::TrackLength() const {
  return std::max(0, Along(size_) - thumb_length_);
}

Si32 Slider::ThumbStart() const {
  if (max_value_ <= min_value_) {
    return 0;
  }
  Si64 range = Si64(max_value_) - Si64(min_value_);
  Si64 offset = Si64(value_) - Si64(min_value_);
  return Si32((offset * Si64(TrackLength()) + range / 2) / range);
}

Si32 Slider::ValueAt(Si32 thumb_start) const {
  Si32 length = TrackLength();
  if (length <= 0 || max_value_ <= min_value_) {
    return min_value_;
  }
  Si64 range = Si64(max_value_) - Si64(min_value_);
  Si64 clamped = std::min(std::max(Si64(thumb_start), Si64(0)), Si64(length));
  Si64 value = Si64(min_value_) + (clamped * range + length / 2) / length;
  return Si32(std::min(std::max(value, Si64(min_value_)), Si64(max_value_)));
}

void Slider::GetThumbRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const {
  if (is_horizontal_) {
    *out_pos = Vec2Si32(ThumbStart(), 0);
    *out_size = Vec2Si32(thumb_length_, size_.y);
  } else {
    *out_pos = Vec2Si32(0, ThumbStart());
    *out_size = Vec2Si32(size_.x, thumb_length_);
  }
}

void Slider::RegenerateSprites() {
  track_ = track_frame_.DrawExternalSize(size_);
  Vec2Si32 thumb_pos;
  Vec2Si32 thumb_size;
  GetThumbRect(&thumb_pos, &thumb_size);
  for (Si32 i = 0; i < 4; ++i) {
    thumb_[i] = thumb_frame_[i].DrawExternalSize(thumb_size);
  }
}

void Slider::SetRange(Si32 min_value, Si32 max_value) {
  min_value_ = min_value;
  max_value_ = std::max(min_value, max_value);
  value_ = std::min(std::max(value_, min_value_), max_value_);
}

Si32 Slider::GetMinValue() const {
  return min_value_;
}

Si32 Slider::GetMaxValue() const {
  return max_value_;
}

void Slider::SetValue(Si32 value) {
  value_ = std::min(std::max(value, min_value_), max_value_);
}

Si32 Slider::GetValue() const {
  return value_;
}

void Slider::SetStep(Si32 step) {
  step_ = std::max(1, step);
}

Si32 Slider::GetStep() const {
  return step_;
}

bool Slider::IsHorizontal() const {
  return is_horizontal_;
}

void Slider::SetValueAndNotify(Si32 value,
                               std::deque<GuiMessage> *out_gui_messages) {
  Si32 clamped = std::min(std::max(value, min_value_), max_value_);
  if (clamped == value_) {
    return;
  }
  value_ = clamped;
  Emit(kGuiSliderChange, out_gui_messages);
  OnSliderChange();
}

void Slider::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  track_.Draw(absolute_pos, size_);
  Vec2Si32 thumb_pos;
  Vec2Si32 thumb_size;
  GetThumbRect(&thumb_pos, &thumb_size);
  thumb_[state_].Draw(absolute_pos + thumb_pos, thumb_size);
  Panel::Draw(parent_absolute_pos);
}

void Slider::SetEnabled(bool is_enabled) {
  if (is_enabled) {
    if (state_ == kDisabled) {
      state_ = kNormal;
    }
  } else {
    state_ = kDisabled;
  }
}

bool Slider::IsEnabled() {
  return state_ != kDisabled;
}

bool Slider::TakesMouse() const {
  return true;
}

void Slider::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                         bool is_top_level,
                         bool *in_out_is_applied,
                         std::deque<GuiMessage> *out_gui_messages,
                         std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - (parent_pos + pos_);
    bool is_inside = IsWithin(relative_pos);
    if (state_ == kDown) {
      if (message.keyboard.state[kKeyMouseLeft] == 1) {
        // Dragging: the grab point stays under the cursor.
        SetValueAndNotify(ValueAt(Along(relative_pos) - drag_offset_),
                          out_gui_messages);
        *in_out_is_applied = true;
        return;
      }
      state_ = is_inside ? kHovered : kNormal;
    }
    if (is_inside && !*in_out_is_applied) {
      *in_out_is_applied = true;
      state_ = kHovered;
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1) {
        if (GetTabOrder() != 0) {
          *out_current_tab = shared_from_this();
          is_current_tab_ = true;
        }
        Vec2Si32 thumb_pos;
        Vec2Si32 thumb_size;
        GetThumbRect(&thumb_pos, &thumb_size);
        Vec2Si32 in_thumb = relative_pos - thumb_pos;
        bool is_on_thumb = in_thumb.x >= 0 && in_thumb.y >= 0 &&
            in_thumb.x < thumb_size.x && in_thumb.y < thumb_size.y;
        state_ = kDown;
        if (is_on_thumb) {
          drag_offset_ = Along(relative_pos) - Along(thumb_pos);
        } else {
          // A click on the track centers the thumb on the cursor.
          drag_offset_ = thumb_length_ / 2;
          SetValueAndNotify(ValueAt(Along(relative_pos) - drag_offset_),
                            out_gui_messages);
        }
      } else if (message.mouse.wheel_delta != 0) {
        Si64 dv = Si64(message.mouse.wheel_delta) * Si64(step_) / 120;
        if (dv == 0) {
          dv = message.mouse.wheel_delta > 0 ? step_ : -step_;
        }
        SetValueAndNotify(value_ + Si32(dv), out_gui_messages);
      }
    } else if (state_ == kHovered && !is_inside) {
      state_ = kNormal;
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (!*in_out_is_applied && is_current_tab_ &&
        (message.keyboard.key_state & 1u) == 1u) {
      Ui32 key = message.keyboard.key;
      Si32 dec_key = is_horizontal_ ? kKeyLeft : kKeyDown;
      Si32 inc_key = is_horizontal_ ? kKeyRight : kKeyUp;
      if (key == Ui32(dec_key)) {
        *in_out_is_applied = true;
        SetValueAndNotify(value_ - step_, out_gui_messages);
      } else if (key == Ui32(inc_key)) {
        *in_out_is_applied = true;
        SetValueAndNotify(value_ + step_, out_gui_messages);
      } else if (key == kKeyHome) {
        *in_out_is_applied = true;
        SetValueAndNotify(min_value_, out_gui_messages);
      } else if (key == kKeyEnd) {
        *in_out_is_applied = true;
        SetValueAndNotify(max_value_, out_gui_messages);
      }
    }
  }
}

namespace {

const std::string &EmptyString() {
  static const std::string empty;
  return empty;
}

// Seconds between two clicks on the same row that make a double click.
const double kListDoubleClickSeconds = 0.4;

}  // namespace

ListBox::ListBox(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
                 Font font, Rgba color, DecoratedFrame background,
                 DecoratedFrame selection, DecoratedFrame hover)
: Panel(tag, pos, size, tab_order)
, font_(font)
, background_frame_(background)
, selection_frame_(selection)
, hover_frame_(hover) {
  palete_.push_back(color);
  disabled_palete_.push_back(Rgba(128, 128, 128));
  SetFont(font);
}

ListBox::ListBox(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(200, 150), (Ui32)tag)
, font_(theme->text_->font_)
, palete_(theme->text_->palete_)
, disabled_palete_(theme->text_->disabled_palete_)
, background_frame_(theme->listbox_background_)
, selection_frame_(theme->listbox_selection_)
, hover_frame_(theme->listbox_hover_)
, theme_(theme) {
  scrollbar_ = std::make_shared<Scrollbar>(0, theme->v_scrollbar_);
  scrollbar_->SetMinValue(0);
  scrollbar_->SetMaxValue(0);
  scrollbar_->SetLineStep(1);
  Panel::AddChild(scrollbar_);
  // The scrollbar grows upward, the rows grow downward: the top row is shown
  // when the scrollbar is at its maximum.
  scrollbar_->OnScrollChange = [this]() {
    first_visible_ = MaxFirstVisible() - scrollbar_->GetValue();
    ClampScroll();
  };
  SetFont(font_);
}

Si32 ListBox::InnerLeft() const {
  return border_.x + padding_;
}

Si32 ListBox::InnerTop() const {
  return size_.y - border_.y - padding_;
}

Si32 ListBox::RowsWidth() const {
  Si32 scrollbar_width = scrollbar_ ? scrollbar_->GetSize().x : 0;
  return std::max(0, size_.x - 2 * border_.x - 2 * padding_ - scrollbar_width);
}

Si32 ListBox::VisibleRows() const {
  Si32 inner_height = size_.y - 2 * border_.y - 2 * padding_;
  return std::max(1, inner_height / std::max(1, row_height_));
}

Si32 ListBox::GetVisibleRows() const {
  return VisibleRows();
}

Si32 ListBox::HeightForRows(Si32 rows) const {
  return std::max(0, rows) * row_height_ + 2 * (border_.y + padding_);
}

Si32 ListBox::MaxFirstVisible() const {
  return std::max(0, (Si32)items_.size() - VisibleRows());
}

Si32 ListBox::RowAt(Vec2Si32 relative_pos) const {
  Si32 left = InnerLeft();
  if (relative_pos.x < left || relative_pos.x >= left + RowsWidth()) {
    return -1;
  }
  Si32 from_top = InnerTop() - 1 - relative_pos.y;
  if (from_top < 0 || row_height_ <= 0) {
    return -1;
  }
  Si32 row = from_top / row_height_;
  if (row >= VisibleRows()) {
    return -1;
  }
  Si32 item = first_visible_ + row;
  if (item >= (Si32)items_.size()) {
    return -1;
  }
  return item;
}

Si32 ListBox::ItemAt(Vec2Si32 relative_pos) const {
  return RowAt(relative_pos);
}

bool ListBox::GetRowRect(Si32 index, Vec2Si32 *out_pos,
                         Vec2Si32 *out_size) const {
  if (index < first_visible_ || index >= first_visible_ + VisibleRows() ||
      index >= (Si32)items_.size()) {
    return false;
  }
  Si32 row = index - first_visible_;
  *out_pos = Vec2Si32(InnerLeft(), InnerTop() - (row + 1) * row_height_);
  *out_size = Vec2Si32(RowsWidth(), row_height_);
  return true;
}

void ListBox::ClampScroll() {
  first_visible_ = std::min(std::max(first_visible_, 0), MaxFirstVisible());
}

void ListBox::SyncScrollbar() {
  if (!scrollbar_) {
    return;
  }
  Si32 max_first = MaxFirstVisible();
  scrollbar_->SetMinValue(0);
  scrollbar_->SetMaxValue(max_first);
  scrollbar_->SetValue(max_first - first_visible_);
  scrollbar_->SetStep(VisibleRows());
  scrollbar_->SetEnabled(is_enabled_ && max_first > 0);
}

void ListBox::LayoutScrollbar() {
  if (!scrollbar_) {
    return;
  }
  Si32 width = scrollbar_->GetSize().x;
  scrollbar_->SetPos(Vec2Si32(size_.x - border_.x - width, border_.y));
  scrollbar_->SetSize(Vec2Si32(width, std::max(0, size_.y - 2 * border_.y)));
}

void ListBox::EnsureVisible(Si32 item) {
  if (item < 0 || item >= (Si32)items_.size()) {
    return;
  }
  if (item < first_visible_) {
    first_visible_ = item;
  } else if (item >= first_visible_ + VisibleRows()) {
    first_visible_ = item - VisibleRows() + 1;
  }
  ClampScroll();
  SyncScrollbar();
}

void ListBox::SelectAndNotify(Si32 item,
                              std::deque<GuiMessage> *out_gui_messages) {
  if (item == selected_) {
    return;
  }
  selected_ = item;
  Emit(kGuiListSelectionChange, out_gui_messages);
  OnSelectionChange();
}

void ListBox::RegenerateSprites() {
  background_ = background_frame_.DrawExternalSize(size_);
  border_ = background_frame_.BorderSize();
  LayoutScrollbar();
  Vec2Si32 row_size(std::max(1, RowsWidth()), std::max(1, row_height_));
  selection_row_ = selection_frame_.DrawExternalSize(row_size);
  hover_row_ = hover_frame_.DrawExternalSize(row_size);
  ClampScroll();
  SyncScrollbar();
}

void ListBox::SetItems(const std::vector<std::string> &items) {
  items_ = items;
  selected_ = -1;
  hovered_ = -1;
  first_visible_ = 0;
  last_click_item_ = -1;
  SyncScrollbar();
}

void ListBox::AddItem(const std::string &item) {
  items_.push_back(item);
  SyncScrollbar();
}

void ListBox::RemoveItem(Si32 index) {
  if (index < 0 || index >= (Si32)items_.size()) {
    return;
  }
  items_.erase(items_.begin() + index);
  if (selected_ == index) {
    selected_ = -1;
  } else if (selected_ > index) {
    --selected_;
  }
  hovered_ = -1;
  last_click_item_ = -1;
  ClampScroll();
  SyncScrollbar();
}

void ListBox::ClearItems() {
  SetItems(std::vector<std::string>());
}

Si32 ListBox::GetItemCount() const {
  return (Si32)items_.size();
}

const std::string &ListBox::GetItem(Si32 index) const {
  if (index < 0 || index >= (Si32)items_.size()) {
    return EmptyString();
  }
  return items_[static_cast<size_t>(index)];
}

void ListBox::SetSelectedIndex(Si32 index) {
  if (index < 0 || index >= (Si32)items_.size()) {
    selected_ = -1;
    return;
  }
  selected_ = index;
}

Si32 ListBox::GetSelectedIndex() const {
  return selected_;
}

const std::string &ListBox::GetSelectedItem() const {
  return GetItem(selected_);
}

void ListBox::SetRowHeight(Si32 row_height) {
  row_height_ = std::max(1, row_height);
  RegenerateSprites();
}

Si32 ListBox::GetRowHeight() const {
  return row_height_;
}

void ListBox::SetFirstVisible(Si32 index) {
  first_visible_ = index;
  ClampScroll();
  SyncScrollbar();
}

Si32 ListBox::GetFirstVisible() const {
  return first_visible_;
}

void ListBox::SetFont(Font font) {
  font_ = font;
  Si32 line_height = font_.FontInstance() ? font_.LineHeight() : 1;
  row_height_ = std::max(1, line_height + 2 * padding_);
  RegenerateSprites();
}

void ListBox::SetEnabled(bool is_enabled) {
  is_enabled_ = is_enabled;
  hovered_ = -1;
  SyncScrollbar();
}

bool ListBox::IsEnabled() {
  return is_enabled_;
}

bool ListBox::TakesMouse() const {
  return true;
}

void ListBox::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  Panel::Draw(parent_absolute_pos);
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  Si32 scrollbar_width = scrollbar_ ? scrollbar_->GetSize().x : 0;
  Vec2Si32 clip_pos = absolute_pos + border_;
  Vec2Si32 clip_size(size_.x - 2 * border_.x - scrollbar_width,
                     size_.y - 2 * border_.y);
  if (clip_size.x <= 0 || clip_size.y <= 0) {
    return;
  }
  ClipScope clip(clip_pos, clip_size);
  Vec2Si32 shift = absolute_pos - clip.Origin();
  Si32 last = std::min((Si32)items_.size(), first_visible_ + VisibleRows());
  for (Si32 i = first_visible_; i < last; ++i) {
    Vec2Si32 row_pos;
    Vec2Si32 row_size;
    if (!GetRowRect(i, &row_pos, &row_size)) {
      continue;
    }
    Vec2Si32 at = shift + row_pos;
    if (i == selected_) {
      selection_row_.Draw(at, row_size);
    } else if (i == hovered_ && is_enabled_) {
      hover_row_.Draw(at, row_size);
    }
    if (font_.FontInstance()) {
      font_.Draw(items_[static_cast<size_t>(i)].c_str(),
                 at.x + padding_, at.y + row_size.y - padding_,
                 kTextOriginTop, kTextAlignmentLeft,
                 kDrawBlendingModeColorize, kFilterNearest,
                 is_enabled_ ? palete_ : disabled_palete_);
    }
  }
}

void ListBox::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                          bool is_top_level,
                          bool *in_out_is_applied,
                          std::deque<GuiMessage> *out_gui_messages,
                          std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - (parent_pos + pos_);
    if (IsWithin(relative_pos) && !*in_out_is_applied) {
      *in_out_is_applied = true;
      hovered_ = RowAt(relative_pos);
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1) {
        if (GetTabOrder() != 0) {
          *out_current_tab = shared_from_this();
          is_current_tab_ = true;
        }
        Si32 item = RowAt(relative_pos);
        if (item >= 0) {
          double now = Time();
          bool is_double_click = item == last_click_item_ &&
              now - last_click_time_ < kListDoubleClickSeconds;
          SelectAndNotify(item, out_gui_messages);
          Emit(kGuiListItemClick, out_gui_messages);
          OnItemClick();
          if (is_double_click) {
            last_click_item_ = -1;
            Emit(kGuiListItemActivate, out_gui_messages);
            OnItemActivate();
          } else {
            last_click_item_ = item;
            last_click_time_ = now;
          }
        }
      } else if (message.mouse.wheel_delta != 0) {
        // Wheel up shows the rows above.
        Si32 rows = -message.mouse.wheel_delta / 120;
        if (rows == 0) {
          rows = message.mouse.wheel_delta > 0 ? -1 : 1;
        }
        SetFirstVisible(first_visible_ + rows);
        hovered_ = RowAt(relative_pos);
      }
    } else {
      hovered_ = -1;
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (!*in_out_is_applied && is_current_tab_ &&
        (message.keyboard.key_state & 1u) == 1u && !items_.empty()) {
      Si32 n = (Si32)items_.size();
      Si32 target = -2;
      switch (message.keyboard.key) {
        case kKeyUp:
          target = selected_ < 0 ? 0 : selected_ - 1;
          break;
        case kKeyDown:
          target = selected_ < 0 ? 0 : selected_ + 1;
          break;
        case kKeyHome:
          target = 0;
          break;
        case kKeyEnd:
          target = n - 1;
          break;
        case kKeyPageUp:
          target = (selected_ < 0 ? 0 : selected_) - VisibleRows();
          break;
        case kKeyPageDown:
          target = (selected_ < 0 ? 0 : selected_) + VisibleRows();
          break;
        case kKeyEnter:
          if (selected_ >= 0) {
            *in_out_is_applied = true;
            Emit(kGuiListItemActivate, out_gui_messages);
            OnItemActivate();
          }
          break;
        default:
          break;
      }
      if (target != -2) {
        *in_out_is_applied = true;
        target = std::min(std::max(target, 0), n - 1);
        SelectAndNotify(target, out_gui_messages);
        EnsureVisible(target);
      }
    }
  }
}

Dropdown::Dropdown(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
                   Font font, Rgba color,
                   DecoratedFrame normal, DecoratedFrame hovered,
                   DecoratedFrame down, DecoratedFrame disabled,
                   DecoratedFrame list_background,
                   DecoratedFrame list_selection,
                   DecoratedFrame list_hover)
: Panel(tag, pos, size, tab_order)
, font_(font) {
  palete_.push_back(color);
  disabled_palete_.push_back(Rgba(128, 128, 128));
  frame_[kNormal] = normal;
  frame_[kHovered] = hovered;
  frame_[kDown] = down;
  frame_[kDisabled] = disabled;
  list_ = std::make_shared<ListBox>(0, Vec2Si32(0, 0), Vec2Si32(size.x, 1), 0,
                                    font, color, list_background,
                                    list_selection, list_hover);
  RegenerateSprites();
}

Dropdown::Dropdown(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(200, 32), (Ui32)tag)
, arrow_(theme->dropdown_arrow_)
, font_(theme->text_->font_)
, palete_(theme->text_->palete_)
, disabled_palete_(theme->text_->disabled_palete_)
, theme_(theme) {
  frame_[kNormal] = theme->button_->normal_;
  frame_[kHovered] = theme->button_->hovered_;
  frame_[kDown] = theme->button_->down_;
  frame_[kDisabled] = theme->button_->disabled_;
  list_ = std::make_shared<ListBox>(0, theme);
  RegenerateSprites();
}

void Dropdown::RegenerateSprites() {
  for (Si32 i = 0; i < 4; ++i) {
    box_[i] = frame_[i].DrawExternalSize(size_);
  }
  if (list_) {
    list_->SetSize(Vec2Si32(size_.x, list_->GetSize().y));
  }
}

void Dropdown::SetItems(const std::vector<std::string> &items) {
  items_ = items;
  selected_ = -1;
  if (is_open_) {
    RefreshList();
  }
}

void Dropdown::AddItem(const std::string &item) {
  items_.push_back(item);
  if (is_open_) {
    RefreshList();
  }
}

void Dropdown::ClearItems() {
  SetItems(std::vector<std::string>());
}

Si32 Dropdown::GetItemCount() const {
  return (Si32)items_.size();
}

const std::string &Dropdown::GetItem(Si32 index) const {
  if (index < 0 || index >= (Si32)items_.size()) {
    return EmptyString();
  }
  return items_[static_cast<size_t>(index)];
}

void Dropdown::SetSelectedIndex(Si32 index) {
  if (index < 0 || index >= (Si32)items_.size()) {
    selected_ = -1;
  } else {
    selected_ = index;
  }
  if (is_open_) {
    list_->SetSelectedIndex(selected_);
  }
}

Si32 Dropdown::GetSelectedIndex() const {
  return selected_;
}

const std::string &Dropdown::GetSelectedItem() const {
  return GetItem(selected_);
}

void Dropdown::SetMaxVisibleItems(Si32 max_visible_items) {
  max_visible_items_ = std::max(1, max_visible_items);
}

bool Dropdown::IsOpen() const {
  return is_open_;
}

void Dropdown::Close() {
  if (is_open_) {
    CloseList();
  }
}

std::shared_ptr<ListBox> Dropdown::GetList() const {
  return list_;
}

void Dropdown::ChooseAndNotify(Si32 index,
                               std::deque<GuiMessage> *out_gui_messages) {
  if (index < 0 || index >= (Si32)items_.size()) {
    index = -1;
  }
  if (index == selected_) {
    return;
  }
  selected_ = index;
  Emit(kGuiDropdownChange, out_gui_messages);
  OnChange();
}

void Dropdown::RefreshList() {
  list_->SetItems(items_);
  Si32 rows = std::min(max_visible_items_,
                       std::max(1, (Si32)items_.size()));
  list_->SetSize(Vec2Si32(size_.x, list_->HeightForRows(rows)));
  list_->SetSelectedIndex(selected_);
  list_->SetFirstVisible(selected_);
}

void Dropdown::OpenList(Vec2Si32 absolute_pos) {
  is_open_ = true;
  state_ = kDown;
  RefreshList();
  Si32 list_height = list_->GetSize().y;
  // Below the box when there is room, above it otherwise.
  if (absolute_pos.y - list_height >= 0) {
    list_->SetPos(Vec2Si32(0, -list_height));
  } else {
    list_->SetPos(Vec2Si32(0, size_.y));
  }
  list_->SetCurrentTab(true);
}

void Dropdown::CloseList() {
  is_open_ = false;
  list_->SetCurrentTab(false);
  if (state_ == kDown) {
    state_ = kNormal;
  }
}

Vec2Si32 Dropdown::ListParentPos(Vec2Si32 absolute_pos) const {
  return absolute_pos;
}

void Dropdown::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  box_[state_].Draw(absolute_pos, size_);
  const std::vector<Rgba> &palete =
      state_ == kDisabled ? disabled_palete_ : palete_;
  Si32 border = frame_[state_].BorderSize().x;
  Si32 text_pad = border + 4;
  Si32 arrow_width = arrow_.Width() > 0 ? arrow_.Width() : size_.y / 2;
  if (font_.FontInstance() && selected_ >= 0) {
    ClipScope clip(absolute_pos,
                   Vec2Si32(std::max(0, size_.x - arrow_width - text_pad),
                            size_.y));
    Vec2Si32 at = absolute_pos - clip.Origin();
    font_.Draw(items_[static_cast<size_t>(selected_)].c_str(),
               at.x + text_pad, at.y + size_.y / 2,
               kTextOriginCenter, kTextAlignmentLeft,
               kDrawBlendingModeColorize, kFilterNearest, palete);
  }
  if (arrow_.Width() > 0) {
    arrow_.Draw(absolute_pos + Vec2Si32(size_.x - text_pad - arrow_.Width(),
                                        (size_.y - arrow_.Height()) / 2));
  } else if (!palete.empty()) {
    // A triangle pointing down, the height of a third of the box.
    Si32 h = std::max(2, size_.y / 3);
    Si32 w = h * 2 - 1;
    Si32 x0 = absolute_pos.x + size_.x - text_pad - w;
    Si32 y_top = absolute_pos.y + (size_.y + h) / 2;
    DrawTriangle(Vec2Si32(x0, y_top), Vec2Si32(x0 + w, y_top),
                 Vec2Si32(x0 + w / 2, y_top - h), palete[0]);
  }
  Panel::Draw(parent_absolute_pos);
}

void Dropdown::DrawOverlays(Vec2Si32 absolute_pos) {
  Panel::DrawOverlays(absolute_pos);
  if (is_open_) {
    list_->Draw(ListParentPos(absolute_pos));
  }
}

void Dropdown::HandleOverlayInput(Vec2Si32 absolute_pos,
                                  const InputMessage &message,
                                  bool *in_out_is_applied,
                                  std::deque<GuiMessage> *out_gui_messages,
                                  std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleOverlayInput(absolute_pos, message, in_out_is_applied,
                            out_gui_messages, out_current_tab);
  if (!is_open_ || *in_out_is_applied) {
    return;
  }
  // The list reports into a queue of its own: the host hears about the
  // dropdown, not about the list inside it.
  std::deque<GuiMessage> list_messages;
  if (message.kind == InputMessage::kMouse) {
    list_->ApplyInput(ListParentPos(absolute_pos), message, false,
                      in_out_is_applied, &list_messages, out_current_tab);
    for (auto it = list_messages.begin(); it != list_messages.end(); ++it) {
      if (it->kind == kGuiListItemClick) {
        ChooseAndNotify(list_->GetSelectedIndex(), out_gui_messages);
        CloseList();
        break;
      }
    }
    if (!*in_out_is_applied &&
        message.keyboard.key == kKeyMouseLeft &&
        message.keyboard.key_state == 1 &&
        !IsWithin(message.mouse.backbuffer_pos - absolute_pos)) {
      // A press anywhere else closes the list and goes no further. A press
      // on the box itself is left to HandleInput, which closes it as well.
      CloseList();
      *in_out_is_applied = true;
    }
  } else if (message.kind == InputMessage::kKeyboard &&
             (message.keyboard.key_state & 1u) == 1u) {
    if (message.keyboard.key == kKeyEscape) {
      CloseList();
      *in_out_is_applied = true;
    } else if (message.keyboard.key == kKeyEnter) {
      if (list_->GetSelectedIndex() >= 0) {
        ChooseAndNotify(list_->GetSelectedIndex(), out_gui_messages);
      }
      CloseList();
      *in_out_is_applied = true;
    } else {
      list_->ApplyInput(ListParentPos(absolute_pos), message, false,
                        in_out_is_applied, &list_messages, out_current_tab);
    }
  }
}

bool Dropdown::IsOverlayTransparentAt(Vec2Si32 absolute_pos,
                                      Vec2Si32 mouse_pos) {
  if (is_open_) {
    Vec2Si32 relative = mouse_pos - ListParentPos(absolute_pos) -
        list_->GetPos();
    Vec2Si32 list_size = list_->GetSize();
    if (relative.x >= 0 && relative.y >= 0 &&
        relative.x < list_size.x && relative.y < list_size.y) {
      return false;
    }
  }
  return Panel::IsOverlayTransparentAt(absolute_pos, mouse_pos);
}

void Dropdown::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                           bool is_top_level,
                           bool *in_out_is_applied,
                           std::deque<GuiMessage> *out_gui_messages,
                           std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  Vec2Si32 absolute_pos = parent_pos + pos_;
  if (message.kind == InputMessage::kMouse) {
    bool is_inside = IsWithin(message.mouse.backbuffer_pos - absolute_pos);
    if (is_inside && !*in_out_is_applied) {
      *in_out_is_applied = true;
      if (!is_open_) {
        state_ = kHovered;
      }
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1) {
        if (GetTabOrder() != 0) {
          *out_current_tab = shared_from_this();
          is_current_tab_ = true;
        }
        if (is_open_) {
          CloseList();
          state_ = kHovered;
        } else if (!items_.empty()) {
          OpenList(absolute_pos);
        }
      }
    } else if (!is_open_ && state_ == kHovered && !is_inside) {
      state_ = kNormal;
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (!*in_out_is_applied && is_current_tab_ && !is_open_ &&
        (message.keyboard.key_state & 1u) == 1u && !items_.empty()) {
      Si32 n = (Si32)items_.size();
      switch (message.keyboard.key) {
        case kKeyDown:
          *in_out_is_applied = true;
          ChooseAndNotify(std::min(selected_ + 1, n - 1), out_gui_messages);
          break;
        case kKeyUp:
          *in_out_is_applied = true;
          ChooseAndNotify(std::max(selected_ - 1, 0), out_gui_messages);
          break;
        case kKeyHome:
          *in_out_is_applied = true;
          ChooseAndNotify(0, out_gui_messages);
          break;
        case kKeyEnd:
          *in_out_is_applied = true;
          ChooseAndNotify(n - 1, out_gui_messages);
          break;
        case kKeyEnter:
        case kKeySpace:
          *in_out_is_applied = true;
          OpenList(absolute_pos);
          break;
        default:
          break;
      }
    }
  }
}

bool Dropdown::TakesMouse() const {
  return true;
}

void Dropdown::SetEnabled(bool is_enabled) {
  if (is_enabled) {
    if (state_ == kDisabled) {
      state_ = kNormal;
    }
  } else {
    Close();
    state_ = kDisabled;
  }
}

bool Dropdown::IsEnabled() {
  return state_ != kDisabled;
}

void Dropdown::SetCurrentTab(bool is_current_tab) {
  Panel::SetCurrentTab(is_current_tab);
  if (!is_current_tab) {
    // The focus went elsewhere, and a list nobody can type into is a list
    // nobody expects to see.
    Close();
  }
}

TabControl::TabControl(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
                       Font font, Rgba color,
                       DecoratedFrame header_normal,
                       DecoratedFrame header_selected,
                       DecoratedFrame header_hovered, DecoratedFrame page,
                       Si32 header_height)
: Panel(tag, pos, size, tab_order)
, header_height_(std::max(1, header_height))
, font_(font)
, page_frame_(page) {
  palete_.push_back(color);
  header_frame_[0] = header_normal;
  header_frame_[1] = header_selected;
  header_frame_[2] = header_hovered;
}

TabControl::TabControl(Ui64 tag, std::shared_ptr<GuiTheme> theme)
: Panel(tag, Vec2Si32(0, 0), Vec2Si32(300, 200), (Ui32)tag)
, font_(theme->text_->font_)
, palete_(theme->text_->palete_)
, page_frame_(theme->tab_page_)
, theme_(theme) {
  header_frame_[0] = theme->tab_normal_;
  header_frame_[1] = theme->tab_selected_;
  header_frame_[2] = theme->tab_hovered_;
  Si32 line_height = font_.FontInstance() ? font_.LineHeight() : 16;
  header_height_ = line_height + 2 * header_padding_;
}

void TabControl::GetPageRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const {
  *out_pos = Vec2Si32(0, 0);
  *out_size = Vec2Si32(size_.x, std::max(0, size_.y - header_height_));
}

bool TabControl::GetHeaderRect(Si32 index, Vec2Si32 *out_pos,
                               Vec2Si32 *out_size) const {
  if (index < 0 || index >= (Si32)tabs_.size()) {
    return false;
  }
  Si32 x = 0;
  for (Si32 i = 0; i < index; ++i) {
    x += tabs_[static_cast<size_t>(i)].width;
  }
  *out_pos = Vec2Si32(x, size_.y - header_height_);
  *out_size = Vec2Si32(tabs_[static_cast<size_t>(index)].width,
                       header_height_);
  return true;
}

Si32 TabControl::HeaderAt(Vec2Si32 relative_pos) const {
  if (relative_pos.y < size_.y - header_height_ || relative_pos.y >= size_.y ||
      relative_pos.x < 0) {
    return -1;
  }
  Si32 x = 0;
  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (relative_pos.x < x + tabs_[i].width) {
      return (Si32)i;
    }
    x += tabs_[i].width;
  }
  return -1;
}

void TabControl::LayoutTabs() {
  for (size_t i = 0; i < tabs_.size(); ++i) {
    Tab &tab = tabs_[i];
    Si32 text_width = font_.FontInstance() ?
        font_.EvaluateSize(tab.title.c_str(), false).x : 0;
    tab.width = std::max(1, text_width + 2 * header_padding_);
    Vec2Si32 header_size(tab.width, header_height_);
    for (Si32 k = 0; k < 3; ++k) {
      tab.header[k] = header_frame_[k].DrawExternalSize(header_size);
    }
  }
}

void TabControl::RegenerateSprites() {
  Vec2Si32 page_pos;
  Vec2Si32 page_size;
  GetPageRect(&page_pos, &page_size);
  Sprite page_background = page_frame_.DrawExternalSize(page_size);
  for (size_t i = 0; i < tabs_.size(); ++i) {
    tabs_[i].page->SetPos(page_pos);
    tabs_[i].page->SetSize(page_size);
    tabs_[i].page->SetBackground(page_background);
  }
  LayoutTabs();
}

std::shared_ptr<Panel> TabControl::AddTab(const std::string &title,
                                          Ui64 tag) {
  Tab tab;
  tab.title = title;
  if (tag == 0) {
    ++next_page_tag_;
    tag = tag_ * 1000 + next_page_tag_;
  }
  Vec2Si32 page_pos;
  Vec2Si32 page_size;
  GetPageRect(&page_pos, &page_size);
  tab.page = std::make_shared<Panel>(tag, page_pos, page_size, 0,
                                     page_frame_.DrawExternalSize(page_size));
  tab.page->SetVisible(tabs_.empty());
  Panel::AddChild(tab.page);
  tabs_.push_back(tab);
  if (selected_ < 0) {
    selected_ = 0;
  }
  LayoutTabs();
  return tab.page;
}

void TabControl::RemoveTab(Si32 index) {
  if (index < 0 || index >= (Si32)tabs_.size()) {
    return;
  }
  Panel::RemoveChild(tabs_[static_cast<size_t>(index)].page);
  tabs_.erase(tabs_.begin() + index);
  hovered_ = -1;
  if (tabs_.empty()) {
    selected_ = -1;
    return;
  }
  if (selected_ == index) {
    selected_ = std::min(index, (Si32)tabs_.size() - 1);
    tabs_[static_cast<size_t>(selected_)].page->SetVisible(true);
  } else if (selected_ > index) {
    --selected_;
  }
}

Si32 TabControl::GetTabCount() const {
  return (Si32)tabs_.size();
}

std::shared_ptr<Panel> TabControl::GetPage(Si32 index) const {
  if (index < 0 || index >= (Si32)tabs_.size()) {
    return Panel::Invalid();
  }
  return tabs_[static_cast<size_t>(index)].page;
}

const std::string &TabControl::GetTitle(Si32 index) const {
  if (index < 0 || index >= (Si32)tabs_.size()) {
    return EmptyString();
  }
  return tabs_[static_cast<size_t>(index)].title;
}

void TabControl::SetTitle(Si32 index, const std::string &title) {
  if (index < 0 || index >= (Si32)tabs_.size()) {
    return;
  }
  tabs_[static_cast<size_t>(index)].title = title;
  LayoutTabs();
}

void TabControl::SetSelectedIndex(Si32 index) {
  if (index < 0 || index >= (Si32)tabs_.size() || index == selected_) {
    return;
  }
  if (selected_ >= 0) {
    tabs_[static_cast<size_t>(selected_)].page->SetVisible(false);
  }
  selected_ = index;
  tabs_[static_cast<size_t>(selected_)].page->SetVisible(true);
}

Si32 TabControl::GetSelectedIndex() const {
  return selected_;
}

void TabControl::SelectAndNotify(Si32 index,
                                 std::deque<GuiMessage> *out_gui_messages) {
  if (index == selected_ || index < 0 || index >= (Si32)tabs_.size()) {
    return;
  }
  SetSelectedIndex(index);
  Emit(kGuiTabChange, out_gui_messages);
  OnTabChange();
}

void TabControl::SetHeaderHeight(Si32 header_height) {
  header_height_ = std::max(1, header_height);
  RegenerateSprites();
}

void TabControl::SetEnabled(bool is_enabled) {
  is_enabled_ = is_enabled;
  hovered_ = -1;
}

bool TabControl::IsEnabled() {
  return is_enabled_;
}

void TabControl::Draw(Vec2Si32 parent_absolute_pos) {
  if (!IsVisible()) {
    return;
  }
  Panel::Draw(parent_absolute_pos);
  Vec2Si32 absolute_pos = parent_absolute_pos + pos_;
  for (size_t i = 0; i < tabs_.size(); ++i) {
    Vec2Si32 header_pos;
    Vec2Si32 header_size;
    GetHeaderRect((Si32)i, &header_pos, &header_size);
    Si32 look = 0;
    if ((Si32)i == selected_) {
      look = 1;
    } else if ((Si32)i == hovered_ && is_enabled_) {
      look = 2;
    }
    Vec2Si32 at = absolute_pos + header_pos;
    tabs_[i].header[look].Draw(at, header_size);
    if (font_.FontInstance() && !palete_.empty()) {
      font_.Draw(tabs_[i].title.c_str(),
                 at.x + header_size.x / 2, at.y + header_size.y / 2,
                 kTextOriginCenter, kTextAlignmentCenter,
                 kDrawBlendingModeColorize, kFilterNearest, palete_);
    }
  }
}

bool TabControl::IsMouseTransparentAt(Vec2Si32 parent_pos,
                                      Vec2Si32 mouse_pos) {
  if (!Panel::IsMouseTransparentAt(parent_pos, mouse_pos)) {
    return false;
  }
  Vec2Si32 relative_pos = mouse_pos - (parent_pos + pos_);
  return !(IsWithin(relative_pos) &&
           relative_pos.y >= size_.y - header_height_);
}

void TabControl::HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
                             bool is_top_level,
                             bool *in_out_is_applied,
                             std::deque<GuiMessage> *out_gui_messages,
                             std::shared_ptr<Panel> *out_current_tab) {
  Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
                     out_gui_messages, out_current_tab);
  if (message.kind == InputMessage::kMouse) {
    Vec2Si32 relative_pos = message.mouse.backbuffer_pos - (parent_pos + pos_);
    bool is_on_headers = IsWithin(relative_pos) &&
        relative_pos.y >= size_.y - header_height_;
    if (is_on_headers && !*in_out_is_applied) {
      *in_out_is_applied = true;
      hovered_ = HeaderAt(relative_pos);
      if (message.keyboard.key == kKeyMouseLeft &&
          message.keyboard.key_state == 1 && hovered_ >= 0) {
        if (GetTabOrder() != 0) {
          *out_current_tab = shared_from_this();
          is_current_tab_ = true;
        }
        SelectAndNotify(hovered_, out_gui_messages);
      }
    } else {
      hovered_ = -1;
    }
  } else if (message.kind == InputMessage::kKeyboard) {
    if (!*in_out_is_applied && is_current_tab_ && !tabs_.empty() &&
        (message.keyboard.key_state & 1u) == 1u) {
      Si32 n = (Si32)tabs_.size();
      if (message.keyboard.key == kKeyLeft) {
        *in_out_is_applied = true;
        SelectAndNotify(std::max(0, selected_ - 1), out_gui_messages);
      } else if (message.keyboard.key == kKeyRight) {
        *in_out_is_applied = true;
        SelectAndNotify(std::min(n - 1, selected_ + 1), out_gui_messages);
      }
    }
  }
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

  // The entries below are optional: a theme written before these widgets
  // existed borrows their looks from the widgets it does describe.
  auto optional_sprite = [&ctx](const char *node_name, const Sprite &fallback) {
    pugi::XmlNode node = ctx.doc.child(node_name);
    if (node && node.attribute("path")) {
      return ctx.LoadSprite(node.attribute("path").as_string());
    }
    return fallback;
  };
  auto optional_frame = [&ctx](const char *node_name,
                               const DecoratedFrame &fallback,
                               DecoratedFrame *out_frame) {
    if (ctx.doc.child(node_name)) {
      LoadDecoratedFrame(ctx, node_name, out_frame);
    } else {
      *out_frame = fallback;
    }
  };

  radio_clear_normal_ = optional_sprite("radio_clear_normal", checkbox_clear_normal_);
  radio_checked_normal_ = optional_sprite("radio_checked_normal", checkbox_checked_normal_);
  radio_clear_down_ = optional_sprite("radio_clear_down", checkbox_clear_down_);
  radio_checked_down_ = optional_sprite("radio_checked_down", checkbox_checked_down_);
  radio_clear_hovered_ = optional_sprite("radio_clear_hovered", checkbox_clear_hovered_);
  radio_checked_hovered_ = optional_sprite("radio_checked_hovered", checkbox_checked_hovered_);
  radio_clear_disabled_ = optional_sprite("radio_clear_disabled", checkbox_clear_disabled_);
  radio_checked_disabled_ = optional_sprite("radio_checked_disabled", checkbox_checked_disabled_);

  h_slider_ = std::make_shared<GuiThemeSlider>();
  optional_frame("slider_track", h_scrollbar_->normal_background_, &h_slider_->track_);
  optional_frame("slider_thumb_normal", button_->normal_, &h_slider_->thumb_normal_);
  optional_frame("slider_thumb_hovered", button_->hovered_, &h_slider_->thumb_hovered_);
  optional_frame("slider_thumb_down", button_->down_, &h_slider_->thumb_down_);
  optional_frame("slider_thumb_disabled", button_->disabled_, &h_slider_->thumb_disabled_);
  h_slider_->thumb_length_ = ctx.doc.child("slider_thumb_normal").attribute("thumb_length").as_int(16);
  h_slider_->is_horizontal_ = true;
  v_slider_ = std::make_shared<GuiThemeSlider>(*h_slider_);
  optional_frame("slider_track", v_scrollbar_->normal_background_, &v_slider_->track_);
  v_slider_->is_horizontal_ = false;

  optional_frame("listbox_background", editbox_normal_, &listbox_background_);
  optional_frame("listbox_selection", button_->down_, &listbox_selection_);
  optional_frame("listbox_hover", button_->hovered_, &listbox_hover_);

  dropdown_arrow_ = optional_sprite("dropdown_arrow", Sprite());

  optional_frame("tab_normal", button_->normal_, &tab_normal_);
  optional_frame("tab_selected", button_->down_, &tab_selected_);
  optional_frame("tab_hovered", button_->hovered_, &tab_hovered_);
  optional_frame("tab_page", panel_background_, &tab_page_);

  tooltip_ = std::make_shared<GuiThemeTooltip>();
  optional_frame("tooltip_frame", panel_background_, &tooltip_->frame_);
  tooltip_->font_ = text_->font_;
  tooltip_->color_ = text_->palete_.empty() ? Rgba(255, 255, 255) : text_->palete_[0];
  tooltip_->delay_seconds_ = ctx.doc.child("tooltip_frame").attribute("delay_seconds").as_double(0.5);
  tooltip_->padding_ = ctx.doc.child("tooltip_frame").attribute("padding").as_int(4);
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

std::shared_ptr<RadioButton> GuiFactory::MakeRadioButton(Si32 group) {
  ++last_tag_;
  return std::make_shared<RadioButton>(last_tag_, theme_, group);
}

std::shared_ptr<Image> GuiFactory::MakeImage(Sprite sprite,
                                             Image::ScaleMode scale_mode) {
  ++last_tag_;
  Vec2Si32 size = sprite.Size();
  return std::make_shared<Image>(last_tag_, Vec2Si32(0, 0), size, sprite,
                                 scale_mode);
}

std::shared_ptr<Slider> GuiFactory::MakeHorizontalSlider() {
  ++last_tag_;
  return std::make_shared<Slider>(last_tag_, theme_->h_slider_);
}

std::shared_ptr<Slider> GuiFactory::MakeVerticalSlider() {
  ++last_tag_;
  return std::make_shared<Slider>(last_tag_, theme_->v_slider_);
}

std::shared_ptr<ListBox> GuiFactory::MakeListBox() {
  ++last_tag_;
  return std::make_shared<ListBox>(last_tag_, theme_);
}

std::shared_ptr<Dropdown> GuiFactory::MakeDropdown() {
  ++last_tag_;
  return std::make_shared<Dropdown>(last_tag_, theme_);
}

std::shared_ptr<TabControl> GuiFactory::MakeTabControl() {
  ++last_tag_;
  return std::make_shared<TabControl>(last_tag_, theme_);
}



}  // namespace arctic


