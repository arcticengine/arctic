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

#include "engine/arctic_platform.h"

#include <chrono>
#include <thread>

namespace arctic {
namespace easy {

static Ui32 g_key_state[kKeyCount] = {0};
static Engine *g_engine = nullptr;
static Vec2Si32 g_mouse_pos_prev = Vec2Si32(0, 0);
static Vec2Si32 g_mouse_pos = Vec2Si32(0, 0);
static Vec2Si32 g_mouse_move = Vec2Si32(0, 0);

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color);
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c);

void ShowFrame() {
    GetEngine()->Draw2d();

    Vec2Si32 size = GetEngine()->GetBackbuffer().Size();
    Vec2F scale(static_cast<float>(size.x - 1),
        static_cast<float>(size.y - 1));
    InputMessage message;
    g_mouse_pos_prev = g_mouse_pos;
    while (PopInputMessage(&message)) {
        if (message.kind == InputMessage::kKeyboard) {
            g_key_state[message.keyboard.key] = message.keyboard.key_state;
        } else if (message.kind == InputMessage::kMouse) {
            Vec2F float_pos(scale.x * message.mouse.pos.x,
                scale.y * message.mouse.pos.y);
            Vec2Si32 pos(static_cast<Si32>(float_pos.x),
                static_cast<Si32>(float_pos.y));
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
    if (duration_seconds <= 0.0) {
        return;
    }
    double usec = duration_seconds * 1000000.0;
    double limit = nexttoward(std::numeric_limits<Si64>::max(), 0ll);
    Check(usec < limit, "Sleep duration is too long");
    Si64 usec_int = static_cast<Si64>(usec);
    std::chrono::duration<Si64, std::micro> dur(usec_int);
    std::this_thread::sleep_for(dur);
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
