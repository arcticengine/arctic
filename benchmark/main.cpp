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

#include "engine/easy.h"
#include "engine/gl_buffer.h"
#include "engine/gl_framebuffer.h"
#include "engine/gl_program.h"
#include "engine/gl_texture2d.h"
#include "engine/mesh.h"
#include "engine/opengl.h"
#include <cstdio>
#include <vector>

using namespace arctic;  // NOLINT


const auto WND_WIDTH = 1920;
const auto WND_HEIGHT = 1080;

const Si32 kBlockSpriteCount = 4;
HwSprite g_hw_blocks[kBlockSpriteCount];
Sprite g_sw_blocks[kBlockSpriteCount];

Font g_font;

bool g_is_hw_enabled = true;
Si32 g_bench_idx = 1;
double g_prev_time;
double g_frame_acc = 1.0;
double g_time_acc = 0.001;
double g_fps = 0.0;

struct Tile {
    int block_idx;
    int x, y;
    int w, h;
    float zoom;
    float angle = 0;
    Rgba color;
    DrawBlendingMode blending;
};

std::vector<Tile> tiles;

void Init() {
  ResizeScreen(WND_WIDTH, WND_HEIGHT);

  g_sw_blocks[1].Load("data/block_1.tga");
  g_sw_blocks[2].Load("data/block_2.tga");
  g_sw_blocks[0].Create(g_sw_blocks[1].Size());
  g_sw_blocks[1].Draw(g_sw_blocks[0], 0, 0, kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 255, 160));
  g_sw_blocks[1].SetPivot(g_sw_blocks[1].Size() / 2 + Vec2Si32(1, 1));

  g_hw_blocks[1].Load("data/block_1.tga");
  g_hw_blocks[2].Load("data/block_2.tga");
  g_hw_blocks[0].LoadFromSoftwareSprite(g_sw_blocks[0]);
  g_hw_blocks[1].SetPivot(g_hw_blocks[1].Size() / 2 + Vec2Si32(1, 1));

  
  g_font.Load("data/arctic_one_bmf.fnt");

  const char *string = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus, pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu tellus scelerisque bibendum.
)";
  Sprite tmp;
  tmp.Create(g_font.EvaluateSize(string, false));

  g_font.Draw(tmp, string, 0, tmp.Size().y, kTextOriginTop);

  g_hw_blocks[3].LoadFromSoftwareSprite(tmp);
  g_sw_blocks[3] = tmp;

  srand(100500);

  srand((Ui32)time(nullptr));
}

void InitTiles() {
  tiles.clear();
  if (g_bench_idx == 0) {
    for (int x = 0; x < WND_WIDTH; x += g_sw_blocks[2].Size().x) {
      for (int y = 0; y < WND_HEIGHT; y += g_sw_blocks[2].Size().y) {
        for (int i = 0; i < 4; i++) {
            Tile tile;
            tile.block_idx = 1 + (x + y*WND_WIDTH) % 2;
            tile.x = x + (i - 2)*5;
            tile.y = y + (i - 2)*5;
            tile.w = g_sw_blocks[2].Size().x;
            tile.h = g_sw_blocks[2].Size().y;
            tile.zoom = 1.0f;
            tile.color = Rgba(255, 255, 255, 127);
            tile.blending = kDrawBlendingModeColorize;
            tiles.push_back(tile);
        }
      }
    }
  }
// )         (background),         
//     (   ),        ( ),
//         ()
  if (g_bench_idx == 1) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = WND_WIDTH;
    tile.h = WND_HEIGHT;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 1;
    tile.x = 0;
    tile.y = 0;
    tile.w = WND_WIDTH;
    tile.h = WND_HEIGHT;
    tile.zoom = 1.0f;
    tile.blending = kDrawBlendingModeColorize;

    tile.color = Rgba(255, 0, 0, 127);
    tiles.push_back(tile);
    tile.color = Rgba(0, 255, 0, 127);
    tiles.push_back(tile);
    tile.color = Rgba(0, 0, 255, 127);
    tiles.push_back(tile);

    srand(100500);
    for (int i = 0; i < 2000; i++) {
      tile.block_idx = 1;
      tile.x = rand() % WND_WIDTH;
      tile.y = rand() % WND_HEIGHT;
      tile.w = g_sw_blocks[1].Size().x;
      tile.h = g_sw_blocks[1].Size().y;
      tile.zoom = 1.0f;
      tile.color = Rgba(255, 255, 255, 192);
      tile.blending = kDrawBlendingModeColorize;
      tiles.push_back(tile);
    }

    for (int i = 0; i < 5000; i++) {
      tile.block_idx = 2;
      tile.x = rand() % WND_WIDTH;
      tile.y = rand() % WND_HEIGHT;
      tile.w = g_sw_blocks[1].Size().x / 4;
      tile.h = g_sw_blocks[1].Size().y / 4;
      tile.zoom = 1.0f;
      tile.color = Rgba(255, 255, 255, 127);
      tile.blending = kDrawBlendingModeSolidColor;
//    tile.blending = kDrawBlendingModeCopyRgba;
      tiles.push_back(tile);
    }
  }
