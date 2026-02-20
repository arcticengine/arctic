// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2021 Huldra
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

#include <chrono>  // NOLINT
#include <deque>
#include <fstream>
#include <limits>
#include <thread>  // NOLINT
#include <utility>

#include "engine/arctic_platform.h"
#include "engine/easy_advanced.h"
#include "engine/easy_drawing.h"
#include "engine/easy_files.h"
#include "engine/easy_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/easy_util.h"
#include "engine/log.h"
#include "engine/vec4si32.h"
#include "engine/vec4f.h"
#include "vec2d.h"


namespace arctic {



struct KeyState {
  bool current_state_is_down = false;
  bool previous_state_is_down = false;
  bool was_pressed_this_frame = false;
  bool was_released_this_frame = false;

  void Init() {
    current_state_is_down = false;
    previous_state_is_down = false;
    was_pressed_this_frame = false;
    was_released_this_frame = false;
  }

  void OnShowFrame() {
    previous_state_is_down = current_state_is_down;
    was_pressed_this_frame = false;
    was_released_this_frame = false;
  }

  void OnControllerState(bool is_down) {
    was_pressed_this_frame = was_pressed_this_frame || (current_state_is_down == false && is_down == true);
    was_released_this_frame = was_released_this_frame || (current_state_is_down == true && is_down == false);
    current_state_is_down = is_down;
  }

  void OnStateChange(bool is_down) {
    if (is_down) {
      was_pressed_this_frame = true;
    } else {
      was_released_this_frame = true;
    }
    current_state_is_down = is_down;
  }

  bool IsDown() const {
    return current_state_is_down;
  }

  bool WasPressed() const {
    return was_pressed_this_frame;
  }

  bool WasReleased() const {
    return was_released_this_frame;
  }
};

static KeyState g_key_state[kKeyCount];
static std::deque<InputMessage> g_input_messages;

static Engine *g_engine = nullptr;
static Vec2Si32 g_mouse_pos_prev = Vec2Si32(0, 0);
static Vec2Si32 g_mouse_pos = Vec2Si32(0, 0);
static InputMessage::Controller g_controller_state[InputMessage::kControllerCount];
static Vec2Si32 g_mouse_move = Vec2Si32(0, 0);
static Si32 g_mouse_wheel_delta = 0;

void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Rgba color) {
  DrawLine(to_sprite, a, b, color, color);
}

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color) {
  DrawLine(GetEngine()->GetBackbuffer(), a, b, color, color);
}

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b) {
  DrawLine(GetEngine()->GetBackbuffer(), a, b, color_a, color_b);
}

