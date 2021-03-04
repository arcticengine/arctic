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
    Rgba color;
    DrawBlendingMode blending;
};

std::vector<Tile> tiles;

void Init() {
  ResizeScreen(WND_WIDTH, WND_HEIGHT);

  g_hw_blocks[1].Load("data/block_1.tga");
  g_hw_blocks[2].Load("data/block_2.tga");
  g_hw_blocks[0].Create(g_hw_blocks[1].Size());
  g_hw_blocks[1].Draw(g_hw_blocks[0], 0, 0, kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 255, 160));
  g_hw_blocks[1].SetPivot(g_hw_blocks[1].Size() / 2 + Vec2Si32(1, 1));

  g_sw_blocks[1].Load("data/block_1.tga");
  g_sw_blocks[2].Load("data/block_2.tga");
  g_sw_blocks[0].Create(g_sw_blocks[1].Size());
  g_sw_blocks[1].Draw(g_sw_blocks[0], 0, 0, kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 255, 255, 160));
  g_sw_blocks[1].SetPivot(g_sw_blocks[1].Size() / 2 + Vec2Si32(1, 1));
  
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

}

void Render() {
  Clear();

  if (g_is_hw_enabled) {
    for (const auto &tile : tiles) {
        g_hw_blocks[tile.block_idx].Draw(tile.x, tile.y, static_cast<int>(tile.w*tile.zoom), static_cast<int>(tile.h*tile.zoom), tile.blending, kFilterNearest, tile.color);
    }
  } else {
    for (const auto &tile : tiles) {
        g_sw_blocks[tile.block_idx].Draw(tile.x, tile.y, static_cast<int>(tile.w*tile.zoom), static_cast<int>(tile.h*tile.zoom), tile.blending, kFilterNearest, tile.color);
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


void EasyMain() {
  SetVSync(false);
  g_prev_time = Time();
  g_frame_acc = 0.0;
  g_time_acc = 0.0;
  Init();
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
    if (IsKeyDownward(kKeyH)) {
      g_is_hw_enabled = !g_is_hw_enabled;
      InitTiles();
    }
    Update();
    Render();
  }
}