// )         (lorem ipsum).         3 .
  if (g_bench_idx == 2) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = WND_WIDTH;
    tile.h = WND_HEIGHT;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 3;
    tile.x = 0;
    tile.y = 0;
    tile.w = WND_WIDTH;
    tile.h = WND_HEIGHT;
    tile.zoom = 1.0f;
    tile.color = Rgba(0, 127, 0, 255);
    tile.blending = kDrawBlendingModeColorize;
    tiles.push_back(tile);
  }
  if (g_bench_idx == 3) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = WND_WIDTH;
    tile.h = WND_HEIGHT;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 1;
    tile.x = WND_WIDTH/2;
    tile.y = WND_HEIGHT/2;
    tile.w = WND_HEIGHT/2;
    tile.h = WND_HEIGHT/2;
    tile.zoom = 1.0f;
    tile.blending = kDrawBlendingModeColorize;
    tile.color = Rgba(255, 0, 0, 127);
    tiles.push_back(tile);
  }
}

void Update() {
  if (g_bench_idx == 1) {
    tiles[1].x = WND_WIDTH / 2 + static_cast<int>(sin(Time())*WND_WIDTH / 4);
    tiles[1].y = WND_HEIGHT / 2;
    tiles[1].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[2].x = WND_WIDTH / 2 + static_cast<int>(cos(Time())*WND_WIDTH / 4);
    tiles[2].y = WND_HEIGHT / 2;
    tiles[2].zoom = cosf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[3].x = WND_WIDTH / 2;
    tiles[3].y = WND_HEIGHT / 2 + static_cast<int>(sin(Time())*WND_HEIGHT / 4);
    tiles[3].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
  }
  if (g_bench_idx == 3) {
    tiles[1].angle = static_cast<float>(Time());
  }

}

void Render() {
  Clear();

  if (g_is_hw_enabled) {
    for (const auto& tile : tiles) {
      g_hw_blocks[tile.block_idx].Draw(tile.color,
        (float)tile.x, (float)tile.y,
        static_cast<float>(tile.w) * tile.zoom,
        static_cast<float>(tile.h) * tile.zoom,
        tile.angle,
        tile.blending, kFilterNearest);
    }
  } else {
    for (const auto& tile : tiles) {
      if (tile.angle) {
        g_sw_blocks[tile.block_idx].Draw(tile.color,
          static_cast<float>(tile.x),
          static_cast<float>(tile.y),
          static_cast<float>(tile.w) * tile.zoom,
          static_cast<float>(tile.h) * tile.zoom,
          tile.angle,
          tile.blending, kFilterNearest);
      }
      else {
        g_sw_blocks[tile.block_idx].Draw(tile.x, tile.y,
          static_cast<int>(static_cast<float>(tile.w) * tile.zoom),
          static_cast<int>(static_cast<float>(tile.h) * tile.zoom),
          tile.blending, kFilterNearest, tile.color);
      }
    }
  }

  double time = Time();
  double dt = time - g_prev_time;
  g_prev_time = time;
  g_frame_acc += 1.0;
  g_time_acc += dt;

  if (g_time_acc > 0.5) {
    g_fps = g_frame_acc / g_time_acc;
    g_frame_acc = 0.0;
    g_time_acc = 0.0;
  }

  char fps_text[128];
  snprintf(fps_text, sizeof(fps_text), u8"Mode: %s FPS: %.1F",
      g_is_hw_enabled ? "Hardware" : "Sowfware", g_fps);
  g_font.Draw(fps_text, 0, ScreenSize().y - 1, kTextOriginTop);



  ShowFrame();
}


