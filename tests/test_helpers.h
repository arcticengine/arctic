// Shared by every test_*.cpp of the suite and by main.cpp.
//
// The suite is one acutest binary made of many files: main.cpp defines
// TEST_LIST and keeps the program entry point of engine/test_main.h, every
// other file defines TEST_NO_MAIN before including this header, and what
// more than one file needs lives here. A test function is a plain non-static
// void f() found by its declaration in main.cpp, so a new test goes into the
// test_*.cpp of its subsystem and into the two lists in main.cpp.
#ifndef TESTS_TEST_HELPERS_H_
#define TESTS_TEST_HELPERS_H_

#include "engine/test_main.h"

#include <algorithm>
#include <array>
#include <chrono>  // NOLINT
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <type_traits>
#include <vector>
#include "engine/arctic_pi.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"
#include "engine/arctic_platform_tcpip.h"
#include "engine/arctic_types.h"
#include "engine/easy.h"
#include "engine/easy_hw_sprite.h"
#include "engine/opengl.h"
#include "engine/gl_state.h"
#include "engine/localization.h"
#include "engine/json.h"
#include "engine/rgb.h"
#include "engine/data_writer.h"
#include "engine/data_reader.h"
#include "engine/easy_sound_instance.h"
#include "engine/quaternion.h"
#include "engine/transform3f.h"
#include "engine/skeleton.h"
#include "engine/unicode.h"
#include "engine/frustum3f.h"
#include "engine/mesh.h"
#include "engine/mesh_gen_mod_complex.h"
#include "engine/gui.h"
#include "engine/csv.h"
#include "engine/ofbx.h"
#include "engine/gl_texture_cache.h"
#include "engine/mesh_fbx.h"
#include "engine/model.h"
#include "engine/sphere_vs_triangle.h"
#include "engine/collide_soup.h"
#include "engine/physics_contact.h"
#include "engine/physics_debug.h"
#include "engine/physics_solver.h"
#include "engine/physics_sphere_body.h"
#include "engine/physics_world.h"

using namespace arctic;  // NOLINT

// The startup mode decider of the suite, defined in main.cpp and registered
// there; the platform tests compare against it.
StartupMode TestsStartupMode();

// tests/data, looked for from the working directory upward: a run from the
// macOS bundle starts several directories below it.
inline std::string find_test_data_dir() {
  std::string data_dir = "data";
  for (int i = 0; i < 10; ++i) {
    if (arctic::DoesDirectoryExist(data_dir.c_str())) {
      return data_dir;
    }
    data_dir = "../" + data_dir;
  }
  return "data";
}

// A pixel of the software backbuffer, in the same coordinates Draw takes.
inline Rgba BackbufferPixel(Vec2Si32 at) {
  Sprite backbuffer = GetEngine()->GetBackbuffer();
  return backbuffer.RgbaData()[backbuffer.StridePixels() * at.y + at.x];
}

// Input messages made by hand, the way ShowFrame would have filled them in,
// for feeding a Panel through ApplyInput without a window.
inline InputMessage LeftClickAt(Vec2Si32 at) {
  InputMessage message;
  message.kind = InputMessage::kMouse;
  message.mouse.backbuffer_pos = at;
  message.keyboard.key = kKeyMouseLeft;
  message.keyboard.key_state = 1;
  message.keyboard.state[kKeyMouseLeft] = 1;
  return message;
}

inline InputMessage TypedLetter(char letter, KeyCode key) {
  InputMessage message;
  message.kind = InputMessage::kKeyboard;
  message.keyboard.key = key;
  message.keyboard.key_state = 1;
  message.keyboard.characters[0] = letter;
  return message;
}

#endif  // TESTS_TEST_HELPERS_H_
