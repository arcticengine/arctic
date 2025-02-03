// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2025 Huldra
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

#include "engine/arctic_platform_def.h"
#include "engine/arctic_platform.h"

#if defined(ARCTIC_PLATFORM_WEB)

#include <cstring>
#include <dirent.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <sys/stat.h>
#include <unistd.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include <iostream>

#include "engine/easy.h"


extern void EasyMain();

namespace arctic {

struct SystemInfo {
  Si32 screen_width;
  Si32 screen_height;
};

static Si32 g_last_mouse_x = 0;
static Si32 g_last_mouse_y = 0;
Si32 g_window_width = 0;
Si32 g_window_height = 0;
Display *g_x_display;
Window g_x_window;
XIM g_x_im;
XIC g_x_ic;

static Colormap g_x_color_map;
static const int kXEventMask = KeyPressMask | KeyReleaseMask | ButtonPressMask
  | ButtonReleaseMask | PointerMotionMask | ExposureMask
  | StructureNotifyMask;

//static arctic::SoundPlayer g_sound_player;


static EGLint const attribute_list[] = {
  EGL_RED_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_BLUE_SIZE, 8,
  EGL_ALPHA_SIZE, 8,
  EGL_NONE
};

const EGLint context_attributes[] = {
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE
};


EGLDisplay g_egl_display;
EGLSurface g_egl_surface;

KeyCode TranslateKeyCode(const std::string& code) {
    // Define a mapping of code values to KeyCode enums
    std::unordered_map<std::string, KeyCode> code_to_key_code = {
        {"KeyA", kKeyA},
        {"KeyB", kKeyB},
        {"KeyC", kKeyC},
        {"KeyD", kKeyD},
        {"KeyE", kKeyE},
        {"KeyF", kKeyF},
        {"KeyG", kKeyG},
        {"KeyH", kKeyH},
        {"KeyI", kKeyI},
        {"KeyJ", kKeyJ},
        {"KeyK", kKeyK},
        {"KeyL", kKeyL},
        {"KeyM", kKeyM},
        {"KeyN", kKeyN},
        {"KeyO", kKeyO},
        {"KeyP", kKeyP},
        {"KeyQ", kKeyQ},
        {"KeyR", kKeyR},
        {"KeyS", kKeyS},
        {"KeyT", kKeyT},
        {"KeyU", kKeyU},
        {"KeyV", kKeyV},
        {"KeyW", kKeyW},
        {"KeyX", kKeyX},
        {"KeyY", kKeyY},
        {"KeyZ", kKeyZ},
        {"ControlLeft", kKeyControl},
        {"ControlRight", kKeyControl},
        {"AltLeft", kKeyAlt},
        {"AltRight", kKeyAlt},
        //{"MetaLeft", },
        //{"MetaRight", },
        {"Space", kKeySpace},
        {"ArrowLeft", kKeyLeft},
        {"ArrowUp", kKeyUp},
        {"ArrowDown", kKeyDown},
        {"ArrowRight", kKeyRight},
        {"Backquote", kKeyGraveAccent},
        {"BracketLeft", kKeyLeftSquareBracket},
        {"BracketRight", kKeyRightSquareBracket},
        {"Semicolon", kKeySemicolon},
        {"Quote", kKeyApostrophe},
        {"Backspace", kKeyBackspace},
        {"Escape", kKeyEscape},
        {"Enter", kKeyEnter},
        {"Tab", kKeyTab},
        {"ShiftLeft", kKeyShift},
        {"ShiftRight", kKeyShift},
        {"CapsLock", kKeyCapsLock},
        {"Digit1", kKey0},
        {"Digit2", kKey1},
        {"Digit3", kKey2},
        {"Digit4", kKey3},
        {"Digit5", kKey4},
        {"Digit6", kKey5},
        {"Digit7", kKey6},
        {"Digit8", kKey7},
        {"Digit9", kKey8},
        {"Digit0", kKey9},
        {"Minus", kKeyMinus},
        {"Equal", kKeyEquals},
        {"Backslash", kKeyBackslash},
        {"IntlBackslash", kKeySectionSign},
        {"F1", kKeyF1},
        {"F2", kKeyF2},
        {"F3", kKeyF3},
        {"F4", kKeyF4},
        {"F5", kKeyF5},
        {"F6", kKeyF6},
        {"F7", kKeyF7},
        {"F8", kKeyF8},
        {"F9", kKeyF9},
        {"F10", kKeyF10},
        {"F11", kKeyF11},
        {"F12", kKeyF12}};
    auto it = code_to_key_code.find(code);
    if (it != code_to_key_code.end()) {
        return it->second;
    }
    return kKeyUnknown;
};

EM_BOOL KeyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
  KeyCode key_code = TranslateKeyCode(e->code);
  bool is_down = false;
  if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
    is_down = true;
  }
  // Handle key press
  //printf("Key pressed: %s code %s\n", e->key, e->code);
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key_code;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  strncpy(msg.keyboard.characters, e->key, sizeof(msg.keyboard.characters));
  msg.keyboard.characters[sizeof(msg.keyboard.characters) - 1] = '\0';
  PushInputMessage(msg);
  return true;
}

