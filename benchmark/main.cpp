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

#include "engine/easy.h"

#define HW_SPRITE 1
#define ENABLE_BENCH 1
#define BENCH_A 0
#define BENCH_B 1
#define BENCH_C 0

using namespace arctic;  // NOLINT

#if ENABLE_BENCH == 1
const auto WND_WIDTH = 1920;
const auto WND_HEIGHT = 1080;

const Si32 kBlockSpriteCount = 4;
#if HW_SPRITE == 1
HwSprite g_blocks[kBlockSpriteCount];
#else
Sprite g_blocks[kBlockSpriteCount];
#endif
Font g_font;

struct Tile {
    int block_idx;
    int x, y;
    int w, h;
    float zoom;
    Rgba color;
    DrawBlendingMode blending;
};

std::vector<Tile> tiles;

void Init() {
  ResizeScreen(WND_WIDTH, WND_HEIGHT);

  g_blocks[1].Load("data/block_1.tga");
  g_blocks[2].Load("data/block_2.tga");
  g_blocks[0].Create(g_blocks[1].Size());
  g_blocks[1].Draw(g_blocks[0], 0, 0, kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 255, 160));

  g_blocks[1].SetPivot(g_blocks[1].Size() / 2 + Vec2Si32(1, 1));
  
  g_font.Load("data/arctic_one_bmf.fnt");

  const char *string = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer
sollicitudin feugiat nulla, vel malesuada tortor varius sed.
Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus,
pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et
pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis
in faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae
imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam
eu tellus scelerisque bibendum.)";
  Sprite tmp;
  tmp.Create(g_font.EvaluateSize(string, false));

  g_font.Draw(tmp, string, 0, tmp.Size().y, kTextOriginTop);

#if HW_SPRITE == 1
  g_blocks[3].LoadFromSoftwareSprite(tmp);
#else
  g_blocks[3] = tmp;
#endif

  srand((Ui32)time(nullptr));

// )              4
#if BENCH_A == 1
  for (int x = 0; x < WND_WIDTH; x += g_blocks[2].Size().x) {
    for (int y = 0; y < WND_HEIGHT; y += g_blocks[2].Size().y) {
      for (int i = 0; i < 4; i++) {
          Tile tile;
          tile.block_idx = 1 + (x + y*WND_WIDTH) % 2;
          tile.x = x + (i - 2)*5;
          tile.y = y + (i - 2)*5;
          tile.w = g_blocks[2].Size().x;
          tile.h = g_blocks[2].Size().y;
          tile.zoom = 1.0f;
          tile.color = Rgba(255, 255, 255, 127);
          tile.blending = kDrawBlendingModeColorize;
          tiles.push_back(tile);
      }
    }
  }
#endif
// )         (background),         
//     (   ),        ( ),
//         ()
#if BENCH_B == 1
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

  for (int i = 0; i < 2000; i++) {
    tile.block_idx = 1;
    tile.x = rand() % WND_WIDTH;
    tile.y = rand() % WND_HEIGHT;
    tile.w = g_blocks[1].Size().x;
    tile.h = g_blocks[1].Size().y;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 192);
    tile.blending = kDrawBlendingModeColorize;
    tiles.push_back(tile);
  }

  for (int i = 0; i < 5000; i++) {
    tile.block_idx = 2;
    tile.x = rand() % WND_WIDTH;
    tile.y = rand() % WND_HEIGHT;
    tile.w = g_blocks[1].Size().x / 4;
    tile.h = g_blocks[1].Size().y / 4;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 127);
    tile.blending = kDrawBlendingModeSolidColor;
    tiles.push_back(tile);
  }
#endif
// )         (lorem ipsum).         3 .
#if BENCH_C == 1
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
#endif
}

