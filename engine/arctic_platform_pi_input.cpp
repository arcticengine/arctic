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

static Si32 g_last_mouse_x = 0;
static Si32 g_last_mouse_y = 0;

extern Display *g_x_display;
extern Si32 g_window_width;
extern Si32 g_window_height;
extern Display *g_x_display;
extern Window g_x_window;

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

}  // namespace arctic

#endif  // ARCTIC_PLATFORM_PI
