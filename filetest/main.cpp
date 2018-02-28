#include "engine/easy.h"

#include <iostream>
#include <sstream>

using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

Sprite g_blocks[3];
Si32 g_field[16][8] = {0};
Si32 g_tetraminoes[5][5 * 7] = {
	{0,0,1,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0},
	{0,0,1,0,0,  0,0,1,0,0,  0,0,1,0,0,  0,0,0,0,0,  0,0,1,1,0,  0,1,1,0,0,  0,0,1,1,0},
	{0,0,1,0,0,  0,0,1,0,0,  0,0,1,0,0,  0,1,1,1,0,  0,1,1,0,0,  0,0,1,1,0,  0,0,1,1,0},
	{0,0,1,0,0,  0,0,1,1,0,  0,1,1,0,0,  0,0,1,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0},
	{0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0,  0,0,0,0,0},
};
Si32 g_current[4][5][5];
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
	Si32 idx = Random32(0, 6);
	for (Si32 y = 0; y < 5; ++y) {
		for (Si32 x = 0; x < 5; ++x) {
			g_current[0][y][x] = g_tetraminoes[y][x + idx * 5];
			g_current[1][x][4 - y] = g_tetraminoes[y][x + idx * 5];
			g_current[2][4 - y][4 - x] = g_tetraminoes[y][x + idx * 5];
			g_current[3][4 - x][y] = g_tetraminoes[y][x + idx * 5];
		}
	}
	g_current_x = 3;
	g_current_y = 0;
	g_current_orientation = Random32(0, 3);
}
void ClearField() {
	for (Si32 y = 0; y < 16; ++y) {
		for (Si32 x = 0; x < 8; ++x) {
		    g_field[y][x] = 0;
		}
	}
}
void Init() {
  std::stringstream str;
  std::deque<DirectoryEntry> list;
  std::string canonic = CanonicalizePath("./..");
  if (canonic.empty()) {
    str << "empty canonic\n";
  } else {
    str << "Canonic: \"" << canonic << "\"\n";
  }
  
  std::string relative = RelativePathFromTo("../../../../../",
                                            "../../../../../../piLibs");
  str << "relative path: \"" << relative << "\"\n";
  str << "from " << CanonicalizePath("../../../../../") << "\n";
  str << "to " << CanonicalizePath("../../../../../../piLibs") << "\n";
  
  bool isok = GetDirectoryEntries("../../../../../../piLibs", &list);
  Check(isok, "Can't list directory");
  for (const auto &entry: list) {
    str << entry.title << "\n";
  }
  std::string res = str.str();
  std::cout << res << std::endl;
  
  WriteFile("../../../result.txt", (const Ui8*)(const void*)res.data(), res.size());
  
	g_blocks[1].Load("data/block_1.tga");
	g_blocks[2].Load("data/block_2.tga");
	ResizeScreen(800, 500);
	StartNewTetramino();
	g_prev_time = Time();
  g_score = 0;
  g_font.Load("data/arctic_one_bmf.fnt");
}
bool IsPositionOk(Si32 test_x, Si32 test_y, Si32 test_orientation) {
	for (Si32 y = 0; y < 5; ++y) {
		for (Si32 x = 0; x < 5; ++x) {
			if (g_current[test_orientation][y][x]) {
				if (x + test_x < 0 || x + test_x > 7 || y + test_y > 15
					    || g_field[y + test_y][x + test_x]) {
					return false;
				}
			}
		}
	}
	return true;
}
void LockTetramino() {
	for (Si32 y = 0; y < 5; ++y) {
		for (Si32 x = 0; x < 5; ++x) {
			if (g_current[g_current_orientation][y][x]) {
				g_field[y + g_current_y][x + g_current_x] = 2;
			}
		}
	}
	bool do_continue = true;
	while (do_continue) {
		do_continue = false;
		for (Si32 y = 15; y >= 0; --y) {
			bool is_full_line = true;
			for (Si32 x = 0; x < 8; ++x) {
				if (!g_field[y][x]) {
					is_full_line = false;
					break;
				}
			}
			if (is_full_line) {
        g_score += 100;
				do_continue = true;
				for (Si32 y2 = y; y2 > 0; --y2) {
					for (Si32 x = 0; x < 8; ++x) {
						g_field[y2][x] = g_field[y2 - 1][x];
					}
				}
			}
		}
	}
}
void Update() {
	double time = Time();
	if (IsKeyDownward(kKeyLeft) || IsKeyDownward("a")) {
		g_dx = -1;
	} else if (IsKeyDownward(kKeyRight) || IsKeyDownward("d")) {
		g_dx = 1;
	}
	g_rotate = g_rotate || IsKeyDownward(kKeyUp) || IsKeyDownward("w") || IsKeyDownward(" ");
	g_drop = g_drop || IsKeyDownward(kKeyDown) || IsKeyDownward("s");
	if (IsKeyDownward("c")) {
		ClearField();
		StartNewTetramino();
		return;
	}
	if (time - g_prev_time < 0.5) {
		return;
	}
	g_prev_time = time;
	g_dy = 1;
	if (g_dx && IsPositionOk(g_current_x + g_dx, g_current_y, g_current_orientation)) {
		g_current_x += g_dx;
		g_dy = 0;
	} else {
		g_dx = 0;
	}
	if (g_rotate) {
		if (IsPositionOk(g_current_x, g_current_y, (g_current_orientation + 1) % 4)) {
			g_current_orientation = (g_current_orientation + 1) % 4;
			g_dy = 0;
		}
	}
	bool is_lock = false;
	if (IsPositionOk(g_current_x, g_current_y + (g_dy ? 1 : 0), g_current_orientation)) {
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
void Render() {
	Clear();
  Si32 x_offset = (ScreenSize().x - 25 * 8) / 2;
	for (Si32 y = 0; y < 16; ++y) {
		for (Si32 x = 0; x < 8; ++x) {
			g_blocks[g_field[y][x]].Draw(x_offset + x * 25, (15 - y) * 25);
		}
	}
	for (Si32 y = 0; y < 5; ++y) {
		for (Si32 x = 0; x < 5; ++x) {
			g_blocks[g_current[g_current_orientation][y][x]].Draw(
        x_offset + (x + g_current_x) * 25, (15 - y - g_current_y) * 25);
		}
	}
  char score[128];
  snprintf(score, sizeof(score), u8"Score: %d", g_score);
  g_font.Draw(score, 0, ScreenSize().y, kTextOriginTop);
	ShowFrame();
}
void EasyMain() {
	Init();
	while (!IsKeyDownward(kKeyEscape)) {
		Update();
		Render();
	}
}
