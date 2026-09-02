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

#include "ui.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include "game.h"

namespace pyramids {

Menu g_menu;
Hud g_hud;
QuitDialog g_quit_dialog;
GameOverScreen g_game_over;
Font g_font;

namespace {

// The world is drawn with HwSprite: every tile, item and creature goes
// straight to the GPU, and the health bars are DrawRectangleHw so that a
// rectangle does not need a sprite of its own. The text and the GUI are drawn
// into the software backbuffer with Font and Panel::Draw. The engine composes
// the two in a fixed order: the hardware sprites first, in the order they were
// drawn, then the whole software backbuffer on top of them, so the text and
// the panels are always above the maze whatever the order of the calls.
HwSprite g_blood[kDecalCount];
HwSprite g_floor;
HwSprite g_floor_dark;
HwSprite g_hero[kCreatureHeroEnd - kCreatureHeroBegin];
HwSprite g_intro_airplane;
HwSprite g_intro_pyramids;
HwSprite g_monster[kCreatureMonsterEnd - kCreatureMonsterBegin];
HwSprite g_stairs_down_left;
HwSprite g_stairs_down_left_dark;
HwSprite g_stairs_down_right;
HwSprite g_stairs_down_right_dark;
HwSprite g_stairs_up_left;
HwSprite g_stairs_up_left_dark;
HwSprite g_stairs_up_right;
HwSprite g_stairs_up_right_dark;
HwSprite g_stick;
HwSprite g_stone;
HwSprite g_wall;
HwSprite g_wall_dark;
HwSprite g_empty;

std::shared_ptr<GuiTheme> g_theme;

// Walls are drawn a second time over the row below, so a wall hides what
// stands behind it.
bool IsHigh(CellKind kind) {
  return kind == kCellWall;
}

HwSprite& CellSprite(CellKind kind, bool is_visible) {
  switch (kind) {
    case kCellFloor:
      return is_visible ? g_floor : g_floor_dark;
    case kCellStairsDownLeft:
      return is_visible ? g_stairs_down_left : g_stairs_down_left_dark;
    case kCellStairsDownRight:
      return is_visible ? g_stairs_down_right : g_stairs_down_right_dark;
    case kCellStairsUpLeft:
      return is_visible ? g_stairs_up_left : g_stairs_up_left_dark;
    case kCellStairsUpRight:
      return is_visible ? g_stairs_up_right : g_stairs_up_right_dark;
    case kCellWall:
      return is_visible ? g_wall : g_wall_dark;
    default:
      Fatal("Unknown cell kind in CellSprite");
      return g_empty;
  }
}

HwSprite& ItemSprite(ItemKind kind) {
  switch (kind) {
    case kItemStone:
      return g_stone;
    case kItemStick:
      return g_stick;
    default:
      return g_empty;
  }
}

HwSprite& CreatureSprite(CreatureKind kind) {
  if (kind >= kCreatureMonsterBegin && kind < kCreatureMonsterEnd) {
    return g_monster[kind - kCreatureMonsterBegin];
  }
  if (kind >= kCreatureHeroBegin && kind < kCreatureHeroEnd) {
    return g_hero[kind - kCreatureHeroBegin];
  }
  Fatal("Unknown creature kind in CreatureSprite.");
  return g_empty;
}

Vec2Si32 CellToScreen(Vec2Si32 cell) {
  return Vec2Si32(cell.x * kTileSize, cell.y * kTileSize + kHudHeight);
}

void LoadSprites() {
  const char *blood_names[kDecalCount] = {
    "data/blood_0.tga", "data/blood_1.tga", "data/blood_2.tga",
    "data/blood_3.tga", "data/blood_4.tga", "data/blood_5.tga",
    "data/blood_6.tga"
  };
  for (Si32 idx = 0; idx < kDecalCount; ++idx) {
    g_blood[idx].Load(blood_names[idx]);
  }
  g_floor.Load("data/floor_1.tga");
  g_floor_dark.Load("data/floor_1_dark.tga");
  g_hero[0].Load("data/hero_1.tga");
  g_hero[1].Load("data/hero_2.tga");
  g_intro_airplane.Load("data/intro_airplane_1.tga");
  g_intro_pyramids.Load("data/intro_pyramids_1.tga");
  g_monster[0].Load("data/monster_0.tga");
  g_monster[1].Load("data/monster_1.tga");
  g_monster[2].Load("data/monster_2.tga");
  g_stairs_down_left.Load("data/stairs_down_left_1.tga");
  g_stairs_down_left_dark.Load("data/stairs_down_left_1_dark.tga");
  g_stairs_down_right.Load("data/stairs_down_right_1.tga");
  g_stairs_down_right_dark.Load("data/stairs_down_right_1_dark.tga");
  g_stairs_up_left.Load("data/stairs_up_left_1.tga");
  g_stairs_up_left_dark.Load("data/stairs_up_left_1_dark.tga");
  g_stairs_up_right.Load("data/stairs_up_right_1.tga");
  g_stairs_up_right_dark.Load("data/stairs_up_right_1_dark.tga");
  g_stick.Load("data/stick_1.tga");
  g_stick.SetPivot(Vec2Si32(12, 12));
  g_stone.Load("data/stone_1.tga");
  g_stone.SetPivot(Vec2Si32(12, 12));
  g_wall.Load("data/wall_1.tga");
  g_wall_dark.Load("data/wall_1_dark.tga");
}

std::string ActionTooltip(const Action &action) {
  char text[256];
  Si32 length = snprintf(text, sizeof(text), "%s\n%d damage at range %d",
      action.name.c_str(), action.damage_hitpoints, action.damage_distance);
  if (action.cost_ammo > 0) {
    length += snprintf(text + length, sizeof(text) - length,
        "\n-%d ammo", action.cost_ammo);
  }
  if (action.cost_endurance > 0) {
    length += snprintf(text + length, sizeof(text) - length,
        "\n-%d endurance", action.cost_endurance);
  }
  if (action.produce_warmth > 0) {
    length += snprintf(text + length, sizeof(text) - length,
        "\n+%d warmth", action.produce_warmth);
  }
  return std::string(text);
}

std::shared_ptr<Panel> MakeCenteredPanel(GuiFactory *gf, Vec2Si32 size) {
  std::shared_ptr<Panel> panel = gf->MakePanel();
  panel->SetSize(size);
  panel->SetPos((Vec2Si32(kScreenWidth, kScreenHeight) - size) / 2);
  return panel;
}

void BuildMenu(GuiFactory *gf) {
  const Vec2Si32 size(360, 400);
  g_menu.root = MakeCenteredPanel(gf, size);
  const Si32 width = 300;
  const Si32 x = (size.x - width) / 2;

  g_menu.continue_button = gf->MakeButton();
  g_menu.continue_button->SetText("Continue");
  g_menu.continue_button->SetWidth(width);
  g_menu.continue_button->SetPos(Vec2Si32(x, 320));
  g_menu.root->AddChild(g_menu.continue_button);

  g_menu.new_game_button = gf->MakeButton();
  g_menu.new_game_button->SetText("New game");
  g_menu.new_game_button->SetWidth(width);
  g_menu.new_game_button->SetPos(Vec2Si32(x, 250));
  g_menu.root->AddChild(g_menu.new_game_button);

  g_menu.quit_button = gf->MakeButton();
  g_menu.quit_button->SetText("Quit");
  g_menu.quit_button->SetWidth(width);
  g_menu.quit_button->SetPos(Vec2Si32(x, 180));
  g_menu.root->AddChild(g_menu.quit_button);

  g_menu.music_checkbox = gf->MakeCheckbox();
  g_menu.music_checkbox->SetText(" Music");
  g_menu.music_checkbox->SetPos(Vec2Si32(x, 100));
  g_menu.root->AddChild(g_menu.music_checkbox);

  g_menu.volume_scrollbar = gf->MakeHorizontalScrollbar();
  g_menu.volume_scrollbar->SetWidth(width);
  g_menu.volume_scrollbar->SetMinValue(0);
  g_menu.volume_scrollbar->SetMaxValue(100);
  g_menu.volume_scrollbar->SetStep(5);
  g_menu.volume_scrollbar->SetPos(Vec2Si32(x, 40));
  g_menu.volume_scrollbar->SetTooltip("Volume");
  g_menu.root->AddChild(g_menu.volume_scrollbar);

  g_menu.root->SetVisible(false);
}

void BuildHud(GuiFactory *gf) {
  g_hud.root = gf->MakeTransparentPanel();
  g_hud.root->SetPos(Vec2Si32(0, 0));
  g_hud.root->SetSize(Vec2Si32(kScreenWidth, kHudHeight));

  // One button per action of data/actions.csv, in a row along the top of the
  // panel; the keys 0, 1, 2, 3 are the same buttons.
  const Si32 count = static_cast<Si32>(g_game.actions.size());
  const Si32 button_width = kScreenWidth / std::max(count, 1);
  for (Si32 idx = 0; idx < count; ++idx) {
    const Action &action = g_game.actions[idx];
    std::shared_ptr<Button> button = gf->MakeButton();
    char label[128];
    snprintf(label, sizeof(label), "%d %s", idx, action.name.c_str());
    button->SetText(label);
    button->SetWidth(button_width);
    button->SetPos(Vec2Si32(idx * button_width, kHudHeight - 56));
    button->SetTooltip(ActionTooltip(action));
    g_hud.root->AddChild(button);
    g_hud.action_buttons.push_back(button);
  }

  // The bars along the bottom: what keeps the hero alive.
  const Si32 bar_width = kScreenWidth / 3;
  g_hud.hitpoints_bar = gf->MakeProgressbar();
  g_hud.hitpoints_bar->SetWidth(bar_width);
  g_hud.hitpoints_bar->SetPos(Vec2Si32(0, 4));
  g_hud.hitpoints_bar->SetTooltip("Hitpoints. Monsters bite.");
  g_hud.root->AddChild(g_hud.hitpoints_bar);

  g_hud.warmth_bar = gf->MakeProgressbar();
  g_hud.warmth_bar->SetWidth(bar_width);
  g_hud.warmth_bar->SetPos(Vec2Si32(bar_width, 4));
  g_hud.warmth_bar->SetTooltip(
      "Warmth. Every turn costs one; kicks and sticks give it back.");
  g_hud.root->AddChild(g_hud.warmth_bar);

  g_hud.endurance_bar = gf->MakeProgressbar();
  g_hud.endurance_bar->SetWidth(bar_width);
  g_hud.endurance_bar->SetPos(Vec2Si32(bar_width * 2, 4));
  g_hud.endurance_bar->SetTooltip(
      "Endurance. Kicks spend it, resting (Space) brings it back.");
  g_hud.root->AddChild(g_hud.endurance_bar);

  g_hud.root->SetVisible(false);
}

void BuildQuitDialog(GuiFactory *gf) {
  const Vec2Si32 size(400, 190);
  g_quit_dialog.root = MakeCenteredPanel(gf, size);

  std::shared_ptr<Text> text = gf->MakeText();
  text->SetText("Quit the game?");
  text->SetPos(Vec2Si32(20, 110));
  text->SetSize(Vec2Si32(size.x - 40, 60));
  text->SetOrigin(kTextOriginCenter);
  text->SetAlignment(kTextAlignmentCenter);
  g_quit_dialog.root->AddChild(text);

  g_quit_dialog.quit_button = gf->MakeButton();
  g_quit_dialog.quit_button->SetText("Quit");
  g_quit_dialog.quit_button->SetWidth(170);
  g_quit_dialog.quit_button->SetPos(Vec2Si32(20, 30));
  g_quit_dialog.root->AddChild(g_quit_dialog.quit_button);

  g_quit_dialog.cancel_button = gf->MakeButton();
  g_quit_dialog.cancel_button->SetText("Cancel");
  g_quit_dialog.cancel_button->SetWidth(170);
  g_quit_dialog.cancel_button->SetPos(Vec2Si32(size.x - 190, 30));
  g_quit_dialog.root->AddChild(g_quit_dialog.cancel_button);

  g_quit_dialog.root->SetVisible(false);
}

void BuildGameOver(GuiFactory *gf) {
  const Vec2Si32 size(480, 300);
  g_game_over.root = MakeCenteredPanel(gf, size);

  g_game_over.text = gf->MakeText();
  g_game_over.text->SetText("");
  g_game_over.text->SetPos(Vec2Si32(20, 110));
  g_game_over.text->SetSize(Vec2Si32(size.x - 40, 170));
  g_game_over.text->SetOrigin(kTextOriginCenter);
  g_game_over.text->SetAlignment(kTextAlignmentCenter);
  g_game_over.root->AddChild(g_game_over.text);

  g_game_over.again_button = gf->MakeButton();
  g_game_over.again_button->SetText("Again");
  g_game_over.again_button->SetWidth(200);
  g_game_over.again_button->SetPos(Vec2Si32(30, 30));
  g_game_over.root->AddChild(g_game_over.again_button);

  g_game_over.menu_button = gf->MakeButton();
  g_game_over.menu_button->SetText("Menu");
  g_game_over.menu_button->SetWidth(200);
  g_game_over.menu_button->SetPos(Vec2Si32(size.x - 230, 30));
  g_game_over.root->AddChild(g_game_over.menu_button);

  g_game_over.root->SetVisible(false);
}

void DrawHealthBar(Vec2Si32 scr_pos, Si32 hitpoints, Si32 full_hitpoints) {
  const Si32 y = scr_pos.y + kTileSize + 1;
  DrawRectangleHw(Vec2Si32(scr_pos.x, y),
      Vec2Si32(scr_pos.x + kTileSize - 1, y + 2), Rgba(64, 0, 0, 255));
  Si32 width = kTileSize * hitpoints / std::max(1, full_hitpoints);
  if (width > 0) {
    DrawRectangleHw(Vec2Si32(scr_pos.x, y),
        Vec2Si32(scr_pos.x + width - 1, y + 2), Rgba(0, 200, 0, 255));
  }
}

}  // namespace

void LoadUi() {
  g_font.Load("data/arctic_one_bmf.fnt");
  // The stats line is drawn over the tiles, and a dark border keeps it
  // readable on the light floor as well as on the dark walls.
  g_font.AddBorder(1.0f, Rgba(0, 0, 0, 255));

  LoadSprites();

  g_theme = std::make_shared<GuiTheme>();
  g_theme->Load("data/gui_theme.xml");
  GuiFactory gf;
  gf.theme_ = g_theme;
  BuildMenu(&gf);
  BuildHud(&gf);
  BuildQuitDialog(&gf);
  BuildGameOver(&gf);
}

void PlayIntro() {
  // The intro is drawn at 320x200 in the software backbuffer, pixel by pixel:
  // the snow is a cellular automaton over the pixels themselves. The two
  // hardware sprites (the pyramids and the plane) end up below it, which is
  // the composition order described at the top of this file.
  ResizeScreen(320, 200);
  const Si32 width = 320;
  const Si32 height = 200;

  std::vector<Ui8> snow[2];
  snow[0].resize(width * height);
  snow[1].resize(width * height);
  for (Si32 i = 0; i < width * height; ++i) {
    snow[0][i] = (Random32(0, 15) == 0)
        ? static_cast<Ui8>(Random32(0, 255)) : 0;
  }
  Ui8 *cur_snow = snow[0].data();
  Ui8 *next_snow = snow[1].data();

  Vec2Si32 pyramids_pos(0, 10);
  Vec2Si32 airplane_pos_begin(width, height - g_intro_airplane.Height() / 2);
  Vec2Si32 airplane_pos_end(-g_intro_airplane.Width(), 0);
  const Si32 duration1 = 380;
  const Si32 duration2 = 500;
  const Si32 duration3 = 560;
  double start_time = Time();
  while (true) {
    Si32 frame = static_cast<Si32>((Time() - start_time) * 60.0);
    Clear();
    if (IsAnyKeyDownward() || frame > duration3) {
      break;
    }
    if (frame > duration1 && frame < duration2) {
      pyramids_pos = Vec2Si32(Random32(-1, 1), 10 + Random32(-1, 1));
    }
    g_intro_pyramids.Draw(pyramids_pos);
    if (frame < duration1) {
      Vec2F airplane_pos = Vec2F(airplane_pos_begin)
          + Vec2F(airplane_pos_end - airplane_pos_begin)
          * static_cast<float>(frame) / static_cast<float>(duration1);
      g_intro_airplane.Draw(airplane_pos);
    }

    Rgba *back_buffer = GetEngine()->GetBackbuffer().RgbaData();
    memset(next_snow, 0, width * height);
    for (Si32 y = 10; y < 190; ++y) {
      for (Si32 x = 0; x < width; ++x) {
        Si32 z = cur_snow[x + y * width];
        if (z == 0) {
          continue;
        }
        Si32 next_x = x + 8 - z / 42;
        Si32 next_y = y - 1;
        if (next_x >= width) {
          next_x -= width;
        }
        if (next_y < 10) {
          next_y = 189;
        }
        if (next_snow[next_x + next_y * width] == 0
            || next_snow[next_x + next_y * width] > z) {
          next_snow[next_x + next_y * width] = static_cast<Ui8>(z);
        }
        if (Random32(0, 15) == 0) {
          Si32 z2 = Random32(0, 255);
          if (z2 > z) {
            next_snow[x + y * width] = static_cast<Ui8>(z2);
          }
        }
        back_buffer[x + y * width] = Rgba(255 - z / 8, 255 - z / 8, 255 - z / 8);
      }
    }
    std::swap(cur_snow, next_snow);
    ShowFrame();
  }
  ResizeScreen(kScreenWidth, kScreenHeight);
}

void DrawWorld(double anim_part) {
  Vec2Si32 pos;
  // The ground first, row by row from the top of the screen down.
  for (pos.y = kMazeHeight - 1; pos.y >= 0; --pos.y) {
    for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
      const Cell &cell = Maze(pos);
      if (cell.is_known) {
        CellSprite(cell.kind, cell.is_visible).Draw(CellToScreen(pos));
      }
    }
  }
  // Then, row by row again, the walls over what is behind them, the decals,
  // the items and the creatures of the row.
  for (pos.y = kMazeHeight - 1; pos.y >= 0; --pos.y) {
    for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
      const Cell &cell = Maze(pos);
      if (!cell.is_known) {
        continue;
      }
      Vec2Si32 scr_pos = CellToScreen(pos);
      if (IsHigh(cell.kind)) {
        CellSprite(cell.kind, cell.is_visible).Draw(scr_pos);
      }
      if (!cell.is_visible) {
        continue;
      }
      for (size_t idx = 0; idx < cell.decals.size(); ++idx) {
        g_blood[cell.decals[idx]].Draw(scr_pos);
      }
      for (size_t idx = 0; idx < cell.items.size(); ++idx) {
        HwSprite &sprite = ItemSprite(cell.items[idx].kind);
        sprite.Draw(Vec2F(scr_pos + sprite.Pivot()),
            static_cast<float>(Time()));
      }
    }
    for (size_t idx = 0; idx < g_game.creatures.size(); ++idx) {
      const Creature &creature = g_game.creatures[idx];
      if (creature.pos.y != pos.y || !Maze(creature.pos).is_visible) {
        continue;
      }
      Vec2Si32 from = CellToScreen(creature.prev_pos);
      Vec2Si32 to = CellToScreen(creature.pos);
      Vec2Si32 scr_pos = from
          + Vec2Si32(static_cast<Si32>((to.x - from.x) * anim_part),
                     static_cast<Si32>((to.y - from.y) * anim_part));
      CreatureSprite(creature.kind).Draw(scr_pos);
      DrawHealthBar(scr_pos, creature.hitpoints, creature.full_hitpoints);
    }
  }
}

