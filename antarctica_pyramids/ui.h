// The MIT License (MIT)
//
// Copyright (c) 2016 - 2026 Huldra
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

#ifndef ANTARCTICA_PYRAMIDS_UI_H_
#define ANTARCTICA_PYRAMIDS_UI_H_

// Everything the player sees: the sprites of the world, the intro, and the
// GUI screens built from the theme in data/gui_theme.xml. The screens are
// plain structs of widgets; main.cpp wires their buttons to what they do.

#include <memory>
#include <vector>
#include "engine/easy.h"
#include "engine/gui.h"

namespace pyramids {

using namespace arctic;  // NOLINT

const Si32 kTileSize = 25;
const Si32 kHudHeight = 120;
const Si32 kScreenWidth = 800;
const Si32 kScreenHeight = 500 + kHudHeight;

struct Menu {
  std::shared_ptr<Panel> root;
  std::shared_ptr<Button> continue_button;
  std::shared_ptr<Button> new_game_button;
  std::shared_ptr<Button> quit_button;
  std::shared_ptr<Checkbox> music_checkbox;
  std::shared_ptr<Scrollbar> volume_scrollbar;  // 0..100
};

struct Hud {
  std::shared_ptr<Panel> root;
  std::vector<std::shared_ptr<Button>> action_buttons;
  std::shared_ptr<Progressbar> hitpoints_bar;
  std::shared_ptr<Progressbar> warmth_bar;
  std::shared_ptr<Progressbar> endurance_bar;
};

struct QuitDialog {
  std::shared_ptr<Panel> root;
  std::shared_ptr<Button> quit_button;
  std::shared_ptr<Button> cancel_button;
};

struct GameOverScreen {
  std::shared_ptr<Panel> root;
  std::shared_ptr<Text> text;
  std::shared_ptr<Button> again_button;
  std::shared_ptr<Button> menu_button;
};

extern Menu g_menu;
extern Hud g_hud;
extern QuitDialog g_quit_dialog;
extern GameOverScreen g_game_over;
extern Font g_font;

// Loads the font, the sprites and the GUI theme, builds the screens. The
// action buttons are built from the actions of the game, so the actions have
// to be loaded first.
void LoadUi();

// The plane, the pyramids and the snow; skipped by any key or click.
void PlayIntro();

// Draws the maze, the items and the creatures; `anim_part` in [0, 1] is how
// far the creatures have walked from prev_pos to pos.
void DrawWorld(double anim_part);

// Draws the stats line and the seed over the world.
void DrawStats(double fps);

// The cell under a backbuffer position, or (-1, -1) outside the maze.
Vec2Si32 CellAt(Vec2Si32 backbuffer_pos);

// Copies the state of the hero into the bars and the buttons; call every frame
// while the game is on screen.
void UpdateHud();

// Fills the game over screen with the outcome.
void ShowGameOver();

}  // namespace pyramids

#endif  // ANTARCTICA_PYRAMIDS_UI_H_
