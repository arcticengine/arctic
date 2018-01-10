// The MIT License(MIT)
//
// Copyright 2017 - 2018 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

#include <AudioToolbox/AudioToolbox.h>

#include <arpa/inet.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <mutex>  // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/easy.h"
#include "engine/arctic_input.h"
#include "engine/arctic_platform.h"
#include "engine/byte_array.h"
#include "engine/log.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"


extern void EasyMain();

namespace arctic {
  KeyCode TranslateKeyCode(Ui32 key_unicode);
  void PushInputKey(KeyCode key, bool is_down);
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

static ArcticWindow *g_main_window = nil;
static ArcticView *g_main_view = nil;
static NSApplication *g_app = nil;
static ArcticAppDelegate *g_app_delegate = nil;

static bool g_is_full_screen = false;


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

  exit(0);
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
  arctic::easy::GetEngine()->OnWindowResize(
      rect.size.width, rect.size.height);
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
  [g_app terminate: nil];
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

  bool is_caps_lock = (modifier_flags & NSEventModifierFlagCapsLock);
  bool is_shift = (modifier_flags & NSEventModifierFlagShift);
  bool is_control = (modifier_flags & NSEventModifierFlagControl);
  bool is_option = (modifier_flags & NSEventModifierFlagOption);
  //bool is_command = (modifier_flags & NSEventModifierFlagCommand);

  if (is_caps_lock != was_caps_lock) {
    arctic::PushInputKey(arctic::kKeyCapsLock, is_caps_lock);
  }
  if (is_shift != was_shift) {
    arctic::PushInputKey(arctic::kKeyShift, is_shift);
  }
  if (is_control != was_control) {
    arctic::PushInputKey(arctic::kKeyControl, is_control);
  }
  if (is_option != was_option) {
    arctic::PushInputKey(arctic::kKeyAlt, is_option);
  }
  //    if (is_command != was_command) {
  //        PushInputKey(kKeyControl, is_command);
  //    }
}

- (void) keyDown: (NSEvent *)theEvent {
  NSString *characters = [theEvent charactersIgnoringModifiers];
  if ([characters length] > 0) {
    unsigned key_unicode = [characters characterAtIndex: 0];
    arctic::KeyCode key = arctic::TranslateKeyCode(key_unicode);
    if (key == arctic::kKeyUnknown) {
      NSLog(@"Unknown character key_unicode: %d", key_unicode);
    }
    PushInputKey(key, true);
  }
}

