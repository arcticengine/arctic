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

// Antarctica Pyramids: a small roguelike under the ice, and a tour of the
// Arctic Engine. Each subsystem is used where a roguelike needs it:
//
//   HwSprite, DrawRectangleHw    the maze, the creatures, their health bars
//   Sprite, Font                 the intro snow and the text, in the software
//                                backbuffer that is composed over the sprites
//   GuiFactory, Panel::IsInside  the menu, the panel of actions, the dialogs,
//                                and the line between a click on the interface
//                                and a click on the world
//   Sound                        the music from an ogg and effects synthesized
//                                in sfx.cpp
//   CsvTable                     data/actions.csv, the hero's moves
//   IniFile                      settings.ini, music and volume
//   SetRandomSeed                --seed N replays the same mazes
//   Log                          log.txt, where every level and outcome goes
//   SetMainWindowCloseHandler    the "Quit?" dialog on the window's close box
//   ARCTIC_STARTUP_MODE_DECIDER  --selftest plays 300 random turns in a
//                                hidden window and is registered with ctest
//
// The 3D renderer, the physics, the sockets and HTTP have nothing to do in a
// maze of tiles and are not here; see hover_racer for the first two and
// headless_server for the sockets.
//
// The keys: arrows or WASD walk, 0-3 are the actions of the panel, Space rests,
// Escape opens the menu, F12 saves screenshot.png. The mouse does the same:
// a click on a neighbouring cell walks, a click on a monster attacks it with
// the first action that reaches it, a click on the hero rests.

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include "engine/easy.h"
#include "engine/ini.h"
#include "engine/log.h"
#include "game.h"
#include "sfx.h"
#include "ui.h"

using namespace arctic;  // NOLINT
using namespace pyramids;  // NOLINT