EM_BOOL MouseCallback(int eventType, const EmscriptenMouseEvent* e, void* userData) {
  g_last_mouse_x = e->targetX;
  g_last_mouse_y = e->targetY;  
  if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN || eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
    bool is_down = false;
    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
      is_down = true;
    }
    KeyCode key_code = kKeyUnknown;
    switch (e->button) {
      case 0 : key_code = kKeyMouseLeft; break;
      case 1 : key_code = kKeyMouseWheel; break;
      case 2 : key_code = kKeyMouseRight; break;
      default: break;
    }
    InputMessage msg;
    msg.kind = InputMessage::kKeyboard;
    msg.keyboard.key = key_code;
    msg.keyboard.key_state = (is_down ? 1 : 2);
    msg.keyboard.characters[0] = '\0';
    PushInputMessage(msg);
    return true;
  }
  if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE) {
    Vec2F pos(static_cast<float>(g_last_mouse_x) / static_cast<float>(g_window_width - 1),
      static_cast<float>(g_window_height - g_last_mouse_y) / static_cast<float>(g_window_height - 1));
    InputMessage msg;
    msg.kind = InputMessage::kMouse;
    msg.mouse.pos = pos;
    msg.mouse.wheel_delta = 0;
    PushInputMessage(msg);
    return true;
  }
  return false;
}

EM_BOOL WheelCallback(int eventType, const EmscriptenWheelEvent* e, void* userData) {
  Vec2F pos(static_cast<float>(g_last_mouse_x) / static_cast<float>(g_window_width - 1),
      static_cast<float>(g_window_height - g_last_mouse_y) / static_cast<float>(g_window_height - 1));
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = e->deltaY;
  PushInputMessage(msg);
  return true;
}

void HeadlessPlatformInit() {
}