void Update() {
#if BENCH_B == 1
    tiles[1].x = WND_WIDTH / 2 + static_cast<int>(sin(Time())*WND_WIDTH / 4);
    tiles[1].y = WND_HEIGHT / 2;
    tiles[1].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[2].x = WND_WIDTH / 2 + static_cast<int>(cos(Time())*WND_WIDTH / 4);
    tiles[2].y = WND_HEIGHT / 2;
    tiles[2].zoom = cosf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[3].x = WND_WIDTH / 2;
    tiles[3].y = WND_HEIGHT / 2 + static_cast<int>(sin(Time())*WND_HEIGHT / 4);
    tiles[3].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
#endif
}

void Render() {
  Clear();

  static DrawBlendingMode blend_modes[] = {
    kDrawBlendingModeCopyRgba,
    kDrawBlendingModeAlphaBlend,
    kDrawBlendingModeColorize,
    kDrawBlendingModeAdd
  };
  static DrawFilterMode filter_modes[] = {
      kFilterNearest,
      kFilterBilinear,
  };

  constexpr const auto total_count = WND_WIDTH*WND_HEIGHT*4*2 / 20 / 10; 

  for (const auto &tile : tiles) {
      g_blocks[tile.block_idx].Draw(tile.x, tile.y, static_cast<int>(tile.w*tile.zoom), static_cast<int>(tile.h*tile.zoom), tile.blending, kFilterNearest, tile.color);
  }
  /*for (int x = 0; x < WND_WIDTH; x += g_blocks[2].Size().x) {
    for (int y = 0; y < WND_HEIGHT; y += g_blocks[2].Size().y) {
      for (int i = 0; i < 4; i++) {
        g_blocks[(x + y*WND_WIDTH) % 3].Draw(x + i*5, y + i*5, kDrawBlendingModeAlphaBlend);
      }
    }
  }*/

  /*for (const auto &blend_mode : blend_modes) {
    for (const auto &filter_mode : filter_modes) { 
      for (int x = 0; x < WND_WIDTH; x += 20) {
        for (int y = 0; y < WND_HEIGHT; y += 10) {
          g_blocks[2].Draw(x, y, blend_mode, filter_mode);
        }
      }
    }
  }*/

  ShowFrame();
}
void EasyMain() {
  Init();

  while (!IsKeyDownward(kKeyEscape)) {
    Update();
    Render();
  }
}