void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b) {
  Si64 ab_x = Si64(b.x) - Si64(a.x);
  Si64 ab_y = Si64(b.y) - Si64(a.y);
  if (std::abs(ab_x) >= std::abs(ab_y)) {
    // The line is closer to horizontal
    if (ab_x == 0) {
      // Ths line is single pixel
      if (a.x >= 0 && a.y >= 0) {
        const Vec2Si32 back_size = to_sprite.Size();
        if (a.x < back_size.x && a.y < back_size.y) {
          const Rgba color((Si16(color_a.r) + Si16(color_b.r))/2,
                           (Si16(color_a.g) + Si16(color_b.g))/2,
                           (Si16(color_a.b) + Si16(color_b.b))/2,
                           (Si16(color_a.a) + Si16(color_b.a))/2);
          to_sprite.RgbaData()[a.x + a.y * to_sprite.StridePixels()] = color;
        }
      }
      return;
    }
    // The line is at least one pixel long
    if (a.x > b.x) {
      std::swap(a, b);
      std::swap(color_a, color_b);
      ab_x = -ab_x;
      ab_y = -ab_y;
    }
    // a--b
    if (b.x < 0) {
      return;
    }
    if (std::max(a.y, b.y) < 0) {
      return;
    }
    const Vec2Si32 back_size = to_sprite.Size();
    if (a.x >= back_size.x) {
      return;
    }
    if (std::min(a.y, b.y) >= back_size.y) {
      return;
    }
    Si64 y_for_x_16 = ab_y * 65536ll / ab_x;
    Si64 ax = Si64(a.x);
    Si64 bx = Si64(b.x);
    Si64 ay_16 = Si64(a.y) * 65536ll + 32767ll;


    Si64 x_exc = 0ll - ax;
    if (x_exc > 0) {
      ay_16 += y_for_x_16 * x_exc;
      ax = 0;
    }
    Si64 x_ovr = bx - Si64(back_size.x) + 1ll;
    if (x_ovr > 0) {
      bx = Si64(back_size.x) - 1ll;
    }
    Si64 by_16 = ay_16 + (bx - ax) * y_for_x_16;

    if (y_for_x_16 > 0) {
      Si64 y_exc_16 = 0ll - ay_16;
      if (y_exc_16 > 0) {
        Si64 dax = y_exc_16 / y_for_x_16;
        ay_16 = ay_16 + dax * y_for_x_16;
        ax += dax;
        while (ay_16 < 0) {
          ay_16 += y_for_x_16;
          ax++;
        }
        by_16 = ay_16 + (bx - ax) * y_for_x_16;
      }
      Si64 y_ovr_16 = by_16 - Si64(back_size.y) * 65536ll + 1ll;
      if (y_ovr_16 > 0) {
        bx -= y_ovr_16 / y_for_x_16;
        do {
          by_16 = ay_16 + (bx - ax) * y_for_x_16;
          if (by_16 < Si64(back_size.y) * 65536ll) {
            break;
          }
          bx--;
        } while (true);
      }
      if (bx < ax) {
        return;
      }
    } else if (y_for_x_16 < 0) {
      Si64 y_exc_16 = ay_16 - Si64(back_size.y)*65536ll + 1ll;
      if (y_exc_16 > 0) {
        Si64 dax = -y_exc_16 / y_for_x_16;
        ay_16 = ay_16 + dax * y_for_x_16;
        ax += dax;
        while (ay_16 >= Si64(back_size.y)*65536ll) {
          ay_16 += y_for_x_16;
          ax++;
        }
        by_16 = ay_16 + (bx - ax) * y_for_x_16;
      }
      Si64 y_ovr_16 = 0ll - by_16;
      if (y_ovr_16 > 0) {
        bx += y_ovr_16 / y_for_x_16;
        do {
          by_16 = ay_16 + (bx - ax) * y_for_x_16;
          if (by_16 >= 0) {
            break;
          }
          bx--;
        } while (true);
      }
      if (bx < ax) {
        return;
      }
    }

    Vec4Si32 rgba_a_16(
                       static_cast<Si32>(color_a.r) * 65536,
                       static_cast<Si32>(color_a.g) * 65536,
                       static_cast<Si32>(color_a.b) * 65536,
                       static_cast<Si32>(color_a.a) * 65536);
    Vec4Si32 rgba_b_16(
                       static_cast<Si32>(color_b.r) * 65536,
                       static_cast<Si32>(color_b.g) * 65536,
                       static_cast<Si32>(color_b.b) * 65536,
                       static_cast<Si32>(color_b.a) * 65536);
    Vec4Si32 rgba_ab_16 = rgba_b_16 - rgba_a_16;
    Vec4Si32 rgba_1_16;
    rgba_1_16.x = Si32(rgba_a_16.x + rgba_ab_16.x * (ax - Si64(a.x)) / ab_x);
    rgba_1_16.y = Si32(rgba_a_16.y + rgba_ab_16.y * (ax - Si64(a.x)) / ab_x);
    rgba_1_16.z = Si32(rgba_a_16.z + rgba_ab_16.z * (ax - Si64(a.x)) / ab_x);
    rgba_1_16.w = Si32(rgba_a_16.w + rgba_ab_16.w * (ax - Si64(a.x)) / ab_x);
    Vec4Si32 rgba_12_16_step;
    rgba_12_16_step.x = Si32(rgba_ab_16.x / ab_x);
    rgba_12_16_step.y = Si32(rgba_ab_16.y / ab_x);
    rgba_12_16_step.z = Si32(rgba_ab_16.z / ab_x);
    rgba_12_16_step.w = Si32(rgba_ab_16.w / ab_x);

    Si32 stride = to_sprite.StridePixels();
    Rgba *dst = to_sprite.RgbaData();
    for (Si64 x = ax; x <= bx; ++x) {
      Rgba color(
                 static_cast<Ui8>(rgba_1_16.x >> 16),
                 static_cast<Ui8>(rgba_1_16.y >> 16),
                 static_cast<Ui8>(rgba_1_16.z >> 16),
                 static_cast<Ui8>(rgba_1_16.w >> 16));
      dst[x + (ay_16 >> 16) * stride] = color;
      rgba_1_16 += rgba_12_16_step;
      ay_16 += y_for_x_16;
    }
  } else {
    // The line is closer to vertical
    // The line is at least one pixel long
    if (a.y > b.y) {
      std::swap(a, b);
      std::swap(color_a, color_b);
      ab_x = -ab_x;
      ab_y = -ab_y;
    }
    // b
    // |
    // |
    // a

    if (b.y < 0) {
          return;
        }
        if (std::max(a.x, b.x) < 0) {
          return;
        }
        const Vec2Si32 back_size = to_sprite.Size();
        if (a.y >= back_size.y) {
          return;
        }
        if (std::min(a.x, b.x) >= back_size.x) {
          return;
        }
        Si64 x_for_y_16 = ab_x * 65536ll / ab_y;
        Si64 ay = Si64(a.y);
        Si64 by = Si64(b.y);
        Si64 ax_16 = Si64(a.x) * 65536ll + 32767ll;


        Si64 y_exc = 0ll - ay;
        if (y_exc > 0) {
          ax_16 += x_for_y_16 * y_exc;
          ay = 0;
        }
        Si64 y_ovr = by - Si64(back_size.y) + 1ll;
        if (y_ovr > 0) {
          by = Si64(back_size.y) - 1ll;
        }
        Si64 bx_16 = ax_16 + (by - ay) * x_for_y_16;

        if (x_for_y_16 > 0) {
          Si64 x_exc_16 = 0ll - ax_16;
          if (x_exc_16 > 0) {
            Si64 day = x_exc_16 / x_for_y_16;
            ax_16 = ax_16 + day * x_for_y_16;
            ay += day;
            while (ax_16 < 0) {
              ax_16 += x_for_y_16;
              ay++;
            }
            bx_16 = ax_16 + (by - ay) * x_for_y_16;
          }
          Si64 x_ovr_16 = bx_16 - Si64(back_size.x) * 65536ll + 1ll;
          if (x_ovr_16 > 0) {
            by -= x_ovr_16 / x_for_y_16;
            do {
              bx_16 = ax_16 + (by - ay) * x_for_y_16;
              if (bx_16 < Si64(back_size.x) * 65536ll) {
                break;
              }
              by--;
            } while (true);
          }
          if (by < ay) {
            return;
          }
        } else if (x_for_y_16 < 0) {
          Si64 x_exc_16 = ax_16 - Si64(back_size.x)*65536ll + 1ll;
          if (x_exc_16 > 0) {
            Si64 day = -x_exc_16 / x_for_y_16;
            ax_16 = ax_16 + day * x_for_y_16;
            ay += day;
            while (ax_16 >= Si64(back_size.x)*65536ll) {
              ax_16 += x_for_y_16;
              ay++;
            }
            bx_16 = ax_16 + (by - ay) * x_for_y_16;
          }
          Si64 x_ovr_16 = 0ll - bx_16;
          if (x_ovr_16 > 0) {
            by += x_ovr_16 / x_for_y_16;
            do {
              bx_16 = ax_16 + (by - ay) * x_for_y_16;
              if (bx_16 >= 0) {
                break;
              }
              by--;
            } while (true);
          }
          if (by < ay) {
            return;
          }
        }

        Vec4Si32 rgba_a_16(
                           static_cast<Si32>(color_a.r) * 65536,
                           static_cast<Si32>(color_a.g) * 65536,
                           static_cast<Si32>(color_a.b) * 65536,
                           static_cast<Si32>(color_a.a) * 65536);
        Vec4Si32 rgba_b_16(
                           static_cast<Si32>(color_b.r) * 65536,
                           static_cast<Si32>(color_b.g) * 65536,
                           static_cast<Si32>(color_b.b) * 65536,
                           static_cast<Si32>(color_b.a) * 65536);
        Vec4Si32 rgba_ab_16 = rgba_b_16 - rgba_a_16;
        Vec4Si32 rgba_1_16;
        rgba_1_16.x = Si32(rgba_a_16.x + rgba_ab_16.x * (ay - Si64(a.y)) / ab_y);
        rgba_1_16.y = Si32(rgba_a_16.y + rgba_ab_16.y * (ay - Si64(a.y)) / ab_y);
        rgba_1_16.z = Si32(rgba_a_16.z + rgba_ab_16.z * (ay - Si64(a.y)) / ab_y);
        rgba_1_16.w = Si32(rgba_a_16.w + rgba_ab_16.w * (ay - Si64(a.y)) / ab_y);
        Vec4Si32 rgba_12_16_step;
        rgba_12_16_step.x = Si32(rgba_ab_16.x / ab_y);
        rgba_12_16_step.y = Si32(rgba_ab_16.y / ab_y);
        rgba_12_16_step.z = Si32(rgba_ab_16.z / ab_y);
        rgba_12_16_step.w = Si32(rgba_ab_16.w / ab_y);

        Si32 stride = to_sprite.StridePixels();
    Rgba *dst = to_sprite.RgbaData();
    for (Si64 y = ay; y <= by; ++y) {
      Rgba color(
                 static_cast<Ui8>(rgba_1_16.x >> 16),
                 static_cast<Ui8>(rgba_1_16.y >> 16),
                 static_cast<Ui8>(rgba_1_16.z >> 16),
                 static_cast<Ui8>(rgba_1_16.w >> 16));
      dst[(ax_16 >> 16) + y * stride] = color;
      rgba_1_16 += rgba_12_16_step;
      ax_16 += x_for_y_16;
    }
  }
}

void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color) {
  DrawTriangle(GetEngine()->GetBackbuffer(), a, b, c, color, color, color);
}

void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color) {
  DrawTriangle(to_sprite, a, b, c, color, color, color);
}

