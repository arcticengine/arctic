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

#define _USE_MATH_DEFINES
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
  KeyCode TranslateKeyCode(Ui32 key_unicode);
  void PushInputKey(KeyCode key, bool is_down, std::string characters);
}  // namespace arctic

@interface ArcticAppDelegate : NSObject {
}
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:
(NSApplication *)theApplication;
- (void) fullScreenToggle:(NSNotification *)notification;
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

  if (g_is_full_screen) {
    if (CGDisplayRelease(kCGDirectMainDisplay) != kCGErrorSuccess) {
      arctic::Fatal("Couldn't release the display(s)!", "");
    }
  }

  [[NSApplication sharedApplication] terminate:nil];
//  exit(0);
}
- (void) fullScreenToggle:(NSNotification *)notification {
  if (!g_is_full_screen) {
    if (CGDisplayCapture( kCGDirectMainDisplay ) != kCGErrorSuccess) {
      arctic::Fatal("Couldn't capture the main display!", "");
    }
    [g_main_window setLevel: CGShieldingWindowLevel()];
    [g_main_window setStyleMask: NSWindowStyleMaskBorderless];
    [g_main_window setFrame: [[NSScreen mainScreen] frame]
      display: YES animate: NO];
  } else {
    if (CGDisplayRelease(kCGDirectMainDisplay) != kCGErrorSuccess) {
      NSLog(@"Couldn't release the display(s)!");
      [g_main_window setLevel: CGShieldingWindowLevel()];
    } else {
      [g_main_window setLevel:NSNormalWindowLevel];
    }
    [g_main_window setStyleMask:
      NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable];
    [g_main_window setFrame: [[NSScreen mainScreen] visibleFrame]
      display: YES animate: NO];
  }
  [g_main_window makeFirstResponder: g_main_view];
  g_is_full_screen = !g_is_full_screen;
  
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
  NSString *characters = [theEvent charactersIgnoringModifiers];
  if ([characters length] > 0) {
    unsigned key_unicode = [characters characterAtIndex: 0];
    arctic::KeyCode key = arctic::TranslateKeyCode(key_unicode);
    if (key == arctic::kKeyUnknown) {
      NSLog(@"Unknown character key_unicode: %d", key_unicode);
    }
    PushInputKey(key, true, [[theEvent characters] UTF8String]);
  }
}

- (void) keyUp: (NSEvent *)theEvent {
  NSString *characters = [theEvent charactersIgnoringModifiers];
  if ([characters length] > 0) {
    unsigned key_unicode = [characters characterAtIndex: 0];
    arctic::KeyCode key = arctic::TranslateKeyCode(key_unicode);
    PushInputKey(key, false, "");
  }
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
  [self mouseEvent: event key: arctic::kKeyCount state: 0 isScroll: true];
}

- (void) mouseMoved: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyCount state: 0 isScroll: false];
}

- (void) mouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyCount state: 0 isScroll: false];
}

- (void) rightMouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyCount state: 0 isScroll: false];
}