namespace {

enum State {
  kStateMenu = 0,
  kStatePlaying,
  kStateGameOver
};

const char *kSettingsFileName = "settings.ini";
const char *kActionsFileName = "data/actions.csv";
const char *kMusicFileName = "data/snowflake_-_Living_Nightmare.ogg";
const Si32 kSelftestTurns = 300;

State g_state = kStateMenu;
bool g_has_game = false;  // a game is on, the menu offers Continue
bool g_is_quit_requested = false;
bool g_is_selftest = false;
bool g_is_seed_set = false;
Ui64 g_seed = 0;

// The creatures walk from prev_pos to pos over this many seconds after a
// turn; input waits for the walk to end. The self-test sets it to zero.
double g_anim_duration = 0.15;
double g_anim_start = 0.0;
double g_last_step_time = 0.0;

// Command line

// Xcode appends `-NSDocumentRevisionsDebugMode YES` to a Debug run and Finder
// used to append `-psn_0_...`; neither is an argument of this program.
bool IsLauncherArgument(const char *arg) {
  return std::strcmp(arg, "-NSDocumentRevisionsDebugMode") == 0
      || std::strcmp(arg, "YES") == 0
      || std::strncmp(arg, "-psn_", 5) == 0;
}

bool HasArgument(const char *name) {
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

void ParseArguments() {
  const Engine *engine = GetEngine();
  const Si32 argc = engine->GetArgc();
  const char *const *argv = engine->GetArgv();
  for (Si32 i = 1; i < argc; ++i) {
    if (argv[i] == nullptr || IsLauncherArgument(argv[i])) {
      continue;
    }
    if (std::strcmp(argv[i], "--selftest") == 0) {
      g_is_selftest = true;
    } else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      g_seed = std::strtoull(argv[i + 1], nullptr, 10);
      g_is_seed_set = true;
      ++i;
    } else {
      *Log() << "Unknown argument \"" << argv[i]
          << "\". Usage: antarctica_pyramids [--seed N] [--selftest]";
    }
  }
}

// The window is decided before it exists, see ARCTIC_STARTUP_MODE_DECIDER in
// engine/arctic_platform.h. The self-test needs the GL context for the
// sprites but nobody to look at it, so it asks for a hidden window; the
// environment (ARCTIC_HEADLESS) still wins over this.
StartupMode DecideStartupMode() {
  return HasArgument("--selftest") ? StartupMode::kHiddenWindow
      : StartupMode::kWindowed;
}

// A seed for a game nobody asked for a particular one of.
Ui64 FreshSeed() {
  if (g_is_seed_set) {
    return g_seed;
  }
  return static_cast<Ui64>(std::time(nullptr)) ^ (Random64() & 0xffff);
}

// Settings

void LoadSettings() {
  IniFile ini;
  bool is_music = true;
  Si32 volume = 80;
  if (ini.LoadFile(kSettingsFileName)) {
    is_music = ini.GetBool("audio", "music", true);
    volume = Clamp(ini.GetInt("audio", "volume", 80), 0, 100);
  }
  SetMusicEnabled(is_music);
  SetMasterVolume(static_cast<float>(volume) / 100.0f);
  g_menu.music_checkbox->SetChecked(is_music);
  g_menu.volume_scrollbar->SetValue(volume);
}

void SaveSettings() {
  IniFile ini;
  ini.LoadFile(kSettingsFileName);  // keeps whatever else is in there
  IniSection *audio = ini.GetSection("audio");
  if (audio == nullptr) {
    audio = ini.AddSection("audio");
  }
  if (audio == nullptr) {
    return;
  }
  audio->SetValue("music", IsMusicEnabled() ? "true" : "false");
  audio->SetValue("volume", std::to_string(g_menu.volume_scrollbar->GetValue()));
  if (!ini.SaveFile(kSettingsFileName)) {
    *Log() << "Can't save " << kSettingsFileName;
  }
}

// States

void EnterMenu() {
  g_state = kStateMenu;
  g_menu.continue_button->SetVisible(g_has_game);
  g_menu.root->SetVisible(true);
  g_hud.root->SetVisible(false);
  g_game_over.root->SetVisible(false);
}

void LeaveMenu() {
  SaveSettings();
  g_menu.root->SetVisible(false);
}

void StartGame() {
  NewGame(FreshSeed());
  g_has_game = true;
  g_state = kStatePlaying;
  g_anim_start = 0.0;
  LeaveMenu();
  g_game_over.root->SetVisible(false);
  g_hud.root->SetVisible(true);
}

void ContinueGame() {
  g_state = kStatePlaying;
  LeaveMenu();
  g_hud.root->SetVisible(true);
}

void RequestQuit() {
  g_quit_dialog.root->SetVisible(true);
}

void ConfirmQuit() {
  g_is_quit_requested = true;
}

void CancelQuit() {
  g_quit_dialog.root->SetVisible(false);
}

void GameOverMenu() {
  g_has_game = false;
  EnterMenu();
}

// The close box of the window asks first. Returning false keeps the window
// open; the flag is atomic because on Windows the handler runs on the window's
// thread, and it is a flag of its own rather than IsMainWindowCloseRequested()
// because that one stays true after Cancel, while a second click on the box
// must open the dialog a second time.
std::atomic<bool> g_is_close_pending{false};

bool OnMainWindowClose() {
  g_is_close_pending.store(true);
  return false;
}

// Input

double AnimPart() {
  if (g_anim_duration <= 0.0) {
    return 1.0;
  }
  return Clamp((Time() - g_anim_start) / g_anim_duration, 0.0, 1.0);
}

void OnTurn() {
  g_anim_start = Time();
  g_last_step_time = Time();
}

// A press walks one cell; a key held down keeps walking.
bool WantsStep(KeyCode arrow, const char *letter) {
  if (IsKeyDownward(arrow) || IsKeyDownward(letter)) {
    return true;
  }
  const double held = std::max(KeyDownSeconds(arrow), KeyDownSeconds(letter));
  return held > 0.3 && Time() - g_last_step_time > 0.12;
}

void ProcessGameKeys() {
  if (AnimPart() < 1.0) {
    return;
  }
  Vec2Si32 step(0, 0);
  if (WantsStep(kKeyUp, "w")) {
    step = Vec2Si32(0, 1);
  } else if (WantsStep(kKeyDown, "s")) {
    step = Vec2Si32(0, -1);
  } else if (WantsStep(kKeyLeft, "a")) {
    step = Vec2Si32(-1, 0);
  } else if (WantsStep(kKeyRight, "d")) {
    step = Vec2Si32(1, 0);
  }
  if (step != Vec2Si32(0, 0)) {
    if (HeroStep(step)) {
      OnTurn();
    }
    return;
  }
  for (Si32 idx = 0; idx < static_cast<Si32>(g_game.actions.size())
      && idx < 10; ++idx) {
    if (IsKeyDownward(static_cast<KeyCode>(kKey0 + idx))) {
      if (HeroAct(idx)) {
        OnTurn();
      }
      return;
    }
  }
  if (IsKeyDownward(kKeySpace)) {
    if (HeroRest()) {
      OnTurn();
    }
    return;
  }
  // Cheats
  if (IsKeyDownward("v")) {
    RevealMaze();
  }
  if (IsKeyDownward("n")) {
    RegenerateLevel();
  }
}

// The first action that can hit the monster from where the hero stands.
Si32 ActionReaching(const Creature &monster) {
  for (size_t idx = 0; idx < g_game.actions.size(); ++idx) {
    const Action &action = g_game.actions[idx];
    if (IsActionPossible(Hero(), action)
        && CanSee(Hero().pos, monster.pos, action.damage_distance)) {
      return static_cast<Si32>(idx);
    }
  }
  return -1;
}

void OnWorldClick(Vec2Si32 backbuffer_pos) {
  if (AnimPart() < 1.0) {
    return;
  }
  Vec2Si32 cell = CellAt(backbuffer_pos);
  if (cell == Vec2Si32(-1, -1) || !Maze(cell).is_known) {
    return;
  }
  bool is_turn = false;
  Vec2Si32 delta = cell - Hero().pos;
  Creature *creature = CreatureAt(cell);
  if (creature == &Hero()) {
    is_turn = HeroRest();
  } else if (creature != nullptr && Maze(cell).is_visible) {
    is_turn = HeroAct(ActionReaching(*creature));
  } else if (std::abs(delta.x) + std::abs(delta.y) == 1) {
    is_turn = HeroStep(delta);
  }
  if (is_turn) {
    OnTurn();
  }
}

void OnActionButton(Si32 idx) {
  if (AnimPart() < 1.0) {
    return;
  }
  if (HeroAct(idx)) {
    OnTurn();
  }
}

// Buttons take a function of no arguments, and there is one per action, so the
// four are spelled out rather than bound.
void OnAction0() { OnActionButton(0); }
void OnAction1() { OnActionButton(1); }
void OnAction2() { OnActionButton(2); }
void OnAction3() { OnActionButton(3); }

void OnMusicCheckbox() {
  SetMusicEnabled(g_menu.music_checkbox->IsChecked());
}

void OnVolumeScrollbar() {
  SetMasterVolume(static_cast<float>(g_menu.volume_scrollbar->GetValue())
      / 100.0f);
}

void WireGui() {
  g_menu.new_game_button->OnButtonClick = StartGame;
  g_menu.continue_button->OnButtonClick = ContinueGame;
  g_menu.quit_button->OnButtonClick = RequestQuit;
  g_menu.music_checkbox->OnButtonClick = OnMusicCheckbox;
  g_menu.volume_scrollbar->OnScrollChange = OnVolumeScrollbar;
  g_quit_dialog.quit_button->OnButtonClick = ConfirmQuit;
  g_quit_dialog.cancel_button->OnButtonClick = CancelQuit;
  g_game_over.again_button->OnButtonClick = StartGame;
  g_game_over.menu_button->OnButtonClick = GameOverMenu;
  void (*handlers[4])() = {OnAction0, OnAction1, OnAction2, OnAction3};
  for (size_t idx = 0; idx < g_hud.action_buttons.size() && idx < 4; ++idx) {
    g_hud.action_buttons[idx]->OnButtonClick = handlers[idx];
  }
}

// Every input message goes to the interface that is on screen. In the game,
// a mouse click that the panel does not claim is a click on the world; this
// is the one place where the two are told apart, with Panel::IsInside.
void ProcessInput() {
  const bool is_dialog = g_quit_dialog.root->IsVisible();
  for (Si32 i = 0; i < InputMessageCount(); ++i) {
    const InputMessage &message = GetInputMessage(i);
    if (is_dialog) {
      g_quit_dialog.root->ApplyInput(message, nullptr);
      continue;
    }
    switch (g_state) {
      case kStateMenu:
        g_menu.root->ApplyInput(message, nullptr);
        break;
      case kStateGameOver:
        g_game_over.root->ApplyInput(message, nullptr);
        break;
      case kStatePlaying:
        g_hud.root->ApplyInput(message, nullptr);
        if (message.kind == InputMessage::kMouse
            && message.keyboard.key == kKeyMouseLeft
            && message.keyboard.key_state == 1
            && !g_hud.root->IsInside(message.mouse.backbuffer_pos)) {
          OnWorldClick(message.mouse.backbuffer_pos);
        }
        break;
    }
  }

  if (g_is_close_pending.exchange(false) && !is_dialog) {
    RequestQuit();
  }
  if (IsKeyDownward(kKeyF12)) {
    // Screenshot reads the frame before it is shown and hands it over as a
    // software sprite, which knows how to save itself as png.
    Sprite frame = Screenshot();
    if (frame.Width() > 0) {
      frame.Save("screenshot.png");
    }
  }
  if (is_dialog) {
    if (IsKeyDownward(kKeyEscape)) {
      CancelQuit();
    }
    return;
  }
  switch (g_state) {
    case kStateMenu:
      if (IsKeyDownward(kKeyEscape)) {
        if (g_has_game) {
          ContinueGame();
        } else {
          RequestQuit();
        }
      }
      break;
    case kStatePlaying:
      if (IsKeyDownward(kKeyEscape)) {
        EnterMenu();
      } else {
        ProcessGameKeys();
      }
      break;
    case kStateGameOver:
      if (IsKeyDownward(kKeyEnter) || IsKeyDownward(kKeySpace)) {
        StartGame();
      }
      break;
  }
}

void PlayEvents() {
  std::vector<GameEvent> events = TakeEvents();
  for (size_t idx = 0; idx < events.size(); ++idx) {
    switch (events[idx]) {
      case kEventStep:
        PlaySfx(kSfxStep);
        break;
      case kEventShot:
        PlaySfx(kSfxShot);
        break;
      case kEventKick:
        PlaySfx(kSfxKick);
        break;
      case kEventHit:
        PlaySfx(kSfxHit);
        break;
      case kEventKill:
        PlaySfx(kSfxKill);
        break;
      case kEventPickup:
        PlaySfx(kSfxPickup);
        break;
      case kEventStairs:
        PlaySfx(kSfxStairs);
        break;
      case kEventRest:
        PlaySfx(kSfxRest);
        break;
    }
  }
}

void Update() {
  UpdateMusic();
  PlayEvents();
  if (g_state == kStatePlaying && g_game.outcome != kOutcomePlaying
      && AnimPart() >= 1.0) {
    g_state = kStateGameOver;
    g_has_game = false;
    ShowGameOver();
  }
}

double SmoothFps() {
  static double prev_time = Time();
  static double smooth_fps = 0.0;
  double time = Time();
  double fps = 1.0 / std::max(time - prev_time, 0.001);
  smooth_fps = smooth_fps * 0.95 + 0.05 * fps;
  prev_time = time;
  return smooth_fps;
}

void Render() {
  Clear();
  if (g_has_game || g_state == kStateGameOver) {
    DrawWorld(AnimPart());
    DrawStats(SmoothFps());
    UpdateHud();
    g_hud.root->Draw(Vec2Si32(0, 0));
  } else {
    g_font.Draw("Antarctica Pyramids", kScreenWidth / 2, kScreenHeight - 40,
        kTextOriginTop, kTextAlignmentCenter);
  }
  g_menu.root->Draw(Vec2Si32(0, 0));
  g_game_over.root->Draw(Vec2Si32(0, 0));
  g_quit_dialog.root->Draw(Vec2Si32(0, 0));
  ShowFrame();
}

void Frame() {
  ProcessInput();
  Update();
  Render();
}

// Self-test

void SelftestCheck(bool is_ok, const char *what) {
  if (is_ok) {
    return;
  }
  *Log() << "selftest: " << what << " on turn " << g_game.turns;
  ExitProgram(1);
}

void SelftestInvariants() {
  const Creature &hero = Hero();
  SelftestCheck(IsInsideMaze(hero.pos), "hero left the maze");
  SelftestCheck(Maze(hero.pos).kind != kCellWall, "hero stands in a wall");
  SelftestCheck(hero.hitpoints >= 0 && hero.hitpoints <= hero.full_hitpoints,
      "hitpoints out of range");
  SelftestCheck(hero.warmth >= 0 && hero.warmth <= hero.full_warmth,
      "warmth out of range");
  SelftestCheck(hero.endurance >= 0 && hero.endurance <= hero.full_endurance,
      "endurance out of range");
  SelftestCheck(hero.ammo >= 0, "negative ammo");
  for (size_t a = 0; a < g_game.creatures.size(); ++a) {
    const Creature &creature = g_game.creatures[a];
    SelftestCheck(IsInsideMaze(creature.pos), "a creature left the maze");
    SelftestCheck(Maze(creature.pos).kind != kCellWall,
        "a creature stands in a wall");
    for (size_t b = a + 1; b < g_game.creatures.size(); ++b) {
      SelftestCheck(creature.pos != g_game.creatures[b].pos,
          "two creatures in one cell");
    }
  }
}

// Where a purposeful player would head: the nearest monster in sight, or the
// way down when none is.
Vec2Si32 SelftestGoal() {
  const Vec2Si32 hero_pos = Hero().pos;
  Vec2Si32 goal = hero_pos;
  Si32 best = 1 << 30;
  for (size_t idx = 1; idx < g_game.creatures.size(); ++idx) {
    const Vec2Si32 pos = g_game.creatures[idx].pos;
    const Si32 distance = std::abs(pos.x - hero_pos.x)
        + std::abs(pos.y - hero_pos.y);
    if (Maze(pos).is_visible && distance < best) {
      best = distance;
      goal = pos;
    }
  }
  if (goal != hero_pos) {
    return goal;
  }
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
    for (pos.y = 0; pos.y < kMazeHeight; ++pos.y) {
      const CellKind kind = Maze(pos).kind;
      if (kind == kCellStairsDownLeft || kind == kCellStairsDownRight) {
        return pos;
      }
    }
  }
  return hero_pos;
}

