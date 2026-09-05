// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#if defined(ARCTIC_PLATFORM_PI)

#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include "engine/easy.h"

extern void EasyMain();

namespace arctic {

static Si32 g_last_mouse_x = 0;
static Si32 g_last_mouse_y = 0;
static bool g_is_mouse_captured = false;
static Cursor g_invisible_cursor = None;

extern Display *g_x_display;
extern Si32 g_window_width;
extern Si32 g_window_height;
extern Window g_x_window;
extern XIM g_x_im;
extern XIC g_x_ic;
extern Atom g_x_wm_delete_window;

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
    case XK_KP_Insert:
      return kKeyNumpad0;
    case XK_KP_1:
    case XK_KP_End:
      return kKeyNumpad1;
    case XK_KP_2:
    case XK_KP_Down:
      return kKeyNumpad2;
    case XK_KP_3:
    case XK_KP_Page_Down:
      return kKeyNumpad3;
    case XK_KP_4:
    case XK_KP_Left:
      return kKeyNumpad4;
    case XK_KP_5:
    case XK_KP_Begin:
      return kKeyNumpad5;
    case XK_KP_6:
    case XK_KP_Right:
      return kKeyNumpad6;
    case XK_KP_7:
    case XK_KP_Home:
      return kKeyNumpad7;
    case XK_KP_8:
    case XK_KP_Up:
      return kKeyNumpad8;
    case XK_KP_9:
    case XK_KP_Page_Up:
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

/// @brief Translates an XKB key name into a key code
/// @param name Four characters of an XkbKeyNameRec, not zero terminated
/// @return The key code of the physical key, kKeyUnknown for an unnamed one
///
/// XKB names every physical key the same way no matter which layout is loaded
/// on top of it: "AC01" is the leftmost letter key of the home row whether it
/// prints "a", "q" or "ф". That makes the names, and not the keysyms, the thing
/// to look at when the engine has to report a key by position.
static KeyCode TranslateXkbKeyName(const char *name) {
  struct NamedKey {
    const char *name;
    KeyCode key;
  };
  static const NamedKey kNamedKeys[] = {
    {"ESC\0", kKeyEscape}, {"TLDE", kKeyGraveAccent},
    {"AE01", kKey1}, {"AE02", kKey2}, {"AE03", kKey3}, {"AE04", kKey4},
    {"AE05", kKey5}, {"AE06", kKey6}, {"AE07", kKey7}, {"AE08", kKey8},
    {"AE09", kKey9}, {"AE10", kKey0}, {"AE11", kKeyMinus}, {"AE12", kKeyEquals},
    {"BKSP", kKeyBackspace}, {"TAB\0", kKeyTab},
    {"AD01", kKeyQ}, {"AD02", kKeyW}, {"AD03", kKeyE}, {"AD04", kKeyR},
    {"AD05", kKeyT}, {"AD06", kKeyY}, {"AD07", kKeyU}, {"AD08", kKeyI},
    {"AD09", kKeyO}, {"AD10", kKeyP}, {"AD11", kKeyLeftSquareBracket},
    {"AD12", kKeyRightSquareBracket}, {"BKSL", kKeyBackslash},
    {"CAPS", kKeyCapsLock},
    {"AC01", kKeyA}, {"AC02", kKeyS}, {"AC03", kKeyD}, {"AC04", kKeyF},
    {"AC05", kKeyG}, {"AC06", kKeyH}, {"AC07", kKeyJ}, {"AC08", kKeyK},
    {"AC09", kKeyL}, {"AC10", kKeySemicolon}, {"AC11", kKeyApostrophe},
    {"RTRN", kKeyEnter},
    {"LFSH", kKeyLeftShift},
    {"AB01", kKeyZ}, {"AB02", kKeyX}, {"AB03", kKeyC}, {"AB04", kKeyV},
    {"AB05", kKeyB}, {"AB06", kKeyN}, {"AB07", kKeyM}, {"AB08", kKeyComma},
    {"AB09", kKeyPeriod}, {"AB10", kKeySlash}, {"RTSH", kKeyRightShift},
    {"LCTL", kKeyLeftControl}, {"RCTL", kKeyRightControl},
    {"LALT", kKeyLeftAlt}, {"RALT", kKeyRightAlt}, {"SPCE", kKeySpace},
    {"FK01", kKeyF1}, {"FK02", kKeyF2}, {"FK03", kKeyF3}, {"FK04", kKeyF4},
    {"FK05", kKeyF5}, {"FK06", kKeyF6}, {"FK07", kKeyF7}, {"FK08", kKeyF8},
    {"FK09", kKeyF9}, {"FK10", kKeyF10}, {"FK11", kKeyF11}, {"FK12", kKeyF12},
    {"PRSC", kKeyPrintScreen}, {"SCLK", kKeyScrollLock}, {"PAUS", kKeyPause},
    {"INS\0", kKeyInsert}, {"HOME", kKeyHome}, {"PGUP", kKeyPageUp},
    {"DELE", kKeyDelete}, {"END\0", kKeyEnd}, {"PGDN", kKeyPageDown},
    {"UP\0\0", kKeyUp}, {"LEFT", kKeyLeft}, {"DOWN", kKeyDown},
    {"RGHT", kKeyRight},
    {"NMLK", kKeyNumLock}, {"KPDV", kKeyNumpadSlash},
    {"KPMU", kKeyNumpadAsterisk}, {"KPSU", kKeyNumpadMinus},
    {"KPAD", kKeyNumpadPlus}, {"KPEN", kKeyEnter}, {"KPDL", kKeyNumpadPeriod},
    {"KP0\0", kKeyNumpad0}, {"KP1\0", kKeyNumpad1}, {"KP2\0", kKeyNumpad2},
    {"KP3\0", kKeyNumpad3}, {"KP4\0", kKeyNumpad4}, {"KP5\0", kKeyNumpad5},
    {"KP6\0", kKeyNumpad6}, {"KP7\0", kKeyNumpad7}, {"KP8\0", kKeyNumpad8},
    {"KP9\0", kKeyNumpad9}
  };
  for (const NamedKey &named : kNamedKeys) {
    if (strncmp(name, named.name, 4) == 0) {
      return named.key;
    }
  }
  return kKeyUnknown;
}

/// @brief Resolves the physical key that an X11 keycode belongs to
/// @param x_keycode Keycode of an XKeyEvent
/// @return The key code, kKeyUnknown when neither names nor keysyms describe it
///
/// The XKB key names come first because they do not depend on the layout at
/// all. Only when a key has no name the keysyms are searched, and every group
/// is looked at rather than group 0 alone: a machine whose only layout is
/// Cyrillic has no Latin keysym in group 0, which used to make WASD unusable.
static KeyCode TranslatePhysicalKeyCode(unsigned int x_keycode) {
  static XkbDescPtr xkb_names = nullptr;
  static bool is_names_requested = false;
  if (!is_names_requested) {
    is_names_requested = true;
    xkb_names = XkbGetMap(g_x_display, 0, XkbUseCoreKbd);
    if (xkb_names != nullptr) {
      if (XkbGetNames(g_x_display, XkbKeyNamesMask, xkb_names) != Success) {
        XkbFreeKeyboard(xkb_names, 0, True);
        xkb_names = nullptr;
      }
    }
  }
  if (xkb_names != nullptr && xkb_names->names != nullptr
      && xkb_names->names->keys != nullptr
      && x_keycode >= static_cast<unsigned int>(xkb_names->min_key_code)
      && x_keycode <= static_cast<unsigned int>(xkb_names->max_key_code)) {
    KeyCode key = TranslateXkbKeyName(xkb_names->names->keys[x_keycode].name);
    if (key != kKeyUnknown) {
      return key;
    }
  }
  for (int level = 0; level < 2; ++level) {
    for (int group = 0; group < 4; ++group) {
      KeySym ks = XkbKeycodeToKeysym(g_x_display,
          static_cast<::KeyCode>(x_keycode), group, level);
      if (ks == NoSymbol) {
        continue;
      }
      KeyCode key = TranslateKeyCode(ks);
      if (key != kKeyUnknown) {
        return key;
      }
    }
  }
  return kKeyUnknown;
}

void OnMouse(KeyCode key, Si32 mouse_x, Si32 mouse_y, bool is_down) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouse");
  Check(g_window_height != 0, "Could not obtain window height in OnMouse");
  Si32 x = mouse_x;
  Si32 y = g_window_height - mouse_y;
  Vec2F pos(0.f, 0.f);
  if (g_window_width > 1) {
    pos.x = static_cast<float>(x) / static_cast<float>(g_window_width - 1);
  }
  if (g_window_height > 1) {
    pos.y = static_cast<float>(y) / static_cast<float>(g_window_height - 1);
  }
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = 0;
  msg.mouse.delta = Vec2F(
    static_cast<float>(mouse_x - g_last_mouse_x),
    -static_cast<float>(mouse_y - g_last_mouse_y));
  g_last_mouse_x = mouse_x;
  g_last_mouse_y = mouse_y;
  if (g_is_mouse_captured) {
    Si32 cx = g_window_width / 2;
    Si32 cy = g_window_height / 2;
    XWarpPointer(g_x_display, None, g_x_window, 0, 0, 0, 0, cx, cy);
    g_last_mouse_x = cx;
    g_last_mouse_y = cy;
  }
  PushInputMessage(msg);
}