// Minimal shaders used by bug-reproduction tests below.
// The vertex shader does nothing interesting; the fragment shader declares
// a single float uniform so we can verify its value from the CPU side.
static const char *kTestVS = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
attribute vec2 vPosition;
void main() {
  gl_Position = vec4(vPosition, 0.0, 1.0);
}
)SHADER";

static const char *kTestFS = R"SHADER(
#ifdef GL_ES
precision lowp float;
#endif
uniform float u_test;
void main() {
  gl_FragColor = vec4(u_test, 0.0, 0.0, 1.0);
}
)SHADER";

// -----------------------------------------------------------------------
// Bug 1.  GlProgram::Create() leaks vertex and fragment shader objects.
//
// After linking the program, the two shader handles created by LoadShader()
// are never deleted with glDeleteShader().  They stay attached to the
// program and consume driver memory for the lifetime of the program (and
// remain as orphaned GL objects if the program is later destroyed while
// shaders are still attached).
//
// We detect this by querying GL_ATTACHED_SHADERS on the program right
// after Create().  If the shaders were properly detached and deleted the
// count would be 0.  With the bug the count is 2.
// -----------------------------------------------------------------------
void TestShaderLeak() {
  GlProgram program;
  program.Create(kTestVS, kTestFS);
  program.Bind();

  GLint prog_id = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &prog_id);

  GLint attached = 0;
  glGetProgramiv(prog_id, GL_ATTACHED_SHADERS, &attached);

  if (attached > 0) {
    std::printf("[gl_program BUG] shader leak: %d shader(s) still attached "
                "after Create() -- they should be deleted after linking\n",
                attached);
  } else {
    std::printf("[gl_program OK ] shaders properly cleaned up after Create()\n");
  }
}

// -----------------------------------------------------------------------
// Bug 2.  UniformsTable::SetUniform() silently ignores updates.
//
// Every SetUniform overload does:
//     table_.insert(std::make_pair(name, data));
//
// std::unordered_map::insert() is a no-op when the key already exists, so
// calling SetUniform("u_test", 1.0f) followed by SetUniform("u_test", 99.0f)
// leaves the map with value 1.0f.  The second call is silently discarded.
//
// We prove this by creating a real GL program, setting a uniform via
// UniformsTable, "updating" it, calling Apply(), and reading back the
// actual GL uniform value with glGetUniformfv.
// -----------------------------------------------------------------------
void TestUniformsTableStaleValue() {
  GlProgram program;
  program.Create(kTestVS, kTestFS);
  program.Bind();

  UniformsTable table;
  table.SetUniform(std::string("u_test"), 1.0f);
  table.SetUniform(std::string("u_test"), 99.0f);  // should overwrite, but doesn't
  table.Apply(program);

  GLint prog_id = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &prog_id);

  GLint loc = glGetUniformLocation(prog_id, "u_test");
  float readback = -1.0f;
  glGetUniformfv(prog_id, loc, &readback);

  if (readback == 1.0f) {
    std::printf("[gl_program BUG] stale uniform: SetUniform(\"u_test\", 99.0f) "
                "was ignored, GPU value is still %.1f (insert() does not "
                "overwrite)\n", readback);
  } else if (readback == 99.0f) {
    std::printf("[gl_program OK ] SetUniform update applied correctly "
                "(value = %.1f)\n", readback);
  } else {
    std::printf("[gl_program ??? ] unexpected uniform value: %.1f\n", readback);
  }
}