// A random key most of the time, so that every input path gets hit, but a
// monster in reach is attacked half of the time and a quarter of the steps
// go toward the goal, so that fights, kills, respawns, the stairs and the
// two ends of a game all get their share of a 300-turn run.
KeyCode SelftestKey() {
  static const KeyCode kKeys[] = {
    kKeyUp, kKeyDown, kKeyLeft, kKeyRight,
    kKey0, kKey1, kKey2, kKey3, kKeySpace
  };
  static const KeyCode kStepKeys[] = {kKeyUp, kKeyDown, kKeyLeft, kKeyRight};
  static const Vec2Si32 kSteps[] = {
    Vec2Si32(0, 1), Vec2Si32(0, -1), Vec2Si32(-1, 0), Vec2Si32(1, 0)
  };
  const Si32 key_count = static_cast<Si32>(sizeof(kKeys) / sizeof(kKeys[0]));
  const Si32 roll = Random32(0, 3);
  if (roll < 2) {
    for (size_t idx = 0; idx < g_game.actions.size() && idx < 10; ++idx) {
      const Action &action = g_game.actions[idx];
      if (IsActionPossible(Hero(), action) && FindTarget(action) != nullptr) {
        return static_cast<KeyCode>(kKey0 + idx);
      }
    }
  }
  if (roll == 2) {
    const Vec2Si32 hero_pos = Hero().pos;
    Vec2Si32 next;
    if (FirstStepTowards(hero_pos, SelftestGoal(), kMazeWidth * kMazeHeight,
        &next)) {
      for (Si32 dir = 0; dir < 4; ++dir) {
        if (hero_pos + kSteps[dir] == next) {
          return kStepKeys[dir];
        }
      }
    }
  }
  return kKeys[Random32(0, key_count - 1)];
}