// Just like MasterBoy wrote in HUGi 17, but without subpixel
// see http://www.hugi.scene.org/online/coding/hugi%2017%20-%20cotriang.htm
// or http://www.hugi.scene.org/online/hugi17/
inline void DrawTrianglePart(Rgba *dst, Si32 stride,
        float *x1, float *x2, Vec4F *rgba_a, Vec4F *rgba_b,
        float dxdy1, float dxdy2,
        Vec4F dcdy1, Vec4F dcdy2,
        Si32 width, Si32 height,
        Si32 y1, Si32 y2) {
    Si32 y = y1;
    if (y1 < 0) {
        Si32 yc = std::min(0, y2);
        float d = static_cast<float>(yc - y);
        y = yc;
        *x1 += dxdy1 * d;
        *x2 += dxdy2 * d;
        *rgba_a += dcdy1 * d;
        *rgba_b += dcdy2 * d;
    }
    Si32 ye = std::min(height, y2);
    dst += y * stride;
    for (; y < ye; y++) {
        Si32 x1i = static_cast<Si32>(*x1);
        Si32 x2i = static_cast<Si32>(*x2);

        Si32 x12i = x2i - x1i;
        Si32 x1c = std::max(0, x1i);
        Si32 x2c = std::min(width, x2i);

        if (x2c <= x1c) {
            if (x2c == x1c && x2c < width) {
                Rgba color(Ui8(rgba_a->x),
                    Ui8(rgba_a->y),
                    Ui8(rgba_a->z),
                    Ui8(rgba_a->w));
                Rgba *p = dst + x1c;
                p->rgba = color.rgba;
            }
        } else {
            Vec4F rgba_ab = *rgba_b - *rgba_a;
            Vec4F rgba_1c = *rgba_a + rgba_ab * static_cast<float>(x1c - x1i) /
                static_cast<float>(x12i);
            Vec4F rgba_2c = *rgba_a + rgba_ab * static_cast<float>(x2c - x1i) /
                static_cast<float>(x12i);
            Vec4F rgba_1c2c = rgba_2c - rgba_1c;

            Vec4Si32 rgba_16(rgba_1c * 65536.f);
            Vec4Si32 rgba_12_16(rgba_1c2c * 65536.f);
            Vec4Si32 rgba_12_16_step = rgba_12_16 / (x2c - x1c);

            Rgba *p = dst + x1c;
            for (Si32 x = x1c; x < x2c; ++x) {
                Rgba color(
                    static_cast<Ui8>(rgba_16.x >> 16u),
                    static_cast<Ui8>(rgba_16.y >> 16u),
                    static_cast<Ui8>(rgba_16.z >> 16u),
                    static_cast<Ui8>(rgba_16.w >> 16u));
                p->rgba = color.rgba;
                p++;
                rgba_16 += rgba_12_16_step;
            }
        }
        *x1 += dxdy1;
        *x2 += dxdy2;
        *rgba_a += dcdy1;
        *rgba_b += dcdy2;
        dst += stride;
    }
}


void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c) {
  DrawTriangle(GetEngine()->GetBackbuffer(), a, b, c, color_a, color_b, color_c);
}

void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c) {
  if (a.y > b.y) {
      std::swap(a, b);
      std::swap(color_a, color_b);
  }
  if (a.y > c.y) {
      std::swap(a, c);
      std::swap(color_a, color_c);
  }
  if (b.y > c.y) {
      std::swap(b, c);
      std::swap(color_b, color_c);
  }
  if (a.y == c.y) {
      return;
  }

  Si32 stride = to_sprite.StridePixels();
  Rgba *dst = to_sprite.RgbaData();
  Si32 width = to_sprite.Width();
  Si32 height = to_sprite.Height();

  float dxdy_ac = static_cast<float>(c.x - a.x) /
      static_cast<float>(c.y - a.y);
  float dxdy_bc = (c.y != b.y) ? static_cast<float>(c.x - b.x) /
      static_cast<float>(c.y - b.y) : 0.0f;
  float dxdy_ab = (b.y != a.y) ? static_cast<float>(b.x - a.x) /
      static_cast<float>(b.y - a.y) : 0.0f;

  Vec4F rgba_a(color_a.r, color_a.g, color_a.b, color_a.a);
  Vec4F rgba_b(color_b.r, color_b.g, color_b.b, color_b.a);
  Vec4F rgba_c(color_c.r, color_c.g, color_c.b, color_c.a);

  Vec4F dcdy_ac = (rgba_c - rgba_a) / static_cast<float>(c.y - a.y);
  Vec4F dcdy_bc = (c.y != b.y) ? (rgba_c - rgba_b) / static_cast<float>(c.y - b.y) : Vec4F(0.f, 0.f, 0.f, 0.f);
  Vec4F dcdy_ab = (b.y != a.y) ? (rgba_b - rgba_a) / static_cast<float>(b.y - a.y) : Vec4F(0.f, 0.f, 0.f, 0.f);

  float x1;
  float x2;
  float dxdy1;
  float dxdy2;
  Vec4F rgba1;
  Vec4F rgba2;
  Vec4F dcdy1;
  Vec4F dcdy2;

  bool is_b_at_the_right_side = dxdy_ac < dxdy_ab;
  if (is_b_at_the_right_side) {
      dxdy1 = dxdy_ac;
      dcdy1 = dcdy_ac;
      if (a.y == b.y) {
          dxdy2 = dxdy_bc;
          x1 = static_cast<float>(a.x);
          x2 = static_cast<float>(b.x);
          dcdy2 = dcdy_bc;
          rgba1 = rgba_a;
          rgba2 = rgba_b;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, a.y, c.y);
          return;
      }
      if (a.y < b.y) {
          dxdy2 = dxdy_ab;
          x1 = static_cast<float>(a.x);
          x2 = static_cast<float>(a.x);
          dcdy2 = dcdy_ab;
          rgba1 = rgba_a;
          rgba2 = rgba_a;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, a.y, b.y);
      }
      if (b.y < c.y) {
          dxdy2 = dxdy_bc;
          x2 = static_cast<float>(b.x);
          dcdy2 = dcdy_bc;
          rgba2 = rgba_b;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, b.y, c.y);
      }
  } else {
      // b is at the left side
      dxdy2 = dxdy_ac;
      dcdy2 = dcdy_ac;
      if (a.y == b.y) {
          dxdy1 = dxdy_bc;
          x1 = static_cast<float>(b.x);
          x2 = static_cast<float>(a.x);
          dcdy1 = dcdy_bc;
          rgba1 = rgba_b;
          rgba2 = rgba_a;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, a.y, c.y);
          return;
      }
      if (a.y < b.y) {
          dxdy1 = dxdy_ab;
          x1 = static_cast<float>(a.x);
          x2 = static_cast<float>(a.x);
          dcdy1 = dcdy_ab;
          rgba1 = rgba_a;
          rgba2 = rgba_a;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, a.y, b.y);
      }
      if (b.y < c.y) {
          dxdy1 = dxdy_bc;
          x1 = static_cast<float>(b.x);
          dcdy1 = dcdy_bc;
          rgba1 = rgba_b;
          DrawTrianglePart(dst, stride, &x1, &x2, &rgba1, &rgba2,
              dxdy1, dxdy2,
              dcdy1, dcdy2, width, height, b.y, c.y);
      }
  }
}

void DrawRectangle(Vec2Si32 ll, Vec2Si32 ur, Rgba color) {
  DrawRectangle(GetEngine()->GetBackbuffer(), ll, ur, color);
}