void OnMouseWheel(bool is_down) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouseWheel");
  Check(g_window_height != 0,
      "Could not obtain window height in OnMouseWheel");

  Si32 z_delta = is_down ? -1 : 1;

  Si32 x = g_last_mouse_x;
  Si32 y = g_window_height - g_last_mouse_y;

  Vec2F pos(0.f, 0.f);
  if (g_window_width > 1) {
    pos.x = static_cast<float>(x) / static_cast<float>(g_window_width - 1);
  }
  if (g_window_height > 1) {
    pos.y = static_cast<float>(y) / static_cast<float>(g_window_height - 1);
  }
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = kKeyNone;
  msg.keyboard.key_state = false;
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = z_delta;
  PushInputMessage(msg);
}

void OnMouseHWheel(bool is_right) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouseHWheel");
  Check(g_window_height != 0,
      "Could not obtain window height in OnMouseHWheel");

  Si32 z_delta = is_right ? 1 : -1;

  Si32 x = g_last_mouse_x;
  Si32 y = g_window_height - g_last_mouse_y;

  Vec2F pos(0.f, 0.f);
  if (g_window_width > 1) {
    pos.x = static_cast<float>(x) / static_cast<float>(g_window_width - 1);
  }
  if (g_window_height > 1) {
    pos.y = static_cast<float>(y) / static_cast<float>(g_window_height - 1);
  }
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = kKeyNone;
  msg.keyboard.key_state = false;
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta_x = z_delta;
  PushInputMessage(msg);
}

