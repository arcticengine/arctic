// The MIT License (MIT)
//
// Copyright (c) 2020 - 2022 Huldra
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

#ifndef ENGINE_NODE2F_H_
#define ENGINE_NODE2F_H_

#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/arctic_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/font.h"
#include "engine/transform2f.h"

namespace arctic {

class Node2F : public std::enable_shared_from_this<Node2F> {
 protected:
  Transform2F transform_;
  float z_order_ = 0;
  Ui32 flags_ = 0;
  Ui32 child_idx_ = 0;
  Node2F *parent_ = nullptr;
  std::deque<std::shared_ptr<Node2F>> children_;

 public:
  static constexpr Ui32 kFlagVisible = 1;
  static constexpr Ui32 kFlagRelativeZOrder = 2;

  Node2F() {
  }

  virtual ~Node2F() {
    while (!children_.empty()) {
      children_.back()->RemoveFromParent();
    }
  }

  Transform2F GetTransform() const {
    return transform_;
  }

  void SetTransform(Transform2F transform) {
    transform_ = transform;
  }

  Transform2F &Transform() {
    return transform_;
  }

  void SetPosition(Vec2F position) {
    transform_.SetPosition(position);
  }

  void SetPosition(float x, float y) {
    transform_.SetPosition(x, y);
  }

  Ui32 GetZOrder() const {
    return z_order_;
  }

  void SetZOrder(Ui32 z_order) {
    z_order_ = z_order;
  }

  void SetVisible(bool is_visible) {
    flags_ = is_visible ? (flags_ | kFlagVisible) : (flags_ & ~kFlagVisible);
  }

  bool IsVisible() const {
    return flags_ & kFlagVisible;
  }

  void SetRelativeZOrder(bool is_relative) {
    flags_ = is_relative
      ? (flags_ | kFlagRelativeZOrder)
      : (flags_ & ~kFlagRelativeZOrder);
  }

  bool IsRelativeZOrder() const {
    return flags_ & kFlagRelativeZOrder;
  }

  void AddChild(std::shared_ptr<Node2F> child, float z_order) {
    if (child) {
      if (child.get() == this) {
        return;
      }
      if (child->parent_) {
        if (child->parent_ == this) {
          return;
        }
        child->RemoveFromParent();
      }
      child->parent_ = this;
      child->child_idx_ = (Ui32)children_.size();
      children_.push_back(child);
    }
  }

  void RemoveFromParent() {
    if (parent_) {
      if (parent_->children_.size() <= child_idx_) {
        *Log() << "Error: child out of bounds. children.size="
          << parent_->children_.size() << " child_idx_=" << child_idx_;
        return;
      }
      if (parent_->children_[child_idx_].get() != this) {
        *Log() << "Error: child is not located at child_idx_";
        return;
      }
      std::shared_ptr<Node2F> this_ptr(
          std::move(parent_->children_[child_idx_]));
      if (child_idx_ != parent_->children_.size() - 1) {
        parent_->children_[child_idx_] = std::move(parent_->children_.back());
        parent_->children_[child_idx_]->child_idx_ = child_idx_;
      }
      parent_->children_.pop_back();
      parent_ = nullptr;
      child_idx_ = 0;
      return;
    }
  }

  void Draw() {
    DrawSelf(transform_);
    for (auto &child : children_) {
      child->Draw(transform_);
    }
  }

  void Draw(Transform2F parent_transform) {
    Transform2F transform;
    transform.dc = parent_transform.dc * transform_.dc.TranslationScaled(
        parent_transform.scale);
    transform.scale = parent_transform.scale * transform_.scale;
    DrawSelf(transform);
    for (auto &child : children_) {
      child->Draw(transform);
    }
  }

  virtual void DrawSelf(const Transform2F &transform) {
  }
};

class SpriteNode2F : public Node2F {
 protected:
  Sprite sprite_;
  DrawBlendingMode blending_mode_ = kDrawBlendingModeCopyRgba;
  DrawFilterMode filter_mode_ = kFilterNearest;
  Rgba color_ = Rgba(255, 255, 255);

 public:
  explicit SpriteNode2F(Sprite sprite)
    : sprite_(sprite)
  {}

  void SetDrawBlendingMode(DrawBlendingMode blending_mode) {
    blending_mode_ = blending_mode;
  }

  void SetDrawFilterMode(DrawFilterMode filter_mode) {
    filter_mode_ = filter_mode;
  }

  void SetColor(Rgba color) {
    color_ = color;
  }

  void DrawSelf(const Transform2F &transform) override {
    Vec2F pos = transform.dc.Transform(Vec2F(0.f, 0.f));
    sprite_.Draw(color_, pos.x, pos.y, transform.dc.GetAngle(), transform.scale,
      blending_mode_, filter_mode_);
  }
};

}  // namespace arctic

#endif  // ENGINE_NODE2F_H_