void DrawRectangle(Sprite to_sprite, Vec2Si32 ll, Vec2Si32 ur, Rgba color) {
  Vec2Si32 limit = to_sprite.Size();
  Si32 x1 = std::max(std::min(ll.x, ur.x), 0);
  Si32 x2 = std::min(std::max(ll.x, ur.x) + 1, limit.x);
  Si32 y1 = std::max(std::min(ll.y, ur.y), 0);
  Si32 y2 = std::min(std::max(ll.y, ur.y) + 1, limit.y);
  if (x1 < x2 && y1 < y2) {
    Rgba *data = to_sprite.RgbaData();
    Si32 stride = to_sprite.StridePixels();
    Rgba *p_begin = data + stride * y1 + x1;
    Si32 w = x2 - x1;
    Rgba *p = p_begin;
    Si32 stride_rem = stride - w;
    if (color.a == 255) {
      for (Si32 y = y1; y < y2; ++y) {
        Rgba *p_end = p + w;
        for (; p < p_end; ++p) {
          p->rgba = color.rgba;
        }
        p += stride_rem;
      }
    } else if (color.a != 0) {
      Ui32 m = 255 - color.a;
      Ui32 m2 = color.a;
      Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
      Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8u) * m2;
      for (Si32 y = y1; y < y2; ++y) {
        Rgba *p_end = p + w;
        for (; p < p_end; ++p) {
          Ui32 rb = (p->rgba & 0x00ff00fful) * m;
          Ui32 g = ((p->rgba & 0x0000ff00ul) >> 8u) * m;
          p->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) | ((g + g2) & 0x0000ff00ul) | (m2 << 24);
        }
        p += stride_rem;
      }
    }
  }
}

void SetPixel(const Sprite &to_sprite, Si32 x, Si32 y, Rgba color) {
  Rgba *data = const_cast<Rgba*>(to_sprite.RgbaData());
  Si32 stride = to_sprite.StridePixels();
  if (x >= 0 && x < to_sprite.Width() && y >= 0 && y < to_sprite.Height()) {
    data[x + y * stride] = color;
  }
}

void SetPixel(Si32 x, Si32 y, Rgba color) {
  Sprite to_sprite = GetEngine()->GetBackbuffer();
  Rgba *data = to_sprite.RgbaData();
  Si32 stride = to_sprite.StridePixels();
  if (x >= 0 && x < to_sprite.Width() && y >= 0 && y < to_sprite.Height()) {
    data[x + y * stride] = color;
  }
}

void ReplaceColor(Sprite to_sprite, Rgba old_color, Rgba new_color) {
  Rgba *data = to_sprite.RgbaData();
  Si32 stride = to_sprite.StridePixels();
  const Si32 height = to_sprite.Height();
  const Si32 width = to_sprite.Width();
  for (Si32 y = 0; y < height; ++y) {
    Rgba *p_begin = data + stride * y;
    Rgba *p_end = p_begin + width;
    for (Rgba *p = p_begin; p < p_end; ++p) {
      if (*p == old_color) {
        *p = new_color;
      }
    }
  }
}

Rgba GetPixel(const Sprite &from_sprite, Si32 x, Si32 y) {
  const Rgba *data = from_sprite.RgbaData();
  Si32 stride = from_sprite.StridePixels();
  if (x >= 0 && x < from_sprite.Width() && y >= 0 && y < from_sprite.Height()) {
    return data[x + y * stride];
  } else {
    return Rgba(0, 0, 0);
  }
}

Rgba GetPixel(Si32 x, Si32 y) {
  Sprite from_sprite = GetEngine()->GetBackbuffer();
  Rgba *data = from_sprite.RgbaData();
  Si32 stride = from_sprite.StridePixels();
  if (x >= 0 && x < from_sprite.Width() && y >= 0 && y < from_sprite.Height()) {
    Rgba color = data[x + y * stride];
    color.a = 255;
    return color;
  } else {
    return Rgba(0, 0, 0);
  }
}

void DrawCircle(Vec2Si32 c, Si32 r, Rgba color) {
  DrawOval(GetEngine()->GetBackbuffer(), c, Vec2Si32(r, r), color);
}

void DrawCircle(Sprite to_sprite, Vec2Si32 c, Si32 r, Rgba color) {
  DrawOval(to_sprite, c, Vec2Si32(r, r), color);
}

void DrawOval(Vec2Si32 c, Vec2Si32 r, Rgba color) {
  DrawOval(GetEngine()->GetBackbuffer(), c, r, color);
}

void DrawOval(Sprite to_sprite, Vec2Si32 c, Vec2Si32 r, Rgba color) {
  DrawOval(to_sprite, color, c-r, c+r);
}

void DrawOval(Sprite to_sprite, Rgba color, Vec2Si32 ll, Vec2Si32 ur) {
  if (ll.x <= ur.x && ll.y <= ur.y) {
    Sprite back = to_sprite;
    Vec2Si32 limit = back.Size();
    MathTables &tables = GetEngine()->GetMathTables();
    Rgba *data = back.RgbaData();
    Si32 stride = back.StridePixels();
    Vec2Si32 half_axis_31_1 = (ur - ll) + Vec2Si32(1, 1);
    Vec2Si32 c_31_1 = ll + ur;
    Si32 y_off_31_1_min;
    Si32 y_off_31_1_max;
    Si32 y_dwn_31_1_min;
    Si32 y_dwn_31_1_max;
    if (c_31_1.y % 2) {
      c_31_1 += Vec2Si32(1, 1);
      y_off_31_1_min = std::max(0, -c_31_1.y) + 1;
      y_off_31_1_max = std::min(half_axis_31_1.y, std::max(0, limit.y*2 - c_31_1.y));
      if (c_31_1.y <= limit.y*2 - 2) {
        y_dwn_31_1_min = 1;
      } else {
        y_dwn_31_1_min = c_31_1.y - limit.y*2 + 1;
      }
      y_dwn_31_1_max = half_axis_31_1.y - std::max(0, half_axis_31_1.y - c_31_1.y+1);
    } else {
      c_31_1 += Vec2Si32(1, 0);
      y_off_31_1_min = std::max(0, -c_31_1.y);
      y_off_31_1_max = std::min(half_axis_31_1.y, std::max(0, limit.y*2 - c_31_1.y)) - 1;

      y_dwn_31_1_min = std::max(0, c_31_1.y - limit.y*2+2);
      y_dwn_31_1_max = half_axis_31_1.y - std::max(0, half_axis_31_1.y - c_31_1.y);
    }
    // from c to top
    for (Si32 y_off_31_1 = y_off_31_1_min; y_off_31_1 <= y_off_31_1_max; y_off_31_1 += 2) {
      Si32 y = (c_31_1.y + y_off_31_1) / 2;
      Si32 table_y = y_off_31_1 * tables.circle_16_16_one / (half_axis_31_1.y);
      Si32 table_x = tables.circle_16_16[static_cast<size_t>(table_y)];
      Si32 x_off_31_1 = (table_x * half_axis_31_1.x) >> 16;
      Si32 x1 = std::max((c_31_1.x - x_off_31_1) / 2 , 0);
      Si32 x2 = std::min((c_31_1.x + x_off_31_1 + 1) / 2, limit.x);
      if (x1 < x2) {
        Rgba *p = data + stride * y + x1;
        Rgba *p_max = p + (x2 - x1);
        for (; p < p_max; ++p) {
          p->rgba = color.rgba;
        }
      }
    }
    // from c to bottom
    for (Si32 y_off_31_1 = y_dwn_31_1_min; y_off_31_1 <= y_dwn_31_1_max; y_off_31_1 += 2) {
      Si32 y = (c_31_1.y - y_off_31_1) / 2;
      Si32 table_y = y_off_31_1 * tables.circle_16_16_one / (half_axis_31_1.y);
      Si32 table_x = tables.circle_16_16[static_cast<size_t>(table_y)];
      Si32 x_off_31_1 = (table_x * half_axis_31_1.x) >> 16;
      Si32 x1 = std::max((c_31_1.x - x_off_31_1) / 2 , 0);
      Si32 x2 = std::min((c_31_1.x + x_off_31_1 + 1) / 2, limit.x);
      if (x1 < x2) {
        Rgba *p = data + stride * y + x1;
        Rgba *p_max = p + (x2 - x1);
        for (; p < p_max; ++p) {
          p->rgba = color.rgba;
        }
      }
    }
  }
}