// -----------------------------------------------------------------------
// Bug 3.  GlTexture2D::Bind() skips glActiveTexture on cache hit.
//
// The bind-cache logic is structured so that glActiveTexture is only
// called *inside* the "cache miss" branch:
//
//   if (current_texture_id_[slot] != texture_id_) {
//       if (current_texture_slot_ != slot) {
//           glActiveTexture(GL_TEXTURE0 + slot);        // <-- here
//       }
//       glBindTexture(GL_TEXTURE_2D, texture_id_);
//   }
//
// If the texture is already cached for the requested slot, the entire
// block is skipped -- including glActiveTexture.  This means the GL
// active texture unit can be left pointing at the wrong slot.  Any
// subsequent GL texture operation (glTexImage2D, glTexSubImage2D, etc.)
// will silently modify the wrong texture.
//
// Reproduction: bind texA to slot 0, then texB to slot 1 (active
// slot moves to 1), then call texA.Bind(0) again.  Since texA is
// cached in slot 0, everything is skipped.  GL_ACTIVE_TEXTURE stays
// at GL_TEXTURE1 instead of switching to GL_TEXTURE0.
// -----------------------------------------------------------------------
void TestGlTexture2DBindSkipsActiveSlot() {
  GlTexture2D texA;
  texA.Create(1, 1);

  GlTexture2D texB;
  texB.Create(1, 1);

  // Step 1: bind texA to slot 0.  Active slot becomes 0.
  texA.Bind(0);

  // Step 2: bind texB to slot 1.  Active slot becomes 1.
  texB.Bind(1);

  // Step 3: re-bind texA to slot 0.
  // With the bug, this is a cache hit and glActiveTexture is NOT called.
  texA.Bind(0);

  GLint active_texture = 0;
  ARCTIC_GL_CHECK_ERROR(glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture));

  if (active_texture != GL_TEXTURE0) {
    std::printf("[gl_texture2d BUG] Bind(0) cache hit skipped glActiveTexture. "
                "Active slot is %d, expected 0. Subsequent GL texture "
                "operations would corrupt the wrong texture.\n",
                active_texture - GL_TEXTURE0);
  } else {
    std::printf("[gl_texture2d OK ] Bind(0) correctly set the active "
                "texture slot.\n");
  }
}

// -----------------------------------------------------------------------
// Bug 4.  GlTexture2D::Create() does not invalidate the bind cache.
//
// Create() deletes the old GL texture via glDeleteTextures, then
// generates a new one via glGenTextures.  However, the static cache
// array current_texture_id_[] is never updated.  If the old texture
// was cached in some slot, the cache still maps that slot to the old
// (now-deleted) ID.
//
// OpenGL drivers commonly reuse texture IDs once they are freed.  So
// the newly generated texture may receive the exact same GLuint as
// the old one.  When Bind() is called, it compares new_id == cached_id,
// finds a "hit", and skips glBindTexture entirely.  But the actual GL
// binding was cleared by glDeleteTextures, so the new texture is never
// bound and subsequent texture uploads go nowhere (or to texture 0).
//
// We reproduce this by creating a texture, binding it, then calling
// Create() again on the same object.  If the driver reuses the ID,
// the cache hit prevents the new texture from being bound.
// -----------------------------------------------------------------------
void TestGlTexture2DStaleCacheAfterRecreate() {
  GlTexture2D tex;
  tex.Create(1, 1);
  tex.Bind(0);

  GLuint old_id = tex.texture_id();

  // Re-create.  Internally: glDeleteTextures(old_id), glGenTextures(&new_id),
  // Bind(0).  If new_id == old_id, the cache thinks it's already bound, but
  // glDeleteTextures already unbound it in GL.
  tex.Create(2, 2);

  // After Create, the texture must be bound.  Query what GL actually has.
  GLint bound = 0;
  ARCTIC_GL_CHECK_ERROR(glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound));

  GLuint new_id = tex.texture_id();

  if (old_id == new_id && static_cast<GLuint>(bound) != new_id) {
    std::printf("[gl_texture2d BUG] stale cache: after re-Create(), driver "
                "reused texture ID %u, but GL binding is %d (not %u). "
                "The cache 'hit' prevented glBindTexture.\n",
                new_id, bound, new_id);
  } else if (static_cast<GLuint>(bound) != new_id) {
    std::printf("[gl_texture2d BUG] after re-Create(), GL binding is %d "
                "but expected %u. Cache was not invalidated.\n",
                bound, new_id);
  } else if (old_id == new_id) {
    std::printf("[gl_texture2d OK ] driver reused ID %u and texture is "
                "correctly bound (cache was invalidated or driver kept "
                "binding alive).\n", new_id);
  } else {
    std::printf("[gl_texture2d OK ] new texture ID %u correctly bound "
                "(old was %u, cache miss).\n", new_id, old_id);
  }
}

