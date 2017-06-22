// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#include "engine/easy.h"

#include <algorithm>
#include <chrono>  // NOLINT
#include <limits>
#include <thread>  // NOLINT

#include "engine/arctic_platform.h"

namespace arctic {
namespace easy {

static Ui32 g_key_state[kKeyCount] = {0};
static Engine *g_engine = nullptr;
static Vec2Si32 g_mouse_pos_prev = Vec2Si32(0, 0);
static Vec2Si32 g_mouse_pos = Vec2Si32(0, 0);
static Vec2Si32 g_mouse_move = Vec2Si32(0, 0);

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color) {
    DrawLine(a, b, color, color);
}

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b) {
    Vec2Si32 ab = b - a;
    Vec2Si32 abs_ab(std::abs(ab.x), std::abs(ab.y));
    if (abs_ab.x >= abs_ab.y) {
        if (a.x > b.x) {
            DrawLine(b, a, color_b, color_a);
        } else {
            Sprite back = GetEngine()->GetBackbuffer();
            Vec2Si32 back_size = back.Size();
            if (ab.x == 0) {
                if (a.x >= 0 && a.x < back_size.x &&
                        a.y >= 0 && a.y < back_size.y) {
                    back.RgbaData()[a.x + a.y * back.StridePixels()] = color_a;
                }
                return;
            }
            Si32 x1 = std::max(0, a.x);
            Si32 x2 = std::min(back_size.x - 1, b.x);
            Si32 y1 = a.y + ab.y * (x1 - a.x) / ab.x;
            Si32 y2 = a.y + ab.y * (x2 - a.x) / ab.x;
            if (y1 < 0) {
                if (y2 < 0) {
                    return;
                }
                // lower left -> upper right
                y1 = 0;
                x1 = a.x + ab.x * (y1 - a.y) / ab.y;
                x1 = std::max(0, x1);
            } else if (y1 >= back_size.y) {
                if (y2 >= back_size.y) {
                    return;
                }
                // upper left -> lower right
                y1 = back_size.y - 1;
                x1 = a.x + ab.x * (y1 - a.y) / ab.y;
                x1 = std::max(0, x1);
            }
            if (y2 < 0) {
                // upper left -> lower right
                y2 = 0;
                x2 = a.x + ab.x * (y2 - a.y) / ab.y;
                x2 = std::min(back_size.x - 1, x2);
            } else if (y2 >= back_size.y) {
                // lower left -> upper right
                y2 = back_size.y - 1;
                x2 = a.x + ab.x * (y2 - a.y) / ab.y;
                x2 = std::min(back_size.x - 1, x2);
            }

            Vec4Si32 rgba_a(
                static_cast<Si32>(color_a.r),
                static_cast<Si32>(color_a.g),
                static_cast<Si32>(color_a.b),
                static_cast<Si32>(color_a.a));
            Vec4Si32 rgba_b(
                static_cast<Si32>(color_b.r),
                static_cast<Si32>(color_b.g),
                static_cast<Si32>(color_b.b),
                static_cast<Si32>(color_b.a));
            Vec4Si32 rgba_ab = rgba_b - rgba_a;
            Vec4Si32 rgba_1 = rgba_a + rgba_ab * (x1 - a.x) / ab.x;
            Vec4Si32 rgba_2 = rgba_a + rgba_ab * (x2 - a.x) / ab.x;
            Vec4Si32 rgba_12 = rgba_2 - rgba_1;

            if (x2 <= x1) {
                if (x2 == x1) {
                    Rgba color(rgba_1.x, rgba_1.y, rgba_1.z, rgba_1.w);
                    back.RgbaData()[x1 + y1 * back.StridePixels()] = color;
                }
                return;
            }
            Vec4Si32 rgba_16 = rgba_1 * 65536;
            Vec4Si32 rgba_12_16 = rgba_12 * 65536;
            Vec4Si32 rgba_12_16_step = rgba_12_16 / (x2 - x1);
            Si32 y_16 = y1 * 65536;
            Si32 y12_16_step = ((y2 - y1) * 65536) / (x2 - x1);
            Si32 stride = back.StridePixels();
            for (Si32 x = x1; x <= x2; ++x) {
                Rgba color(
                    rgba_16.x >> 16,
                    rgba_16.y >> 16,
                    rgba_16.z >> 16,
                    rgba_16.w >> 16);
                back.RgbaData()[x + (y_16 >> 16) * stride] = color;
                rgba_16 += rgba_12_16_step;
                y_16 += y12_16_step;
            }
        }
    } else {
        if (a.y > b.y) {
            DrawLine(b, a, color_b, color_a);
        } else {
            Sprite back = GetEngine()->GetBackbuffer();
            Vec2Si32 back_size = back.Size();
            if (ab.y == 0) {
                if (a.y >= 0 && a.y < back_size.y &&
                    a.x >= 0 && a.x < back_size.x) {
                    back.RgbaData()[a.x + a.y * back.StridePixels()] = color_a;
                }
                return;
            }
            Si32 y1 = std::max(0, a.y);
            Si32 y2 = std::min(back_size.y - 1, b.y);
            Si32 x1 = a.x + ab.x * (y1 - a.y) / ab.y;
            Si32 x2 = a.x + ab.x * (y2 - a.y) / ab.y;
            if (x1 < 0) {
                if (x2 < 0) {
                    return;
                }
                // lower left -> upper right
                x1 = 0;
                y1 = a.y + ab.y * (x1 - a.x) / ab.x;
                y1 = std::max(0, y1);
            } else if (x1 >= back_size.x) {
                if (x2 >= back_size.x) {
                    return;
                }
                // lower right -> upper left
                x1 = back_size.x - 1;
                y1 = a.y + ab.y * (x1 - a.x) / ab.x;
                y1 = std::max(0, y1);
            }
            if (x2 < 0) {
                // lower right -> upper left
                x2 = 0;
                y2 = a.y + ab.y * (x2 - a.x) / ab.x;
                y2 = std::min(back_size.y - 1, y2);
            } else if (x2 >= back_size.x) {
                // lower left -> upper right
                x2 = back_size.x - 1;
                y2 = a.y + ab.y * (x2 - a.x) / ab.x;
                y2 = std::min(back_size.y - 1, y2);
            }

            Vec4Si32 rgba_a(
                static_cast<Si32>(color_a.r),
                static_cast<Si32>(color_a.g),
                static_cast<Si32>(color_a.b),
                static_cast<Si32>(color_a.a));
            Vec4Si32 rgba_b(
                static_cast<Si32>(color_b.r),
                static_cast<Si32>(color_b.g),
                static_cast<Si32>(color_b.b),
                static_cast<Si32>(color_b.a));
            Vec4Si32 rgba_ab = rgba_b - rgba_a;
            Vec4Si32 rgba_1 = rgba_a + rgba_ab * (y1 - a.y) / ab.y;
            Vec4Si32 rgba_2 = rgba_a + rgba_ab * (y2 - a.y) / ab.y;
            Vec4Si32 rgba_12 = rgba_2 - rgba_1;

            if (y2 <= y1) {
                if (y2 == y1) {
                    Rgba color(rgba_1.y, rgba_1.x, rgba_1.z, rgba_1.w);
                    back.RgbaData()[x1 + y1 * back.StridePixels()] = color;
                }
                return;
            }
            Vec4Si32 rgba_16 = rgba_1 * 65536;
            Vec4Si32 rgba_12_16 = rgba_12 * 65536;
            Vec4Si32 rgba_12_16_step = rgba_12_16 / (y2 - y1);
            Si32 x_16 = x1 * 65536;
            Si32 x12_16_step = ((x2 - x1) * 65536) / (y2 - y1);
            Si32 stride = back.StridePixels();
            for (Si32 y = y1; y <= y2; ++y) {
                Rgba color(
                    rgba_16.x >> 16,
                    rgba_16.y >> 16,
                    rgba_16.z >> 16,
                    rgba_16.w >> 16);
                back.RgbaData()[(x_16 >> 16) + y * stride] = color;
                rgba_16 += rgba_12_16_step;
                x_16 += x12_16_step;
            }
        }
    }
}