/// @brief Draw a rounded corner rectangular block shape.
/// @param [in] to_sprite Sprite to draw the block on.
/// @param [in] lower_left_pos Lower-left block corner position (as if it was not rounded).
/// @param [in] size Block size.
/// @param [in] corner_radius External radius of block corners.
/// @param [in] color Fill color of the block.
void DrawBlock(Sprite &to_sprite, Vec2F lower_left_pos, Vec2F size, float corner_radius, Rgba color) {
  if (size.x > 0 && size.y > 0 &&
      lower_left_pos.x < to_sprite.Width() &&
      lower_left_pos.y < to_sprite.Height() &&
      lower_left_pos.x + size.x >= 0.f &&
      lower_left_pos.y + size.y >= 0.f) {
    corner_radius = std::max(0.f, std::min(corner_radius, std::min(size.x, size.y) * 0.5f));
    float d = corner_radius * 2.f;
    if (corner_radius > 0.f) {
      Sprite frame;
      frame.Reference(to_sprite, Vec2Si32(lower_left_pos), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, color, Vec2Si32(0,0), Vec2Si32(Vec2F(d, d)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y)),
                      Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, color, Vec2Si32(Si32(-corner_radius),0), Vec2Si32(Vec2F(corner_radius, d)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + size.y - corner_radius)),
                      Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, color, Vec2Si32(0,Si32(-corner_radius)), Vec2Si32(Vec2F(d, corner_radius)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y - corner_radius)), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, color, Vec2Si32(Vec2F(-corner_radius,-corner_radius)), Vec2Si32(Vec2F(corner_radius, corner_radius)));

      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + corner_radius)), color);
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y + size.y - corner_radius)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y)), color);
    }
    DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + corner_radius)),
                  Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + size.y - corner_radius)), color);
  }
}

/// @brief Draw a rounded corner rectangular block shape. The shape has a border.
/// @param [in] to_sprite Sprite to draw the block on.
/// @param [in] lower_left_pos Lower-left block corner position (as if it was not rounded).
/// @param [in] size Block size.
/// @param [in] corner_radius External radius of block corners.
/// @param [in] color Fill color of the block.
/// @param [in] border_size Border width.
/// @param [in] border_color Border color.
void DrawBlock(Sprite &to_sprite, Vec2F lower_left_pos, Vec2F size, float corner_radius,
               Rgba color, float border_size, Rgba border_color) {

  if (size.x > 0 && size.y > 0 &&
      lower_left_pos.x < to_sprite.Width() &&
      lower_left_pos.y < to_sprite.Height() &&
      lower_left_pos.x + size.x >= 0.f &&
      lower_left_pos.y + size.y >= 0.f) {
    corner_radius = std::max(0.f, std::min(corner_radius, std::min(size.x, size.y) * 0.5f));
    float d = corner_radius * 2.f;
    if (corner_radius > 0.f) {
      Sprite frame;
      frame.Reference(to_sprite, Vec2Si32(lower_left_pos), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, border_color, Vec2Si32(0,0), Vec2Si32(Vec2F(d, d)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y)),
                      Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, border_color, Vec2Si32(Si32(-corner_radius),0), Vec2Si32(Vec2F(corner_radius, d)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + size.y - corner_radius)),
                      Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, border_color, Vec2Si32(0,Si32(-corner_radius)), Vec2Si32(Vec2F(d, corner_radius)));
      frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y - corner_radius)), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
      DrawOval(frame, border_color, Vec2Si32(Vec2F(-corner_radius,-corner_radius)), Vec2Si32(Vec2F(corner_radius, corner_radius)));

      float stripe_size = border_size < corner_radius ? border_size : corner_radius;
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + stripe_size)), border_color);
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y + size.y - stripe_size)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y)), border_color);
    }

    if (border_size >= size.x * 0.5f || border_size >= size.y * 0.5f) {
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + corner_radius)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + size.y - corner_radius)), border_color);
    } else {
      if (border_size > corner_radius) {
        DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + corner_radius)),
                      Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + border_size)), border_color);
        DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + size.y - border_size)),
                      Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + size.y - corner_radius)), border_color);
      }
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + corner_radius)),
                    Vec2Si32(Si32(lower_left_pos.x + border_size), Si32(lower_left_pos.y + size.y - corner_radius)), border_color);
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - border_size), Si32(lower_left_pos.y + corner_radius)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + size.y - corner_radius)), border_color);
    }

    lower_left_pos.x += border_size;
    lower_left_pos.y += border_size;
    size.x -= border_size * 2.f;
    size.y -= border_size * 2.f;
    corner_radius = std::max(0.f, corner_radius - border_size);

    if (size.x > 0 && size.y > 0) {
      corner_radius = std::max(0.f, std::min(corner_radius, std::min(size.x, size.y) * 0.5f));
      float d = corner_radius * 2.f;
      if (corner_radius > 0.f) {
        Sprite frame;
        frame.Reference(to_sprite, Vec2Si32(lower_left_pos), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
        DrawOval(frame, color, Vec2Si32(0,0), Vec2Si32(Vec2F(d, d)));
        frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y)),
                        Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
        DrawOval(frame, color, Vec2Si32(Si32(-corner_radius),0), Vec2Si32(Vec2F(corner_radius, d)));
        frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + size.y - corner_radius)),
                        Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
        DrawOval(frame, color, Vec2Si32(0,Si32(-corner_radius)), Vec2Si32(Vec2F(d, corner_radius)));
        frame.Reference(to_sprite, Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y - corner_radius)), Vec2Si32(Si32(corner_radius+1.f), Si32(corner_radius+1.f)));
        DrawOval(frame, color, Vec2Si32(Si32(-corner_radius),Si32(-corner_radius)), Vec2Si32(Vec2F(corner_radius, corner_radius)));

        DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y)),
                      Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + corner_radius)), color);
        DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x + corner_radius), Si32(lower_left_pos.y + size.y - corner_radius)),
                      Vec2Si32(Si32(lower_left_pos.x + size.x - corner_radius), Si32(lower_left_pos.y + size.y)), color);
      }
      DrawRectangle(to_sprite, Vec2Si32(Si32(lower_left_pos.x), Si32(lower_left_pos.y + corner_radius)),
                    Vec2Si32(Si32(lower_left_pos.x + size.x), Si32(lower_left_pos.y + size.y - corner_radius)), color);
    }
  }
}