// -----------------------------------------------------------------------
// Bug 5.  GlFramebuffer destructor and Create() do not invalidate
//         current_framebuffer_id_.
//
// When a GlFramebuffer is destroyed, glDeleteFramebuffers is called, but
// the static cache current_framebuffer_id_ is not cleared.  OpenGL
// automatically unbinds a deleted framebuffer (binding reverts to 0),
// yet the cache still holds the old ID.
//
// If Create() is later called on the same object, glGenFramebuffers may
// reuse the old ID.  Bind() then sees current_framebuffer_id_ == new id
// (a false "cache hit") and skips glBindFramebuffer.  The framebuffer
// is never actually bound, so glFramebufferTexture2D inside Create()
// attaches the texture to FBO 0 (the default framebuffer) instead.
//
// We reproduce this by creating a framebuffer, binding it, then
// re-creating it.  After re-creation we query GL_FRAMEBUFFER_BINDING
// to see whether the new FBO is actually bound.
// -----------------------------------------------------------------------
void TestGlFramebufferStaleCacheAfterRecreate() {
  GlTexture2D tex;
  tex.Create(1, 1);

  GlFramebuffer fb;
  fb.Create(tex);
  fb.Bind();

  // Query the currently bound FBO -- this is the "old" ID.
  GLint old_bound = 0;
  ARCTIC_GL_CHECK_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_bound));

  // Re-create.  Internally: glDeleteFramebuffers(old_id) (GL unbinds it,
  // binding -> 0, but cache keeps old_id), glGenFramebuffers(&new_id),
  // Bind() -- if new_id == old_id, cache hit, skip.
  GlTexture2D tex2;
  tex2.Create(1, 1);
  fb.Create(tex2);

  GLint new_bound = 0;
  ARCTIC_GL_CHECK_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &new_bound));

  if (new_bound == 0 && old_bound != 0) {
    std::printf("[gl_framebuf BUG] stale cache: after re-Create(), GL "
                "framebuffer binding is 0 (default). The new FBO was "
                "never bound because the cache saw a false hit on the "
                "reused ID %d.\n", old_bound);
  } else if (new_bound == 0) {
    std::printf("[gl_framebuf BUG] after re-Create(), GL framebuffer "
                "binding is 0 -- expected the new FBO to be bound.\n");
  } else {
    std::printf("[gl_framebuf OK ] framebuffer correctly bound after "
                "re-Create() (GL binding = %d).\n", new_bound);
  }

  // Restore default framebuffer so subsequent rendering is not affected.
  GlFramebuffer::BindDefault();
}

// -----------------------------------------------------------------------
// Bug 6.  GlBuffer destructor and Create() do not invalidate
//         current_buffer_id_.
//
// Identical pattern to the framebuffer bug.  When a GlBuffer is
// destroyed or re-created, glDeleteBuffers frees the old GL name and
// OpenGL unbinds it, but the static cache current_buffer_id_ still
// holds the stale value.  If glGenBuffers reuses the same name,
// Bind() sees a false cache hit and skips glBindBuffer.
//
// We reproduce this by creating a buffer, binding it to GL_ARRAY_BUFFER,
// then calling Create() again.  After the re-creation we query
// GL_ARRAY_BUFFER_BINDING to check whether the new buffer is bound.
// -----------------------------------------------------------------------
void TestGlBufferStaleCacheAfterRecreate() {
  GlBuffer buf;
  buf.Create();
  buf.Bind(GL_ARRAY_BUFFER);

  GLuint old_id = buf.buffer_id();

  // Re-create.  Internally: glDeleteBuffers(old_id) (GL unbinds,
  // binding -> 0, cache keeps old_id), glGenBuffers(&new_id).
  // No Bind() is called from Create(), so the new buffer isn't bound
  // at all.  A subsequent buf.Bind(GL_ARRAY_BUFFER) checks the cache.
  buf.Create();
  buf.Bind(GL_ARRAY_BUFFER);

  GLint bound = 0;
  ARCTIC_GL_CHECK_ERROR(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound));

  GLuint new_id = buf.buffer_id();

  if (old_id == new_id && static_cast<GLuint>(bound) != new_id) {
    std::printf("[gl_buffer   BUG] stale cache: driver reused buffer ID %u, "
                "but GL binding is %d (not %u). Cache 'hit' prevented "
                "glBindBuffer.\n", new_id, bound, new_id);
  } else if (static_cast<GLuint>(bound) != new_id) {
    std::printf("[gl_buffer   BUG] after re-Create(), GL_ARRAY_BUFFER "
                "binding is %d but expected %u.\n", bound, new_id);
  } else if (old_id == new_id) {
    std::printf("[gl_buffer   OK ] driver reused ID %u and buffer is "
                "correctly bound.\n", new_id);
  } else {
    std::printf("[gl_buffer   OK ] new buffer ID %u correctly bound "
                "(old was %u, cache miss).\n", new_id, old_id);
  }

  GlBuffer::BindDefault(GL_ARRAY_BUFFER);
}