- (void) keyUp: (NSEvent *)theEvent {
  NSString *characters = [theEvent charactersIgnoringModifiers];
  if ([characters length] > 0) {
    unsigned key_unicode = [characters characterAtIndex: 0];
    arctic::KeyCode key = arctic::TranslateKeyCode(key_unicode);
    PushInputKey(key, false);
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

  arctic::InputMessage msg;
  msg.kind = arctic::InputMessage::kMouse;
  msg.keyboard.key = key_code;
  msg.keyboard.key_state = state;
  msg.mouse.pos = pos;
  if (is_scroll) {
    if (event.hasPreciseScrollingDeltas) {
      msg.mouse.wheel_delta = [event scrollingDeltaY];
    } else {
      msg.mouse.wheel_delta = [event deltaY];
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
@end

namespace arctic {

Ui16 FromBe(Ui16 x) {
  return ntohs(x);
}
Si16 FromBe(Si16 x) {
  return ntohs(x);
}
Ui32 FromBe(Ui32 x) {
  return ntohl(x);
}
Si32 FromBe(Si32 x) {
  return ntohl(x);
}
Ui16 ToBe(Ui16 x) {
  return htons(x);
}
Si16 ToBe(Si16 x) {
  return htons(x);
}
Ui32 ToBe(Ui32 x) {
  return htonl(x);
}
Si32 ToBe(Si32 x) {
  return htonl(x);
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

  NSLog(@"Fatal: %s", full_message);

  NSAlert *alert = [[NSAlert alloc] init];
  [alert addButtonWithTitle: @"OK"];
  [alert setMessageText: @"Arctic Engine Error"];
  [alert setInformativeText:
    [[NSString alloc] initWithUTF8String: full_message]];
  [alert setAlertStyle: NSAlertStyleCritical];
  [alert runModal];
  exit(1);
}

void CheckStatus(OSStatus status, const char *message) {
  if (status == noErr) {
    return;
  }
  char code[20];
  *(UInt32 *)(code + 1) = CFSwapInt32HostToBig(status);
  if (isprint(code[1])
      && isprint(code[2])
      && isprint(code[3])
      && isprint(code[4])) {
    code[0] = '\'';
    code[5] = '\'';
    code[6] = '\0';
  } else {
    snprintf(code, 20, "%d", (int)status);
  }
  Fatal(message, code);
}

void PushInputKey(KeyCode key, bool is_down) {
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key;
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

void CreateMainWindow() {
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
  }
}

static std::mutex g_sound_mixer_mutex;
struct SoundBuffer {
  easy::Sound sound;
  float volume = 1.0f;
  Si32 next_position = 0;
};
struct SoundMixerState {
  float master_volume = 0.7f;
  std::vector<SoundBuffer> buffers;
};
SoundMixerState g_sound_mixer_state;

struct SoundMixer {
  AudioUnit output_unit = {0};
  std::vector<Si16> tmp;
  double starting_frame_count = 0.0;
  bool is_initialized = false;

  void Initialize();
  void Deinitialize();
  ~SoundMixer() {
    Deinitialize();
  }
};

OSStatus SoundRenderProc(void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData);

OSStatus SoundRenderProc(void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData) {
  SoundMixer *mixer = (SoundMixer*)inRefCon;

  Float32 *mixL = (Float32*)ioData->mBuffers[0].mData;
  Float32 *mixR = (Float32*)ioData->mBuffers[1].mData;
  memset(mixL, 0, inNumberFrames * sizeof(Float32));
  memset(mixR, 0, inNumberFrames * sizeof(Float32));

  float master_volume = 1.0f;
  {
    std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
    master_volume = g_sound_mixer_state.master_volume / 32767.0;
    for (Ui32 idx = 0; idx < g_sound_mixer_state.buffers.size(); ++idx) {
      SoundBuffer &sound = g_sound_mixer_state.buffers[idx];

      Ui32 size = inNumberFrames;
      size = sound.sound.StreamOut(sound.next_position, inNumberFrames,
          mixer->tmp.data(), inNumberFrames * 2);
      Si16 *in_data = mixer->tmp.data();
      for (Ui32 i = 0; i < size; ++i) {
        mixL[i] += static_cast<Si32>(
            static_cast<float>(in_data[i * 2]) * sound.volume);
        mixR[i] += static_cast<Si32>(
            static_cast<float>(in_data[i * 2 + 1]) * sound.volume);
        ++sound.next_position;
      }

      if (sound.next_position == sound.sound.DurationSamples()
          || size == 0) {
        sound.sound.GetInstance()->DecPlaying();
        g_sound_mixer_state.buffers[idx] =
          g_sound_mixer_state.buffers[
          g_sound_mixer_state.buffers.size() - 1];
        g_sound_mixer_state.buffers.pop_back();
        --idx;
      }
    }
  }

  for (int frame = 0; frame < inNumberFrames; ++frame) {
    mixL[frame] = Clamp(mixL[frame] * master_volume, -1.0, 1.0);
    mixR[frame] = Clamp(mixR[frame] * master_volume, -1.0, 1.0);
  }
  return noErr;
}

void SoundMixer::Initialize() {
  if (is_initialized) {
    return;
  }
  tmp.resize(2 << 20);

  AudioComponentDescription outputcd = {0};
  outputcd.componentType = kAudioUnitType_Output;
  outputcd.componentSubType = kAudioUnitSubType_DefaultOutput;
  outputcd.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent comp = AudioComponentFindNext(NULL, &outputcd);
  if (comp == NULL) {
    printf("can't get output unit");
    exit(-1);
  }
  OSStatus status = AudioComponentInstanceNew(comp, &output_unit);
  CheckStatus(status, "Couldn't open component for output_unit");

  AURenderCallbackStruct render;
  render.inputProc = SoundRenderProc;
  render.inputProcRefCon = this;
  status = AudioUnitSetProperty(output_unit,
      kAudioUnitProperty_SetRenderCallback,
      kAudioUnitScope_Input,
      0,
      &render,
      sizeof(render));
  CheckStatus(status, "AudioUnitSetProperty failed");

  status = AudioUnitInitialize(output_unit);
  CheckStatus(status, "Couldn't initialize output unit");

  status = AudioOutputUnitStart(output_unit);
  CheckStatus(status, "Couldn't start output unit");

  is_initialized = true;
}

void SoundMixer::Deinitialize() {
  if (is_initialized) {
    AudioOutputUnitStop(output_unit);
    AudioUnitUninitialize(output_unit);
    AudioComponentInstanceDispose(output_unit);
    is_initialized = false;
  }
}


void StartSoundBuffer(easy::Sound sound, float volume) {
  SoundBuffer buffer;
  buffer.sound = sound;
  buffer.volume = volume;
  buffer.next_position = 0;
  buffer.sound.GetInstance()->IncPlaying();
  std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
  g_sound_mixer_state.buffers.push_back(buffer);
}

void StopSoundBuffer(easy::Sound sound) {
  std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
  for (size_t idx = 0; idx < g_sound_mixer_state.buffers.size(); ++idx) {
    SoundBuffer &buffer = g_sound_mixer_state.buffers[idx];
    if (buffer.sound.GetInstance() == sound.GetInstance()) {
      buffer.sound.GetInstance()->DecPlaying();
      if (idx != g_sound_mixer_state.buffers.size() - 1) {
        g_sound_mixer_state.buffers[idx] =
          g_sound_mixer_state.buffers[
          g_sound_mixer_state.buffers.size() - 1];
      }
      g_sound_mixer_state.buffers.pop_back();
      idx--;
    }
  }
}

void SetMasterVolume(float volume) {
  std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
  g_sound_mixer_state.master_volume = volume;
}

float GetMasterVolume() {
  std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
  return g_sound_mixer_state.master_volume;
}

void PumpMessages() {
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

void Swap() {
  [[g_main_view openGLContext] flushBuffer];
  PumpMessages();

  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::easy::GetEngine()->OnWindowResize(
      rect.size.width, rect.size.height);
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
      forParameter: NSOpenGLCPSwapInterval];
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

}  // namespace arctic

int main(int argc, char *argv[]) {
  [[NSFileManager defaultManager] changeCurrentDirectoryPath:
    [NSString stringWithFormat:@"%@/Contents/Resources",
    [[NSBundle mainBundle] bundlePath]]];



  arctic::StartLogger();
  arctic::CreateMainWindow();

  NSRect rect = [g_main_view convertRectToBacking: [g_main_view frame]];
  arctic::easy::GetEngine()->Init(rect.size.width, rect.size.height);

  arctic::SoundMixer mixer;
  mixer.Initialize();

  EasyMain();

  arctic::StopLogger();

  return 0;
}

#endif