Vec2F BlockEdgePos(Vec2F lower_left_pos, Vec2F size, float corner_radius, Vec2F direction) {
  Vec2F half_size = size * 0.5f;
  float time;
  if (size.x == 0.f || size.y == 0.f) {
    return lower_left_pos + half_size;
  }
  direction = NormalizeSafe(direction);
  if (direction.x == 0.f && direction.y == 0.f) {
    return lower_left_pos + half_size;
  }
  if (std::abs(half_size.x * direction.y) < std::abs(half_size.y * direction.x)) {
    time = std::abs(half_size.x / direction.x);
  } else {
    time = std::abs(half_size.y / direction.y);
  }
  float min_r = std::min(half_size.x, half_size.y);
  corner_radius = std::max(0.f, std::min(min_r, corner_radius));
  Vec2F vec = direction * time;
  Vec2F O;
  if (vec.x > half_size.x - corner_radius) {
    if (vec.y > half_size.y - corner_radius) {
      O = Vec2F(half_size.x - corner_radius, half_size.y - corner_radius);
    } else if (vec.y < -half_size.y + corner_radius) {
      O = Vec2F(half_size.x - corner_radius, -half_size.y + corner_radius);
    } else {
      return lower_left_pos + half_size + vec;
    }
  } else if (vec.x < -half_size.x + corner_radius) {
    if (vec.y > half_size.y - corner_radius) {
      O = Vec2F(-half_size.x + corner_radius, half_size.y - corner_radius);
    } else if (vec.y < -half_size.y + corner_radius) {
      O = Vec2F(-half_size.x + corner_radius, -half_size.y + corner_radius);
    } else {
      return lower_left_pos + half_size + vec;
    }
  } else {
    return lower_left_pos + half_size + vec;
  }

  float proj = direction.x * O.x + direction.y * O.y;
  float dSq = LengthSquared(O) - proj * proj;
  float xSq = corner_radius * corner_radius - dSq;
  float length = std::sqrt(xSq) + proj;
  Vec2F vecC = direction * length;
  return lower_left_pos + half_size + vecC;
}

/// @brief Draw an arrow shape.
/// @param [in] to_sprite Sprite to draw the arrow on.
/// @param [in] source_pos Tail position (source point).
/// @param [in] destination_pos Head position (destination point).
/// @param [in] body_width Tail width.
/// @param [in] head_width Head width.
/// @param [in] head_length Head length.
/// @param [in] color Fill color of the arrow.
void DrawArrow(Sprite &to_sprite, Vec2F source_pos, Vec2F destination_pos,
               float body_width, float head_width, float head_length, Rgba color) {
  float maxw = std::max(body_width, head_width) * 0.5f;
  if (std::min(source_pos.x, destination_pos.x) - maxw < to_sprite.Width() &&
      std::max(source_pos.x, destination_pos.x) + maxw >= 0 &&
      std::min(source_pos.y, destination_pos.y) - maxw < to_sprite.Height() &&
      std::max(source_pos.y, destination_pos.y) + maxw >= 0) {
    Vec2F SD = destination_pos - source_pos;

    float sd_length = Length(SD);
    if (sd_length <= 0.f) {
      return;
    }
    if (sd_length < head_length) {
      head_width = head_width * sd_length / head_length;
      head_length = sd_length;
      body_width = 0.f;
    }

    Vec2F ED = SD * head_length / sd_length;
    Vec2F SE = SD - ED;
    Vec2F E = source_pos + SE;
    Vec2F SDn = SD / sd_length;
    Vec2F Perp = Vec2F(-SDn.y, SDn.x);
    Vec2F EL = Perp * head_width * 0.5f;
    Vec2F L = E + EL;
    Vec2F R = E - EL;
    Vec2F SA = Perp * body_width * 0.5f;
    Vec2F A = source_pos + SA;
    Vec2F B = source_pos - SA;
    Vec2F M = E + SA;
    Vec2F N = E - SA;

    //                   L-
    // A-----------------M --
    // S                     -D
    // B-----------------N --
    //                   R-

    DrawTriangle(to_sprite, Vec2Si32(A), Vec2Si32(M), Vec2Si32(N), color);
    DrawTriangle(to_sprite, Vec2Si32(A), Vec2Si32(N), Vec2Si32(B), color);
    DrawTriangle(to_sprite, Vec2Si32(M), Vec2Si32(L), Vec2Si32(destination_pos), color);
    DrawTriangle(to_sprite, Vec2Si32(M), Vec2Si32(destination_pos), Vec2Si32(N), color);
    DrawTriangle(to_sprite, Vec2Si32(N), Vec2Si32(destination_pos), Vec2Si32(R), color);
  }
}

/// @brief Draw an arrow shape. The shape has a border.
/// @param [in] to_sprite Sprite to draw the arrow on.
/// @param [in] source_pos Tail position (source point).
/// @param [in] destination_pos Head position (destination point).
/// @param [in] body_width Tail width.
/// @param [in] head_width Head width.
/// @param [in] head_length Head length.
/// @param [in] color Fill color of the arrow.
/// @param [in] border_size Border width.
/// @param [in] border_color Border color.
void DrawArrow(Sprite &to_sprite, Vec2F source_pos, Vec2F destination_pos,
               float body_width, float head_width, float head_length, Rgba color,
               float border_size, Rgba border_color) {
  Vec2F SD = destination_pos - source_pos;
  float sd_length = Length(SD);
  if (sd_length > 0.f) {
    if (sd_length < head_length) {
      head_width = head_width * sd_length / head_length;
      head_length = sd_length;
      body_width = 0;
    }
    DrawArrow(to_sprite, source_pos, destination_pos, body_width, head_width, head_length, border_color);

    if (head_width > 0.f && head_length > 0.f) {
      Vec2F S2 = source_pos + (SD * border_size / sd_length);
      float L=std::sqrt((head_width * 0.5f) * (head_width * 0.5f) + head_length * head_length);
      float DestDist = L*border_size / (head_width * 0.5f);

      Vec2F D2 = destination_pos - (SD / sd_length * DestDist);
      float BodyWidth2 = std::max(0.f, body_width - (border_size * 2.f));
      float HeadLength2 = head_length - border_size -DestDist;
      float HeadWidth2 = head_width / head_length * HeadLength2;

      if (Dot(D2-S2, SD) > 0.f) {
        DrawArrow(to_sprite, S2, D2, BodyWidth2, HeadWidth2, HeadLength2, color);
      }
    }
  }
}