// -----------------------------------------------------------------------
// Bug 7.  Mesh::Clone() forgets to copy vertex and face counts.
//
// Clone() calls Init() on the destination mesh, which allocates
// buffers with the correct *capacity* (mMax) but sets mNum = 0 for
// both vertex arrays and index arrays.  Clone() then memcpy's the
// source data into the destination buffers but never sets
// dst.mVertexArray[0].mNum or dst.mIndexArray[0].mNum.
//
// As a result, the cloned mesh reports 0 vertices and 0 faces even
// though the data is present in the buffers.  Any code that uses
// GetCurrentVertexCount / GetCurrentFaceCount (or mNum directly)
// will see an empty mesh.
// -----------------------------------------------------------------------
void TestMeshCloneForgetsCount() {
  MeshVertexFormat vf;
  vf.AddElement(3, kRMVEDT_Float);  // position: 3 floats

  Mesh src;
  // 1 vertex stream, up to 8 vertices, polygon mesh, 1 index array, up to 4 faces
  if (!src.Init(1, 8, &vf, kRMVEDT_Polys, 1, 4)) {
    std::printf("[mesh        ???] Init failed, cannot run Clone test.\n");
    return;
  }

  // Add 3 vertices (a triangle).
  src.AddVertex(0, 0.0f, 0.0f, 0.0f);
  src.AddVertex(0, 1.0f, 0.0f, 0.0f);
  src.AddVertex(0, 0.0f, 1.0f, 0.0f);

  // Add 1 face.
  src.AddFace(0, 0, 1, 2);

  int src_vcount = src.GetCurrentVertexCount(0);
  int src_fcount = src.GetCurrentFaceCount(0);

  Mesh dst;
  if (!src.Clone(&dst)) {
    std::printf("[mesh        ???] Clone failed.\n");
    return;
  }

  int dst_vcount = dst.GetCurrentVertexCount(0);
  int dst_fcount = dst.GetCurrentFaceCount(0);

  bool vbug = (dst_vcount != src_vcount);
  bool fbug = (dst_fcount != src_fcount);

  if (vbug || fbug) {
    std::printf("[mesh        BUG] Clone() lost counts: "
                "src has %d verts / %d faces, "
                "dst has %d verts / %d faces. "
                "Init() zeroes mNum and Clone() never restores it.\n",
                src_vcount, src_fcount, dst_vcount, dst_fcount);
  } else {
    std::printf("[mesh        OK ] Clone() preserved counts correctly "
                "(%d verts, %d faces).\n", dst_vcount, dst_fcount);
  }
}

// -----------------------------------------------------------------------
// Bug 8.  Mesh::AddFace() has no bounds check.
//
// AddFace() writes to ia->mBuffer[ia->mNum] and increments mNum
// without checking whether mNum < mMax.  If more faces are added
// than were allocated, this overflows the heap buffer.
//
// We demonstrate this by filling a mesh to capacity and then
// attempting to add one more face.  Without the bounds check the
// write would go past the allocated buffer (heap overflow).  With
// the fix, AddFace() returns -1 and the count stays at capacity.
// -----------------------------------------------------------------------
void TestMeshAddFaceNoBoundsCheck() {
  MeshVertexFormat vf;
  vf.AddElement(3, kRMVEDT_Float);

  Mesh mesh;
  // Allocate space for exactly 1 face.
  if (!mesh.Init(1, 8, &vf, kRMVEDT_Polys, 1, 1)) {
    std::printf("[mesh        ???] Init failed, cannot run AddFace test.\n");
    return;
  }

  mesh.AddVertex(0, 0.0f, 0.0f, 0.0f);
  mesh.AddVertex(0, 1.0f, 0.0f, 0.0f);
  mesh.AddVertex(0, 0.0f, 1.0f, 0.0f);

  // Add one face -- this fills the buffer to capacity.
  mesh.AddFace(0, 0, 1, 2);

  // Try to add a second face past the capacity limit.
  int overflow_result = mesh.AddFace(0, 0, 1, 2);
  int count_after = mesh.GetCurrentFaceCount(0);
  int capacity = static_cast<int>(mesh.mFaceData.mIndexArray[0].mMax);

  if (overflow_result >= 0 || count_after > capacity) {
    std::printf("[mesh        BUG] AddFace() has no bounds check. "
                "Buffer is full (%d/%d) but AddFace() returned %d "
                "instead of -1 (heap overflow).\n",
                count_after, capacity, overflow_result);
  } else {
    std::printf("[mesh        OK ] AddFace() rejected overflow "
                "(returned %d, count stayed at %d/%d).\n",
                overflow_result, count_after, capacity);
  }
}

