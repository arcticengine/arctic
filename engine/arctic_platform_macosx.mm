// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2022 Huldra
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

#ifdef ARCTIC_PLATFORM_MACOSX


#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#import <AppKit/AppKit.h>
#import <CoreText/CoreText.h>
#import <OpenGL/OpenGL.h>
#import <GameController/GameController.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cmath>
#include <map>
#include <memory>
#include <sstream>
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/easy_advanced.h"
#include "engine/arctic_input.h"
#include "engine/arctic_mixer.h"
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"

extern void EasyMain();

#ifdef ARCTIC_NO_HARD_EXIT
#include <setjmp.h>
extern jmp_buf arctic_jmp_env;
#endif  // ARCTIC_NO_HARD_EXIT

namespace arctic {
  struct SystemInfo {
    Si32 screen_width;
    Si32 screen_height;
  };
  KeyCode TranslateVirtualKeyCode(unsigned short vk);
  void PushInputKey(KeyCode key, bool is_down, std::string characters);
}  // namespace arctic

@interface ArcticAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
}
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:
(NSApplication *)theApplication;
- (BOOL) windowShouldClose:(NSWindow *)sender;
- (NSApplicationTerminateReply) applicationShouldTerminate:
(NSApplication *)sender;
- (void) fullScreenToggle:(NSNotification *)notification;
- (void) windowDidEnterFullScreen:(NSNotification *)notification;
- (void) windowDidExitFullScreen:(NSNotification *)notification;
@end

@interface ArcticWindow : NSWindow {
}
- (void) windowWillClose: (NSNotification *)notification;
@end

@interface ArcticView : NSOpenGLView {
}
- (void) drawRect: (NSRect) bounds;
- (void) captureMouse;
- (void) releaseMouse;
@end

static arctic::Si32 g_exit_code = 0;
static ArcticWindow *g_main_window = nil;
static ArcticView *g_main_view = nil;
static NSApplication *g_app = nil;
static ArcticAppDelegate *g_app_delegate = nil;
static arctic::SoundPlayer *g_mixer = nil;

// Releases the sound device and flushes the log. Every way out of the program
// goes through it, so it has to survive being called twice.
static void ShutdownEnginePlatform() {
  if (g_mixer) {
    g_mixer->Deinitialize();
    delete g_mixer;
    g_mixer = nil;
  }
  arctic::StopLogger();
}

static bool g_is_full_screen = false;
static bool g_is_cursor_desired_visible = true;
static bool g_is_cursor_set_visible = true;
static bool g_is_cursor_in_bounds = false;
static bool g_is_mouse_captured = false;

static GCController *g_controller = nil;

// The size of the view in real pixels, the unit both the GL viewport and the
// backbuffer of the engine are measured in.
//
// convertRectToBacking: asks the window for the scale, and a window that the
// run loop has not put on a screen yet answers 1.0, which is half the pixels
// on a Retina display. Measuring the window before the first frame that way
// used to give the engine a backbuffer of a quarter of the area, and the first
// Swap() then reported the honest size and the two disagreed forever after.
// Hence the fallback to the scale of the screen the window is headed for.
static NSSize BackingPixelSize(NSView *view) {
  if (view == nil) {
    return NSMakeSize(0.0, 0.0);
  }
  const NSRect frame = [view frame];
  const NSRect backing = [view convertRectToBacking: frame];
  NSScreen *screen = [[view window] screen];
  if (screen == nil) {
    screen = [NSScreen mainScreen];
  }
  const CGFloat scale = (screen == nil) ? 1.0 : [screen backingScaleFactor];
  const NSSize scaled = NSMakeSize(frame.size.width * scale,
      frame.size.height * scale);
  if (backing.size.width < scaled.width
      || backing.size.height < scaled.height) {
    return scaled;
  }
  return backing.size;
}

@implementation ArcticAppDelegate
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:
(NSApplication *)application {
  return YES;
}
- (BOOL) windowShouldClose:(NSWindow *)sender {
  // The only place where the close can still be refused: windowWillClose: below
  // is told that the window is closing, not asked whether it should.
  return arctic::OnMainWindowCloseRequested() ? YES : NO;
}
- (NSApplicationTerminateReply) applicationShouldTerminate:
(NSApplication *)sender {
  // Cmd+Q and "Quit" in the menu ask the same thing of the application as the
  // red button does, so they go through the same handler.
  return arctic::OnMainWindowCloseRequested()
    ? NSTerminateNow : NSTerminateCancel;
}
- (void)applicationWillTerminate:(NSNotification *)notification {
  [g_main_window orderOut: self];
}
- (void) fullScreenToggle:(NSNotification *)notification {
  [g_main_window toggleFullScreen: nil];
}
- (void) windowDidEnterFullScreen:(NSNotification *)notification {
  g_is_full_screen = true;
  [[g_main_view openGLContext] update];
  NSSize size = BackingPixelSize(g_main_view);
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)size.width, (arctic::Si32)size.height);
}
- (void) windowDidExitFullScreen:(NSNotification *)notification {
  g_is_full_screen = false;
  [[g_main_view openGLContext] update];
  NSSize size = BackingPixelSize(g_main_view);
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)size.width, (arctic::Si32)size.height);
}
@end