void ShowFrame() {
  GetEngine()->Draw2d();

  for (Si32 i = 0; i < kKeyCount; ++i) {
    g_key_state[i].OnShowFrame();
  }
  InputMessage message;
  g_mouse_pos_prev = g_mouse_pos;
  g_mouse_wheel_delta = 0;
  g_input_messages.clear();
  while (PopInputMessage(&message)) {
    if (message.kind == InputMessage::kKeyboard) {
      g_key_state[message.keyboard.key].OnStateChange(
        message.keyboard.key_state == 1);
    } else if (message.kind == InputMessage::kMouse) {
      message.mouse.backbuffer_pos =
        GetEngine()->MouseToBackbuffer(message.mouse.pos);
      g_mouse_pos = message.mouse.backbuffer_pos;
      g_mouse_wheel_delta += message.mouse.wheel_delta;
      if (message.keyboard.key != kKeyNone) {
        g_key_state[message.keyboard.key].OnStateChange(
          message.keyboard.key_state == 1);
      }
    } else if (message.kind == InputMessage::kController) {
      if (message.controller.controller_idx >= 0 &&
          message.controller.controller_idx < InputMessage::kControllerCount) {
        for (Si32 axis_idx = 0; axis_idx < kAxisCount; ++axis_idx) {
          g_controller_state[message.controller.controller_idx].axis[axis_idx] = message.controller.axis[axis_idx];
        }
        for (Si32 button_idx = 0; button_idx < 32; ++button_idx) {
          Si32 key_code = kKeyController0Button0 + 32 * message.controller.controller_idx + button_idx;
          g_key_state[key_code].OnControllerState(message.keyboard.state[key_code] == 1);
        }
      }
    }
    for (Si32 i = 0; i < kKeyCount; ++i) {
      message.keyboard.state[i] = g_key_state[i].IsDown() ? 1 : 0;
    }
    g_input_messages.push_back(message);
  }
  g_mouse_move = g_mouse_pos - g_mouse_pos_prev;
}

bool IsKeyDownwardImpl(Ui32 key_code) {
  if (key_code >= kKeyCount) {
    return false;
  }
  return g_key_state[key_code].WasPressed();
}

bool IsKeyUpwardImpl(Ui32 key_code) {
  if (key_code >= kKeyCount) {
    return false;
  }
  return g_key_state[key_code].WasReleased();
}

bool IsKeyDownward(const KeyCode key_code) {
  return IsKeyDownwardImpl(static_cast<Ui32>(key_code));
}

bool IsKeyDownward(const char *keys) {
  for (const char *key = keys; *key != 0; ++key) {
    if (IsKeyDownward(*key)) {
      return true;
    }
  }
  return false;
}

bool IsKeyDownward(const Si32 key_code) {
  return IsKeyDownwardImpl(key_code);
}

bool IsKeyDownward(const char key) {
  if (key >= 'a' && key <= 'z') {
    return IsKeyDownwardImpl(static_cast<Ui32>(key)
      + static_cast<Ui32>('A')
      - static_cast<Ui32>('a'));
  }
  return IsKeyDownwardImpl(static_cast<Ui32>(key));
}

bool IsKeyDownward(const std::string &keys) {
  return IsKeyDownward(keys.c_str());
}

bool IsKeyUpward(const KeyCode key_code) {
  return IsKeyUpwardImpl(static_cast<Ui32>(key_code));
}

bool IsKeyUpward(const char *keys) {
  for (const char *key = keys; *key != 0; ++key) {
    if (IsKeyUpward(*key)) {
      return true;
    }
  }
  return false;
}

bool IsKeyUpward(const char key) {
  if (key >= 'a' && key <= 'z') {
    return IsKeyUpwardImpl(static_cast<Ui32>(key)
      + static_cast<Ui32>('A')
      - static_cast<Ui32>('a'));
  }
  return IsKeyUpwardImpl(static_cast<Ui32>(key));
}

bool IsKeyUpward(const Si32 key_code) {
  return IsKeyUpwardImpl(key_code);
}

bool IsKeyUpward(const std::string &keys) {
  return IsKeyUpward(keys.c_str());
}


bool IsKeyDownImpl(Ui32 key_code) {
  if (key_code >= kKeyCount) {
    return false;
  }
  return g_key_state[key_code].IsDown();
}

bool IsKey(const KeyCode key_code) {
  return IsKeyDownward(key_code);
}

bool IsKey(const char *keys) {
  return IsKeyDownward(keys);
}

bool IsKey(const char key) {
  return IsKeyDownward(key);
}

bool IsKey(const Si32 key_code) {
  return IsKeyDownward(key_code);
}

bool IsKey(const std::string &keys) {
  return IsKeyDownward(keys);
}

bool IsKeyDown(const KeyCode key_code) {
  return IsKeyDownImpl(static_cast<Ui32>(key_code));
}

bool IsKeyDown(const char *keys) {
  for (const char *key = keys; *key != 0; ++key) {
    if (IsKeyDown(*key)) {
      return true;
    }
  }
  return false;
}

bool IsKeyDown(const Si32 key_code) {
  return IsKeyDownImpl(key_code);
}

bool IsKeyDown(const char key) {
  if (key >= 'a' && key <= 'z') {
    return IsKeyDownImpl(static_cast<Ui32>(key)
      + static_cast<Ui32>('A')
      - static_cast<Ui32>('a'));
  }
  return IsKeyDownImpl(static_cast<Ui32>(key));
}

bool IsKeyDown(const std::string &keys) {
  return IsKeyDown(keys.c_str());
}

bool IsAnyKeyDownward() {
  for (Si32 key = 0; key < kKeyCount; ++key) {
    if (key == kKeyMouseLeft || key == kKeyMouseRight
        || key == kKeyMouseWheel) {
      continue;
    }
    if (g_key_state[key].WasPressed()) {
      return true;
    }
  }
  return false;
}
// true is key is currently down
bool IsAnyKeyDown() {
  for (Si32 key = 0; key < kKeyCount; ++key) {
    if (key == kKeyMouseLeft || key == kKeyMouseRight
        || key == kKeyMouseWheel) {
      continue;
    }
    if (g_key_state[key].IsDown()) {
      return true;
    }
  }
  return false;
}


void SetKeyImpl(Ui32 key_code, bool is_down) {
  if (key_code < kKeyCount) {
    g_key_state[key_code].OnStateChange(is_down);
  }
}

void SetKey(const KeyCode key_code, bool is_pressed) {
  SetKeyImpl(static_cast<Ui32>(key_code), is_pressed);
}

void SetKey(const char key, bool is_pressed) {
  if (key >= 'a' && key <= 'z') {
    return SetKeyImpl(static_cast<Ui32>(key)
      + static_cast<Ui32>('A')
      - static_cast<Ui32>('a'), is_pressed);
  }
  return SetKeyImpl(static_cast<Ui32>(key), is_pressed);
}

void ClearKeyStateTransitions() {
  for (Si32 i = 0; i < kKeyCount; ++i) {
    g_key_state[i].OnShowFrame();
  }
}

float ControllerAxis(Si32 controller_idx, Si32 axis_idx) {
  if (controller_idx >= 0 && controller_idx < InputMessage::kControllerCount &&
      axis_idx >= 0 && axis_idx < kAxisCount) {
    return g_controller_state[controller_idx].axis[axis_idx];
  } else {
    return 0.f;
  }
}

Vec2Si32 MousePos() {
  return g_mouse_pos;
}

Si32 MouseX() {
  return g_mouse_pos.x;
}

Si32 MouseY() {
  return g_mouse_pos.y;
}

Vec2Si32 MouseMove() {
  return g_mouse_move;
}

Si32 MouseWheelDelta() {
  return g_mouse_wheel_delta;
}

// Size depends on OS window parameters and/or hardware
Vec2Si32 WindowSize() {
  return GetEngine()->GetWindowSize();
}