- (void) otherMouseDragged: (NSEvent *)event {
  [self mouseEvent: event key: arctic::kKeyCount state: 0 isScroll: false];
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

  NSLog(@"%d controller idx.", (int)controller_idx);

  if (controller_idx < 0 || controller_idx >= arctic::InputMessage::kControllerCount) {
    return;
  }
  arctic::InputMessage msg;
  msg.kind = arctic::InputMessage::kController;

  msg.keyboard.characters[0] = '\0';
  msg.keyboard.key = arctic::kKeyCount;
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
#ifndef ARCTIC_NO_HARD_EXIT
  exit(1);
#else
  delete[] full_message;
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

KeyCode TranslateKeyCode(Ui32 key_unicode) {
  if (key_unicode >= 'A' && key_unicode <= 'Z') {
    return static_cast<KeyCode>(key_unicode - 'A' + kKeyA);
  }
  if (key_unicode >= 'a' && key_unicode <= 'z') {
    return static_cast<KeyCode>(key_unicode - 'a' + kKeyA);
  }
  if (key_unicode >= '0' && key_unicode <= '9') {
    return static_cast<KeyCode>(key_unicode - '0' + kKey0);
  }
  if (key_unicode >= 0xf704 && key_unicode <= 0xf70f) {
    return static_cast<KeyCode>(key_unicode - 0xf704 + kKeyF1);
  }

  switch (key_unicode) {
    case 33:
      return kKey1;
    case 64:
      return kKey2;
    case 35:
      return kKey3;
    case 36:
      return kKey4;
    case 37:
      return kKey5;
    case 94:
      return kKey6;
    case 38:
      return kKey7;
    case 42:
      return kKey8;
    case 40:
      return kKey9;
    case 41:
      return kKey0;

    case 0xf702:
      return kKeyLeft;
    case 0xf703:
      return kKeyRight;
    case 0xf700:
      return kKeyUp;
    case 0xf701:
      return kKeyDown;
    case 127:
      return kKeyBackspace;
    case 9:
    case 25:
      return kKeyTab;
    case 3:
    case 13:
      return kKeyEnter;
    case 0xF729:
      return kKeyHome;
    case 0xF72B:
      return kKeyEnd;
    case 0xF72C:
      return kKeyPageUp;
    case 0xF72D:
      return kKeyPageDown;
      /*    case VK_SHIFT:
            return kKeyShift;
            case VK_LSHIFT:
            return kKeyLeftShift;
            case VK_RSHIFT:
            return kKeyRightShift;
            case VK_CONTROL:
            return kKeyControl;
            case VK_LCONTROL:
            return kKeyLeftControl;
            case VK_RCONTROL:
            return kKeyRightControl;
            case VK_MENU:
            return kKeyAlt;
            case VK_LMENU:
            return kKeyLeftAlt;
            case VK_RMENU:
            return kKeyRightAlt;*/
    case 27:
      return kKeyEscape;
    case 32:
      return kKeySpace;
    case 0xF730:
      return kKeyPause;
      /*    case VK_NUMLOCK:
            return kKeyNumLock;*/
    case 0xF72F:
      return kKeyScrollLock;
      /*    case VK_CAPITAL:
            return kKeyCapsLock;*/
    case 0xF72E:
      return kKeyPrintScreen;
    case 0xF727:
      return kKeyInsert;
    case 0xF728:
      return kKeyDelete;
      /*    case VK_DIVIDE:
            return kKeyNumpadSlash;
            case VK_MULTIPLY:
            return kKeyNumpadAsterisk;
            case VK_SUBTRACT:
            return kKeyNumpadMinus;
            case VK_ADD:
            return kKeyNumpadPlus;
            case VK_DECIMAL:
            return kKeyNumpadPeriod;*/
    case 44:
    case 60:
      return kKeyComma;
    case 46:
    case 62:
      return kKeyPeriod;
    case 45:
    case 95:
      return kKeyMinus;
    case 43:
    case 61:
      return kKeyEquals;
    case 58:
    case 59:
      return kKeySemicolon;
    case 47:
    case 63:
      return kKeySlash;
    case 96:
    case 126:
      return kKeyGraveAccent;
    case 91:
    case 123:
      return kKeyLeftSquareBracket;
    case 92:
    case 124:
      return kKeyBackslash;
    case 93:
    case 125:
      return kKeyRightSquareBracket;
    case 34:
    case 39:
      return kKeyApostrophe;
    case 167:
    case 177:
      return kKeySectionSign;
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


    if (g_is_full_screen) {
      if (CGDisplayCapture(kCGDirectMainDisplay) != kCGErrorSuccess) {
        arctic::Fatal("Couldn't capture the main display!", "");
      }
      int windowLevel = CGShieldingWindowLevel();
      g_main_window = [[ArcticWindow alloc]
        initWithContentRect: [[NSScreen mainScreen] frame]
        styleMask: NSWindowStyleMaskBorderless
        backing: NSBackingStoreBuffered
        defer: NO
        screen: [NSScreen mainScreen]];
      [g_main_window setLevel: windowLevel];
    } else {
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
    }

    NSOpenGLPixelFormatAttribute format_attribute[] = {
      NSOpenGLPFADepthSize,
      (NSOpenGLPixelFormatAttribute)32,
      NSOpenGLPFADoubleBuffer,
      0};
    NSOpenGLPixelFormat *format =
      [[NSOpenGLPixelFormat alloc] initWithAttributes: format_attribute];

    g_main_view = [[ArcticView alloc]
      initWithFrame: [g_main_window frame] pixelFormat: format];

    [g_main_view setWantsBestResolutionOpenGLSurface: YES];

    [[g_main_view openGLContext] makeCurrentContext];

    [g_main_window setContentView: g_main_view];
    [g_main_window makeFirstResponder: g_main_view];
    [g_main_window makeKeyAndOrderFront: nil];
    [g_main_window makeMainWindow];
    [NSApp activateIgnoringOtherApps: YES];


    NSArray *controllers = [GCController controllers];
    NSLog(@"%d controllers found.", (int)controllers.count);

    for (GCController *element in [GCController controllers]) {

      if (element.extendedGamepad) {
          element.extendedGamepad.valueChangedHandler=^(GCExtendedGamepad *gamepad, GCControllerElement *element) {
              [g_main_view extendedGamepadAction:gamepad forElement:element];
         };
      }
    }

    NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
    system_info->screen_width = (arctic::Si32)rect.size.width;
    system_info->screen_height = (arctic::Si32)rect.size.height;
  }
}

void PumpMessages() {

  NSArray *controllers = [GCController controllers];
  if (controllers.count) {
    //NSLog(@"%d controllers found.", (int)controllers.count);
    for (Si32 controller_idx = 0; controller_idx < controllers.count; ++controller_idx) {
      if ([GCController controllers][controller_idx].playerIndex == -1) {
        std::map<Si32, Si32> player_to_idx;
        for (Si32 controller_idx = 0; controller_idx < controllers.count; ++controller_idx) {
          Si32 player =(Si32)[GCController controllers][controller_idx].playerIndex;
          if (player != -1) {
            player_to_idx[player] = controller_idx;
          }
        }
        for (Si32 player_idx = 0; player_idx < InputMessage::kControllerCount; ++player_idx) {
          if (player_to_idx.find(player_idx) == player_to_idx.end()) {
            [GCController controllers][controller_idx].playerIndex = (GCControllerPlayerIndex)player_idx;
            break;
          }
        }
      }
      if ([GCController controllers][controller_idx].playerIndex != -1) {
        [g_main_view extendedGamepadAction:[GCController controllers][controller_idx].extendedGamepad forElement: nil];
      }
    }

    g_controller = [GCController controllers][0];
    if (@available(macOS 11.0, *)) {
      //NSLog(@"%d axes", (int)g_controller.physicalInputProfile.allAxes.count);
      //NSLog(@"axis[0]: %f", (float)g_controller.physicalInputProfile.allAxes.allObjects[0].value);
    } else {
      // Fallback on earlier versions
    }
   /* NSLog(@"axis[0]: %f", (float)g_controller.extendedGamepad.leftThumbstick.xAxis.value);
    NSLog(@"axis[1]: %f", (float)g_controller.extendedGamepad.leftThumbstick.yAxis.value);
    NSLog(@"axis[2]: %f", (float)g_controller.extendedGamepad.rightThumbstick.xAxis.value);
    NSLog(@"axis[3]: %f", (float)g_controller.extendedGamepad.rightThumbstick.yAxis.value);
    NSLog(@"axis[4]: %f", (float)g_controller.extendedGamepad.leftTrigger.value);
    NSLog(@"axis[5]: %f", (float)g_controller.extendedGamepad.rightTrigger.value);
*/

  } else {
    g_controller = nil;
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
        [NSApp updateWindows];
      } else {
        break;
      }
    }
  }
}

void ExitProgram(Si32 exit_code) {
  g_exit_code = exit_code;
  [g_app terminate: nil];  // It will stop the logger
}

void Swap() {
  [[g_main_view openGLContext] flushBuffer];
  PumpMessages();

  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::GetEngine()->OnWindowResize(
      (arctic::Si32)rect.size.width, (arctic::Si32)rect.size.height);
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
    snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);
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
