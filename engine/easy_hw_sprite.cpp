// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
// Copyright (c) 2021 Vlad2001_MFS
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

#include "engine/easy_hw_sprite.h"

#include <cstring>

#include <memory>
#include <utility>
#include <vector>
#include <sstream>

#include "engine/arctic_types.h"
#include "engine/vec2f.h"
#include "engine/vec3f.h"
#include "engine/log.h"
#include "engine/easy_advanced.h"
#include "engine/easy_files.h"
#include "engine/rgba.h"
#include "engine/gl_state.h"

namespace arctic {

void HwSprite::DrawSprite(const HwSprite &to_sprite,
    const float to_x_pivot, const float to_y_pivot, const float to_width, const float to_height,
    const HwSprite &from_sprite, const float from_x, const float from_y, const float from_width, const float from_height,
    Rgba in_color, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, float angle_radians) {
  HwSpriteDrawing *h = GetEngine()->AddHwSpriteDrawing();
  h->to_x_pivot = to_x_pivot;
  h->to_y_pivot = to_y_pivot;
  h->to_width = to_width;
  h->to_height = to_height;
  h->from_sprite = from_sprite;
  h->from_x = from_x;
  h->from_y = from_y;
  h->from_width = from_width;
  h->from_height = from_height;
  h->in_color = in_color;
  h->blending_mode = blending_mode;
  h->filter_mode = filter_mode;
  h->angle_radians = angle_radians;

  return;
}

void HwSprite::UpdateVertexBuffer(float angle) const {
    if (!gl_buffer_) {
        gl_buffer_ = std::make_unique<GlBuffer>();
        gl_buffer_->Create();
        gl_buffer_->SetData(nullptr, sizeof(float)*4*6);
    }

    if (!gl_buffer_->IsValid()
      || last_buffer_pivot_ != pivot_
      || last_buffer_ref_size_ != ref_size_
      || fabs(last_angle_ - angle) > 0.001f) {
        last_buffer_pivot_ = pivot_;
        last_buffer_ref_size_ = ref_size_;
        last_angle_ = angle;

        float sin_a = sinf(angle);
        float cos_a = cosf(angle);
        Vec2F left = Vec2F(-cos_a, -sin_a) * static_cast<float>(Pivot().x);
        Vec2F right = Vec2F(cos_a, sin_a) * static_cast<float>(Width() - Pivot().x);
        Vec2F up = Vec2F(-sin_a, cos_a) * static_cast<float>(Height() - Pivot().y);
        Vec2F down = Vec2F(sin_a, -cos_a) * static_cast<float>(Pivot().y);

        // d c
        // a b
        Vec2F a(left + down);
        Vec2F b(right + down);
        Vec2F c(right + up);
        Vec2F d(left + up);

        const Vec2F kVerts[] = {
            a, Vec2F(0.0f, 0.0f),
            b, Vec2F(1.0f, 0.0f),
            c, Vec2F(1.0f, 1.0f),

            d, Vec2F(0.0f, 1.0f),
            a, Vec2F(0.0f, 0.0f),
            c, Vec2F(1.0f, 1.0f),
        };

        gl_buffer_->UpdateData(kVerts);
    }
}



HwSprite::HwSprite() {
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(0, 0);
  pivot_ = Vec2Si32(0, 0);
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;
}

HwSprite::HwSprite(const HwSprite &other) {
  sprite_instance_ = other.sprite_instance_;
  ref_pos_ = other.ref_pos_;
  ref_size_ = other.ref_size_;
  pivot_ = other.pivot_;
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;
}

HwSprite::HwSprite(HwSprite &&other) noexcept {
  sprite_instance_ = other.sprite_instance_;
  ref_pos_ = other.ref_pos_;
  ref_size_ = other.ref_size_;
  pivot_ = other.pivot_;
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;

  other.sprite_instance_ = nullptr;
  other.ref_pos_ = Vec2Si32(0, 0);
  other.ref_size_ = Vec2Si32(0, 0);
  other.pivot_ = Vec2Si32(0, 0);
  other.gl_buffer_ = nullptr;
  other.last_buffer_pivot_ = Vec2Si32(0, 0);
  other.last_buffer_ref_size_ = Vec2Si32(0, 0);
  other.last_angle_ = 0.0f;
}

HwSprite &HwSprite::operator=(const HwSprite &other) {
  sprite_instance_ = other.sprite_instance_;
  ref_pos_ = other.ref_pos_;
  ref_size_ = other.ref_size_;
  pivot_ = other.pivot_;
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;

  return *this;
}

HwSprite &HwSprite::operator=(HwSprite &&other) noexcept {
  sprite_instance_ = other.sprite_instance_;
  ref_pos_ = other.ref_pos_;
  ref_size_ = other.ref_size_;
  pivot_ = other.pivot_;
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;

  other.sprite_instance_ = nullptr;
  other.ref_pos_ = Vec2Si32(0, 0);
  other.ref_size_ = Vec2Si32(0, 0);
  other.pivot_ = Vec2Si32(0, 0);
  other.gl_buffer_ = nullptr;
  other.last_buffer_pivot_ = Vec2Si32(0, 0);
  other.last_buffer_ref_size_ = Vec2Si32(0, 0);
  other.last_angle_ = 0.0f;

  return *this;
}

void HwSprite::LoadFromData(const Ui8* data, Ui64 size_bytes,
    const char *file_name) {
  if (!file_name) {
    *Log() << "Error in HwSprite::Load, file_name is nullptr."
      " Not loading sprite.";
    return;
  }
  if (data == nullptr) {
    *Log() << "Error in HwSprite::Load, file: \""
      << file_name << "\" could not be loaded, data=nullptr."
      " Not loading sprite.";
    return;
  }
  const char *last_dot = strrchr(file_name, '.');
  if (!last_dot) {
    *Log() << "Error in HwSprite::Load, file: \""
      << file_name << "\" has no extension."
      " Not loading sprite.";
    return;
  }
  if (strcmp(last_dot, ".tga") == 0) {
    if (size_bytes == 0) {
      *Log() << "Error in HwSprite::Load, file: \""
        << file_name << "\" could not be loaded (size=0)."
          " Not loading sprite.";
      return;
    }
    sprite_instance_ = HwSpriteInstance::LoadTga(data, static_cast<Si64>(size_bytes));
    if (!sprite_instance_) {
      *Log() << "Error in HwSprite::Load, file: \""
        << file_name << "\" could not be loaded with LoadTga."
          " Not loading sprite.";
      return;
    }
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(sprite_instance_->width(),
                         sprite_instance_->height());
    pivot_ = Vec2Si32(0, 0);
    gl_buffer_ = nullptr;
    last_buffer_pivot_ = Vec2Si32(0, 0);
    last_buffer_ref_size_ = Vec2Si32(0, 0);
    last_angle_ = 0.0f;
  } else {
    *Log() << "Error in HwSprite::Load, file: \""
      << file_name << "\" could not be loaded,"
        " unknown file extension: \"" << last_dot << "\"."
        " Not loading sprite.";
    return;
  }
}

void HwSprite::Load(const char *file_name) {
  if (!file_name) {
    *Log() << "Error in HwSprite::Load, file_name is nullptr."
      " Not loading sprite.";
    return;
  }
  const char *last_dot = strrchr(file_name, '.');
  if (!last_dot) {
    *Log() << "Error in HwSprite::Load, file: \""
      << file_name << "\" has no extension."
      " Not loading sprite.";
    return;
  }
  if (strcmp(last_dot, ".tga") == 0) {
    std::vector<Ui8> data = ReadFile(file_name, true);
    if (data.empty()) {
      *Log() << "Error in HwSprite::Load, file: \""
        << file_name << "\" could not be loaded (data is empty)."
          " Not loading sprite.";
      return;
    }
    sprite_instance_ = HwSpriteInstance::LoadTga(data.data(), static_cast<Si64>(data.size()));
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = sprite_instance_ ? Vec2Si32(sprite_instance_->width(),
      sprite_instance_->height()) : Vec2Si32(0, 0);
    pivot_ = Vec2Si32(0, 0);
    gl_buffer_ = nullptr;
    last_buffer_pivot_ = Vec2Si32(0, 0);
    last_buffer_ref_size_ = Vec2Si32(0, 0);
    last_angle_ = 0.0f;
  } else {
    *Log() << "Error in HwSprite::Load, file: \""
      << file_name << "\" could not be loaded,"
        " unknown file extension: \"" << last_dot << "\"."
        " Not loading sprite.";
    return;
  }
}

void HwSprite::Load(const std::string &file_name) {
  Load(file_name.c_str());
}

void HwSprite::LoadFromSoftwareSprite(Sprite sw_sprite) {
    const std::shared_ptr<SpriteInstance> &sw_sprite_instance = sw_sprite.SpriteInstance();
    sprite_instance_ = std::make_shared<HwSpriteInstance>(sw_sprite_instance->width(), sw_sprite_instance->height());
    sprite_instance_->texture().UpdateData(sw_sprite_instance->RawData());
    ref_pos_ = sw_sprite.RefPos();
    ref_size_ = sw_sprite.Size();
    pivot_ = sw_sprite.Pivot();
    gl_buffer_ = nullptr;
    last_buffer_pivot_ = Vec2Si32(0, 0);
    last_buffer_ref_size_ = Vec2Si32(0, 0);
    last_angle_ = 0.0f;
}

/*void HwSprite::Save(const char *file_name) {
  std::vector<Ui8> data = SaveToData(file_name);
  if (!data.empty()) {
    WriteFile(file_name, data.data(), data.size());
  }
}

void HwSprite::Save(const std::string &file_name) {
  Save(file_name.c_str());
}

std::vector<Ui8> HwSprite::SaveToData(const char *file_name) {
  std::vector<Ui8> data;
  Check(!!file_name, "Error in HwSprite::Save, file_name is nullptr.");
  const char *last_dot = strrchr(file_name, '.');
  Check(!!last_dot, "Error in HwSprite::Save, file_name has no extension.");
  if (strcmp(last_dot, ".tga") == 0) {
      HwSpriteInstance::SaveTga(sprite_instance_, &data);
  } else {
    Fatal("Error in HwSprite::Save, unknown file extension.");
  }
  return data;
}*/

void HwSprite::Create(const Vec2Si32 size) {
  Create(size.x, size.y);
}

void HwSprite::Create(const Si32 width, const Si32 height) {
  sprite_instance_ = std::make_shared<HwSpriteInstance>(width, height);
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(width, height);
  pivot_ = Vec2Si32(0, 0);
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;
  Clear();
}

void HwSprite::InvReference(const HwSprite &from, const Si32 from_x, const Si32 from_inv_y,
    const Si32 from_width, const Si32 from_height) {
  Reference(from, from_x, from.ref_size_.y - from_inv_y - from_height, from_width , from_height);
}

void HwSprite::Reference(const HwSprite &from, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height) {
  ref_pos_ = Vec2Si32(
    from.ref_pos_.x + std::min(std::max(from_x, 0), from.ref_size_.x - 1),
    from.ref_pos_.y + std::min(std::max(from_y, 0), from.ref_size_.y - 1));
  const Vec2Si32 max_size = from.ref_pos_ + from.ref_size_ - ref_pos_;
  ref_size_ = Vec2Si32(
    std::min(from_width, max_size.x),
    std::min(from_height, max_size.y));
  pivot_ = Vec2Si32(0, 0);
  sprite_instance_ = from.sprite_instance_;
  gl_buffer_ = nullptr;
  last_buffer_pivot_ = Vec2Si32(0, 0);
  last_buffer_ref_size_ = Vec2Si32(0, 0);
  last_angle_ = 0.0f;
}

void HwSprite::Clear() {
  Clear(Rgba(0u));
}

void HwSprite::Clear(Rgba color) {
  if (!sprite_instance_) {
    return;
  }

  sprite_instance_->framebuffer().Bind();
  glClearColor(
    static_cast<float>(color.r) / 255.0f,
    static_cast<float>(color.g) / 255.0f,
    static_cast<float>(color.b) / 255.0f,
    static_cast<float>(color.a) / 255.0f
  );
  glClear(GL_COLOR_BUFFER_BIT);
  //GlFramebuffer::BindDefault();
}

void HwSprite::Clone(const HwSprite &from, CloneTransform transform) {
  if (!from.sprite_instance_) {
    sprite_instance_ = nullptr;
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(0, 0);
    pivot_ = Vec2Si32(0, 0);
    gl_buffer_ = nullptr;
    last_buffer_pivot_ = Vec2Si32(0, 0);
    last_buffer_ref_size_ = Vec2Si32(0, 0);
    last_angle_ = 0.0f;
    return;
  }

  if (transform == kCloneUntransformed) {
    Create(from.Width(), from.Height());
    DrawSprite(*this, static_cast<float>(from.Pivot().x), static_cast<float>(from.Pivot().y), static_cast<float>(from.Width()), static_cast<float>(from.Height()),
          from, 0, 0, static_cast<float>(from.Width()), static_cast<float>(from.Height()), Rgba(255, 255, 255, 255),
          kDrawBlendingModeCopyRgba, kFilterNearest, 0.0f);
    SetPivot(from.Pivot());
    return;
  }

  Vec2Si32 dst_base;
  Vec2Si32 dst_dir_x;
  Vec2Si32 dst_dir_y;
  if (transform == kCloneRotateCw90 || transform == kCloneRotateCcw90 || transform == kCloneRotate180) {
      float angle = 0.0;
      if (transform == kCloneRotateCw90) {
          Create(from.Height(), from.Width());
          dst_base = Vec2Si32(0, Height() - 1);
          dst_dir_x = Vec2Si32(0, -1);
          dst_dir_y = Vec2Si32(1, 0);
          angle = -3.14f / 2.0f;
      } else if (transform == kCloneRotateCcw90) {
          Create(from.Height(), from.Width());
          dst_base = Vec2Si32(Width() - 1, 0);
          dst_dir_x = Vec2Si32(0, 1);
          dst_dir_y = Vec2Si32(-1, 0);
          angle = 3.14f / 2.0f;
      } else if (transform == kCloneRotate180) {
          Create(from.Width(), from.Height());
          dst_base = Vec2Si32(Width() - 1, Height() - 1);
          dst_dir_x = Vec2Si32(-1, 0);
          dst_dir_y = Vec2Si32(0, -1);
          angle = 3.14f;
      }
      DrawSprite(*this, static_cast<float>(dst_base.x), static_cast<float>(dst_base.y), static_cast<float>(from.Width()), static_cast<float>(from.Height()),
          from, 0, 0, static_cast<float>(from.Width()), static_cast<float>(from.Height()), Rgba(255, 255, 255, 255),
          kDrawBlendingModeCopyRgba, kFilterNearest, angle);
  } else {
      Create(from.Width(), from.Height());
      int x_factor = 1;
      int y_factor = 1;
      if (transform == kCloneMirrorLr) {
          dst_base = Vec2Si32(Width() - 1, 0);
          dst_dir_x = Vec2Si32(-1, 0);
          dst_dir_y = Vec2Si32(0, 1);
          x_factor = -1;
      } else if (transform == kCloneMirrorUd) {
          dst_base = Vec2Si32(0, Height() - 1);
          dst_dir_x = Vec2Si32(1, 0);
          dst_dir_y = Vec2Si32(0, -1);
          y_factor = -1;
      }
      DrawSprite(*this, static_cast<float>(dst_base.x), static_cast<float>(dst_base.y), static_cast<float>(from.Width()), static_cast<float>(from.Height()),
          from, 0, 0, static_cast<float>(from.Width()*x_factor), static_cast<float>(from.Height()*y_factor), Rgba(255, 255, 255, 255),
          kDrawBlendingModeCopyRgba, kFilterNearest, 0.0f);
  }
  SetPivot(dst_base + from.Pivot().x * dst_dir_x + from.Pivot().y * dst_dir_y);
}

void HwSprite::SetPivot(Vec2Si32 pivot) {
  pivot_ = pivot;
}

Vec2Si32 HwSprite::Pivot() const {
  return pivot_;
}

void HwSprite::Draw(const HwSprite &to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
    if (!sprite_instance_) {
        return;
    }
    DrawSprite(
        to_sprite, static_cast<float>(to_x_pivot), static_cast<float>(to_y_pivot), static_cast<float>(Width()), static_cast<float>(Height()),
        *this, 0, 0, static_cast<float>(Width()), static_cast<float>(Height()), color, blending_mode, filter_mode, 0.0f);
}

void HwSprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
    Draw(GetEngine()->GetHwBackbuffer(), to_x_pivot, to_y_pivot, blending_mode, filter_mode, color);
}