// Virtual screen size, previously set by the game developer
Vec2Si32 ScreenSize() {
  return GetEngine()->GetBackbuffer().Size();
}

// Sets virtual screen size
void ResizeScreen(const Si32 width, const Si32 height) {
  Vec2Si32 prev_size = GetEngine()->GetBackbuffer().Size();
  GetEngine()->ResizeBackbuffer(width, height);
  Vec2Si32 new_size = GetEngine()->GetBackbuffer().Size();
  if (prev_size.x > 1 && prev_size.y > 1) {
    Vec2F scale((prev_size.x - 1) ? static_cast<float>(new_size.x - 1) / static_cast<float>(prev_size.x - 1) : 1.f,
                (prev_size.y - 1) ? static_cast<float>(new_size.y - 1) / static_cast<float>(prev_size.y - 1) : 1.f);
    g_mouse_pos_prev = Vec2Si32(Vec2F(g_mouse_pos_prev) * scale);
    g_mouse_pos = Vec2Si32(Vec2F(g_mouse_pos) * scale);
    g_mouse_pos_prev = Clamp(g_mouse_pos_prev, Vec2Si32(0, 0), Vec2Si32(new_size.x - 1, new_size.y - 1));
    g_mouse_pos = Clamp(g_mouse_pos, Vec2Si32(0, 0), Vec2Si32(new_size.x - 1, new_size.y - 1));
  } else {
    g_mouse_pos_prev = Vec2Si32(0, 0);
    g_mouse_pos = Vec2Si32(0, 0);
  }
}

void ResizeScreen(const Vec2Si32 size) {
  ResizeScreen(size.x, size.y);
}

void SetInverseY(bool is_inverse) {
  GetEngine()->SetInverseY(is_inverse);
}

void Clear() {
  GetEngine()->GetBackbuffer().Clear();
  GetEngine()->GetHwBackbuffer().Clear();
}

void Clear(Rgba color) {
  GetEngine()->GetBackbuffer().Clear(color);
  GetEngine()->GetHwBackbuffer().Clear(color);
}

double Time() {
  return GetEngine()->GetTime();
}

Si64 Random(Si64 min, Si64 max) {
  return GetEngine()->GetRandom(min, max);
}

Si32 Random32(Si32 min, Si32 max) {
  return static_cast<Si32>(GetEngine()->GetRandom(min, max));
}

Ui64 Random64() {
  return GetEngine()->GetRandom64();
}

Ui32 Random32() {
  return GetEngine()->GetRandom32();
}

Ui16 Random16() {
  return GetEngine()->GetRandom16();
}

Ui8 Random8() {
  return GetEngine()->GetRandom8();
}

float RandomF() {
  return GetEngine()->GetRandomF();
}

float RandomSF() {
  return GetEngine()->GetRandomSF();
}

double RandomD() {
  return GetEngine()->GetRandomD();
}

double RandomSD() {
  return GetEngine()->GetRandomSD();
}


Si32 InputMessageCount() {
  return static_cast<Si32>(g_input_messages.size());
}

const InputMessage& GetInputMessage(Si32 idx) {
  Check(idx >= 0, "GetInputMessage called with idx < 0");
  Check(idx < static_cast<Si32>(g_input_messages.size()),
    "GetInputMessage called with idx >= InputMessagesSize()");
  return g_input_messages[static_cast<size_t>(idx)];
}


void Sleep(double duration_seconds) {
  std::this_thread::sleep_for(
    std::chrono::duration<double>(duration_seconds));
}

std::vector<Ui8> ReadFile(const char *file_name, bool is_bulletproof) {
  std::ifstream in(file_name, std::ios_base::in | std::ios_base::binary);
  std::vector<Ui8> data;
  if (in.rdstate() & std::ios_base::failbit) {
    if (is_bulletproof) {
      return data;
    }
    Fatal("Error in ReadFile. Can't open the file, file_name: ",
      file_name);
  }
  in.exceptions(std::ios_base::goodbit);
  in.seekg(0, std::ios_base::end);
  if (in.rdstate() & std::ios_base::failbit) {
    if (is_bulletproof) {
      in.close();
      return data;
    }
    Fatal("Error in ReadFile. Can't seek to the end, file_name: ",
      file_name);
  }
    std::streampos pos = in.tellg();
  if (pos == std::streampos(-1)) {
    if (is_bulletproof) {
      in.close();
      return data;
    }
    Fatal("Error in ReadFile."
      " Can't determine file size via tellg, file_name: ",
      file_name);
  }
  in.seekg(0, std::ios_base::beg);
  if (in.rdstate() & std::ios_base::failbit) {
    if (is_bulletproof) {
      in.close();
      return data;
    }
    Fatal("Error in ReadFile. Can't seek to the beg, file_name: ",
      file_name);
  }

  if (static_cast<Ui64>(pos) > 0ull) {
    data.resize(static_cast<size_t>(pos));
    in.read(reinterpret_cast<char*>(data.data()), pos);
    if (in.rdstate() != std::ios_base::goodbit) {
      if (is_bulletproof) {
        in.close();
        data.clear();
        return data;
      }
      Check((in.rdstate() & (std::ios_base::failbit | std::ios_base::eofbit))
          != (std::ios_base::failbit | std::ios_base::eofbit),
        "Error in ReadFile."
        " Can't read the data, eofbit is set, file_name: ",
        file_name);
      Check(!(in.rdstate() & std::ios_base::badbit),
        "Error in ReadFile."
        " Can't read the data, badbit is set, file_name: ",
        file_name);
      Check(in.rdstate() == std::ios_base::goodbit,
        "Error in ReadFile."
        " Can't read the data, non-goodbit, file_name: ",
        file_name);
    }
  }
  in.close();
  Check(!(in.rdstate() & std::ios_base::failbit) || is_bulletproof,
    "Error in ReadFile. Can't close the file, file_name: ",
    file_name);
  return data;
}

void WriteFile(const char *file_name, const Ui8 *data, const Ui64 data_size) {
    std::ofstream out(file_name,
      std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    Check(!(out.rdstate() & std::ios_base::failbit),
      "Error in WriteFile. Can't create/open the file, file_name: ",
      file_name);
    out.exceptions(std::ios_base::goodbit);
    out.write(reinterpret_cast<const char*>(data),
      static_cast<std::streamsize>(data_size));
    Check(!(out.rdstate() & std::ios_base::badbit),
      "Error in WriteFile. Can't write the file, file_name: ",
      file_name);
    out.close();
    Check(!(out.rdstate() & std::ios_base::failbit),
      "Error in WriteFile. Can't close the file, file_name: ",
      file_name);
}

Engine *GetEngine() {
  if (g_engine) {
    return g_engine;
  } else {
    for (Si32 i = 0; i < kKeyCount; ++i) {
      g_key_state[i].Init();
    }
    Log("\r\nStarting the engine.");
    g_engine = new Engine();
    return g_engine;
  }
}

void PrepareForTheEasyMainCall() {
  g_mouse_pos = GetEngine()->MouseToBackbuffer(Vec2F(0.5f, 0.5f));
  g_mouse_pos_prev = GetEngine()->MouseToBackbuffer(Vec2F(0.5f, 0.5f));
}

}  // namespace arctic