@implementation ArcticWindow
- (BOOL)canBecomeMainWindow {
  return YES;
}
- (BOOL)canBecomeKeyWindow {
  return YES;
}
- (id) initWithContentRect: (NSRect)rect styleMask: (NSWindowStyleMask)wndStyle
backing: (NSBackingStoreType)bufferingType defer: (BOOL)deferFlg {
  self = [super initWithContentRect: rect styleMask: wndStyle
    backing: bufferingType defer: deferFlg];

  [[NSNotificationCenter defaultCenter]
    addObserver: self selector: @selector(windowDidResize:)
      name: NSWindowDidResizeNotification object: self];

  [[NSNotificationCenter defaultCenter]
    addObserver: self selector: @selector(windowWillClose:)
      name: NSWindowWillCloseNotification object: self];

  [self setAcceptsMouseMovedEvents: YES];

  return self;
}

- (void) windowDidResize: (NSNotification *)notification {
  [[g_main_view openGLContext] update];
  NSSize size = BackingPixelSize(g_main_view);
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)size.width, (arctic::Si32)size.height);
}

- (void) windowWillClose: (NSNotification *)notification {
  ShutdownEnginePlatform();
  exit(g_exit_code);
}
@end

@implementation ArcticView
- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];
  [[self layer] setContentsScale: [[self window] backingScaleFactor]];
}

-(void) drawRect: (NSRect) bounds {
}

-(void) prepareOpenGL {
    [super prepareOpenGL];
}

-(NSMenu *)menuForEvent: (NSEvent *)theEvent {
  return [NSView defaultMenu];
}

- (void) flagsChanged: (NSEvent *)theEvent {
  unsigned long modifier_flags = [theEvent modifierFlags];
  static bool was_caps_lock = false;
  static bool was_shift = false;
  static bool was_control = false;
  static bool was_option = false;
  static bool was_command = false;

  bool is_caps_lock = (modifier_flags & NSEventModifierFlagCapsLock) != 0;
  bool is_shift     = (modifier_flags & NSEventModifierFlagShift)    != 0;
  bool is_control   = (modifier_flags & NSEventModifierFlagControl)  != 0;
  bool is_option    = (modifier_flags & NSEventModifierFlagOption)   != 0;
  // On macOS the Command key plays the role of Ctrl for application shortcuts
  // (Cmd+C / Cmd+V / Cmd+Z etc.), so we report it as kKeyControl.
  bool is_command   = (modifier_flags & NSEventModifierFlagCommand)  != 0;

  if (is_caps_lock != was_caps_lock) {
    arctic::PushInputKey(arctic::kKeyCapsLock, is_caps_lock, "");
  }
  if (is_shift != was_shift) {
    arctic::PushInputKey(arctic::kKeyShift, is_shift, "");
  }
  // Fire kKeyControl when either physical Control or Command changes.
  bool ctrl_or_cmd     = is_control || is_command;
  bool was_ctrl_or_cmd = was_control || was_command;
  if (ctrl_or_cmd != was_ctrl_or_cmd) {
    arctic::PushInputKey(arctic::kKeyControl, ctrl_or_cmd, "");
  }
  if (is_option != was_option) {
    arctic::PushInputKey(arctic::kKeyAlt, is_option, "");
  }

  was_caps_lock = is_caps_lock;
  was_shift     = is_shift;
  was_control   = is_control;
  was_option    = is_option;
  was_command   = is_command;

}

- (void) keyDown: (NSEvent *)theEvent {
  // The virtual key code of an NSEvent is the physical key, the same number for
  // the same key under any input source, so it is what the engine reports as
  // the key. The typed text is asked for separately, right below, because that
  // one does depend on the layout.
  arctic::KeyCode key = arctic::TranslateVirtualKeyCode([theEvent keyCode]);
  if (key == arctic::kKeyUnknown) {
    NSLog(@"Unknown virtual keyCode: %d", [theEvent keyCode]);
  }
  NSString *typed = [theEvent characters];
  PushInputKey(key, true, typed ? [typed UTF8String] : "");
}

- (void) keyUp: (NSEvent *)theEvent {
  arctic::KeyCode key = arctic::TranslateVirtualKeyCode([theEvent keyCode]);
  PushInputKey(key, false, "");
}

- (void) mouseEvent: (NSEvent *)event key: (int)key_code state: (int)state
isScroll: (bool)is_scroll {
  NSRect loc_rect = [self convertRectToBacking:
    NSMakeRect([event locationInWindow].x,
        [event locationInWindow].y, 0, 0)];

  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];

  arctic::Check(rect.size.width != 0,
      "Could not obtain view width in mouseEvent:key:state");
  arctic::Check(rect.size.height != 0,
      "Could not obtain view height in mouseEvent:key:state");

  arctic::Vec2F pos((float)loc_rect.origin.x / (float)rect.size.width,
      (float)loc_rect.origin.y / (float)rect.size.height);
  
  g_is_cursor_in_bounds = (pos.x >= 0.0f && pos.y >= 0.0f &&
    pos.x <= 1.0f && pos.y <= 1.0f);
  arctic::SetCursorVisible(g_is_cursor_desired_visible);

  arctic::InputMessage msg;
  msg.mouse.delta = arctic::Vec2F((float)[event deltaX], -(float)[event deltaY]);
  msg.kind = arctic::InputMessage::kMouse;
  msg.keyboard.key = static_cast<unsigned int>(key_code);
  msg.keyboard.characters[0] = '\0';
  msg.keyboard.key_state = static_cast<unsigned int>(state);
  msg.mouse.pos = pos;
  if (is_scroll) {
    if (event.hasPreciseScrollingDeltas) {
      msg.mouse.wheel_delta = (arctic::Si32)[event scrollingDeltaY];
      msg.mouse.wheel_delta_x = (arctic::Si32)[event scrollingDeltaX];
    } else {
      msg.mouse.wheel_delta = (arctic::Si32)[event deltaY];
      msg.mouse.wheel_delta_x = (arctic::Si32)[event deltaX];
    }
  } else {
    msg.mouse.wheel_delta = 0;
    msg.mouse.wheel_delta_x = 0;
  }

  PushInputMessage(msg);
}