void HwSprite::Draw(const Si32 to_x, const Si32 to_y, const Si32 to_width, const Si32 to_height, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_x, to_y, to_width, to_height, 0, 0, ref_size_.x, ref_size_.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Si32 to_x, const Si32 to_y, const Si32 to_width, const Si32 to_height, const Si32 from_x, const Si32 from_y, const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_x, to_y, to_width, to_height, from_x, from_y, from_width, from_height, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const HwSprite &to_sprite, const Si32 to_x, const Si32 to_y, const Si32 to_width, const Si32 to_height, const Si32 from_x, const Si32 from_y, const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_x, to_y, to_width, to_height, from_x, from_y, from_width, from_height, to_sprite, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const HwSprite &to_sprite, const Vec2Si32 to_pos, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_sprite, to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const HwSprite &to_sprite, const Vec2Si32 to_pos, const Vec2Si32 to_size, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_sprite, to_pos.x, to_pos.y, to_size.x, to_size.y, 0, 0, ref_size_.x, ref_size_.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y, 0, 0, ref_size_.x, ref_size_.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size, const Vec2Si32 from_pos, const Vec2Si32 from_size, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y, from_pos.x, from_pos.y, from_size.x, from_size.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot, const Si32 to_width, const Si32 to_height, const Si32 from_x, const Si32 from_y, const Si32 from_width, const Si32 from_height,
    const HwSprite &to_sprite, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) const {
    if (!sprite_instance_) {
        return;
    }
    DrawSprite(
        to_sprite, static_cast<float>(to_x_pivot), static_cast<float>(to_y_pivot), static_cast<float>(to_width), static_cast<float>(to_height), *this,
        static_cast<float>(from_x), static_cast<float>(from_y), static_cast<float>(from_width), static_cast<float>(from_height), in_color, blending_mode, filter_mode, 0.0f);
}