void CreateMainWindow(SystemInfo *system_info) {
  const char *title = "Arctic Engine";

  g_x_display = XOpenDisplay(NULL);
  Check(g_x_display != NULL, "Can't open display.");

  int width, height;
  emscripten_get_canvas_element_size("#canvas", &width, &height);
  g_window_width = width;
  g_window_height = height;

  XSetWindowAttributes swa;
  swa.colormap = g_x_color_map;
  swa.border_pixel = 0;
  swa.event_mask = kXEventMask;

  Window root = DefaultRootWindow(g_x_display);
  g_x_window = XCreateWindow(g_x_display,
      root,
      0, 0, g_window_width, g_window_height, 0,
      CopyFromParent,
      InputOutput,
      CopyFromParent,
      CWEventMask | CWBorderPixel | CWColormap, &swa);


  system_info->screen_width = g_window_width;
  system_info->screen_height = g_window_height;

  XStoreName(g_x_display, g_x_window, title);

  XMapWindow(g_x_display, g_x_window);

  EGLConfig config = 0;
  EGLContext context = 0;
  EGLint num_config = 0;

  g_egl_display = eglGetDisplay((EGLNativeDisplayType)g_x_display);
  eglInitialize(g_egl_display, NULL, NULL);
  eglChooseConfig(g_egl_display, attribute_list, &config, 1, &num_config);
  Check(num_config == 1, "Error in eglChooseConfig, unexpected num_config.");
  context = eglCreateContext(g_egl_display, config,
      EGL_NO_CONTEXT, context_attributes);
  if (context == EGL_NO_CONTEXT) {
    std::ostringstream info;
    info << "Unable to create EGL context (eglError: "
      << eglGetError() << ")" << std::endl;
    Log(info.str().c_str());
    Check(false, info.str().c_str());
  }
  g_egl_surface = eglCreateWindowSurface(g_egl_display, config,
      (EGLNativeWindowType)g_x_window, NULL);
  if ( g_egl_surface == EGL_NO_SURFACE ) {
    std::ostringstream info;
    info << "Unable to create EGL surface (eglError: "
      << eglGetError() << ")" << std::endl;
    Log(info.str().c_str());
    Check(false, info.str().c_str());
  }
  EGLBoolean mcr = eglMakeCurrent(g_egl_display, g_egl_surface,
      g_egl_surface, context);
  Check(mcr, "Error in eglMakeCurrent");

  glClearColor(1.0F, 1.0F, 1.0F, 0.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  return;
}

void ExitProgram(Si32 exit_code) {
//  arctic::g_sound_player.Deinitialize();
  arctic::StopLogger();

  exit(exit_code);
}

void Swap() {
  glFlush();
  eglSwapBuffers(g_egl_display, g_egl_surface);
  emscripten_sleep(1);
  int width, height;
  emscripten_get_canvas_element_size("#canvas", &width, &height);
  g_window_width = width;
  g_window_height = height;
  arctic::GetEngine()->OnWindowResize(g_window_width, g_window_height);
}

bool IsVSyncSupported() {
  return true;
}

bool SetVSync(bool is_enable) {
  EGLBoolean result = eglSwapInterval(g_egl_display, is_enable ? 1 : 0);
  if (result == EGL_TRUE) {
    return true;
  }
  return false;
}

bool IsFullScreen() {
  return false;
}

void SetFullScreen(bool/* is_enable*/) {
  return;
}

void SetCursorVisible(bool/* is_enable*/) {
  return;
}

std::string PrepareInitialPath() {
  return "";
}

}  // namespace arctic


#ifndef ARCTIC_NO_MAIN
namespace arctic {
  void PrepareForTheEasyMainCall();
}

int main(int argc, char **argv) {
  arctic::SystemInfo system_info;

  std::string initial_path = arctic::PrepareInitialPath();
  arctic::StartLogger();
//  arctic::g_sound_player.Initialize();
  CreateMainWindow(&system_info);
  arctic::GetEngine()->SetArgcArgv(argc,
    const_cast<const char **>(argv));

  arctic::GetEngine()->SetInitialPath(initial_path);
  arctic::GetEngine()->Init(system_info.screen_width,
    system_info.screen_height);

  // Set up event listeners
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, arctic::KeyCallback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, arctic::KeyCallback);

  emscripten_set_mousedown_callback("#canvas", nullptr, true, arctic::MouseCallback);
  emscripten_set_mouseup_callback("#canvas", nullptr, true, arctic::MouseCallback);
  emscripten_set_mousemove_callback("#canvas", nullptr, true, arctic::MouseCallback);
  
  emscripten_set_wheel_callback("#canvas", nullptr, true, arctic::WheelCallback);

  arctic::PrepareForTheEasyMainCall();
  EasyMain();

//  arctic::g_sound_player.Deinitialize();
  arctic::StopLogger();

  return 0;
}
#endif  // ARCTIC_NO_MAIN

#endif  // defined(ARCTIC_PLATFORM_WEB)