// Plays kSelftestTurns random turns through the very same input path a
// player uses: a key is set as pressed, the frame runs, the key is released.
// A game that ends is followed by the next one. The process exits with 0 if
// nothing broke, with 1 on the first invariant that did not hold.
void RunSelftest() {
  g_anim_duration = 0.0;
  StartGame();
  Si32 turns_played = 0;
  Si32 games_played = 1;
  Si32 kills = 0;
  Si32 deepest_level = 1;
  for (Si32 iteration = 0; iteration < kSelftestTurns * 20
      && turns_played < kSelftestTurns; ++iteration) {
    const Si32 turns_before = g_game.turns;
    const KeyCode key = SelftestKey();
    SetKey(key, true);
    Frame();
    SetKey(key, false);
    ClearKeyStateTransitions();
    SelftestInvariants();
    if (g_game.turns > turns_before) {
      turns_played++;
    }
    kills = std::max(kills, g_game.kills);
    deepest_level = std::max(deepest_level, g_game.level);
    if (g_state == kStateGameOver) {
      // The keys come from the same generator as the maze, so the next game
      // on the same seed would be the same game; it gets the next seed.
      g_seed++;
      StartGame();
      games_played++;
    }
  }
  SelftestCheck(turns_played >= kSelftestTurns,
      "the random keys did not produce enough turns");
  *Log() << "selftest: " << turns_played << " turns in " << games_played
      << " games, most kills " << kills << ", deepest level " << deepest_level
      << ", hero ends at " << Hero().pos.x << "," << Hero().pos.y
      << " with " << Hero().hitpoints << " hp";
  ExitProgram(0);
}

}  // namespace

ARCTIC_STARTUP_MODE_DECIDER(DecideStartupMode)

void EasyMain() {
  SetWindowTitle("Antarctica Pyramids");
  SetVSync(false);
  SetLogSizeLimit(4 * 1024 * 1024);
  ParseArguments();
  *Log() << "Antarctica Pyramids starts, log at " << LogFilePath();

  if (!LoadActions(kActionsFileName)) {
    Fatal("Can't load the actions, see log.txt");
    return;
  }
  LoadUi();
  InitSfx();
  LoadMusic(kMusicFileName);
  LoadSettings();
  WireGui();
  SetMainWindowCloseHandler(OnMainWindowClose);

  if (g_is_selftest) {
    ResizeScreen(kScreenWidth, kScreenHeight);
    RunSelftest();
    return;
  }

  PlayIntro();
  EnterMenu();
  while (!g_is_quit_requested) {
    Frame();
  }
  SaveSettings();
}