void HwSprite::Draw(const Vec2F to, float angle_radians, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to.x, to.y, (float)Width(), (float)Height(), angle_radians, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y, float angle_radians, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_x, to_y, (float)Width(), (float)Height(), angle_radians, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2F to, float to_width, float to_height, float angle_radians, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to.x, to.y, to_width, to_height, angle_radians, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(Rgba in_color, const float to_x, const float to_y, float to_width, float to_height, float angle_radians, DrawBlendingMode blending_mode, DrawFilterMode filter_mode) {
    Draw(to_x, to_y, to_width, to_height, angle_radians, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y, float to_width, float to_height, float angle_radians,
      DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    Draw(to_x, to_y, to_width, to_height, angle_radians, GetEngine()->GetHwBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y, float to_width, float to_height, float angle_radians,
                    const HwSprite &to_sprite, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
    if (!sprite_instance_) {
        return;
    }
    DrawSprite(
        to_sprite, static_cast<float>(to_x), static_cast<float>(to_y),
        to_width, to_height,
        *this,
        0, 0, static_cast<float>(Width()), static_cast<float>(Height()), in_color, blending_mode, filter_mode, angle_radians);
}

Si32 HwSprite::Width() const {
  return ref_size_.x;
}

Si32 HwSprite::Height() const {
  return ref_size_.y;
}

Vec2Si32 HwSprite::Size() const {
  return ref_size_;
}

bool HwSprite::IsRef() const {
  return (ref_pos_.x
      || ref_pos_.y
      || ref_size_.x != sprite_instance_->width()
      || ref_size_.y != sprite_instance_->height());
}

}  // namespace arctic
