// Array2: in-bounds At and IsInBounds. The battle field is 7x5; a flee
// step can ask for (-1, 0), which must not be treated as a cell.
#define TEST_NO_MAIN
#include "test_helpers.h"
#include "engine/array2.h"

void test_array2_empty_has_no_cells() {
  Array2<Si32> grid;
  TEST_CHECK(grid.Size() == Vec2Si32(0, 0));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(0, 0)));
  TEST_CHECK(!grid.IsInBounds(0, 0));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(-1, 0)));
}

void test_array2_one_by_one_only_origin() {
  Array2<Si32> grid(1, 1);
  TEST_CHECK(grid.IsInBounds(Vec2Si32(0, 0)));
  TEST_CHECK(grid.IsInBounds(0, 0));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(1, 0)));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(0, 1)));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(-1, 0)));
  TEST_CHECK(!grid.IsInBounds(Vec2Si32(0, -1)));
  grid.At(0, 0) = 17;
  TEST_CHECK(grid.At(Vec2Si32(0, 0)) == 17);
}

void test_array2_battle_grid_corners_are_inside() {
  Array2<Si32> battle(7, 5);
  TEST_CHECK(battle.IsInBounds(Vec2Si32(0, 0)));
  TEST_CHECK(battle.IsInBounds(Vec2Si32(6, 0)));
  TEST_CHECK(battle.IsInBounds(Vec2Si32(0, 4)));
  TEST_CHECK(battle.IsInBounds(Vec2Si32(6, 4)));
  TEST_CHECK(battle.IsInBounds(3, 2));
}

void test_array2_battle_grid_rejects_run_off_the_left() {
  Array2<Si32> battle(7, 5);
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(-1, 0)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(-1, 2)));
  TEST_CHECK(!battle.IsInBounds(-1, 4));
}

void test_array2_battle_grid_rejects_just_outside() {
  Array2<Si32> battle(7, 5);
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(7, 0)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(0, 5)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(7, 5)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(6, 5)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(7, 4)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(0, -1)));
  TEST_CHECK(!battle.IsInBounds(Vec2Si32(-1, -1)));
}

void test_array2_at_roundtrip_on_battle_grid() {
  Array2<Si32> battle(7, 5);
  Si32 filled = 0;
  for (Si32 y = 0; y < battle.Size().y; ++y) {
    for (Si32 x = 0; x < battle.Size().x; ++x) {
      if (!TEST_CHECK(battle.IsInBounds(x, y))) {
        return;
      }
      Si32 value = x + y * 100;
      battle.At(x, y) = value;
      TEST_CHECK_(battle.At(Vec2Si32(x, y)) == value,
          "At(%d, %d) wrote %d, read %d", x, y, value,
          battle.At(Vec2Si32(x, y)));
      ++filled;
    }
  }
  TEST_CHECK_(filled == 35, "7x5 grid must have 35 cells, filled %d", filled);
  TEST_CHECK(battle.At(0, 0) == 0);
  TEST_CHECK(battle.At(6, 4) == 6 + 4 * 100);
}