void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color) {
    DrawTriangle(a, b, c, color, color, color);
}

void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
        Rgba color_a, Rgba color_b, Rgba color_c) {
    DrawLine(a, b, color_a, color_b);
    DrawLine(a, c, color_a, color_c);
    DrawLine(b, c, color_b, color_c);
}

void ShowFrame() {
    GetEngine()->Draw2d();

    InputMessage message;
    g_mouse_pos_prev = g_mouse_pos;
    while (PopInputMessage(&message)) {
        if (message.kind == InputMessage::kKeyboard) {
            g_key_state[message.keyboard.key] = message.keyboard.key_state;
        } else if (message.kind == InputMessage::kMouse) {
            Vec2Si32 pos = GetEngine()->MouseToBackbuffer(message.mouse.pos);
            g_mouse_pos = pos;
            if (message.keyboard.key != kKeyCount) {
                g_key_state[message.keyboard.key] = message.keyboard.key_state;
            }
        }
    }
    g_mouse_move = g_mouse_pos - g_mouse_pos_prev;
}

bool IsKeyImpl(Ui32 key_code) {
    if (key_code >= kKeyCount) {
        return false;
    }
    return ((g_key_state[key_code] & 1) != 0);
}

bool IsKey(const KeyCode key_code) {
    return IsKeyImpl(static_cast<Ui32>(key_code));
}