void OnKey(KeyCode key, bool is_down, char *characters) {
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  // Xutf8LookupString answers "\t" for Tab, "\r" for Enter and a C0 code for
  // Control with a letter, and none of that is text; SetTypedCharacters leaves it
  // out, the same way as on the other platforms.
  SetTypedCharacters(&msg.keyboard, characters);
  PushInputMessage(msg);
}

// ---- Clipboard (X11 CLIPBOARD selection) ----
// The owner of the CLIPBOARD selection keeps the text in g_clipboard_text and
// serves it to other clients on demand via SelectionRequest events handled in
// PumpMessages. Reading pulls the text from whoever currently owns CLIPBOARD.
static std::string g_clipboard_text;

static Atom ClipboardSelectionAtom() {
  static Atom a = None;
  if (a == None) {
    a = XInternAtom(g_x_display, "CLIPBOARD", False);
  }
  return a;
}

static Atom Utf8StringAtom() {
  static Atom a = None;
  if (a == None) {
    a = XInternAtom(g_x_display, "UTF8_STRING", False);
  }
  return a;
}

static Atom TargetsAtom() {
  static Atom a = None;
  if (a == None) {
    a = XInternAtom(g_x_display, "TARGETS", False);
  }
  return a;
}

static Atom ClipboardRecvPropAtom() {
  static Atom a = None;
  if (a == None) {
    a = XInternAtom(g_x_display, "ARCTIC_CLIPBOARD_RECV", False);
  }
  return a;
}