- (void)scrollWheel:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: true];
}

- (void)magnifyWithEvent:(NSEvent *)event {
  NSRect loc_rect = [self convertRectToBacking:
    NSMakeRect([event locationInWindow].x,
        [event locationInWindow].y, 0, 0)];
  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::Vec2F pos(0.0f, 0.0f);
  if (rect.size.width != 0 && rect.size.height != 0) {
    pos = arctic::Vec2F((float)loc_rect.origin.x / (float)rect.size.width,
        (float)loc_rect.origin.y / (float)rect.size.height);
  }

  arctic::InputMessage msg;
  msg.kind = arctic::InputMessage::kMouse;
  msg.keyboard.key = arctic::kKeyNone;
  msg.keyboard.characters[0] = '\0';
  msg.keyboard.key_state = 0;
  msg.mouse.pos = pos;
  msg.mouse.zoom_delta = (float)[event magnification];
  PushInputMessage(msg);
}

- (void) mouseMoved: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: false];
}

- (void) mouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: false];
}

- (void) rightMouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: false];
}

- (void) otherMouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: false];
}

- (void) mouseDown: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseLeft state: 1
    isScroll: false];
}

- (void) mouseUp:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseLeft state: 2
    isScroll: false];
}

- (void) rightMouseDown:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseRight state: 1
    isScroll: false];
}

- (void) rightMouseUp:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseRight state: 2
    isScroll: false];
}

- (void) otherMouseDown:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseWheel state: 1
    isScroll: false];
}

- (void) otherMouseUp: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyMouseWheel state: 2
    isScroll: false];
}


-(void)extendedGamepadAction: (GCExtendedGamepad *)c forElement:(GCControllerElement *)element {
  if (c == nil) {
    NSLog(@"controller nil.");
    return;
  }
  arctic::Si32 controller_idx = (arctic::Si32)c.controller.playerIndex;

  if (controller_idx < 0 || controller_idx >= arctic::InputMessage::kControllerCount) {
    return;
  }
  arctic::InputMessage msg;
  msg.kind = arctic::InputMessage::kController;

  msg.keyboard.characters[0] = '\0';
  msg.keyboard.key = arctic::kKeyNone;
  msg.keyboard.key_state = 0;

  arctic::InputMessage::Controller &controller = msg.controller;
  controller.controller_idx = controller_idx;

  // TODO: Check playerIndex here. if(c.playerIndex==yourindex)
  if (c.dpad==element) {
    if (c.dpad.up.isPressed==1) {
        //Dpad.up of extendedgamepad has been pressed
    }
    if (c.dpad.down.isPressed==1) {
        //Same for down
    }
  }

  controller.axis[0] = c.leftThumbstick.xAxis.value;
  controller.axis[1] = c.leftThumbstick.yAxis.value;
  controller.axis[2] = c.rightThumbstick.xAxis.value;
  controller.axis[3] = c.rightThumbstick.yAxis.value;
  controller.axis[4] = c.rightTrigger.value;
  controller.axis[5] = c.leftTrigger.value;

  msg.keyboard.state[arctic::kKeyController0Button0 + 32*controller_idx] = c.buttonA.pressed;
  msg.keyboard.state[arctic::kKeyController0Button1 + 32*controller_idx] = c.buttonB.pressed;

  msg.keyboard.state[arctic::kKeyController0Button3 + 32*controller_idx] = c.buttonX.pressed;
  msg.keyboard.state[arctic::kKeyController0Button4 + 32*controller_idx] = c.buttonY.pressed;

  msg.keyboard.state[arctic::kKeyController0Button6 + 32*controller_idx] = c.rightShoulder.pressed;
  msg.keyboard.state[arctic::kKeyController0Button7 + 32*controller_idx] = c.leftShoulder.pressed;

  msg.keyboard.state[arctic::kKeyController0Button24 + 32*controller_idx] = c.dpad.up.pressed;
  msg.keyboard.state[arctic::kKeyController0Button25 + 32*controller_idx] = c.dpad.right.pressed;
  msg.keyboard.state[arctic::kKeyController0Button26 + 32*controller_idx] = c.dpad.down.pressed;
  msg.keyboard.state[arctic::kKeyController0Button27 + 32*controller_idx] = c.dpad.left.pressed;

  PushInputMessage(msg);
}

- (void) captureMouse {
  g_is_mouse_captured = true;
  [self.window makeFirstResponder:self];

  NSRect window_frame = [self.window frame];
  CGFloat center_x = window_frame.origin.x + window_frame.size.width / 2;
  CGFloat screen_height = [NSScreen mainScreen].frame.size.height;
  CGFloat center_y = screen_height - (window_frame.origin.y + window_frame.size.height / 2);
  CGWarpMouseCursorPosition(CGPointMake(center_x, center_y));

  CGAssociateMouseAndMouseCursorPosition(false);
  arctic::SetCursorVisible(g_is_cursor_desired_visible);
}

- (void) releaseMouse {
  g_is_mouse_captured = false;
  CGAssociateMouseAndMouseCursorPosition(true);
  arctic::SetCursorVisible(g_is_cursor_desired_visible);
}

@end