bool IsKey(const char *keys) {
    for (const char *key = keys; *key != 0; ++key) {
        if (IsKey(*key)) {
            return true;
        }
    }
    return false;
}

bool IsKey(const char key) {
    if (key >= 'a' && key <= 'z') {
        return IsKeyImpl(static_cast<Ui32>(key)
            + static_cast<Ui32>('A')
            - static_cast<Ui32>('a'));
    }
    return IsKeyImpl(static_cast<Ui32>(key));
}

bool IsKey(const std::string &keys) {
    return IsKey(keys.c_str());
}

Vec2Si32 MousePos() {
    return g_mouse_pos;
}

Vec2Si32 MouseMove() {
    return g_mouse_move;
}

Vec2Si32 ScreenSize() {
    return GetEngine()->GetBackbuffer().Size();
}

void ResizeScreen(const Si32 width, const Si32 height) {
    GetEngine()->ResizeBackbuffer(width, height);
    return;
}

void Clear() {
    GetEngine()->GetBackbuffer().Clear();
}

void Clear(Rgba color) {
    GetEngine()->GetBackbuffer().Clear(color);
}

double Time() {
    return GetEngine()->GetTime();
}

void Sleep(double duration_seconds) {
	SleepSeconds(duration_seconds);
/*    if (duration_seconds <= 0.0) {
        return;
    }
    double usec = duration_seconds * 1000000.0;
    double limit = nexttoward(std::numeric_limits<Si64>::max(), 0ll);
    Check(usec < limit, "Sleep duration is too long");
    Si64 usec_int = static_cast<Si64>(usec);
    std::chrono::duration<Si64, std::micro> dur(usec_int);
    std::this_thread::sleep_for(dur);*/
}

std::vector<Ui8> ReadFile(const char *file_name) {
    return ReadWholeFile(file_name);
}

void WriteFile(const char *file_name, const Ui8 *data, const Ui64 data_size) {
    WriteWholeFile(file_name, data, data_size);
}

Engine *GetEngine() {
    if (!g_engine) {
        g_engine = new Engine();
    }
    return g_engine;
}

}  // namespace easy
}  // namespace arctic