void DrawStats(double fps) {
  const Creature &hero = Hero();
  char text[256];
  snprintf(text, sizeof(text), "Level %d   Kills %d   Turn %d   Ammo %d",
      g_game.level, g_game.kills, g_game.turns, hero.ammo);
  g_font.Draw(text, 4, kScreenHeight - 2, kTextOriginTop, kTextAlignmentLeft);
  snprintf(text, sizeof(text), "Seed %llu   FPS %.0f",
      static_cast<unsigned long long>(g_game.seed), fps);  // NOLINT
  g_font.Draw(text, kScreenWidth - 4, kScreenHeight - 2, kTextOriginTop,
      kTextAlignmentRight);
}

Vec2Si32 CellAt(Vec2Si32 backbuffer_pos) {
  Vec2Si32 cell((backbuffer_pos.x) / kTileSize,
      (backbuffer_pos.y - kHudHeight) / kTileSize);
  if (backbuffer_pos.y < kHudHeight || !IsInsideMaze(cell)) {
    return Vec2Si32(-1, -1);
  }
  return cell;
}

void UpdateHud() {
  const Creature &hero = Hero();
  g_hud.hitpoints_bar->SetTotalValue(static_cast<float>(hero.full_hitpoints));
  g_hud.hitpoints_bar->SetCurrentValue(static_cast<float>(hero.hitpoints));
  g_hud.warmth_bar->SetTotalValue(static_cast<float>(hero.full_warmth));
  g_hud.warmth_bar->SetCurrentValue(static_cast<float>(hero.warmth));
  g_hud.endurance_bar->SetTotalValue(static_cast<float>(hero.full_endurance));
  g_hud.endurance_bar->SetCurrentValue(static_cast<float>(hero.endurance));
  for (size_t idx = 0; idx < g_hud.action_buttons.size()
      && idx < g_game.actions.size(); ++idx) {
    g_hud.action_buttons[idx]->SetEnabled(
        IsActionPossible(hero, g_game.actions[idx]));
  }
}

void ShowGameOver() {
  char text[256];
  snprintf(text, sizeof(text), "%s\n\nLevel %d, %d kills, %d turns",
      g_game.outcome == kOutcomeFrozen ? "You froze to death." : "You were eaten.",
      g_game.level, g_game.kills, g_game.turns);
  g_game_over.text->SetText(text);
  g_game_over.root->SetVisible(true);
}

}  // namespace pyramids
