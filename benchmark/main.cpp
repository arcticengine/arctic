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

// A rendering benchmark: the same scenes drawn with HwSprite (hardware) and
// with Sprite (software backbuffer), with the frame rate on screen.
//
// Keys 1-4 pick a scene, H switches between the hardware and the software
// renderer, Escape quits. Every time a scene starts, the frame rate is
// averaged over kMeasuredFrames frames (after kWarmupFrames are skipped) and
// one line "scene,renderer,fps" is appended to benchmark_results.csv in the
// current directory, so numbers from different commits can be compared.
//
// "benchmark --auto" goes through every scene with both renderers, writes the
// lines and exits. Together with ARCTIC_HEADLESS=1 that is a benchmark run
// with no window on the screen.

#include "engine/easy.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace arctic;  // NOLINT

const Si32 kWndWidth = 1920;
const Si32 kWndHeight = 1080;

const Si32 kSceneCount = 4;
const Si32 kWarmupFrames = 30;
const Si32 kMeasuredFrames = 300;
const char *kResultsFileName = "benchmark_results.csv";

const Si32 kBlockSpriteCount = 4;
HwSprite g_hw_blocks[kBlockSpriteCount];
Sprite g_sw_blocks[kBlockSpriteCount];

Font g_font;

bool g_is_hw_enabled = true;
Si32 g_scene_idx = 1;
bool g_is_auto = false;

double g_prev_time;
double g_frame_acc = 1.0;
double g_time_acc = 0.001;
double g_fps = 0.0;

// The measurement of the scene that is running now.
Si32 g_measure_frames = 0;
double g_measure_start_time = 0.0;
bool g_is_measure_written = false;

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
  ResizeScreen(kWndWidth, kWndHeight);

  g_sw_blocks[1].Load("data/block_1.tga");
  g_sw_blocks[2].Load("data/block_2.tga");
  g_sw_blocks[0].Create(g_sw_blocks[1].Size());
  g_sw_blocks[1].Draw(g_sw_blocks[0], 0, 0, kDrawBlendingModeColorize,
      kFilterNearest, Rgba(255, 255, 255, 160));
  g_sw_blocks[1].SetPivot(g_sw_blocks[1].Size() / 2 + Vec2Si32(1, 1));

  g_hw_blocks[1].Load("data/block_1.tga");
  g_hw_blocks[2].Load("data/block_2.tga");
  g_hw_blocks[0].LoadFromSoftwareSprite(g_sw_blocks[0]);
  g_hw_blocks[1].SetPivot(g_hw_blocks[1].Size() / 2 + Vec2Si32(1, 1));

  g_font.Load("data/arctic_one_bmf.fnt");

  // Sprite 3 is a page of text rendered once with the font, so scene 3
  // measures a big sprite with lots of transparent pixels, not the font.
  const char *paragraph =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit."
      " Integer sollicitudin feugiat nulla, vel malesuada tortor varius sed.\n"
      "Quisque imperdiet tincidunt libero ut pretium. Nullam sem lectus,"
      " pharetra nec felis ut, tempus tincidunt quam. Sed porttitor erat et\n"
      "pharetra suscipit. Interdum et malesuada fames ac ante ipsum primis in"
      " faucibus. Nulla tempor tortor vel nisi maximus rutrum. Cras vitae\n"
      "imperdiet nisl. Phasellus id laoreet sapien. Etiam dignissim diam eu"
      " tellus scelerisque bibendum.\n";
  std::string text = "\n";
  for (Si32 i = 0; i < 9; ++i) {
    text += paragraph;
  }
  Sprite tmp;
  tmp.Create(g_font.EvaluateSize(text.c_str(), false));
  g_font.Draw(tmp, text.c_str(), 0, tmp.Size().y, kTextOriginTop);

  g_hw_blocks[3].LoadFromSoftwareSprite(tmp);
  g_sw_blocks[3] = tmp;
}

void StartMeasurement() {
  g_measure_frames = 0;
  g_measure_start_time = Time();
  g_is_measure_written = false;
}