#else
const Si32 kBlockSpriteCount = 3;
Sprite g_blocks[kBlockSpriteCount];
const Si32 kFieldWidth = 8;
const Si32 kFieldHeight = 16;
Si32 g_field[kFieldHeight][kFieldWidth] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {2, 0, 0, 0, 0, 0, 0, 2},
  {2, 0, 0, 0, 0, 0, 2, 2},
  {2, 0, 0, 0, 0, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
};
const Si32 kTetraminoSide = 5;
const Si32 kTetraminoCount = 7;
Si32 g_tetraminoes[kTetraminoCount * kTetraminoSide][kTetraminoSide] = {
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 1, 1, 1, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
};
const Si32 kOrientationCount = 4;
Si32 g_current[kOrientationCount][kTetraminoSide][kTetraminoSide];
Si32 g_current_x;
Si32 g_current_y;
Si32 g_current_orientation;
double g_prev_time;
Si32 g_dx = 0;
Si32 g_dy = 0;
Si32 g_rotate = 0;
Si32 g_drop = 0;
Si32 g_score = 0;
Font g_font;
void StartNewTetramino() {
  Si32 idx = Random32(0, kTetraminoCount - 1);
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      g_current[0][y][x] = g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[1][x][kTetraminoSide - 1 - y] =
        g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[2][kTetraminoSide - 1 - y][kTetraminoSide - 1 - x] =
        g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[3][kTetraminoSide - 1 - x][y] =
        g_tetraminoes[y + idx * kTetraminoSide][x];
    }
  }
  g_current_x = (kFieldWidth - kTetraminoSide + 1) / 2;
  g_current_y = 0;
  g_current_orientation = Random32(0, kOrientationCount - 1);
}
void ClearField() {
  for (Si32 y = 0; y < kFieldHeight; ++y) {
    for (Si32 x = 0; x < kFieldWidth; ++x) {
        g_field[y][x] = 0;
    }
  }
}
void Init() {
  g_blocks[1].Load("data/block_1.tga");
  g_blocks[2].Load("data/block_2.tga");
  g_blocks[0].Create(g_blocks[1].Size());
  g_blocks[1].Draw(g_blocks[0], 0, 0,
      kDrawBlendingModeColorize, kFilterNearest, Rgba(0, 0, 48));
  ResizeScreen(800, 500);
  StartNewTetramino();
  g_prev_time = Time();
  g_score = 0;
  g_font.Load("data/arctic_one_bmf.fnt");
}
bool IsPositionOk(Si32 test_x, Si32 test_y, Si32 test_orientation) {
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[test_orientation][y][x]) {
        if (x + test_x < 0
            || x + test_x >= kFieldWidth
            || y + test_y >= kFieldHeight
            || g_field[y + test_y][x + test_x]) {
          return false;
        }
      }
    }
  }
  return true;
}
void LockTetramino() {
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[g_current_orientation][y][x]) {
        g_field[y + g_current_y][x + g_current_x] = 2;
      }
    }
  }
  bool do_continue = true;
  Si32 full_lines = 0;
  while (do_continue) {
    do_continue = false;
    for (Si32 y = kFieldHeight - 1; y >= 0; --y) {
      bool is_full_line = true;
      for (Si32 x = 0; x < kFieldWidth; ++x) {
        if (!g_field[y][x]) {
          is_full_line = false;
          break;
        }
      }
      if (is_full_line) {
        full_lines++;
        do_continue = true;
        for (Si32 y2 = y; y2 > 0; --y2) {
          for (Si32 x = 0; x < kFieldWidth; ++x) {
            g_field[y2][x] = g_field[y2 - 1][x];
          }
        }
      }
    }
  }
  g_score += full_lines * full_lines * 100;
}
void Update() {
  double time = Time();
  if (IsKeyDownward(kKeyLeft) || IsKeyDownward("a")) {
    g_dx = -1;
  } else if (IsKeyDownward(kKeyRight) || IsKeyDownward("d")) {
    g_dx = 1;
  }
  g_rotate = g_rotate || IsKeyDownward(kKeyUp)
    || IsKeyDownward("w") || IsKeyDownward(" ");
  g_drop = g_drop || IsKeyDownward(kKeyDown) || IsKeyDownward("s");
  if (IsKeyDownward("c")) {
    ClearField();
    StartNewTetramino();
    return;
  }
  if (time - g_prev_time >= 0.5) {
    g_dy = 1;
    g_prev_time = time;
  } else {
    g_dy = 0;
  }

  if (g_dx && IsPositionOk(g_current_x + g_dx,
        g_current_y, g_current_orientation)) {
    g_current_x += g_dx;
    g_dy = 0;
  } else {
    g_dx = 0;
  }
  if (g_rotate) {
    if (IsPositionOk(g_current_x, g_current_y,
          (g_current_orientation + 1) % kOrientationCount)) {
      g_current_orientation = (g_current_orientation + 1) % kOrientationCount;
      g_dy = 0;
    }
  }
  bool is_lock = false;
  if (IsPositionOk(g_current_x, g_current_y + (g_dy ? 1 : 0),
        g_current_orientation)) {
    g_current_y += (g_dy ? 1 : 0);
  } else {
    is_lock = (g_dx == 0);
  }
  if (g_drop) {
    while (IsPositionOk(g_current_x, g_current_y + 1, g_current_orientation)) {
      g_current_y++;
    }
  }
  g_dx = 0;
  g_dy = 0;
  g_drop = 0;
  g_rotate = 0;
  if (is_lock) {
    LockTetramino();
    StartNewTetramino();
  }
}

HwSprite hw_sprite[4];
Sprite sw_sprite;
std::shared_ptr<GlProgram> program;

