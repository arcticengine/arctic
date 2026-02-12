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
#include "engine/gl_program.h"
#include "engine/gl_texture2d.h"
#include "engine/opengl.h"
#include <cstdio>

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

void EasyMain() {
  SetVSync(false);
  g_prev_time = Time();
  g_frame_acc = 0.0;
  g_time_acc = 0.0;
  Init();

  // GL context is ready after Init() -- run bug reproduction tests.
  std::printf("--- gl_program bug reproduction ---\n");
  TestShaderLeak();
  TestUniformsTableStaleValue();
  std::printf("-----------------------------------\n");

  std::printf("--- gl_texture2d bug reproduction ---\n");
  TestGlTexture2DBindSkipsActiveSlot();
  TestGlTexture2DStaleCacheAfterRecreate();
  std::printf("-------------------------------------\n");
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