static void HandleSelectionRequest(const XSelectionRequestEvent &req) {
  // When a requestor passes property None it is an obsolete client; by
  // convention the owner uses the target atom as the destination property.
  Atom property = (req.property == None) ? req.target : req.property;

  XSelectionEvent resp;
  memset(&resp, 0, sizeof(resp));
  resp.type = SelectionNotify;
  resp.display = req.display;
  resp.requestor = req.requestor;
  resp.selection = req.selection;
  resp.target = req.target;
  resp.time = req.time;
  resp.property = property;

  Atom targets = TargetsAtom();
  Atom utf8 = Utf8StringAtom();
  if (req.target == targets) {
    Atom supported[] = {targets, utf8, XA_STRING};
    XChangeProperty(req.display, req.requestor, property,
        XA_ATOM, 32, PropModeReplace,
        reinterpret_cast<unsigned char*>(supported),
        sizeof(supported) / sizeof(supported[0]));
  } else if (req.target == utf8 || req.target == XA_STRING) {
    XChangeProperty(req.display, req.requestor, property,
        req.target, 8, PropModeReplace,
        reinterpret_cast<const unsigned char*>(g_clipboard_text.data()),
        static_cast<int>(g_clipboard_text.size()));
  } else {
    resp.property = None;  // unsupported target
  }

  XSendEvent(req.display, req.requestor, False, 0,
      reinterpret_cast<XEvent*>(&resp));
  XFlush(req.display);
}

void SetClipboardText(const std::string &text) {
  g_clipboard_text = text;
  if (g_x_display && g_x_window) {
    XSetSelectionOwner(g_x_display, ClipboardSelectionAtom(),
        g_x_window, CurrentTime);
    XFlush(g_x_display);
  }
}

std::string GetClipboardText() {
  if (!g_x_display || !g_x_window) {
    return g_clipboard_text;
  }
  Atom clip = ClipboardSelectionAtom();
  Window owner = XGetSelectionOwner(g_x_display, clip);
  if (owner == None) {
    return std::string();
  }
  if (owner == g_x_window) {
    return g_clipboard_text;  // we own the selection
  }

  Atom prop = ClipboardRecvPropAtom();
  XConvertSelection(g_x_display, clip, Utf8StringAtom(), prop,
      g_x_window, CurrentTime);
  XFlush(g_x_display);

  // Wait (bounded to ~1s) for the owner to answer with SelectionNotify.
  XEvent ev;
  bool got = false;
  for (int i = 0; i < 200; ++i) {
    if (True == XCheckTypedWindowEvent(g_x_display, g_x_window,
          SelectionNotify, &ev)) {
      got = true;
      break;
    }
    usleep(5000);
  }
  if (!got || ev.xselection.property == None) {
    return std::string();
  }

  Atom actual_type = None;
  int actual_format = 0;
  unsigned long nitems = 0;
  unsigned long bytes_after = 0;
  unsigned char *data = nullptr;
  // delete=True removes the property once read. INCR (huge transfers) is not
  // supported; ordinary clipboard text fits in a single property.
  if (Success != XGetWindowProperty(g_x_display, g_x_window, prop, 0, (~0L),
        True, AnyPropertyType, &actual_type, &actual_format,
        &nitems, &bytes_after, &data)) {
    return std::string();
  }
  std::string result;
  if (data) {
    result.assign(reinterpret_cast<char*>(data), nitems);
    XFree(data);
  }
  return result;
}

