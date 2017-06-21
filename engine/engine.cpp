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

#include "engine/engine.h"
#include "engine/opengl.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_math.h"

namespace arctic {

void Engine::Init(Si32 width, Si32 height) {
    width_ = width;
    height_ = height;

    SetVSync(true);

    ResizeBackbuffer(width, height);

    start_time_ = clock_.now();
    time_correction_ = 0.0;
    last_time_ = 0.0;
}

void Engine::Draw2d() {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        backbuffer_texture_.width(), backbuffer_texture_.height(), GL_RGBA,
         GL_UNSIGNED_BYTE, static_cast<GLvoid*>(backbuffer_texture_.RawData()));

    // render
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backbuffer_texture_name_);

	glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // draw quad

    visible_verts_.Resize(16 << 20);
    visible_normals_.Resize(16 << 20);
    visible_indices_.Resize(16 << 20);
    tex_coords_.Resize(16 << 20);

    verts_ = 0;
    normals_ = 0;
    tex_ = 0;
    indices_ = 0;

    Vec3F *vertex = static_cast<Vec3F*>(visible_verts_.GetVoidData());
    Vec3F *normal = static_cast<Vec3F*>(visible_normals_.GetVoidData());
    Vec2F *tex = static_cast<Vec2F*>(tex_coords_.GetVoidData());
    Ui32 *index = static_cast<Ui32*>(visible_indices_.GetVoidData());

    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float back_aspect = static_cast<float>(backbuffer_texture_.width()) /
        static_cast<float>(backbuffer_texture_.height());
    float ratio = back_aspect / aspect;
    float x_aspect = aspect < back_aspect ? 1.f : ratio;
    float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;

    Vec3F base = Vec3F(-1.f * x_aspect, -1.f * y_aspect, 0.f);
    Vec3F tx = Vec3F(2.f * x_aspect, 0.f, 0.f);
    Vec3F ty = Vec3F(0.f, 2.f * y_aspect, 0.f);
    Vec3F n = Vec3F(0.f, 0.f, 1.f);

    Si32 idx = verts_;
    vertex[verts_] = base;
    ++verts_;
    vertex[verts_] = base + tx;
    ++verts_;
    vertex[verts_] = base + ty + tx;
    ++verts_;
    vertex[verts_] = base + ty;
    ++verts_;

    normal[normals_] = n;
    ++normals_;
    normal[normals_] = n;
    ++normals_;
    normal[normals_] = n;
    ++normals_;
    normal[normals_] = n;
    ++normals_;

    tex[tex_] = Vec2F(0.0f, 0.0f);
    ++tex_;
    tex[tex_] = Vec2F(1.0f, 0.0f);
    ++tex_;
    tex[tex_] = Vec2F(1.0f, 1.0f);
    ++tex_;
    tex[tex_] = Vec2F(0.0f, 1.0f);
    ++tex_;

    index[indices_] = idx;
    indices_++;
    index[indices_] = idx + 1;
    indices_++;
    index[indices_] = idx + 2;
    indices_++;
    index[indices_] = idx + 2;
    indices_++;
    index[indices_] = idx + 3;
    indices_++;
    index[indices_] = idx;
    indices_++;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, visible_verts_.GetVoidData());
    glNormalPointer(GL_FLOAT, 0, visible_normals_.GetVoidData());
    glTexCoordPointer(2, GL_FLOAT, 0, tex_coords_.GetVoidData());
    glDrawElements(GL_TRIANGLES, indices_, GL_UNSIGNED_INT,
        visible_indices_.GetVoidData());

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    Swap();
}

void Engine::ResizeBackbuffer(const Si32 width, const Si32 height) {
    backbuffer_texture_.Create(width, height);

    glEnable(GL_TEXTURE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGenTextures(1, &backbuffer_texture_name_);
    // generate a texture handler really reccomanded (mandatory in openGL 3.0)
    glBindTexture(GL_TEXTURE_2D, backbuffer_texture_name_);
    // tell openGL that we are using the texture

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, backbuffer_texture_.RawData());
    // send the texture data
}

double Engine::GetTime() {
    auto now = clock_.now();
    if (now > start_time_) {
        double duration =
            std::chrono::duration<double>(now - start_time_).count();
        double time = duration + time_correction_;
        if (time >= last_time_) {
            last_time_ = time;
            return time;
        }
        time_correction_ = last_time_ - duration;
        return last_time_;
    }
    start_time_ = now;
    time_correction_ = last_time_;
    return last_time_;
}

Vec2Si32 Engine::MouseToBackBuffer(Vec2F pos) const {
    Vec2F rel_pos = pos - Vec2F(0.5f, 0.5f);
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float back_aspect = static_cast<float>(backbuffer_texture_.width()) /
        static_cast<float>(backbuffer_texture_.height());
    float ratio = back_aspect / aspect;
    float x_aspect = aspect < back_aspect ? 1.f : ratio;
    float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;
    Vec2F cor_pos(rel_pos.x / x_aspect, rel_pos.y / y_aspect);
    Vec2F back_rel_pos = cor_pos + Vec2F(0.5f, 0.5f);
    Vec2F scale = Vec2F(backbuffer_texture_.Size() - Vec2Si32(1, 1));
    Vec2F back_pos(
        Clamp(back_rel_pos.x, 0.f, 1.f) * scale.x,
        Clamp(back_rel_pos.y, 0.f, 1.f) * scale.y);

    return Vec2Si32(static_cast<Si32>(back_pos.x),
        static_cast<Si32>(back_pos.y));
}

Vec2Si32 Engine::ScreenSize() const {
    return Vec2Si32(width_, height_);
}

}  // namespace arctic
