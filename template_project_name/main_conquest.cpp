#include "engine/easy.h"
#include <algorithm>
#include <cstdlib>

using namespace arctic;  // NOLINT

// A tiny turn-based strategy for two players at one keyboard. Cities build
// swordsmen and archers, swordsmen capture cities, and the player who owns
// every city wins. Pick a unit with the mouse, then click the cell to move to,
// or the enemy to attack. Enter (or a click on the panel at the top) ends the
// turn, Escape quits.

//////////////////////////////////////////////////////////////////////////////
// TYPES AND DATA STRUCTURES

// The kind of a unit
enum UnitType {
  NONE = 0,       // The unit is not in use: an empty slot of the `units` array
  CITY = 1,       // A city builds units
  SWORDSMAN = 2,  // A swordsman moves and fights hand to hand
  ARCHER = 3,     // An archer shoots over one cell but can't capture cities
};

// Who owns a unit
enum Owner {
  NEUTRAL = 0,  // No player, a neutral unit
  PLAYER1 = 1,
  PLAYER2 = 2,
};

// The kind of ground in a cell
enum Terrain {
  GRASS = 0,  // Plain ground, costs 1 MP to enter
  HILLS = 1,  // Costs 2 MP to enter
  WATER = 2,  // Can't be entered or built on
};

// A unit
struct Unit {
  UnitType type;  // The kind of the unit
  Owner owner;    // The owner of the unit

  // The position of the unit on the map, in cells
  int x;
  int y;

  // Movement points (MP) left in this turn.
  // At the start of a turn every unit of the player gets its MP back and
  // spends them moving. A city doesn't move, but it builds a unit in a
  // neighbouring cell.
  int moves;

  // Removes the unit from the game
  void destroy() {
    type = NONE;
    owner = NEUTRAL;
  }
};

//////////////////////////////////////////////////////////////////////////////
// GLOBAL CONSTANTS

constexpr int map_x = 23, map_y = 11;  // Map size in cells
constexpr int max_units = map_x * map_y;  // At most one unit per cell

// Movement points (MP) of every unit type
const int unit_moves[] = {
  0,  // NONE
  1,  // CITY
  3,  // SWORDSMAN
  2,  // ARCHER
};

// Keys
const KeyCode keyQuitGame = kKeyEscape;  // Quit the game
const KeyCode keyNextTurn = kKeyEnter;   // End the turn

//////////////////////////////////////////////////////////////////////////////
// GRAPHICS

// The sprites are 10x10 pixels and are drawn twice as big, so that a line of
// text fits into the panel at the top.
constexpr int tile_x = 20, tile_y = 20;  // Size of one cell on screen
constexpr int menu_y = 34;  // Height of the panel at the top

// Colors
const Rgba color_move(255, 255, 255);
const Rgba color_text(0, 0, 0);
const Rgba color_owner[3] = {
  Rgba(),           // NEUTRAL
  Rgba(255, 0, 0),  // PLAYER1
  Rgba(0, 0, 255),  // PLAYER2
};

// Unit sprites by owner (first index) and type (second index)
Sprite sprites[3][4];
Sprite sprite_terrain[3];  // Ground sprites by Terrain
Sprite sprite_frame;  // The frame around the selected unit

Font g_font;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES

Unit units[max_units];  // Every unit in the game
Terrain terrain[map_y][map_x];  // The ground of every cell
Owner turn;  // The player whose turn it is
Unit* selected;  // The selected unit
int orderx, ordery;  // The cell the selected unit is ordered to
bool order_is_alt;  // The order was given with the right mouse button
bool turn_is_over;  // The turn has ended
bool game_is_over;  // The game session has ended
Owner winner;  // Who has won

//////////////////////////////////////////////////////////////////////////////
// UNIT FUNCTIONS

// Removes every unit
void ClearUnits() {
  for (int i = 0; i < max_units; i++) {
    units[i].destroy();
  }
}

// Gives the units of `owner` their movement points back
void AddUnitMoves(Owner owner) {
  for (int i = 0; i < max_units; i++) {
    if (units[i].owner == owner) {
      UnitType type = units[i].type;
      units[i].moves = unit_moves[type];
    }
  }
}

