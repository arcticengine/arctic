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

namespace arctic {

HwSprite::HwSprite() {
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(0, 0);
  pivot_ = Vec2Si32(0, 0);
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
  const char *last_dot = strchr(file_name, '.');
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
  const char *last_dot = strchr(file_name, '.');
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

void HwSprite::Save(const char *file_name) {
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
  const char *last_dot = strchr(file_name, '.');
  Check(!!last_dot, "Error in HwSprite::Save, file_name has no extension.");
  if (strcmp(last_dot, ".tga") == 0) {
      HwSpriteInstance::SaveTga(sprite_instance_, &data);
  } else {
    Fatal("Error in HwSprite::Save, unknown file extension.");
  }
  return data;
}

void HwSprite::Create(const Vec2Si32 size) {
  Create(size.x, size.y);
}

void HwSprite::Create(const Si32 width, const Si32 height) {
  sprite_instance_ = std::make_shared<HwSpriteInstance>(width, height);
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(width, height);
  pivot_ = Vec2Si32(0, 0);
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
}

void HwSprite::Clear() {
  Clear(Rgba(0u));
}

void HwSprite::Clear(Rgba color) {
  if (!sprite_instance_) {
    return;
  }

  std::vector<Rgba> data;
  data.resize(Width()*Height(), color);

  sprite_instance_->texture().UpdateData(data.data());
}

/*void HwSprite::Clone(HwSprite from, CloneTransform transform) {
  if (!from.sprite_instance_) {
    sprite_instance_ = nullptr;
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(0, 0);
    pivot_ = Vec2Si32(0, 0);
    return;
  }
  if (transform == kCloneUntransformed) {
    Create(from.Width(), from.Height());
    from.Draw(from.Pivot().x, from.Pivot().y, from.Width(), from.Height(),
      0, 0, from.Width(), from.Height(), *this, kDrawBlendingModeCopyRgba);
    SetPivot(from.Pivot());
    return;
  }
  Vec2Si32 dst_base;
  Vec2Si32 dst_dir_x;
  Vec2Si32 dst_dir_y;
  if (transform == kCloneRotateCw90 || transform == kCloneRotateCcw90) {
    Create(from.Height(), from.Width());
    if (transform == kCloneRotateCw90) {
      dst_base = Vec2Si32(0, Height() - 1);
      dst_dir_x = Vec2Si32(0, -1);
      dst_dir_y = Vec2Si32(1, 0);
    } else {
      dst_base = Vec2Si32(Width() - 1, 0);
      dst_dir_x = Vec2Si32(0, 1);
      dst_dir_y = Vec2Si32(-1, 0);
    }
  } else {
    Create(from.Width(), from.Height());
    if (transform == kCloneMirrorLr) {
      dst_base = Vec2Si32(Width() - 1, 0);
      dst_dir_x = Vec2Si32(-1, 0);
      dst_dir_y = Vec2Si32(0, 1);
    } else if (transform == kCloneMirrorUd) {
      dst_base = Vec2Si32(0, Height() - 1);
      dst_dir_x = Vec2Si32(1, 0);
      dst_dir_y = Vec2Si32(0, -1);
    } else {  // kCloneRotate180
      dst_base = Vec2Si32(Width() - 1, Height() - 1);
      dst_dir_x = Vec2Si32(-1, 0);
      dst_dir_y = Vec2Si32(0, -1);
    }
  }

  Si32 wid = from.Width();
  Si32 hei = from.Height();
  Si32 src_stride = from.StridePixels();
  Si32 dst_stride = StridePixels();
  Rgba *src_data = from.RgbaData();
  Rgba *dst_data = RgbaData();
  for (Si32 y = 0; y < hei; ++y) {
    for (Si32 x = 0; x < wid; ++x) {
      Vec2Si32 dst_pos = dst_base + dst_dir_y * y + dst_dir_x * x;
      dst_data[dst_pos.y * dst_stride + dst_pos.x] =
        src_data[y * src_stride + x];
    }
  }

  SetPivot(dst_base + from.Pivot().x * dst_dir_x + from.Pivot().y * dst_dir_y);
}*/

void HwSprite::SetPivot(Vec2Si32 pivot) {
  pivot_ = pivot;
}

Vec2Si32 HwSprite::Pivot() const {
  return pivot_;
}

void HwSprite::Draw(HwSprite to_sprite, const Si32 to_x, const Si32 to_y, DrawFilterMode filter_mode, Rgba in_color) {
    /*float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float back_aspect = static_cast<float>(backbuffer_texture_.Width()) /
        static_cast<float>(backbuffer_texture_.Height());
    float ratio = back_aspect / aspect;
    float x_aspect = aspect < back_aspect ? 1.f : ratio;
    float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;*/

    Vec3F base = Vec3F(-1.0f, -1.0f, 0.0f);
    Vec3F tx = Vec3F(2.0f*0.1, 0.0f, 0.0f);
    Vec3F ty = Vec3F(0.0f, 2.0f*0.1, 0.0f);
    //Vec3F n = Vec3F(0.0f, 0.0f, 1.0f);

    const Vec3F verts[] = {
        base,
        base + tx,
        base + ty + tx,
        base + ty,
    };

    //const Vec3F normals[] = {
    //    n, n, n, n,
    //};

    const Vec2F tex_coords[] = {
        Vec2F(0.0f, 0.0f),
        Vec2F(1.0f, 0.0f),
        Vec2F(1.0f, 1.0f),
        Vec2F(0.0f, 1.0f),
    };

    const Ui32 indices[] = {
        0, 1, 2, 2, 3, 0,
    };

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,
        verts);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0,
        tex_coords);
    glEnableVertexAttribArray(1);