// -----------------------------------------------------------------------
// Bug 9.  Font::Draw(palette) dereferences palete[0] without checking
//         if the palette is empty.
//
// Both the Sprite-overload (font.h) and the backbuffer-overload
// (font.cpp) pass palete[0] as the color argument to
// DrawEvaluateSizeImpl.  If the caller provides an empty
// std::vector<Rgba>, this is an unconditional out-of-bounds access
// that immediately crashes (SIGSEGV / SIGABRT).
//
// We test this by actually calling Draw with an empty palette on a
// small sprite.  Without the fix this would crash; with the fix it
// should fall back to a default color and return normally.
// -----------------------------------------------------------------------
void TestFontDrawEmptyPaletteCrash() {
  Font font;
  font.CreateEmpty(8, 10);

  Sprite target;
  target.Create(16, 16);

  std::vector<Rgba> empty_palette;

  // Before the fix this line would crash (palete[0] on empty vector).
  // After the fix it should use a default color and return safely.
  font.Draw(target, "x", 0, 0,
            kTextOriginBottom, kTextAlignmentLeft,
            kDrawBlendingModeColorize, kFilterNearest,
            empty_palette);

  std::printf("[font        OK ] Font::Draw(palette) survived an "
              "empty palette without crashing.\n");
}

void EasyMain() {
  SetVSync(false);
  g_prev_time = Time();
  g_frame_acc = 0.0;
  g_time_acc = 0.0;

  // Mesh / font tests do not require a GL context.
  std::printf("--- mesh bug reproduction ---\n");
  TestMeshCloneForgetsCount();
  TestMeshAddFaceNoBoundsCheck();
  std::printf("-----------------------------\n");

  std::printf("--- font bug reproduction ---\n");
  TestFontDrawEmptyPaletteCrash();
  std::printf("-----------------------------\n");

  Init();

  // GL context is ready after Init() -- run GL bug reproduction tests.
  std::printf("--- gl_program bug reproduction ---\n");
  TestShaderLeak();
  TestUniformsTableStaleValue();
  std::printf("-----------------------------------\n");

  std::printf("--- gl_texture2d bug reproduction ---\n");
  TestGlTexture2DBindSkipsActiveSlot();
  TestGlTexture2DStaleCacheAfterRecreate();
  std::printf("-------------------------------------\n");

  std::printf("--- gl_framebuffer bug reproduction ---\n");
  TestGlFramebufferStaleCacheAfterRecreate();
  std::printf("---------------------------------------\n");

  std::printf("--- gl_buffer bug reproduction ---\n");
  TestGlBufferStaleCacheAfterRecreate();
  std::printf("----------------------------------\n");
  InitTiles();

  while (!IsKeyDownward(kKeyEscape)) {
    if (IsKeyDownward(kKey1)) {
      g_bench_idx = 0;
      InitTiles();
    }
    if (IsKeyDownward(kKey2)) {
      g_bench_idx = 1;
      InitTiles();
    }
    if (IsKeyDownward(kKey3)) {
      g_bench_idx = 2;
      InitTiles();
    }
    if (IsKeyDownward(kKey4)) {
      g_bench_idx = 3;
      InitTiles();
    }
    if (IsKeyDownward(kKeyH)) {
      g_is_hw_enabled = !g_is_hw_enabled;
      InitTiles();
    }
    Update();
    Render();
  }
}

