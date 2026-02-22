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
@end

static arctic::Si32 g_exit_code = 0;
static ArcticWindow *g_main_window = nil;
static ArcticView *g_main_view = nil;
static NSApplication *g_app = nil;
static ArcticAppDelegate *g_app_delegate = nil;
static arctic::SoundPlayer *g_mixer = nil;

static bool g_is_full_screen = false;
static bool g_is_cursor_desired_visible = true;
static bool g_is_cursor_set_visible = true;
static bool g_is_cursor_in_bounds = false;

static GCController *g_controller = nil;

@implementation ArcticAppDelegate
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:
(NSApplication *)application {
  return YES;
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
  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)rect.size.width, (arctic::Si32)rect.size.height);
}
- (void) windowDidExitFullScreen:(NSNotification *)notification {
  g_is_full_screen = false;
  [[g_main_view openGLContext] update];
  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)rect.size.width, (arctic::Si32)rect.size.height);
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
  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)rect.size.width, (arctic::Si32)rect.size.height);
}

- (void) windowWillClose: (NSNotification *)notification {
  if (g_mixer) {
    g_mixer->Deinitialize();
    delete g_mixer;
    g_mixer = nil;
  }
  arctic::StopLogger();
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
  //static bool was_command = false;

  bool is_caps_lock = (modifier_flags & NSEventModifierFlagCapsLock) != 0;
  bool is_shift = (modifier_flags & NSEventModifierFlagShift) != 0;
  bool is_control = (modifier_flags & NSEventModifierFlagControl) != 0;
  bool is_option = (modifier_flags & NSEventModifierFlagOption) != 0;
  //bool is_command = (modifier_flags & NSEventModifierFlagCommand);

  if (is_caps_lock != was_caps_lock) {
    arctic::PushInputKey(arctic::kKeyCapsLock, is_caps_lock, "");
  }
  if (is_shift != was_shift) {
    arctic::PushInputKey(arctic::kKeyShift, is_shift, "");
  }
  if (is_control != was_control) {
    arctic::PushInputKey(arctic::kKeyControl, is_control, "");
  }
  if (is_option != was_option) {
    arctic::PushInputKey(arctic::kKeyAlt, is_option, "");
  }
  //    if (is_command != was_command) {
  //        PushInputKey(kKeyControl, is_command);
  //    }

  was_caps_lock = is_caps_lock;
  was_shift = is_shift;
  was_control = is_control;
  was_option = is_option;
  //was_command = is_command;

}

- (void) keyDown: (NSEvent *)theEvent {
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
  msg.kind = arctic::InputMessage::kMouse;
  msg.keyboard.key = static_cast<unsigned int>(key_code);
  msg.keyboard.characters[0] = '\0';
  msg.keyboard.key_state = static_cast<unsigned int>(state);
  msg.mouse.pos = pos;
  if (is_scroll) {
    if (event.hasPreciseScrollingDeltas) {
      msg.mouse.wheel_delta = (arctic::Si32)[event scrollingDeltaY];
    } else {
      msg.mouse.wheel_delta = (arctic::Si32)[event deltaY];
    }
  } else {
    msg.mouse.wheel_delta = 0;
  }

  PushInputMessage(msg);
}

- (void)scrollWheel:(NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyNone state: 0 isScroll: true];
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

  NSAlert *alert = [[NSAlert alloc] init];
  [alert addButtonWithTitle: @"OK"];
  [alert setMessageText: @"Fatal Error"];
  [alert setInformativeText:
    [[NSString alloc] initWithUTF8String: full_message]];
  [alert setAlertStyle: NSAlertStyleCritical];
  [alert runModal];
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
  if (is_down) {
    strncpy(msg.keyboard.characters, characters.c_str(), 16);
    msg.keyboard.characters[15] = '\0';
  } else {
    msg.keyboard.characters[0] = '\0';
  }
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
      arctic::Log("Failed to create NSOpenGLPixelFormat.",
        " No matching pixel format for the requested attributes.");
    }

    g_main_view = [[ArcticView alloc]
      initWithFrame: [g_main_window frame] pixelFormat: format];

    [g_main_view setWantsBestResolutionOpenGLSurface: YES];

    [[g_main_view openGLContext] makeCurrentContext];

    [g_main_window setContentView: g_main_view];
    [g_main_window makeFirstResponder: g_main_view];
    [g_main_window makeKeyAndOrderFront: nil];
    [g_main_window makeMainWindow];
    [NSApp activateIgnoringOtherApps: YES];

    if (g_is_full_screen) {
      g_is_full_screen = false;
      [g_main_window toggleFullScreen: nil];
    }

    NSLog(@"%d controllers found.", (int)[GCController controllers].count);

    NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
    system_info->screen_width = (arctic::Si32)rect.size.width;
    system_info->screen_height = (arctic::Si32)rect.size.height;
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
  [g_app terminate: nil];  // It will stop the logger
}

void Swap() {
  [[g_main_view openGLContext] flushBuffer];
  PumpMessages();

  static arctic::Si32 cached_width = 0;
  static arctic::Si32 cached_height = 0;
  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::Si32 w = (arctic::Si32)rect.size.width;
  arctic::Si32 h = (arctic::Si32)rect.size.height;
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
    !g_is_cursor_in_bounds);
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
  char *canonic_path = realpath(path, nullptr);
  std::string result;
  if (canonic_path) {
    result.assign(canonic_path);
    free(canonic_path);
  }
  return result;
}

// TODO(Huldra): Move common code out of macos and pi specific files.
std::string RelativePathFromTo(const char *from, const char *to) {
  std::string from_abs = CanonicalizePath(from);
  if (from && from[strlen(from) - 1] == '/' &&
      !from_abs.empty() && from_abs[from_abs.size() - 1] != '/') {
    from_abs = from_abs + '/';
  }
  std::string to_abs = CanonicalizePath(to);
  if (to && to[strlen(to) - 1] == '/' &&
      !to_abs.empty() && to_abs[to_abs.size() - 1] != '/') {
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

  const char *from_part = from_abs.c_str() + matching;

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

std::string PrepareInitialPath() {
  std::string initial_path([[[NSBundle mainBundle] bundlePath] UTF8String]);
  initial_path += "/..";
  initial_path = arctic::CanonicalizePath(initial_path.c_str());
  [[NSFileManager defaultManager] changeCurrentDirectoryPath:
    [NSString stringWithFormat:@"%@/Contents/Resources",
    [[NSBundle mainBundle] bundlePath]]];
  return initial_path;
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
  g_mixer = new arctic::SoundPlayer;
  g_mixer->Initialize();
  arctic::CreateMainWindow(&system_info);
  arctic::GetEngine()->SetArgcArgv(argc,
    const_cast<const char **>(argv));

  arctic::GetEngine()->SetInitialPath(initial_path);
  arctic::GetEngine()->Init(system_info.screen_width,
    system_info.screen_height);

  arctic::PumpMessages();

  arctic::PrepareForTheEasyMainCall();
  EasyMain();

  [g_app terminate: nil];  // It will stop the logger
  return 0;
}
#endif // ARCTIC_NO_MAIN

#endif