namespace arctic {

Ui16 FromBe(Ui16 x) {
  return ntohs(x);
}
Si16 FromBe(Si16 x) {
  return static_cast<Si16>(ntohs(x));
}
Ui32 FromBe(Ui32 x) {
  return ntohl(x);
}
Si32 FromBe(Si32 x) {
  return static_cast<Si32>(ntohl(x));
}
Ui16 ToBe(Ui16 x) {
  return htons(x);
}
Si16 ToBe(Si16 x) {
  return static_cast<Si16>(htons(x));
}
Ui32 ToBe(Ui32 x) {
  return htonl(x);
}
Si32 ToBe(Si32 x) {
  return static_cast<Si32>(htonl(x));
}

void Check(bool condition, const char *error_message,
    const char *error_message_postfix) {
  if (condition) {
    return;
  }
  Fatal(error_message, error_message_postfix);
}

/// @brief Tells whether a modal alert can be shown at all
///
/// NSAlert loads its window from a nib in the main bundle, and asserts (aborts
/// the process) when there is no bundle to load it from: a headless run, a
/// binary started as a console tool, or a sandbox that keeps the bundle to
/// itself all end that way, and the crash report replaces the message the user
/// needed. The message goes to stderr and to the log in every case, so skipping
/// the alert loses nothing but the window.
static bool CanShowModalAlert() {
  if (GetEngine()->IsHeadless()) {
    return false;
  }
  if (NSApp == nil) {
    return false;
  }
  return [[NSBundle mainBundle] bundlePath] != nil;
}

void Fatal(const char *message, const char *message_postfix) {
  size_t size = 1 +
    strlen(message) +
    (message_postfix ? strlen(message_postfix) : 0);
  if (g_is_full_screen) {
    [g_app_delegate fullScreenToggle: nil];
  }
  char *full_message = new char[size];
  full_message[size - 1] = 0;
  snprintf(full_message, size, "%s%s", message,
      (message_postfix ? message_postfix : ""));

#ifndef ARCTIC_NO_FATAL_MESSAGES
  NSLog(@"Fatal: %s", full_message);
  fprintf(stderr, "Fatal: %s\n", full_message);
  *Log() << "Fatal: " << full_message;

  if (CanShowModalAlert()) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle: @"OK"];
    [alert setMessageText: @"Fatal Error"];
    [alert setInformativeText:
      [[NSString alloc] initWithUTF8String: full_message]];
    [alert setAlertStyle: NSAlertStyleCritical];
    [alert runModal];
  }
#endif  // ARCTIC_NO_FATAL_MESSAGES
  delete[] full_message;
#ifndef ARCTIC_NO_HARD_EXIT
  exit(1);
#else
  longjmp(arctic_jmp_env, 1337);
#endif  // ARCTIC_NO_HARD_EXIT
}

void PushInputKey(KeyCode key, bool is_down, std::string characters) {
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key;
  // [theEvent characters] answers "\t" for Tab and a C0 code for Control with a
  // letter, and none of that is text; SetTypedCharacters leaves it out.
  SetTypedCharacters(&msg.keyboard, is_down ? characters.c_str() : "");
  msg.keyboard.key_state = (is_down ? 1 : 2);
  PushInputMessage(msg);
}

KeyCode TranslateVirtualKeyCode(unsigned short vk) {
  switch (vk) {
    case 0x00: return kKeyA;
    case 0x0B: return kKeyB;
    case 0x08: return kKeyC;
    case 0x02: return kKeyD;
    case 0x0E: return kKeyE;
    case 0x03: return kKeyF;
    case 0x05: return kKeyG;
    case 0x04: return kKeyH;
    case 0x22: return kKeyI;
    case 0x26: return kKeyJ;
    case 0x28: return kKeyK;
    case 0x25: return kKeyL;
    case 0x2E: return kKeyM;
    case 0x2D: return kKeyN;
    case 0x1F: return kKeyO;
    case 0x23: return kKeyP;
    case 0x0C: return kKeyQ;
    case 0x0F: return kKeyR;
    case 0x01: return kKeyS;
    case 0x11: return kKeyT;
    case 0x20: return kKeyU;
    case 0x09: return kKeyV;
    case 0x0D: return kKeyW;
    case 0x07: return kKeyX;
    case 0x10: return kKeyY;
    case 0x06: return kKeyZ;

    case 0x12: return kKey1;
    case 0x13: return kKey2;
    case 0x14: return kKey3;
    case 0x15: return kKey4;
    case 0x17: return kKey5;
    case 0x16: return kKey6;
    case 0x1A: return kKey7;
    case 0x1C: return kKey8;
    case 0x19: return kKey9;
    case 0x1D: return kKey0;

    case 0x24: return kKeyEnter;
    case 0x30: return kKeyTab;
    case 0x31: return kKeySpace;
    case 0x33: return kKeyBackspace;
    case 0x35: return kKeyEscape;

    case 0x7B: return kKeyLeft;
    case 0x7C: return kKeyRight;
    case 0x7D: return kKeyDown;
    case 0x7E: return kKeyUp;

    case 0x73: return kKeyHome;
    case 0x77: return kKeyEnd;
    case 0x74: return kKeyPageUp;
    case 0x79: return kKeyPageDown;
    case 0x72: return kKeyInsert;
    case 0x75: return kKeyDelete;

    case 0x7A: return kKeyF1;
    case 0x78: return kKeyF2;
    case 0x63: return kKeyF3;
    case 0x76: return kKeyF4;
    case 0x60: return kKeyF5;
    case 0x61: return kKeyF6;
    case 0x62: return kKeyF7;
    case 0x64: return kKeyF8;
    case 0x65: return kKeyF9;
    case 0x6D: return kKeyF10;
    case 0x67: return kKeyF11;
    case 0x6F: return kKeyF12;

    case 0x29: return kKeySemicolon;
    case 0x27: return kKeyApostrophe;
    case 0x2B: return kKeyComma;
    case 0x2F: return kKeyPeriod;
    case 0x2C: return kKeySlash;
    case 0x2A: return kKeyBackslash;
    case 0x21: return kKeyLeftSquareBracket;
    case 0x1E: return kKeyRightSquareBracket;
    case 0x32: return kKeyGraveAccent;
    case 0x1B: return kKeyMinus;
    case 0x18: return kKeyEquals;
    case 0x0A: return kKeySectionSign;

    case 0x47: return kKeyNumLock;
    case 0x52: return kKeyNumpad0;
    case 0x53: return kKeyNumpad1;
    case 0x54: return kKeyNumpad2;
    case 0x55: return kKeyNumpad3;
    case 0x56: return kKeyNumpad4;
    case 0x57: return kKeyNumpad5;
    case 0x58: return kKeyNumpad6;
    case 0x59: return kKeyNumpad7;
    case 0x5B: return kKeyNumpad8;
    case 0x5C: return kKeyNumpad9;
    case 0x4B: return kKeyNumpadSlash;
    case 0x43: return kKeyNumpadAsterisk;
    case 0x4E: return kKeyNumpadMinus;
    case 0x45: return kKeyNumpadPlus;
    case 0x41: return kKeyNumpadPeriod;

    case 0x71: return kKeyPause;
    case 0x6B: return kKeyScrollLock;
    case 0x69: return kKeyPrintScreen;
  }
  return kKeyUnknown;
}

