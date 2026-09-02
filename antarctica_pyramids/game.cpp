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

#include "game.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <utility>
#include "engine/csv.h"
#include "engine/log.h"

namespace pyramids {

Game g_game;

namespace {

const Vec2Si32 kDirections[4] = {
  Vec2Si32(-1, 0), Vec2Si32(1, 0), Vec2Si32(0, -1), Vec2Si32(0, 1)
};
const Vec2Si32 kMazeSize(kMazeWidth, kMazeHeight);
const Si32 kSightRadius = 10;
const Si32 kHuntDistance = 10;  // how far a monster tracks the hero, in steps
const Si32 kStickWarmth = 40;
const Si32 kRestEndurance = 20;
const Si32 kTurnEndurance = 3;

// Which stairs the hero came down, so the stairs up on the new level match.
CellKind g_upper_cell_kind = kCellStairsDownLeft;

void Emit(GameEvent event) {
  g_game.events.push_back(event);
}

Si32 Chebyshev(Vec2Si32 a, Vec2Si32 b) {
  return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

// The maze

void FillMaze() {
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
    for (pos.y = 0; pos.y < kMazeHeight; ++pos.y) {
      Cell &cell = Maze(pos);
      cell.kind = kCellWall;
      cell.is_visible = false;
      cell.is_known = false;
      cell.items.clear();
      cell.decals.clear();
    }
  }
}

// A depth-first carve: from a floor cell, knock through a wall into a cell two
// steps away that is still solid, and go on from there.
void StepMazeGeneration(Vec2Si32 from) {
  Maze(from).kind = kCellFloor;
  Vec2Si32 direction[4] = {
    kDirections[0], kDirections[1], kDirections[2], kDirections[3]
  };
  for (Si32 variants = 4; variants > 0; --variants) {
    Si32 idx = Random32(0, variants - 1);
    Vec2Si32 dir = direction[idx];
    Vec2Si32 path = from + dir;
    Vec2Si32 to = path + dir;
    if (to.x > 0 && to.x < kMazeSize.x - 1
        && to.y > 0 && to.y < kMazeSize.y - 1) {
      if (Maze(to).kind == kCellWall) {
        Maze(path).kind = kCellFloor;
        StepMazeGeneration(to);
      }
    }
    direction[idx] = direction[variants - 1];
  }
}

Si32 CountExits(Vec2Si32 pos, Vec2Si32 *out_exit) {
  Si32 exits = 0;
  for (Si32 idx = 0; idx < 4; ++idx) {
    if (Maze(pos + kDirections[idx]).kind != kCellWall) {
      exits++;
      if (out_exit != nullptr) {
        *out_exit = pos + kDirections[idx];
      }
    }
  }
  return exits;
}

bool IsDeadEnd(Vec2Si32 pos) {
  return Maze(pos).kind == kCellFloor && CountExits(pos, nullptr) == 1;
}

std::deque<Vec2Si32> FindDeadEnds() {
  std::deque<Vec2Si32> res;
  for (Si32 x = 1; x < kMazeSize.x - 1; ++x) {
    for (Si32 y = 1; y < kMazeSize.y - 1; ++y) {
      if (IsDeadEnd(Vec2Si32(x, y))) {
        res.emplace_back(x, y);
      }
    }
  }
  return res;
}

// Walls a dead-end corridor up from its end until it meets a junction.
void EliminateDeadEnd(Vec2Si32 pos) {
  while (true) {
    if (CreatureAt(pos) != nullptr) {
      return;
    }
    if (Maze(pos).kind != kCellFloor) {
      return;
    }
    Vec2Si32 exit;
    if (CountExits(pos, &exit) != 1) {
      return;
    }
    Maze(pos).kind = kCellWall;
    pos = exit;
  }
}

// Opens a dead end into a neighbouring corridor, making a loop.
void CycleDeadEnd(Vec2Si32 pos) {
  if (Maze(pos).kind != kCellFloor) {
    return;
  }
  Si32 rnd_dir = Random32(0, 3);
  for (Si32 i = 0; i < 4; ++i) {
    Si32 idx = (i + rnd_dir) % 4;
    Vec2Si32 p = pos + kDirections[idx];
    if (p.x > 0 && p.x < kMazeSize.x - 1 && p.y > 0 && p.y < kMazeSize.y - 1) {
      Cell &cell = Maze(p);
      if (cell.kind == kCellWall) {
        cell.kind = kCellFloor;
        return;
      }
    }
  }
}

// A monster appears in a random floor cell the hero can't see right now, with
// hitpoints that grow with every kill.
void SpawnMonster() {
  std::vector<Vec2Si32> candidates;
  Vec2Si32 pos;
  for (pos.x = 1; pos.x < kMazeSize.x - 1; ++pos.x) {
    for (pos.y = 1; pos.y < kMazeSize.y - 1; ++pos.y) {
      const Cell &cell = Maze(pos);
      if (cell.kind == kCellFloor && !cell.is_visible
          && CreatureAt(pos) == nullptr
          && Chebyshev(pos, Hero().pos) > 3) {
        candidates.push_back(pos);
      }
    }
  }
  if (candidates.empty()) {
    return;
  }
  Creature monster;
  monster.kind = static_cast<CreatureKind>(
      Random32(kCreatureMonsterBegin, kCreatureMonsterEnd - 1));
  monster.pos = candidates[Random32(0, static_cast<Si32>(candidates.size()) - 1)];
  monster.prev_pos = monster.pos;
  monster.full_hitpoints = 100 + 25 * g_game.kills;
  monster.hitpoints = monster.full_hitpoints;
  monster.damage = 10 + 2 * g_game.kills;
  g_game.creatures.push_back(monster);
}

void GenerateMaze() {
  FillMaze();
  g_game.creatures.resize(1);  // the hero stays, the monsters of the old level go
  Creature &hero = Hero();
  hero.prev_pos = hero.pos;

  StepMazeGeneration(hero.pos);

  if (g_game.level > 1) {
    if (g_upper_cell_kind == kCellStairsDownRight) {
      Maze(hero.pos).kind = kCellStairsUpRight;
    } else {
      Maze(hero.pos).kind = kCellStairsUpLeft;
    }
  }

  // The way down is a dead end, facing the corridor it hangs from. The hero's
  // own cell is not a candidate for anything below.
  std::deque<Vec2Si32> dead_ends = FindDeadEnds();
  for (size_t idx = 0; idx < dead_ends.size(); ++idx) {
    if (dead_ends[idx] == hero.pos) {
      dead_ends[idx] = dead_ends.back();
      dead_ends.pop_back();
      break;
    }
  }
  int attempt = 0;
  while (!dead_ends.empty()) {
    attempt++;
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    Vec2Si32 pos = dead_ends[rnd];
    bool is_ok = false;
    if (Maze(pos + Vec2Si32(-1, 0)).kind == kCellFloor) {
      is_ok = true;
      Maze(pos).kind = kCellStairsDownLeft;
    } else if (Maze(pos + Vec2Si32(1, 0)).kind == kCellFloor
        || attempt > 10) {
      is_ok = true;
      Maze(pos).kind = kCellStairsDownRight;
    }
    if (is_ok) {
      dead_ends[rnd] = dead_ends.back();
      dead_ends.pop_back();
      break;
    }
  }

  // Half of the remaining dead ends are walled up, half of the rest are
  // opened into loops, and the ones left hold the items.
  Si32 to_eliminate = static_cast<Si32>(dead_ends.size() / 2);
  for (Si32 idx = 0; idx < to_eliminate; ++idx) {
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    EliminateDeadEnd(dead_ends[rnd]);
    dead_ends[rnd] = dead_ends.back();
    dead_ends.pop_back();
  }
  Si32 to_cycle = static_cast<Si32>(dead_ends.size() / 2);
  for (Si32 idx = 0; idx < to_cycle; ++idx) {
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    CycleDeadEnd(dead_ends[rnd]);
    dead_ends[rnd] = dead_ends.back();
    dead_ends.pop_back();
  }
  for (Ui32 idx = 0; idx < dead_ends.size(); ++idx) {
    if (hero.pos == dead_ends[idx]) {
      continue;
    }
    Cell &cell = Maze(dead_ends[idx]);
    if (cell.kind != kCellFloor) {
      continue;
    }
    Item item;
    Si32 rnd = Random32(0, 255);
    if (rnd < 48) {
      item.kind = kItemStick;
    } else if (rnd < 160) {
      item.kind = kItemStone;
    } else {
      item.kind = kItemNone;
    }
    if (item.kind != kItemNone) {
      cell.items.push_back(item);
    }
  }

  LookAround();
  // SpawnMonster pushes into g_game.creatures and may reallocate it, so the
  // hero reference above is not to be used past this point.
  const Vec2Si32 hero_pos = hero.pos;
  Si32 monster_count = std::min(4, 1 + (g_game.level - 1) / 2);
  for (Si32 i = 0; i < monster_count; ++i) {
    SpawnMonster();
  }
  *Log() << "Level " << g_game.level << " generated, " << monster_count
      << " monsters, hero at " << hero_pos.x << "," << hero_pos.y;
}

}  // namespace

// Public because the self-test walks the hero with it as well.
bool FirstStepTowards(Vec2Si32 from, Vec2Si32 to, Si32 max_steps,
    Vec2Si32 *out_step) {
  Si32 distance[kMazeWidth][kMazeHeight];
  Vec2Si32 first[kMazeWidth][kMazeHeight];
  for (Si32 x = 0; x < kMazeWidth; ++x) {
    for (Si32 y = 0; y < kMazeHeight; ++y) {
      distance[x][y] = -1;
    }
  }
  std::deque<Vec2Si32> queue;
  distance[from.x][from.y] = 0;
  queue.push_back(from);
  while (!queue.empty()) {
    Vec2Si32 pos = queue.front();
    queue.pop_front();
    Si32 d = distance[pos.x][pos.y];
    if (pos == to) {
      *out_step = first[pos.x][pos.y];
      return true;
    }
    if (d >= max_steps) {
      continue;
    }
    for (Si32 idx = 0; idx < 4; ++idx) {
      Vec2Si32 next = pos + kDirections[idx];
      if (!IsInsideMaze(next) || Maze(next).kind == kCellWall
          || distance[next.x][next.y] != -1) {
        continue;
      }
      // Creatures are not obstacles here: the maze is nearly a tree, and a
      // monster in a corridor would cut off everything behind it. The caller
      // checks the one cell it is about to step into.
      distance[next.x][next.y] = d + 1;
      first[next.x][next.y] = (d == 0) ? next : first[pos.x][pos.y];
      queue.push_back(next);
    }
  }
  return false;
}

namespace {

// The turn

void MonstersAct() {
  Creature &hero = Hero();
  for (size_t idx = 1; idx < g_game.creatures.size(); ++idx) {
    Creature &monster = g_game.creatures[idx];
    monster.prev_pos = monster.pos;
    if (Maze(monster.pos).is_visible) {
      monster.is_hunting = true;
    }
    if (!monster.is_hunting) {
      continue;
    }
    if (Chebyshev(monster.pos, hero.pos) <= 1
        && CanSee(monster.pos, hero.pos, 1)) {
      hero.hitpoints = std::max(0, hero.hitpoints - monster.damage);
      Emit(kEventHit);
      continue;
    }
    Vec2Si32 step;
    if (FirstStepTowards(monster.pos, hero.pos, kHuntDistance, &step)) {
      if (step != hero.pos && CreatureAt(step) == nullptr) {
        monster.pos = step;
      }
    } else {
      monster.is_hunting = false;  // lost the trail
    }
  }
}

void EndTurn() {
  Creature &hero = Hero();
  g_game.turns++;
  hero.warmth = std::max(0, hero.warmth - 1);
  hero.endurance = std::min(hero.full_endurance,
      hero.endurance + kTurnEndurance);
  MonstersAct();
  if (hero.hitpoints <= 0) {
    g_game.outcome = kOutcomeKilled;
  } else if (hero.warmth <= 0) {
    g_game.outcome = kOutcomeFrozen;
  }
  if (g_game.outcome != kOutcomePlaying) {
    *Log() << "Game over on turn " << g_game.turns << ": "
        << (g_game.outcome == kOutcomeKilled ? "killed" : "frozen")
        << ", level " << g_game.level << ", kills " << g_game.kills;
  }
  LookAround();
}

void PickUpItems(Creature *hero) {
  Cell &cell = Maze(hero->pos);
  for (size_t idx = 0; idx < cell.items.size(); ++idx) {
    switch (cell.items[idx].kind) {
      case kItemStone:
        hero->ammo++;
        break;
      case kItemStick:
        hero->warmth = std::min(hero->full_warmth,
            hero->warmth + kStickWarmth);
        break;
      default:
        break;
    }
    Emit(kEventPickup);
  }
  cell.items.clear();
}

void Kill(Creature *monster) {
  Cell &cell = Maze(monster->pos);
  cell.decals.push_back(static_cast<DecalKind>(
      Random32(kDecalBlood0, kDecalCount - 1)));
  g_game.kills++;
  Emit(kEventKill);
  for (size_t idx = 1; idx < g_game.creatures.size(); ++idx) {
    if (&g_game.creatures[idx] == monster) {
      g_game.creatures.erase(g_game.creatures.begin()
          + static_cast<Si64>(idx));
      break;
    }
  }
  SpawnMonster();
}

// The action for a bump into a monster: the first melee action the hero can
// afford, or -1.
Si32 FirstMeleeAction() {
  for (size_t idx = 0; idx < g_game.actions.size(); ++idx) {
    const Action &action = g_game.actions[idx];
    if (action.damage_distance <= 1 && IsActionPossible(Hero(), action)) {
      return static_cast<Si32>(idx);
    }
  }
  return -1;
}

}  // namespace

bool LoadActions(const char *csv_path) {
  CsvTable table;
  if (!table.LoadFile(csv_path)) {
    *Log() << "Can't load " << csv_path << ": " << table.GetErrorDescription();
    return false;
  }
  std::vector<Action> actions;
  for (Ui64 row_idx = 0; row_idx < table.RowCount(); ++row_idx) {
    const CsvRow &row = table[row_idx];
    Action action;
    action.name = row["name"];
    action.cost_endurance = row.GetValue("cost_endurance", 0);
    action.cost_ammo = row.GetValue("cost_ammo", 0);
    action.produce_warmth = row.GetValue("produce_warmth", 0);
    action.damage_hitpoints = row.GetValue("damage_hitpoints", 0);
    action.damage_distance = row.GetValue("damage_distance", 1);
    if (action.name.empty()) {
      *Log() << csv_path << ": row " << row_idx << " has no name";
      return false;
    }
    actions.push_back(action);
  }
  if (actions.empty()) {
    *Log() << csv_path << " has no actions";
    return false;
  }
  g_game.actions = actions;
  return true;
}

void NewGame(Ui64 seed) {
  SetRandomSeed(seed);
  g_game.seed = seed;
  g_game.level = 1;
  g_game.kills = 0;
  g_game.turns = 0;
  g_game.outcome = kOutcomePlaying;
  g_game.events.clear();
  g_game.creatures.clear();
  Creature hero;
  hero.kind = static_cast<CreatureKind>(
      Random32(kCreatureHeroBegin, kCreatureHeroEnd - 1));
  hero.pos = Vec2Si32(1, 1);
  hero.prev_pos = hero.pos;
  g_game.creatures.push_back(hero);
  *Log() << "New game, seed " << seed;
  GenerateMaze();
}

Creature& Hero() {
  return g_game.creatures[0];
}

Cell& Maze(Vec2Si32 pos) {
  Check(IsInsideMaze(pos), "pos out of bounds in Maze");
  return g_game.maze[pos.x][pos.y];
}

bool IsInsideMaze(Vec2Si32 pos) {
  return pos.x >= 0 && pos.y >= 0 && pos.x < kMazeWidth && pos.y < kMazeHeight;
}

Creature* CreatureAt(Vec2Si32 pos) {
  for (size_t idx = 0; idx < g_game.creatures.size(); ++idx) {
    if (g_game.creatures[idx].pos == pos) {
      return &g_game.creatures[idx];
    }
  }
  return nullptr;
}

bool CanSee(Vec2Si32 from, Vec2Si32 to, Si32 max_distance) {
  if (Chebyshev(from, to) > max_distance) {
    return false;
  }
  // The same trace LookAround uses, so what the hero sees is what they can hit.
  Vec2Si32 delta = to - from;
  Si32 steps = kSightRadius * 2;
  for (Si32 step = 1; step < steps; ++step) {
    Vec2Si32 s = from + ((delta * step) / steps);
    if (s == to) {
      break;
    }
    if (s != from && Maze(s).kind == kCellWall) {
      return false;
    }
  }
  return true;
}

bool IsActionPossible(const Creature &hero, const Action &action) {
  return hero.ammo >= action.cost_ammo
      && hero.endurance >= action.cost_endurance;
}

Creature* FindTarget(const Action &action) {
  Creature *best = nullptr;
  Si32 best_distance = 0;
  Vec2Si32 from = Hero().pos;
  for (size_t idx = 1; idx < g_game.creatures.size(); ++idx) {
    Creature &monster = g_game.creatures[idx];
    if (!Maze(monster.pos).is_visible) {
      continue;
    }
    if (!CanSee(from, monster.pos, action.damage_distance)) {
      continue;
    }
    Si32 distance = Chebyshev(from, monster.pos);
    if (best == nullptr || distance < best_distance) {
      best = &monster;
      best_distance = distance;
    }
  }
  return best;
}

bool HeroStep(Vec2Si32 step) {
  if (g_game.outcome != kOutcomePlaying) {
    return false;
  }
  Creature &hero = Hero();
  Vec2Si32 target = hero.pos + step;
  if (!IsInsideMaze(target)) {
    return false;
  }
  Cell &cell = Maze(target);
  if (cell.kind == kCellWall) {
    return false;
  }
  if (CreatureAt(target) != nullptr) {
    // Walking into a monster is a kick.
    return HeroAct(FirstMeleeAction());
  }
  hero.prev_pos = hero.pos;
  hero.pos = target;
  if (cell.kind == kCellStairsDownLeft || cell.kind == kCellStairsDownRight) {
    g_upper_cell_kind = cell.kind;
    g_game.level++;
    Emit(kEventStairs);
    GenerateMaze();
  } else {
    Emit(kEventStep);
    PickUpItems(&hero);
  }
  EndTurn();
  return true;
}

bool HeroAct(Si32 action_idx) {
  if (g_game.outcome != kOutcomePlaying) {
    return false;
  }
  if (action_idx < 0
      || action_idx >= static_cast<Si32>(g_game.actions.size())) {
    return false;
  }
  Creature &hero = Hero();
  const Action &action = g_game.actions[action_idx];
  if (!IsActionPossible(hero, action)) {
    return false;
  }
  Creature *target = FindTarget(action);
  if (target == nullptr) {
    return false;
  }
  hero.prev_pos = hero.pos;
  hero.ammo -= action.cost_ammo;
  hero.endurance -= action.cost_endurance;
  hero.warmth = std::min(hero.full_warmth, hero.warmth + action.produce_warmth);
  Emit(action.cost_ammo > 0 ? kEventShot : kEventKick);
  target->hitpoints = std::max(0, target->hitpoints - action.damage_hitpoints);
  if (target->hitpoints == 0) {
    Kill(target);
  }
  EndTurn();
  return true;
}

bool HeroRest() {
  if (g_game.outcome != kOutcomePlaying) {
    return false;
  }
  Creature &hero = Hero();
  hero.prev_pos = hero.pos;
  hero.endurance = std::min(hero.full_endurance,
      hero.endurance + kRestEndurance);
  Emit(kEventRest);
  EndTurn();
  return true;
}

std::vector<GameEvent> TakeEvents() {
  std::vector<GameEvent> events;
  std::swap(events, g_game.events);
  return events;
}

void LookAround() {
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
    for (pos.y = 0; pos.y < kMazeHeight; ++pos.y) {
      Maze(pos).is_visible = false;
    }
  }
  Vec2Si32 hero_pos = Hero().pos;
  Vec2Si32 min(std::max(0, hero_pos.x - kSightRadius),
      std::max(0, hero_pos.y - kSightRadius));
  Vec2Si32 max(std::min(kMazeWidth - 1, hero_pos.x + kSightRadius),
      std::min(kMazeHeight - 1, hero_pos.y + kSightRadius));
  for (pos.x = min.x; pos.x <= max.x; ++pos.x) {
    for (pos.y = min.y; pos.y <= max.y; ++pos.y) {
      Vec2Si32 hero_to_pos = pos - hero_pos;
      Si32 max_step = kSightRadius * 2;
      for (Si32 step = 0; step <= max_step; ++step) {
        Vec2Si32 s = hero_pos + ((hero_to_pos * step) / max_step);
        Maze(s).is_known = true;
        Maze(s).is_visible = true;
        if (Maze(s).kind == kCellWall) {
          break;
        }
      }
    }
  }
}

void RevealMaze() {
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < kMazeWidth; ++pos.x) {
    for (pos.y = 0; pos.y < kMazeHeight; ++pos.y) {
      Maze(pos).is_known = true;
    }
  }
}

void RegenerateLevel() {
  GenerateMaze();
}

}  // namespace pyramids