    GLint loc = glGetUniformLocation(GetEngine()->GetProgramObject(), "s_texture");
    Check(loc >= 0, "s_texture not found");

    glUniform1i(loc, 0);
    glUseProgram(GetEngine()->GetProgramObject());

    GLint ufs;
    glGetProgramiv(GetEngine()->GetProgramObject(), GL_ACTIVE_UNIFORMS, &ufs);
    Check(ufs == 1, "no ufs");


    //hw_backbuffer_texture_.Clear(Rgba(127, 127, 255)); // dev-version only

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, hw_backbuffer_texture_.sprite_instance()->texture_id());
    glDrawElements(GL_TRIANGLES, indices_, GL_UNSIGNED_INT, visible_indices_.data());

    glBindTexture(GL_TEXTURE_2D, backbuffer_texture_name_);
    glDrawElements(GL_TRIANGLES, indices_, GL_UNSIGNED_INT,
        visible_indices_.data());
}

/*void HwSprite::Draw(HwSprite to_sprite,
    const Si32 to_x_pivot, const Si32 to_y_pivot,
  DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
  if (!sprite_instance_) {
    return;
  }
  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kDrawBlendingModeAlphaBlend:
          DrawSprite<kDrawBlendingModeAlphaBlend, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
        case kDrawBlendingModeCopyRgba:
          DrawSprite<kDrawBlendingModeCopyRgba, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
        case kDrawBlendingModeColorize:
          DrawSprite<kDrawBlendingModeColorize, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
        case kDrawBlendingModeAdd:
          DrawSprite<kDrawBlendingModeAdd, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kDrawBlendingModeAlphaBlend:
          DrawSprite<kDrawBlendingModeAlphaBlend, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
        case kDrawBlendingModeCopyRgba:
          DrawSprite<kDrawBlendingModeCopyRgba, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
        case kDrawBlendingModeColorize:
          DrawSprite<kDrawBlendingModeColorize, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
        case kDrawBlendingModeAdd:
          DrawSprite<kDrawBlendingModeAdd, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
      }
      break;
  }
}


void HwSprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
  Draw(GetEngine()->GetBackbuffer(), to_x_pivot, to_y_pivot,
       blending_mode, filter_mode, color);
}

void HwSprite::Draw(const Vec2F to, float angle_radians,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y, float angle_radians,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2F to, float angle_radians, float zoom,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y,
    float angle_radians, float zoom,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const float to_x, const float to_y,
    float angle_radians, float zoom, HwSprite to_sprite,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  if (!sprite_instance_) {
    return;
  }
  Vec2F pivot = Vec2F(to_x, to_y);
  float sin_a = sinf(angle_radians) * zoom;
  float cos_a = cosf(angle_radians) * zoom;
  Vec2F left = Vec2F(-cos_a, -sin_a) * static_cast<float>(pivot_.x);
  Vec2F right = Vec2F(cos_a, sin_a) *
    static_cast<float>(Width() - 1 - pivot_.x);
  Vec2F up = Vec2F(-sin_a, cos_a) * static_cast<float>(Height() - 1 - pivot_.y);
  Vec2F down = Vec2F(sin_a, -cos_a) * static_cast<float>(pivot_.y);

  // d c
  // a b
  Vec2F a(pivot + left + down);
  Vec2F b(pivot + right + down);
  Vec2F c(pivot + right + up);
  Vec2F d(pivot + left + up);

  Vec2F ta(0.01f,
    0.01f);
  Vec2F tb(static_cast<float>(ref_size_.x) - 1.01f,
    0.01f);
  Vec2F tc(static_cast<float>(ref_size_.x) - 1.01f,
    static_cast<float>(ref_size_.y) - 1.01f);
  Vec2F td(0.01f,
    static_cast<float>(ref_size_.y) - 1.01f);

  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kDrawBlendingModeCopyRgba:
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawTriangle<kDrawBlendingModeColorize, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeColorize, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeAdd:
          DrawTriangle<kDrawBlendingModeAdd, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeAdd, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kDrawBlendingModeCopyRgba:
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawTriangle<kDrawBlendingModeColorize, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeColorize, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kDrawBlendingModeAdd:
          DrawTriangle<kDrawBlendingModeAdd, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kDrawBlendingModeAdd, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
      }

      break;
  }
}

void DrawTriangle(HwSprite to_sprite,
    Vec2F a, Vec2F b, Vec2F c,
    Vec2F ta, Vec2F tb, Vec2F tc,
    HwSprite texture,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kDrawBlendingModeCopyRgba:
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawTriangle<kDrawBlendingModeColorize, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeAdd:
          DrawTriangle<kDrawBlendingModeAdd, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kDrawBlendingModeCopyRgba:
          DrawTriangle<kDrawBlendingModeCopyRgba, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawTriangle<kDrawBlendingModeAlphaBlend, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawTriangle<kDrawBlendingModeColorize, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kDrawBlendingModeAdd:
          DrawTriangle<kDrawBlendingModeAdd, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
      }

      break;
  }
}

void HwSprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
    Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    0, 0, ref_size_.x, ref_size_.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    from_x, from_y, from_width, from_height,
    GetEngine()->GetBackbuffer(), blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(HwSprite to_sprite, const Si32 to_x, const Si32 to_y,
                  const Si32 to_width, const Si32 to_height,
                  const Si32 from_x, const Si32 from_y,
                  const Si32 from_width, const Si32 from_height,
                  DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
                  Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
       from_x, from_y, from_width, from_height,
       to_sprite, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(HwSprite to_sprite, const Vec2Si32 to_pos,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_sprite, to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(HwSprite to_sprite, const Vec2Si32 to_pos,
    const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_sprite, to_pos.x, to_pos.y, to_size.x, to_size.y,
       0, 0, ref_size_.x, ref_size_.y,
       blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    0, 0, ref_size_.x, ref_size_.y,
    blending_mode, filter_mode, in_color);
}

void HwSprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    const Vec2Si32 from_pos, const Vec2Si32 from_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    from_pos.x, from_pos.y, from_size.x, from_size.y,
    blending_mode, filter_mode, in_color);
}


void HwSprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    HwSprite to_sprite, DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode, Rgba in_color) const {
  if (!sprite_instance_) {
    return;
  }
  switch (filter_mode) {
      case kFilterNearest:
      switch (blending_mode) {
        default:
        case kDrawBlendingModeCopyRgba:
          DrawSprite<kDrawBlendingModeCopyRgba, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawSprite<kDrawBlendingModeAlphaBlend, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawSprite<kDrawBlendingModeColorize, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
      }
      break;
      case kFilterBilinear:
      switch (blending_mode) {
        default:
        case kDrawBlendingModeCopyRgba:
          DrawSprite<kDrawBlendingModeCopyRgba, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kDrawBlendingModeAlphaBlend:
          DrawSprite<kDrawBlendingModeAlphaBlend, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kDrawBlendingModeColorize:
          DrawSprite<kDrawBlendingModeColorize, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
      }
      break;
  }
  return;
}*/

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