void CreateMainMenu() {
  @autoreleasepool {
    NSMenu *main_menu = [[NSMenu alloc] initWithTitle: @"MainMenu"];

    NSMenuItem *app_menu = [[NSMenuItem alloc] initWithTitle: @"App"
      action: NULL keyEquivalent: [NSString string]];
    [main_menu addItem: app_menu];

    NSMenu *app_sub_menu = [[NSMenu alloc] initWithTitle: @"App"];
    [app_menu setSubmenu: app_sub_menu];

    NSMenuItem *app_sub_menu_full_screen_toggle = [[NSMenuItem alloc]
      initWithTitle: @"Full Screen Toggle"
      action: @selector(fullScreenToggle:) keyEquivalent: @"m"];
    [app_sub_menu_full_screen_toggle setTarget: g_app_delegate];
    [app_sub_menu addItem: app_sub_menu_full_screen_toggle];

    NSMenuItem *app_sub_menu_quit = [[NSMenuItem alloc]
      initWithTitle: @"Quit" action: @selector(terminate:)
      keyEquivalent: @"q"];
    [app_sub_menu_quit setTarget: NSApp];
    [app_sub_menu addItem: app_sub_menu_quit];

    [NSApp setMainMenu: main_menu];
  }
}

void HeadlessPlatformInit() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    g_app = NSApp;

    g_app_delegate = [ArcticAppDelegate new];
    [NSApp setDelegate:
      ((id<NSApplicationDelegate> _Nullable)g_app_delegate)];

    [NSApp finishLaunching];
  }
}

void CreateMainWindow(SystemInfo *system_info) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    g_app = NSApp;

    g_app_delegate = [ArcticAppDelegate new];
    [NSApp setDelegate:
      ((id<NSApplicationDelegate> _Nullable)g_app_delegate)];

    CreateMainMenu();

    [NSApp finishLaunching];


    {
      unsigned int winStyle =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;
      g_main_window = [[ArcticWindow alloc]
        initWithContentRect: [[NSScreen mainScreen] visibleFrame]
        styleMask: winStyle
        backing: NSBackingStoreBuffered
        defer: NO];
      [g_main_window setCollectionBehavior:
        NSWindowCollectionBehaviorFullScreenPrimary];
      [g_main_window setDelegate:
        ((id<NSWindowDelegate>)g_app_delegate)];
    }

    NSOpenGLPixelFormatAttribute format_attribute[] = {
      NSOpenGLPFADepthSize,
      (NSOpenGLPixelFormatAttribute)32,
      NSOpenGLPFADoubleBuffer,
      0};
    NSOpenGLPixelFormat *format =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: format_attribute];
    if (format == nil) {
      // Going on without a pixel format means going on without a GL context,
      // and the first thing that needs one fails several steps later with a
      // message that says nothing about the real cause ("no texture" from
      // GlTexture2D::Create). The run ends here instead, and it names the way
      // out for a machine that has no display to draw on.
      Fatal("Can't create an OpenGL pixel format: no display capable of the"
        " requested attributes (32 bit depth, double buffering)."
        " A machine with no display can still run the program without a window,"
        " see ARCTIC_HEADLESS_DECIDER and the ARCTIC_HEADLESS and"
        " ARCTIC_DISABLE_HW environment variables.");
    }

    g_main_view = [[ArcticView alloc]
      initWithFrame: [g_main_window frame] pixelFormat: format];

    [g_main_view setWantsBestResolutionOpenGLSurface: YES];

    [[g_main_view openGLContext] makeCurrentContext];

    [g_main_window setContentView: g_main_view];
    [g_main_window makeFirstResponder: g_main_view];
    if (std::getenv("ARCTIC_HEADLESS") == nullptr) {
      [g_main_window makeKeyAndOrderFront: nil];
      [g_main_window makeMainWindow];
      [NSApp activateIgnoringOtherApps: YES];
    } else {
      // Keep the NSOpenGLContext alive for headless hardware rendering, but
      // never present the native window to the user.
      [g_main_window orderOut: nil];
    }

    if (g_is_full_screen) {
      g_is_full_screen = false;
      [g_main_window toggleFullScreen: nil];
    }

    NSLog(@"%d controllers found.", (int)[GCController controllers].count);

    NSSize size = BackingPixelSize(g_main_view);
    system_info->screen_width = (arctic::Si32)size.width;
    system_info->screen_height = (arctic::Si32)size.height;
  }
}

