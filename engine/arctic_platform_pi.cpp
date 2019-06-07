// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#ifdef ARCTIC_PLATFORM_PI

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include "engine/easy.h"
#include "engine/arctic_platform.h"

extern void EasyMain();

namespace arctic {

static int glx_config[] = {
  GLX_DOUBLEBUFFER,
  GLX_RGBA,
  GLX_DEPTH_SIZE,
  16,
  None
};

struct SystemInfo {
  Si32 screen_width;
  Si32 screen_height;
};

static Si32 g_window_width = 0;
static Si32 g_window_height = 0;
static Si32 g_last_mouse_x = 0;
static Si32 g_last_mouse_y = 0;
static Display *g_x_display;
static Window g_x_window;
static Colormap g_x_color_map;
static XVisualInfo *g_glx_visual;
static const int kXEventMask = KeyPressMask | KeyReleaseMask | ButtonPressMask
  | ButtonReleaseMask | PointerMotionMask | ExposureMask
  | StructureNotifyMask;
static GLXContext g_glx_context;

KeyCode TranslateKeyCode(KeySym ks) {
  if (ks >= XK_a && ks <= XK_z) {
    ks = ks + XK_A - XK_a;
  }
  switch (ks) {
    case XK_Left:
      return kKeyLeft;
    case XK_Right:
      return kKeyRight;
    case XK_Up:
      return kKeyUp;
    case XK_Down:
      return kKeyDown;
    case XK_BackSpace:
      return kKeyBackspace;
    case XK_Tab:
      return kKeyTab;

    case XK_Return:
      return kKeyEnter;
    case XK_Home:
      return kKeyHome;
    case XK_End:
      return kKeyEnd;
    case XK_Page_Up:
      return kKeyPageUp;
    case XK_Page_Down:
      return kKeyPageDown;

    case XK_Shift_L:
      return kKeyLeftShift;
    case XK_Shift_R:
      return kKeyRightShift;
    case XK_Control_L:
      return kKeyLeftControl;
    case XK_Control_R:
      return kKeyRightControl;
    case XK_Alt_L:
      return kKeyLeftAlt;
    case XK_Alt_R:
      return kKeyRightAlt;
    case XK_Escape:
      return kKeyEscape;

    case XK_space:
      return kKeySpace;

    case XK_apostrophe:
      return kKeyApostrophe;

    case XK_comma:
      return kKeyComma;
    case XK_minus:
      return kKeyMinus;
    case XK_period:
      return kKeyPeriod;
    case XK_slash:
      return kKeySlash;
    case XK_0:
      return kKey0;
    case XK_1:
      return kKey1;
    case XK_2:
      return kKey2;
    case XK_3:
      return kKey3;
    case XK_4:
      return kKey4;
    case XK_5:
      return kKey5;
    case XK_6:
      return kKey6;
    case XK_7:
      return kKey7;
    case XK_8:
      return kKey8;
    case XK_9:
      return kKey9;

    case XK_semicolon:
      return kKeySemicolon;
    case XK_Cancel:
      return kKeyPause;
    case XK_equal:
      return kKeyEquals;
    case XK_Num_Lock:
      return kKeyNumLock;
    case XK_Scroll_Lock:
      return kKeyScrollLock;
    case XK_Caps_Lock:
      return kKeyCapsLock;
    case XK_A:
      return kKeyA;
    case XK_B:
      return kKeyB;
    case XK_C:
      return kKeyC;
    case XK_D:
      return kKeyD;
    case XK_E:
      return kKeyE;
    case XK_F:
      return kKeyF;
    case XK_G:
      return kKeyG;
    case XK_H:
      return kKeyH;
    case XK_I:
      return kKeyI;
    case XK_J:
      return kKeyJ;
    case XK_K:
      return kKeyK;
    case XK_L:
      return kKeyL;
    case XK_M:
      return kKeyM;
    case XK_N:
      return kKeyN;
    case XK_O:
      return kKeyO;
    case XK_P:
      return kKeyP;
    case XK_Q:
      return kKeyQ;
    case XK_R:
      return kKeyR;
    case XK_S:
      return kKeyS;
    case XK_T:
      return kKeyT;
    case XK_U:
      return kKeyU;
    case XK_V:
      return kKeyV;
    case XK_W:
      return kKeyW;
    case XK_X:
      return kKeyX;
    case XK_Y:
      return kKeyY;
    case XK_Z:
      return kKeyZ;
    case XK_bracketleft:
      return kKeyLeftSquareBracket;
    case XK_backslash:
      return kKeyBackslash;
    case XK_bracketright:
      return kKeyRightSquareBracket;

    case XK_dead_grave:
      return kKeyGraveAccent;
    case XK_F1:
      return kKeyF1;
    case XK_F2:
      return kKeyF2;
    case XK_F3:
      return kKeyF3;
    case XK_F4:
      return kKeyF4;
    case XK_F5:
      return kKeyF5;
    case XK_F6:
      return kKeyF6;
    case XK_F7:
      return kKeyF7;
    case XK_F8:
      return kKeyF8;
    case XK_F9:
      return kKeyF9;
    case XK_F10:
      return kKeyF10;
    case XK_F11:
      return kKeyF11;
    case XK_F12:
      return kKeyF12;

    case XK_KP_0:
      return kKeyNumpad0;
    case XK_KP_1:
      return kKeyNumpad1;
    case XK_KP_2:
      return kKeyNumpad2;
    case XK_KP_3:
      return kKeyNumpad3;
    case XK_KP_4:
      return kKeyNumpad4;
    case XK_KP_5:
      return kKeyNumpad5;
    case XK_KP_6:
      return kKeyNumpad6;
    case XK_KP_7:
      return kKeyNumpad7;
    case XK_KP_8:
      return kKeyNumpad8;
    case XK_KP_9:
      return kKeyNumpad9;
    case XK_KP_Divide:
      return kKeyNumpadSlash;
    case XK_KP_Multiply:
      return kKeyNumpadAsterisk;
    case XK_KP_Subtract:
      return kKeyNumpadMinus;
    case XK_KP_Add:
      return kKeyNumpadPlus;
    case XK_KP_Decimal:
      return kKeyNumpadPeriod;
    case XK_Print:
      return kKeyPrintScreen;
    case XK_KP_Enter:
      return kKeyEnter;
    case XK_Insert:
      return kKeyInsert;
    case XK_Delete:
      return kKeyDelete;
    case XK_section:
      return kKeySectionSign;
  }
  return kKeyUnknown;
}


void OnMouse(KeyCode key, Si32 mouse_x, Si32 mouse_y, bool is_down) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouse");
  Check(g_window_height != 0, "Could not obtain window height in OnMouse");
  g_last_mouse_x = mouse_x;
  g_last_mouse_y = mouse_y;
  Si32 x = mouse_x;
  Si32 y = g_window_height - mouse_y;
  Vec2F pos(static_cast<float>(x) / static_cast<float>(g_window_width - 1),
      static_cast<float>(y) / static_cast<float>(g_window_height - 1));
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = 0;
  PushInputMessage(msg);
}