void Render() {
  Clear();
  Si32 x_offset = (ScreenSize().x - 25 * kFieldWidth) / 2;
  for (Si32 y = 0; y < kFieldHeight; ++y) {
    for (Si32 x = 0; x < kFieldWidth; ++x) {
      g_blocks[g_field[y][x]].Draw(
          x_offset + x * 25, (kFieldHeight - 1 - y) * 25);
    }
  }
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[g_current_orientation][y][x]) {
        g_blocks[g_current[g_current_orientation][y][x]].Draw(
          x_offset + (x + g_current_x) * 25,
          (kFieldHeight - 1 - y - g_current_y) * 25);
      }
    }
  }
  char score[128];
  snprintf(score, sizeof(score), u8"Score: %d", g_score);
  if (sw_sprite.Size() != g_font.EvaluateSize(score, false)) {
    sw_sprite.Create(g_font.EvaluateSize(score, false));
  }
  g_font.Draw(sw_sprite, score, 0, sw_sprite.Size().y, kTextOriginTop);
  //g_font.Draw(score, 0, ScreenSize().y, kTextOriginTop);
  //HwSprite tmp;
  //tmp.LoadFromSoftwareSprite(sw_sprite);
  //hw_sprite[3].Clone(tmp, kCloneRotate180);
  //hw_sprite[3].SetProgram(program);
  //hw_sprite[3].Uniforms().SetUniform("is_invert_color", 1);
  //hw_sprite[3].LoadFromSoftwareSprite(sw_sprite);
  //hw_sprite[3].Clear(Rgba(0, 127, 0));
  hw_sprite[3].Draw(100, ScreenSize().y - hw_sprite[3].Height() - 100);
  hw_sprite[0].Clear(Rgba(127, 127, 255));
  //hw_sprite[2].Draw(16, 16, -15.0f / 180 * 3.14f, 1.0f, hw_sprite[0], kDrawBlendingModeAlphaBlend, kFilterBilinear);
  hw_sprite[2].Draw(16.0f, 16.0f, -15.0f / 180 * 3.14f, 1.0f, hw_sprite[0]);
  static float a = 30.0f;
  a += 1.0f;
  hw_sprite[1].Draw(12.0f, 12.0f, a / 180 * 3.14f, 1, hw_sprite[0], kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 255, 127));
  hw_sprite[0].Draw(Rgba(static_cast<Ui8>(255)), 0.0f, 0.0f, 0.0f, 1.0f);
  ShowFrame();
}
void EasyMain() {
  Init();

  const char vShaderStr[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec2 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
uniform vec4 pivot_scale;
uniform vec2 to_sprite_size;
void main() {
  vec2 position = vPosition;
  position *= pivot_scale.zw;
  position += pivot_scale.xy;
  position *= vec2(2.0 / to_sprite_size.x, 2.0 / to_sprite_size.y);
  position -= vec2(1.0, 1.0);
  gl_Position = vec4(position, 0.0, 1.0);

  v_texCoord = vTex;
}
)SHADER";

  const char fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform vec4 in_color;
uniform int is_invert_color;
void main() {
  if (is_invert_color != 0) {
    gl_FragColor = texture2D(s_texture, v_texCoord)*in_color;
    gl_FragColor.xyz = vec3(1, 1, 1) - gl_FragColor.xyz;
  }
  else {
    gl_FragColor = texture2D(s_texture, v_texCoord)*in_color;
  }
}
)SHADER";

  program = std::make_shared<GlProgram>();
  program->Create(vShaderStr, fShaderStr);

  hw_sprite[0].Create(256, 256);
  hw_sprite[1].Create(64, 64);
  hw_sprite[1].Clear(Rgba(127, 127, 127));
  hw_sprite[2].Load("data/block_1.tga");
  while (!IsKeyDownward(kKeyEscape)) {
    Update();
    Render();
  }
}
#endif