void PumpMessages() {

  @autoreleasepool {
    NSArray<GCController *> *controllers = [GCController controllers];
    if (controllers.count) {
      for (Si32 controller_idx = 0; controller_idx < (Si32)controllers.count; ++controller_idx) {
        GCController *ctrl = controllers[controller_idx];
        if (ctrl.playerIndex == -1) {
          std::map<Si32, Si32> player_to_idx;
          for (Si32 ci = 0; ci < (Si32)controllers.count; ++ci) {
            Si32 player = (Si32)controllers[ci].playerIndex;
            if (player != -1) {
              player_to_idx[player] = ci;
            }
          }
          for (Si32 player_idx = 0; player_idx < InputMessage::kControllerCount; ++player_idx) {
            if (player_to_idx.find(player_idx) == player_to_idx.end()) {
              ctrl.playerIndex = (GCControllerPlayerIndex)player_idx;
              break;
            }
          }
        }
        if (ctrl.playerIndex != -1) {
          GCExtendedGamepad *gamepad = ctrl.extendedGamepad;
          if (gamepad != nil) {
            [g_main_view extendedGamepadAction:gamepad forElement: nil];
          }
        }
      }

      g_controller = controllers[0];
    } else {
      g_controller = nil;
    }
  }

  @autoreleasepool {
    while (true) {
      NSEvent *event = [g_app
        nextEventMatchingMask: NSEventMaskAny
        untilDate: [NSDate distantPast]
        inMode: NSDefaultRunLoopMode
        dequeue: YES];
      if (event != nil) {
        [NSApp sendEvent: event];
      } else {
        break;
      }
    }
    [NSApp updateWindows];
  }
}

void ExitProgram(Si32 exit_code) {
  g_exit_code = exit_code;
  // EasyMain runs on the main thread with no NSApp run loop of its own, so
  // asking AppKit to terminate would only queue the request and the call would
  // return to a caller that believes the program is over. The shutdown is done
  // here instead, in the same order the normal end of main does it, and the code
  // reaches the shell through exit().
  ShutdownEnginePlatform();
  exit(exit_code);
}

void Swap() {
  [[g_main_view openGLContext] flushBuffer];
  PumpMessages();

  static arctic::Si32 cached_width = 0;
  static arctic::Si32 cached_height = 0;
  NSSize size = BackingPixelSize(g_main_view);
  arctic::Si32 w = (arctic::Si32)size.width;
  arctic::Si32 h = (arctic::Si32)size.height;
  if (w != cached_width || h != cached_height) {
    cached_width = w;
    cached_height = h;
    arctic::GetEngine()->OnWindowResize(w, h);
  }
}


bool IsVSyncSupported() {
  return true;
}

bool SetVSync(bool is_enable) {
  if (!IsVSyncSupported()) {
    return false;
  }
  GLint swap_interval = (is_enable ? 1 : 0);
  [[g_main_view openGLContext]
    setValues: &swap_interval
      forParameter: NSOpenGLContextParameterSwapInterval];
  return true;
}

bool IsFullScreen() {
  return g_is_full_screen;
}

void SetFullScreen(bool is_enable) {
  if (is_enable == g_is_full_screen) {
    return;
  }
  [g_app_delegate fullScreenToggle: nil];
}
  
bool IsCursorVisible() {
  return g_is_cursor_desired_visible;
}
  
void SetCursorVisible(bool is_enable) {
  g_is_cursor_desired_visible = is_enable;
  bool next_is_visible = (g_is_cursor_desired_visible ||
    !g_is_cursor_in_bounds) && !g_is_mouse_captured;
  if (next_is_visible == g_is_cursor_set_visible) {
    return;
  }
  g_is_cursor_set_visible = next_is_visible;
  if (g_is_cursor_set_visible) {
    [NSCursor unhide];
  } else {
    [NSCursor hide];
  }
}

void CaptureMouse() {
  [g_main_view captureMouse];
}

void ReleaseMouse() {
  [g_main_view releaseMouse];
}