void PumpMessages() {
  XEvent ev;
  while (True == XCheckWindowEvent(g_x_display, g_x_window,
        KeyPressMask | KeyReleaseMask, &ev)) {
    arctic::KeyCode key = TranslatePhysicalKeyCode(ev.xkey.keycode);
    bool is_down = (ev.type == KeyPress);
    Status status = 0;
    KeySym keysym = 0;
    char buf[20];
    memset(buf, 0, sizeof(buf));
    // The typed text is a separate matter from the key: it is whatever the
    // current layout and the input method make of the event, so a Cyrillic
    // layout gives a Cyrillic character here while the key stays kKeyA.
    int count = Xutf8LookupString(g_x_ic,
        reinterpret_cast<XKeyPressedEvent*>(&ev),
        buf, 20, &keysym, &status);
    buf[std::min(count, 19)] = '\0';
    OnKey(key, is_down, buf);
  }

  while (True == XCheckWindowEvent(g_x_display, g_x_window,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &ev)) {
    if (ButtonPress == ev.type || ButtonRelease == ev.type) {
      if (ev.xbutton.button == Button4) {
        arctic::OnMouseWheel(false);  // up
      } else if (ev.xbutton.button == Button5) {
        arctic::OnMouseWheel(true);  // down
      } else if (ev.xbutton.button == 6) {
        arctic::OnMouseHWheel(false);  // left
      } else if (ev.xbutton.button == 7) {
        arctic::OnMouseHWheel(true);   // right
      } else {
        arctic::KeyCode key_code = kKeyNone;
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
      arctic::OnMouse(kKeyNone, ev.xbutton.x, ev.xbutton.y, false);
    }
  }

  if (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, ConfigureNotify, &ev)) {
    g_window_width = ev.xconfigure.width;
    g_window_height = ev.xconfigure.height;
  }

  while (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, ClientMessage, &ev)) {
    if (g_x_wm_delete_window != None
        && ev.xclient.data.l[0] == static_cast<long>(g_x_wm_delete_window)) {
      if (arctic::OnMainWindowCloseRequested()) {
        arctic::ExitProgram(0);
      }
    }
  }

  if (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, DestroyNotify, &ev)) {
    arctic::OnMainWindowCloseRequested();
    arctic::ExitProgram(0);
  }

  // Serve clipboard contents to other clients while we own the selection.
  while (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, SelectionRequest, &ev)) {
    HandleSelectionRequest(ev.xselectionrequest);
  }
  // Another client took over the clipboard; we simply stop serving it.
  if (True == XCheckTypedWindowEvent(
        g_x_display, g_x_window, SelectionClear, &ev)) {
    // Ownership lost; g_clipboard_text is no longer authoritative.
  }

  return;
}

void CaptureMouse() {
  g_is_mouse_captured = true;
  Si32 cx = g_window_width / 2;
  Si32 cy = g_window_height / 2;
  XWarpPointer(g_x_display, None, g_x_window, 0, 0, 0, 0, cx, cy);
  g_last_mouse_x = cx;
  g_last_mouse_y = cy;
  if (g_invisible_cursor == None) {
    Pixmap blank = XCreatePixmap(g_x_display, g_x_window, 1, 1, 1);
    XColor dummy;
    memset(&dummy, 0, sizeof(dummy));
    g_invisible_cursor = XCreatePixmapCursor(
        g_x_display, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(g_x_display, blank);
  }
  XDefineCursor(g_x_display, g_x_window, g_invisible_cursor);
  XFlush(g_x_display);
}

void ReleaseMouse() {
  g_is_mouse_captured = false;
  XUndefineCursor(g_x_display, g_x_window);
  XFlush(g_x_display);
}

bool IsMouseCaptured() {
  return g_is_mouse_captured;
}

}  // namespace arctic

#endif  // defined(ARCTIC_PLATFORM_PI) 