void OnMouseWheel(bool is_down) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouseWheel");
  Check(g_window_height != 0,
      "Could not obtain window height in OnMouseWheel");

  Si32 z_delta = is_down ? -1 : 1;

  Si32 x = g_last_mouse_x;
  Si32 y = g_window_height - g_last_mouse_y;

  Vec2F pos(static_cast<float>(x) / static_cast<float>(g_window_width - 1),
      static_cast<float>(y) / static_cast<float>(g_window_height - 1));
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = kKeyCount;
  msg.keyboard.key_state = false;
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = z_delta;
  PushInputMessage(msg);
}

void OnKey(KeyCode key, bool is_down) {
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  PushInputMessage(msg);
}


void PumpMessages() {
  XEvent ev;
  while (True == XCheckWindowEvent(g_x_display, g_x_window,
        KeyPressMask | KeyReleaseMask, &ev)) {
    KeySym ks = XkbKeycodeToKeysym(g_x_display, ev.xkey.keycode, 0, 0);
    if (ks) {
      arctic::KeyCode key = TranslateKeyCode(ks);
      if (key == kKeyUnknown) {
        ::KeyCode kcode = XKeysymToKeycode(g_x_display, ks);
        if (kcode != 0) {
          ks = XkbKeycodeToKeysym(g_x_display, kcode, 0, 0);
          key = TranslateKeyCode(ks);
          // std::cerr << "ks: " << ks << " key: " << key << std::endl;
        }
      }
      bool is_down = (ev.type == KeyPress);
      OnKey(key, is_down);
    }
  }

  while (True == XCheckWindowEvent(g_x_display, g_x_window,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &ev)) {
    if (ButtonPress == ev.type || ButtonRelease == ev.type) {
      if (ev.xbutton.button == Button4) {
        arctic::OnMouseWheel(false);  // up
      } else if (ev.xbutton.button == Button5) {
        arctic::OnMouseWheel(true);  // down
      } else {
        arctic::KeyCode key_code = kKeyCount;
        bool is_down = false;
        if (ev.type == ButtonPress) {
          switch (ev.xbutton.button) {
            case Button1:
              key_code = kKeyMouseLeft;
              is_down = true;
              break;
            case Button2:
              key_code = kKeyMouseWheel;
              is_down = true;
              break;
            case Button3:
              key_code = kKeyMouseRight;
              is_down = true;
              break;
          }
        } else if (ev.type == ButtonRelease) {
          switch (ev.xbutton.button) {
            case Button1:
              key_code = kKeyMouseLeft;
              break;
            case Button2:
              key_code = kKeyMouseWheel;
              break;
            case Button3:
              key_code = kKeyMouseRight;
              break;
          }
        }
        arctic::OnMouse(key_code, ev.xbutton.x, ev.xbutton.y, is_down);
      }
    } else if (ev.type == MotionNotify) {
      arctic::OnMouse(kKeyCount, ev.xbutton.x, ev.xbutton.y, false);
    }
  }

  if (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, ConfigureNotify, &ev)) {
    g_window_width = ev.xconfigure.width;
    g_window_height = ev.xconfigure.height;
  }

  if (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, DestroyNotify, &ev)) {
    exit(0);
  }

  return;
}


