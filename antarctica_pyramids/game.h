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

#ifndef ANTARCTICA_PYRAMIDS_GAME_H_
#define ANTARCTICA_PYRAMIDS_GAME_H_

// The rules of the game: the maze, the creatures, the items and one turn.
// Nothing here draws or plays a sound; what happened during a turn comes out
// as GameEvents for main.cpp to turn into sound and pictures.

#include <string>
#include <vector>
#include "engine/easy.h"

namespace pyramids {

using namespace arctic;  // NOLINT

const Si32 kMazeWidth = 32;
const Si32 kMazeHeight = 20;

enum CellKind {
  kCellWall = 0,
  kCellFloor,
  kCellStairsDownLeft,
  kCellStairsDownRight,
  kCellStairsUpLeft,
  kCellStairsUpRight,
};

enum ItemKind {
  kItemNone = 0,
  kItemStone,  // A stone is ammo for the shots
  kItemStick,  // A stick burns at once and warms the hero up
  kItemKindCount
};

enum CreatureKind {
  kCreatureMonsterBegin = 0,
  kCreatureWinged = 0,
  kCreatureTall = 1,
  kCreatureFat = 2,
  kCreatureMonsterEnd = 3,
  kCreatureHeroBegin = 4,
  kCreatureMale = 4,
  kCreatureFemale = 5,
  kCreatureHeroEnd = 6,
  kCreatureCount = 6
};

enum DecalKind {
  kDecalBlood0 = 0,
  kDecalBlood1,
  kDecalBlood2,
  kDecalBlood3,
  kDecalBlood4,
  kDecalBlood5,
  kDecalBlood6,
  kDecalCount
};

// What the hero can do to a monster. The list comes from data/actions.csv, so
// a designer tunes it without a compiler.
struct Action {
  std::string name;
  Si32 cost_endurance = 0;
  Si32 cost_ammo = 0;
  Si32 produce_warmth = 0;
  Si32 damage_hitpoints = 0;
  Si32 damage_distance = 1;
};

struct Creature {
  CreatureKind kind = kCreatureMale;
  Vec2Si32 pos = Vec2Si32(1, 1);
  // Where the creature stood before the last turn, for the walking animation.
  Vec2Si32 prev_pos = Vec2Si32(1, 1);
  Si32 hitpoints = 100;
  Si32 full_hitpoints = 100;
  Si32 warmth = 1000;
  Si32 full_warmth = 1000;
  Si32 endurance = 100;
  Si32 full_endurance = 100;
  Si32 ammo = 6;
  Si32 damage = 10;  // What a monster does to the hero in one bite
  // A monster that has seen the hero keeps chasing them out of sight.
  bool is_hunting = false;
};

struct Item {
  ItemKind kind = kItemNone;
};

struct Cell {
  CellKind kind = kCellWall;
  bool is_known = false;
  bool is_visible = false;
  std::vector<Item> items;
  std::vector<DecalKind> decals;
};

enum GameOutcome {
  kOutcomePlaying = 0,
  kOutcomeFrozen,
  kOutcomeKilled
};

// Something that happened during a turn and deserves a sound.
enum GameEvent {
  kEventStep = 0,
  kEventShot,
  kEventKick,
  kEventHit,       // the hero was bitten
  kEventKill,
  kEventPickup,
  kEventStairs,
  kEventRest
};

// The whole state of one game.
struct Game {
  Cell maze[kMazeWidth][kMazeHeight];
  std::vector<Creature> creatures;  // creatures[0] is the hero
  std::vector<Action> actions;
  Si32 level = 1;
  Si32 kills = 0;
  Si32 turns = 0;
  Ui64 seed = 0;
  GameOutcome outcome = kOutcomePlaying;
  std::vector<GameEvent> events;
};

extern Game g_game;

// Loads data/actions.csv; returns false and leaves g_game.actions as it was
// if the file is missing or malformed.
bool LoadActions(const char *csv_path);

// Starts a new game with the given seed; the actions have to be loaded already.
void NewGame(Ui64 seed);

Creature& Hero();
Cell& Maze(Vec2Si32 pos);
bool IsInsideMaze(Vec2Si32 pos);

// The creature standing in a cell, or nullptr.
Creature* CreatureAt(Vec2Si32 pos);

// True if `to` is within `max_distance` of `from` (Chebyshev) and no wall
// stands on the line between them.
bool CanSee(Vec2Si32 from, Vec2Si32 to, Si32 max_distance);

// The first step of the shortest way from `from` to `to` through the maze,
// at most `max_steps` long; false if there is none. The monsters hunt with
// it, and the self-test walks with it.
bool FirstStepTowards(Vec2Si32 from, Vec2Si32 to, Si32 max_steps,
    Vec2Si32 *out_step);

// True if the hero has the endurance and the ammo for the action.
bool IsActionPossible(const Creature &hero, const Action &action);

// The nearest visible monster the action reaches, or nullptr.
Creature* FindTarget(const Action &action);

// The hero's moves. Each one that succeeds is one turn: it costs a point of
// warmth, the monsters get their move, and the game may be over afterwards.
// Each returns true if the turn happened.
bool HeroStep(Vec2Si32 step);
bool HeroAct(Si32 action_idx);
bool HeroRest();

// Takes the events of the last turns; the vector is empty afterwards.
std::vector<GameEvent> TakeEvents();

// Marks every cell the hero can see from where they stand.
void LookAround();

// Cheats
void RevealMaze();
void RegenerateLevel();

}  // namespace pyramids

#endif  // ANTARCTICA_PYRAMIDS_GAME_H_