// Creates a unit
Unit* CreateUnit(UnitType type, Owner owner, int x, int y, int moves = 0) {
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == NONE) {
      // A free slot, fill it in
      unit->type = type;
      unit->owner = owner;
      unit->x = x;
      unit->y = y;
      unit->moves = moves;
      return unit;
    }
  }
  Fatal("Out of units");  // ERROR: every slot is taken
  return nullptr;
}

// Finds the unit in a cell, returns `nullptr` if the cell is empty
Unit* FindUnit(int x, int y) {
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type != NONE && unit->x == x && unit->y == y) {
      return unit;
    }
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////////
// MAP FUNCTIONS

bool IsOnMap(int x, int y) {
  return x >= 0 && x < map_x && y >= 0 && y < map_y;
}

// Movement points it takes to enter a cell
int MoveCost(int x, int y) {
  if (terrain[y][x] == HILLS) {
    return 2;
  }
  return 1;
}

// True if `unit` may move into the cell: it is on the map, it is not water and
// the unit has enough movement points left for the ground there
bool CanEnter(const Unit* unit, int x, int y) {
  if (!IsOnMap(x, y)) {
    return false;
  }
  if (terrain[y][x] == WATER) {
    return false;
  }
  return unit->moves >= MoveCost(x, y);
}

// Puts a round patch of `kind` ground around a cell
void PaintTerrain(Terrain kind, int center_x, int center_y, int radius) {
  for (int y = center_y - radius; y <= center_y + radius; y++) {
    for (int x = center_x - radius; x <= center_x + radius; x++) {
      if (IsOnMap(x, y)) {
        terrain[y][x] = kind;
      }
    }
  }
}

// Fills the map with grass, a few lakes and a few hills
void GenerateTerrain() {
  for (int y = 0; y < map_y; y++) {
    for (int x = 0; x < map_x; x++) {
      terrain[y][x] = GRASS;
    }
  }
  for (int i = 0; i < 4; i++) {
    PaintTerrain(HILLS, Random32(0, map_x - 1), Random32(0, map_y - 1),
        Random32(1, 2));
  }
  for (int i = 0; i < 3; i++) {
    PaintTerrain(WATER, Random32(0, map_x - 1), Random32(0, map_y - 1),
        Random32(0, 1));
  }
}

//////////////////////////////////////////////////////////////////////////////
// INPUT AND DRAWING

// Draws the map and the units
void DrawGame() {
  int moves_left = 0;

  // The ground
  for (int y = 0; y < map_y; y++) {
    for (int x = 0; x < map_x; x++) {
      sprite_terrain[terrain[y][x]].Draw(x * tile_x, y * tile_y,
          tile_x, tile_y);
    }
  }

  // The units
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == NONE) {
      continue;
    }

    if (unit->owner == turn) {
      moves_left += unit->moves;  // Movement points left in this turn
    }

    sprites[unit->owner][unit->type].Draw(unit->x * tile_x, unit->y * tile_y,
        tile_x, tile_y);

    // One dot per movement point left
    for (int i = 0; i < unit->moves; i++) {
      Vec2Si32 dot(unit->x * tile_x + 4 + 4 * i, unit->y * tile_y + 4);
      DrawRectangle(dot, dot + Vec2Si32(1, 1), color_move);
    }

    if (selected == unit) {
      sprite_frame.Draw(unit->x * tile_x, unit->y * tile_y, tile_x, tile_y);
    }
  }

  // The panel at the top
  Rgba color = color_owner[turn];
  if (moves_left == 0 || winner != NEUTRAL) {
    // The end turn button blinks when there is nothing left to do
    color = Scale(color, 200 + (Ui32)(55.0 * sin(5.0 * Time())));
  }
  DrawRectangle(Vec2Si32(0, tile_y * map_y),
      Vec2Si32(tile_x * map_x - 1, tile_y * map_y + menu_y - 1), color);
  const char *hint = "LMB: sword  RMB: bow  Enter: end turn";
  if (winner != NEUTRAL) {
    hint = winner == PLAYER1 ? "Red wins! Esc to quit"
        : "Blue wins! Esc to quit";
  }
  g_font.Draw(hint, 2, tile_y * map_y, kTextOriginBottom, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest, color_text);

  ShowFrame();
}