void CreateMainWindow(SystemInfo *system_info) {
  const char *title = "Arctic Engine";

  g_x_display = XOpenDisplay(NULL);
  Check(g_x_display != NULL, "Can't open display.");

  XWindowAttributes window_attributes;
  Status is_good = XGetWindowAttributes(g_x_display,
      RootWindow(g_x_display, DefaultScreen(g_x_display)),
      &window_attributes);
  Check(is_good != 0, "Can't get window attributes.");
  g_window_width = window_attributes.width;
  g_window_height = window_attributes.height;

  Bool is_ok = glXQueryExtension(g_x_display, NULL, NULL);
  Check(is_ok, "Can't find OpenGL via glXQueryExtension.");

  g_glx_visual = glXChooseVisual(g_x_display,
      DefaultScreen(g_x_display), glx_config);
  Check(g_glx_visual != NULL, "Can't choose visual via glXChooseVisual.");

  g_x_color_map = XCreateColormap(g_x_display,
      RootWindow(g_x_display, g_glx_visual->screen),
      g_glx_visual->visual,
      AllocNone);

  g_glx_context = glXCreateContext(
      g_x_display, g_glx_visual, None, GL_TRUE);
  Check(g_glx_context != NULL, "Can't create context via glXCreateContext.");

  XSetWindowAttributes swa;
  swa.colormap = g_x_color_map;
  swa.border_pixel = 0;
  swa.event_mask = kXEventMask;

  g_x_window = XCreateWindow(g_x_display,
      RootWindow(g_x_display, g_glx_visual->screen),
      0, 0, g_window_width, g_window_height,
      1,
      g_glx_visual->depth,
      InputOutput,
      g_glx_visual->visual,
      CWEventMask | CWBorderPixel | CWColormap, &swa);


  system_info->screen_width = g_window_width;
  system_info->screen_height = g_window_height;

  XStoreName(g_x_display, g_x_window, title);

  XWMHints wmHints;
  wmHints.flags = 0;
  wmHints.initial_state = NormalState;
  XSetWMHints(g_x_display, g_x_window, &wmHints);

  XSetIconName(g_x_display, g_x_window, title);
  XMapWindow(g_x_display, g_x_window);

  glXMakeCurrent(g_x_display, g_x_window, g_glx_context);

  glClearColor(1.0F, 1.0F, 1.0F, 0.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  return;
}

void ExitProgram() {
  exit(0);
}

void Swap() {
  glFlush();
  glXSwapBuffers(g_x_display, g_x_window);
  PumpMessages();
  arctic::easy::GetEngine()->OnWindowResize(g_window_width, g_window_height);
}

bool IsVSyncSupported() {
  const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
  if (strstr(extensions, "GLX_SGI_swap_control") == nullptr) {
    return false;
  }
  return true;
}

bool SetVSync(bool is_enable) {
  if (!IsVSyncSupported()) {
    return false;
  }
  PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
    (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddress(
        (const GLubyte*)"glXSwapIntervalSGI");
  if (glXSwapIntervalSGI != NULL) {
    glXSwapIntervalSGI(is_enable ? 1 : 0);
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

Trivalent DoesDirectoryExist(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return kTrivalentFalse;
  } else if (info.st_mode & S_IFDIR) {
    return kTrivalentTrue;
  } else {
    return kTrivalentUnknown;
  }
}

bool MakeDirectory(const char *path) {
  Si32 result = mkdir(path,
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
  return (result == 0);
}

bool GetCurrentPath(std::string *out_dir) {
  char cwd[1 << 20];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    out_dir->assign(cwd);
    return true;
  }
  return false;
}
  
bool GetDirectoryEntries(const char *path,
     std::deque<DirectoryEntry> *out_entries) {
  Check(out_entries,
    "GetDirectoryEntries Error. Unexpected nullptr in out_entries!");
  out_entries->clear();
  DIR *dir = opendir(path);
  if (dir == nullptr) {
    std::stringstream info;
    info << "Error errno: " << errno
      << " while opening path: \"" << path << "\"" << std::endl;
    Log(info.str().c_str());
    return false;
  }
  char full_path[1 << 20];
  while (true) {
    struct dirent *dir_entry = readdir(dir);
    if (dir_entry == nullptr) {
      break;
    }
    DirectoryEntry entry;
    entry.title = dir_entry->d_name;
    sprintf(full_path, "%s/%s", path, dir_entry->d_name);
    struct stat info;
    if (stat(full_path, &info) != 0) {
      return false;
    }
    if (info.st_mode & S_IFDIR) {
      entry.is_directory = kTrivalentTrue;
    }
    if (info.st_mode & S_IFREG) {
      entry.is_file = kTrivalentTrue;
    }
    out_entries->push_back(entry);
  }
  closedir(dir);
  return true;
}

  
std::string CanonicalizePath(const char *path) {
  Check(path, "CanonicalizePath error, path can't be nullptr");
  char *canonic_path = realpath(path, nullptr);
  std::string result;
  if (canonic_path) {
    result.assign(canonic_path);
    free(canonic_path);
  }
  return result;
}

std::string RelativePathFromTo(const char *from, const char *to) {
  std::string from_abs = CanonicalizePath(from);
  if (from && from[strlen(from) - 1] == '/' &&
      from_abs.size() && from_abs[from_abs.size() - 1] != '/') {
    from_abs = from_abs + '/';
  }
  std::string to_abs = CanonicalizePath(to);
  if (to && to[strlen(to) - 1] == '/' &&
      to_abs.size() && to_abs[to_abs.size() - 1] != '/') {
    to_abs = to_abs + '/';
  }
  Ui32 matching = 0;
  while (matching < from_abs.size() && matching < to_abs.size()) {
    if (from_abs[matching] == to_abs[matching]) {
      ++matching;
    } else {
      break;
    }
  }
  if (matching == from_abs.size() && matching == to_abs.size()) {
    return "./";
  }
  while (matching && from_abs[matching - 1] != '/') {
    --matching;
  }
  const char *from_part = from_abs.c_str() + matching;
  std::stringstream res;
  while (*from_part != 0) {
    res << "../";
    ++from_part;
    while (*from_part != 0 && *from_part != '/') {
      ++from_part;
    }
  }
  const char *to_part = to_abs.c_str() + matching;
  res << to_part;
  return res.str();
}


}  // namespace arctic


int main() {
  arctic::SystemInfo system_info;

  arctic::StartLogger();
  arctic::SoundPlayer soundPlayer;
  soundPlayer.Initialize();
  CreateMainWindow(&system_info);
  arctic::easy::GetEngine();
  arctic::easy::GetEngine()->Init(system_info.screen_width,
      system_info.screen_height);

  EasyMain();

  XCloseDisplay(arctic::g_x_display);
  soundPlayer.Deinitialize();
  arctic::StopLogger();

  return 0;
}

#endif  // ARCTIC_PLATFORM_PI