bool IsMouseCaptured() {
  return g_is_mouse_captured;
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

Trivalent DoesFileExist(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return kTrivalentFalse;
  } else if ((info.st_mode & S_IFMT) == S_IFREG) {
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

bool ChangeCurrentDirectory(const char *path) {
  if (!path || *path == 0) {
    return false;
  }
  return chdir(path) == 0;
}

std::string GetExecutablePath() {
  char buffer[1 << 12];
  uint32_t size = sizeof(buffer);
  if (_NSGetExecutablePath(buffer, &size) != 0) {
    return std::string();
  }
  char resolved[PATH_MAX];
  if (realpath(buffer, resolved) != nullptr) {
    return std::string(resolved);
  }
  return std::string(buffer);
}

bool GetDirectoryEntries(const char *path,
    std::vector<DirectoryEntry> *out_entries) {
  Check(out_entries != nullptr,
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
    int written = snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);
    Check(written >= 0 && static_cast<size_t>(written) < sizeof(full_path),
      "GetDirectoryEntries: path too long: ", dir_entry->d_name);
    struct stat info;
    if (stat(full_path, &info) != 0) {
      closedir(dir);
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
  Check(path != nullptr, "CanonicalizePath error, path can't be nullptr");
  std::string p(path);
  if (p.empty()) {
    return std::string();
  }
  if (p[0] != '/') {
    char cwd[1 << 20];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
      return std::string();
    }
    p = std::string(cwd) + "/" + p;
  }
  std::vector<std::string> components;
  size_t start = 1;
  while (start <= p.size()) {
    size_t end = p.find('/', start);
    if (end == std::string::npos) {
      end = p.size();
    }
    std::string comp = p.substr(start, end - start);
    if (comp == "..") {
      if (!components.empty()) {
        components.pop_back();
      }
    } else if (!comp.empty() && comp != ".") {
      components.push_back(comp);
    }
    start = end + 1;
  }
  std::string result = "/";
  for (size_t i = 0; i < components.size(); i++) {
    if (i > 0) {
      result += "/";
    }
    result += components[i];
  }
  return result;
}

// TODO(Huldra): Move common code out of macos and pi specific files.
std::string RelativePathFromTo(const char *from, const char *to) {
  std::string from_abs = CanonicalizePath(from);
  if (from && from[0] != 0 && from[strlen(from) - 1] == '/' &&
      !from_abs.empty() && from_abs[from_abs.size() - 1] != '/') {
    from_abs = from_abs + '/';
  }
  std::string to_abs = CanonicalizePath(to);
  if (to && to[0] != 0 && to[strlen(to) - 1] == '/' &&
      !to_abs.empty() && to_abs[to_abs.size() - 1] != '/') {
    to_abs = to_abs + '/';
  }
  // "/a/b/" and "/a/b" are one and the same directory, so the way from a place
  // to itself is "./" whichever of the two spellings each side came in as.
  {
    std::string from_bare = from_abs;
    std::string to_bare = to_abs;
    if (from_bare.size() > 1 && from_bare[from_bare.size() - 1] == '/') {
      from_bare.resize(from_bare.size() - 1);
    }
    if (to_bare.size() > 1 && to_bare[to_bare.size() - 1] == '/') {
      to_bare.resize(to_bare.size() - 1);
    }
    if (from_bare == to_bare) {
      return "./";
    }
  }
  Ui32 matching = 0;
  while (matching < from_abs.size() && matching < to_abs.size()) {
    if (from_abs[matching] == to_abs[matching]) {
      ++matching;
    } else {
      break;
    }
  }
  bool is_one_end = (matching == from_abs.size() || matching == to_abs.size());
  bool is_one_next_slash =
    ((matching < from_abs.size() && from_abs[matching] == '/') ||
     (matching < to_abs.size() && to_abs[matching] == '/'));

  std::stringstream res;
  if (is_one_end && is_one_next_slash) {
    if (from_abs.size() == matching) {
      res << ".";
    }
  } else {
    while (matching && from_abs[matching - 1] != '/') {
      --matching;
    }
  }

  // One ".." per directory left behind. A slash names no directory of its own,
  // so the empty piece a trailing slash leaves at the end of "from" adds none:
  // "/a/b/" and "/a/b" are the same place, and from either of them the way to
  // "/a/c" is one step up.
  const char *from_part = from_abs.c_str() + matching;

  while (*from_part != 0) {
    if (*from_part == '/') {
      ++from_part;
      continue;
    }
    res << "../";
    while (*from_part != 0 && *from_part != '/') {
      ++from_part;
    }
  }
  const char *to_part = to_abs.c_str() + matching;
  res << to_part;
  return res.str();
}

std::string ParentPath(const char *path) {
  size_t len = 0;
  size_t prev_len = 0;
  if (path) {
    const char *p = path;
    while (*p != 0) {
      if (*p == '/') {
        prev_len = len;
        len = p - path + 1;
      }
      ++p;
    }
    if (p - path + 1 == len) {
      len = prev_len;
    }
    if (len == 0) {
      return std::string(path);
    }
    return std::string(path, 0, len);
  }
  return std::string("");
}

std::string GluePath(const char *first_part, const char *second_part) {
  if (!first_part || *first_part == 0) {
    if (!second_part || *second_part == 0) {
      return std::string("");
    }
    return std::string(second_part);
  }
  if (!second_part || *second_part == 0) {
    return std::string(first_part);
  }
  std::stringstream str;
  str << first_part;
  if (first_part[strlen(first_part)-1] != '/') {
    str << "/";
  }
  if (*second_part == '/') {
    ++second_part;
  }
  str << second_part;
  return str.str();
}

std::string FindSystemFont(const char *font_name) {
  if (!font_name) {
    return std::string();
  }
  CFStringRef name = CFStringCreateWithCString(
    nullptr, font_name, kCFStringEncodingUTF8);
  if (!name) {
    return std::string();
  }
  CTFontDescriptorRef descriptor =
    CTFontDescriptorCreateWithNameAndSize(name, 0);
  CFRelease(name);
  if (!descriptor) {
    return std::string();
  }
  CFURLRef url = static_cast<CFURLRef>(
    CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute));
  CFRelease(descriptor);
  if (!url) {
    return std::string();
  }
  char path[PATH_MAX];
  Boolean ok = CFURLGetFileSystemRepresentation(
    url, true, reinterpret_cast<UInt8*>(path), PATH_MAX);
  CFRelease(url);
  if (!ok) {
    return std::string();
  }
  return std::string(path);
}

void SetClipboardText(const std::string &text) {
  @autoreleasepool {
    NSString *s = [[NSString alloc] initWithBytes:text.data()
        length:text.size() encoding:NSUTF8StringEncoding];
    if (!s) {
      s = @"";
    }
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    [pb setString:s forType:NSPasteboardTypeString];
  }
}

std::string GetClipboardText() {
  @autoreleasepool {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSString *s = [pb stringForType:NSPasteboardTypeString];
    if (!s) {
      return std::string();
    }
    const char *utf8 = [s UTF8String];
    return utf8 ? std::string(utf8) : std::string();
  }
}

/// @brief Path of the application bundle the process was loaded from
/// @return Absolute path of the `.app` directory, or an empty string if the
///   executable does not live in a bundle
///
/// NSBundle is the obvious way to ask and it is not always available: a process
/// started under a sandbox that denies it the bundle machinery gets nil for
/// mainBundle, and passing what nil returns to std::string is a crash rather
/// than a diagnostic. The path of the executable file answers the same question
/// without help from the frameworks, so it serves as the fallback.
static std::string BundlePath() {
  NSString *bundle_path = [[NSBundle mainBundle] bundlePath];
  const char *utf8 = (bundle_path == nil) ? nullptr : [bundle_path UTF8String];
  if (utf8 != nullptr && *utf8 != 0) {
    return std::string(utf8);
  }
  const std::string executable_path = GetExecutablePath();
  const std::string kSuffix = "/Contents/MacOS";
  const size_t slash = executable_path.find_last_of('/');
  if (slash == std::string::npos) {
    return std::string();
  }
  const std::string executable_dir = executable_path.substr(0, slash);
  if (executable_dir.size() <= kSuffix.size()
      || executable_dir.compare(executable_dir.size() - kSuffix.size(),
        kSuffix.size(), kSuffix) != 0) {
    return std::string();
  }
  return executable_dir.substr(0, executable_dir.size() - kSuffix.size());
}

std::string PrepareInitialPath() {
  // Remembered before the chdir below takes it away, and it is the only chance:
  // once the current directory is the Resources folder, nothing in the process
  // knows where the user was standing when they typed the command. Every path
  // that comes from argv has to be read through CanonicalizeArgvPath, which is
  // what this value is for.
  std::string startup_directory;
  if (arctic::GetCurrentPath(&startup_directory)) {
    arctic::SetStartupDirectory(startup_directory);
  }
  const std::string bundle_path = BundlePath();
  if (bundle_path.empty()) {
    // Nothing is known about where the application lives, so the directory the
    // user was standing in is the only sensible base for relative paths, and it
    // stays current.
    return startup_directory;
  }
  std::string initial_path = bundle_path + "/..";
  initial_path = arctic::CanonicalizePath(initial_path.c_str());
  // Resources of the bundle become the current directory so that an application
  // can load "data/hero.tga" without knowing where it was installed. The price
  // is that a relative path typed on the command line no longer means what the
  // shell meant by it, and that relative writes land inside the bundle, which a
  // rebuild replaces; see CanonicalizeArgvPath in arctic_platform.h.
  const std::string resources_path = bundle_path + "/Contents/Resources";
  arctic::ChangeCurrentDirectory(resources_path.c_str());
  return initial_path;
}

}  // namespace arctic