// Returns `true` if the turn has to end
bool StopTurn() {
  if (IsKeyUpward(keyQuitGame)) {
    turn_is_over = true;  // The turn ends first, then the game
    game_is_over = true;
  }
  if (IsKeyUpward(keyNextTurn)
      || (IsKeyUpward(kKeyMouseLeft) && MousePos().y > tile_y * map_y)) {
    turn_is_over = true;
  }
  return turn_is_over;
}

// Waits for the player to pick a unit: fills `selected`, or ends the turn
void SelectUnit() {
  while (selected == nullptr) {
    DrawGame();
    if (StopTurn()) {
      return;
    }
    if (IsKeyUpward(kKeyMouseLeft)) {
      int x = MousePos().x / tile_x;
      int y = MousePos().y / tile_y;
      Unit* unit = FindUnit(x, y);
      // Only a unit of the current player with movement points left
      if (unit != nullptr && unit->owner == turn && unit->moves > 0) {
        selected = unit;
      }
    }
  }
}

// True if an archer standing at (`from_x`, `from_y`) can shoot the cell
// (`x`, `y`): the target is up to two cells away and, when it is two cells
// away, the cell between them is free
bool CanShoot(int from_x, int from_y, int x, int y) {
  int dx = x - from_x;
  int dy = y - from_y;
  int distance = std::max(std::abs(dx), std::abs(dy));
  if (distance == 1) {
    return true;
  }
  if (distance != 2) {
    return false;
  }
  int mid_x = from_x + (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
  int mid_y = from_y + (dy > 0 ? 1 : (dy < 0 ? -1 : 0));
  return FindUnit(mid_x, mid_y) == nullptr;
}

// True if the selected unit can be ordered to the cell
bool IsValidOrder(int x, int y) {
  if (!IsOnMap(x, y)) {
    return false;
  }
  int dx = x - selected->x;
  int dy = y - selected->y;
  int distance = std::max(std::abs(dx), std::abs(dy));
  if (distance == 0) {
    return false;
  }
  Unit* target = FindUnit(x, y);
  if (selected->type == CITY) {
    // A city builds in an empty neighbouring cell that is not water
    return distance == 1 && target == nullptr && terrain[y][x] != WATER;
  }
  if (distance == 1) {
    // A step into a free cell or an attack on a neighbour
    return target != nullptr || CanEnter(selected, x, y);
  }
  // Only an archer reaches further than one cell, and only with an arrow
  return selected->type == ARCHER && target != nullptr
      && target->owner != turn && target->type != CITY
      && CanShoot(selected->x, selected->y, x, y);
}

// Waits for the player to pick a cell: fills `orderx`, `ordery` and
// `order_is_alt`, or ends the turn
void SelectOrder() {
  orderx = -1;
  ordery = -1;
  while (orderx == -1) {
    DrawGame();
    if (StopTurn()) {
      return;
    }
    bool is_left = IsKeyUpward(kKeyMouseLeft);
    bool is_right = IsKeyUpward(kKeyMouseRight);
    if (is_left || is_right) {
      int x = MousePos().x / tile_x;
      int y = MousePos().y / tile_y;
      if (IsValidOrder(x, y)) {
        orderx = x;
        ordery = y;
        order_is_alt = is_right;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// GAME LOGIC

// Carries out the order of a city: builds a swordsman, or an archer on a
// right click
void MoveCity() {
  Unit* target = FindUnit(orderx, ordery);
  if (target == nullptr) {
    CreateUnit(order_is_alt ? ARCHER : SWORDSMAN, turn, orderx, ordery);
    selected->moves--;
  }
}

// Moves the selected unit into the ordered cell and pays for the ground there
void StepInto() {
  selected->moves -= MoveCost(orderx, ordery);
  selected->x = orderx;
  selected->y = ordery;
}

// Carries out the order of a swordsman
void MoveSwordsman() {
  Unit* target = FindUnit(orderx, ordery);

  // An empty cell: step into it
  if (target == nullptr) {
    StepInto();
    return;
  }

  // A friendly unit: the order is cancelled
  if (target->owner == turn) {
    return;
  }

  // An enemy city: it is captured
  if (target->type == CITY) {
    target->owner = turn;
    target->moves = 0;
    selected->moves = 0;
    return;
  }

  // An enemy swordsman or archer: it dies and the swordsman takes its place
  if (target->type == SWORDSMAN || target->type == ARCHER) {
    target->destroy();
    selected->x = orderx;
    selected->y = ordery;
    selected->moves = 0;
    return;
  }
}

// Carries out the order of an archer
void MoveArcher() {
  Unit* target = FindUnit(orderx, ordery);

  // An empty cell: step into it
  if (target == nullptr) {
    StepInto();
    return;
  }

  // A friendly unit, or an enemy city an archer can't take: cancelled
  if (target->owner == turn || target->type == CITY) {
    return;
  }

  // An enemy swordsman or archer: shot dead, the archer stays where it is
  target->destroy();
  selected->moves = 0;
}

// The turn of the current player. In a loop:
//  (1) the player picks a unit, `selected`;
//  (2) then picks the cell to move to, `orderx` and `ordery` (for a city, the
//      cell to build in);
//  (3) the order is carried out.
// Until the turn ends, `turn_is_over`.
void OneTurn() {
  AddUnitMoves(turn);
  turn_is_over = false;
  selected = nullptr;
  while (true) {
    SelectUnit();   // (1)
    SelectOrder();  // (2)
    if (turn_is_over) {
      break;
    }

    // (3)
    int moves = selected->moves;
    switch (selected->type) {
      case CITY:
        MoveCity();
        break;
      case SWORDSMAN:
        MoveSwordsman();
        break;
      case ARCHER:
        MoveArcher();
        break;
      default:
        Fatal("unexpected unit type");
    }
    // The unit has no movement points left, or the order was cancelled
    if (selected->moves == 0 || selected->moves == moves) {
      selected = nullptr;
    }
  }

  // Has the current player won?
  int enemy_cities = 0;
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == CITY && unit->owner != turn) {
      enemy_cities++;
    }
  }
  if (enemy_cities == 0) {
    winner = turn;
    game_is_over = true;
    // The victory screen stays until the quit key
    while (!IsKeyUpward(keyQuitGame)) {
      DrawGame();
    }
  }
}

// One game
void GameSession() {
  // Start from a clean state, there may be a previous game in the globals
  ClearUnits();
  game_is_over = false;
  winner = NEUTRAL;
  turn = PLAYER1;  // The first player moves first

  GenerateTerrain();

  // The cities of the players
  CreateUnit(CITY, PLAYER1, 1, 1);
  CreateUnit(CITY, PLAYER2, map_x - 2, map_y - 2);

  // The neutral cities
  for (int y = 1; y < map_y; y += 4) {
    for (int x = 1; x < map_x; x += 4) {
      if (FindUnit(x, y) == nullptr) {
        CreateUnit(CITY, NEUTRAL, x, y);
      }
    }
  }

  // A city stands on grass and has grass around it to build on
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == CITY) {
      PaintTerrain(GRASS, unit->x, unit->y, 1);
    }
  }

  // Players take turns until somebody wins
  while (true) {
    OneTurn();

    // Quit was pressed or a player has won
    if (game_is_over) {
      break;
    }

    if (turn == PLAYER1) {
      turn = PLAYER2;
    } else {
      turn = PLAYER1;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// MAIN

void EasyMain() {
  ResizeScreen(map_x * tile_x, map_y * tile_y + menu_y);

  g_font.Load("data/arctic_one_bmf.fnt");
  sprites[NEUTRAL][CITY     ].Load("data/grey_city.tga");
  sprites[PLAYER1][CITY     ].Load("data/red_city.tga");
  sprites[PLAYER1][SWORDSMAN].Load("data/red_sword.tga");
  sprites[PLAYER1][ARCHER   ].Load("data/red_bow.tga");
  sprites[PLAYER2][CITY     ].Load("data/blue_city.tga");
  sprites[PLAYER2][SWORDSMAN].Load("data/blue_sword.tga");
  sprites[PLAYER2][ARCHER   ].Load("data/blue_bow.tga");
  sprite_terrain[GRASS].Load("data/grass.tga");
  sprite_terrain[HILLS].Load("data/hills.tga");
  sprite_terrain[WATER].Load("data/water.tga");
  sprite_frame.Load("data/frame.tga");

  do {
    GameSession();
  } while (winner != NEUTRAL);  // A finished game is followed by a new one
}