void InitTiles() {
  tiles.clear();
  // Scene 0 (key 1): the screen tiled with small blocks, four half-transparent
  // copies of every block shifted by a few pixels, so the fill rate with
  // colorize blending is measured on many small sprites.
  if (g_scene_idx == 0) {
    for (int x = 0; x < kWndWidth; x += g_sw_blocks[2].Size().x) {
      for (int y = 0; y < kWndHeight; y += g_sw_blocks[2].Size().y) {
        for (int i = 0; i < 4; i++) {
          Tile tile;
          tile.block_idx = 1 + (x + y*kWndWidth) % 2;
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
  // Scene 1 (key 2): a full-screen background, three full-screen tinted
  // sprites that move and zoom (red, green, blue), 2000 random colorized
  // blocks and 5000 random quarter-size blocks drawn as a solid color.
  if (g_scene_idx == 1) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = kWndWidth;
    tile.h = kWndHeight;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 1;
    tile.x = 0;
    tile.y = 0;
    tile.w = kWndWidth;
    tile.h = kWndHeight;
    tile.zoom = 1.0f;
    tile.blending = kDrawBlendingModeColorize;

    tile.color = Rgba(255, 0, 0, 127);
    tiles.push_back(tile);
    tile.color = Rgba(0, 255, 0, 127);
    tiles.push_back(tile);
    tile.color = Rgba(0, 0, 255, 127);
    tiles.push_back(tile);

    // The same seed every time, so the hardware and the software renderer
    // draw the very same picture and the numbers are comparable between runs.
    SetRandomSeed(100500);
    for (int i = 0; i < 2000; i++) {
      tile.block_idx = 1;
      tile.x = Random32(0, kWndWidth - 1);
      tile.y = Random32(0, kWndHeight - 1);
      tile.w = g_sw_blocks[1].Size().x;
      tile.h = g_sw_blocks[1].Size().y;
      tile.zoom = 1.0f;
      tile.color = Rgba(255, 255, 255, 192);
      tile.blending = kDrawBlendingModeColorize;
      tiles.push_back(tile);
    }

    for (int i = 0; i < 5000; i++) {
      tile.block_idx = 2;
      tile.x = Random32(0, kWndWidth - 1);
      tile.y = Random32(0, kWndHeight - 1);
      tile.w = g_sw_blocks[1].Size().x / 4;
      tile.h = g_sw_blocks[1].Size().y / 4;
      tile.zoom = 1.0f;
      tile.color = Rgba(255, 255, 255, 127);
      tile.blending = kDrawBlendingModeSolidColor;
      tiles.push_back(tile);
    }
  }
  // Scene 2 (key 3): a full-screen background and the page of text (sprite 3)
  // stretched over the whole screen with a green tint.
  if (g_scene_idx == 2) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = kWndWidth;
    tile.h = kWndHeight;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 3;
    tile.x = 0;
    tile.y = 0;
    tile.w = kWndWidth;
    tile.h = kWndHeight;
    tile.zoom = 1.0f;
    tile.color = Rgba(0, 127, 0, 255);
    tile.blending = kDrawBlendingModeColorize;
    tiles.push_back(tile);
  }
  // Scene 3 (key 4): a full-screen background and one big rotating sprite.
  if (g_scene_idx == 3) {
    Tile tile;
    tile.block_idx = 0;
    tile.x = 0;
    tile.y = 0;
    tile.w = kWndWidth;
    tile.h = kWndHeight;
    tile.zoom = 1.0f;
    tile.color = Rgba(255, 255, 255, 255);
    tile.blending = kDrawBlendingModeCopyRgba;
    tiles.push_back(tile);

    tile.block_idx = 1;
    tile.x = kWndWidth/2;
    tile.y = kWndHeight/2;
    tile.w = kWndHeight/2;
    tile.h = kWndHeight/2;
    tile.zoom = 1.0f;
    tile.blending = kDrawBlendingModeColorize;
    tile.color = Rgba(255, 0, 0, 127);
    tiles.push_back(tile);
  }
  StartMeasurement();
}

void Update() {
  if (g_scene_idx == 1) {
    tiles[1].x = kWndWidth / 2 + static_cast<int>(sin(Time())*kWndWidth / 4);
    tiles[1].y = kWndHeight / 2;
    tiles[1].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[2].x = kWndWidth / 2 + static_cast<int>(cos(Time())*kWndWidth / 4);
    tiles[2].y = kWndHeight / 2;
    tiles[2].zoom = cosf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
    tiles[3].x = kWndWidth / 2;
    tiles[3].y = kWndHeight / 2 + static_cast<int>(sin(Time())*kWndHeight / 4);
    tiles[3].zoom = sinf(static_cast<float>(Time())) / 2.0f + 0.5f + 0.5f;
  }
  if (g_scene_idx == 3) {
    tiles[1].angle = static_cast<float>(Time());
  }
}

const char *RendererName() {
  return g_is_hw_enabled ? "hw" : "sw";
}

// Appends one line to the results file. The header is written when the file
// is created, so the file stays a valid CSV after any number of runs.
void WriteResult(double fps) {
  std::ifstream probe(kResultsFileName);
  const bool is_new_file = !probe.good();
  probe.close();
  std::ofstream out(kResultsFileName, std::ios::app);
  if (!out) {
    *Log() << "Can't write " << kResultsFileName;
    return;
  }
  if (is_new_file) {
    out << "scene,renderer,fps\n";
  }
  char line[128];
  snprintf(line, sizeof(line), "%d,%s,%.1f\n",
      g_scene_idx, RendererName(), fps);
  out << line;
  *Log() << "scene " << g_scene_idx << " " << RendererName()
      << " " << fps << " fps";
}

// Counts the frame just shown; returns true once, when the measurement of the
// current scene is complete and its line has been written.
bool Measure() {
  if (g_is_measure_written) {
    return false;
  }
  g_measure_frames++;
  if (g_measure_frames == kWarmupFrames) {
    g_measure_start_time = Time();
  }
  if (g_measure_frames < kWarmupFrames + kMeasuredFrames) {
    return false;
  }
  const double elapsed = Time() - g_measure_start_time;
  const double fps = elapsed > 0.0 ? kMeasuredFrames / elapsed : 0.0;
  WriteResult(fps);
  g_is_measure_written = true;
  return true;
}

// The automatic run goes through the hardware scenes first, then the software
// ones, and exits after the last line is written.
void NextAutoScene() {
  g_scene_idx++;
  if (g_scene_idx >= kSceneCount) {
    if (!g_is_hw_enabled) {
      ExitProgram(0);
      return;
    }
    g_scene_idx = 0;
    g_is_hw_enabled = false;
  }
  InitTiles();
}

void Render() {
  Clear();

  if (g_is_hw_enabled) {
    for (const auto& tile : tiles) {
      g_hw_blocks[tile.block_idx].Draw(tile.color,
        static_cast<float>(tile.x), static_cast<float>(tile.y),
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
      } else {
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
  snprintf(fps_text, sizeof(fps_text), u8"Scene: %d Mode: %s FPS: %.1F",
      g_scene_idx + 1, g_is_hw_enabled ? "Hardware" : "Software", g_fps);
  g_font.Draw(fps_text, 0, ScreenSize().y - 1, kTextOriginTop);

  ShowFrame();
}

bool HasArg(const char *name) {
  const Engine *engine = GetEngine();
  const Si32 argc = engine->GetArgc();
  const char *const *argv = engine->GetArgv();
  for (Si32 i = 1; i < argc; ++i) {
    if (argv[i] != nullptr && std::strcmp(argv[i], name) == 0) {
      return true;
    }
  }
  return false;
}

void EasyMain() {
  SetVSync(false);
  g_prev_time = Time();
  g_frame_acc = 0.0;
  g_time_acc = 0.0;
  g_is_auto = HasArg("--auto");
  if (g_is_auto) {
    g_scene_idx = 0;
    g_is_hw_enabled = true;
  }

  Init();
  InitTiles();

  while (!IsKeyDownward(kKeyEscape)) {
    if (!g_is_auto) {
      if (IsKeyDownward(kKey1)) {
        g_scene_idx = 0;
        InitTiles();
      }
      if (IsKeyDownward(kKey2)) {
        g_scene_idx = 1;
        InitTiles();
      }
      if (IsKeyDownward(kKey3)) {
        g_scene_idx = 2;
        InitTiles();
      }
      if (IsKeyDownward(kKey4)) {
        g_scene_idx = 3;
        InitTiles();
      }
      if (IsKeyDownward(kKeyH)) {
        g_is_hw_enabled = !g_is_hw_enabled;
        InitTiles();
      }
    }
    Update();
    Render();
    if (Measure() && g_is_auto) {
      NextAutoScene();
    }
  }
}