#ifndef ARCTIC_NO_MAIN
namespace arctic {
  void PrepareForTheEasyMainCall();
}

int main(int argc, char **argv) {
  arctic::SystemInfo system_info;

  const bool headless = std::getenv("ARCTIC_HEADLESS") != nullptr;

  // The command line is handed to the engine first, so that a decider
  // registered with ARCTIC_HEADLESS_DECIDER can read it and tell a console
  // subcommand from a normal run before anything is created.
  arctic::GetEngine()->SetArgcArgv(argc, const_cast<const char **>(argv));
  if (arctic::IsHeadlessStartupRequested()) {
    // No window, no GL context, no sound device: the backbuffer is memory and
    // nothing here needs a display to be attached to the machine.
    arctic::GetEngine()->SetInitialPath(arctic::PrepareInitialPath());
    arctic::StartLogger();
    arctic::GetEngine()->InitHeadlessScreen(1920, 1080);
    arctic::PrepareForTheEasyMainCall();
    EasyMain();
    ShutdownEnginePlatform();
    return g_exit_code;
  }

  std::string initial_path = arctic::PrepareInitialPath();
  arctic::StartLogger();
  if (std::getenv("ARCTIC_DISABLE_AUDIO") == nullptr) {
    g_mixer = new arctic::SoundPlayer;
    g_mixer->Initialize();
  }
  arctic::CreateMainWindow(&system_info);
  if (headless) {
    [g_main_window orderOut: nil];
    arctic::GetEngine()->SetHeadless(true);
  }

  arctic::GetEngine()->SetInitialPath(initial_path);
  arctic::GetEngine()->Init(system_info.screen_width,
    system_info.screen_height);

  arctic::PumpMessages();

  arctic::PrepareForTheEasyMainCall();
  EasyMain();

  // Returning from EasyMain means the same as ExitProgram(0), unless something
  // asked for another code on the way out.
  ShutdownEnginePlatform();
  return g_exit_code;
}
#endif // ARCTIC_NO_MAIN

#endif
