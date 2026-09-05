// Input: typed text, mouse coordinates, key hold times.
#define TEST_NO_MAIN
#include "test_helpers.h"

// The platform code fills the characters of a message through one function, so
// that every platform reports the same thing for the keys that the system calls
// characters: Tab, Enter, Escape, Backspace, Control with a letter.
void test_typed_characters_of_a_message() {
  InputMessage::Keyboard keyboard;
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\t"),
      "a tabulation was taken for text");
  TEST_CHECK(keyboard.characters[0] == '\0');
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\r"), "Enter was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\x01"),
      "Control with a letter was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, "\x7f"),
      "Backspace was taken for text");
  TEST_CHECK_(!SetTypedCharacters(&keyboard, nullptr),
      "a keystroke with no text at all was taken for text");
  TEST_CHECK(keyboard.characters[0] == '\0');

  TEST_CHECK_(SetTypedCharacters(&keyboard, "a"), "a letter is text");
  TEST_CHECK_(std::string(keyboard.characters) == "a",
      "a latin letter became '%s'", keyboard.characters);
  const char *kEf = "\xd1\x84";  // Cyrillic small letter ef, two bytes
  TEST_CHECK(SetTypedCharacters(&keyboard, kEf));
  TEST_CHECK_(std::string(keyboard.characters) == kEf,
      "a Cyrillic letter became '%s'", keyboard.characters);

  // A control character next to a letter loses only itself.
  TEST_CHECK(SetTypedCharacters(&keyboard, "\tx"));
  TEST_CHECK_(std::string(keyboard.characters) == "x",
      "a tabulation with a letter became '%s'", keyboard.characters);

  // More text than the message can hold stays inside the array and zero ended.
  TEST_CHECK(SetTypedCharacters(&keyboard, "0123456789abcdefghij"));
  TEST_CHECK_(std::string(keyboard.characters) == "0123456789abcde",
      "a long text became '%s'", keyboard.characters);
}

// The keys are physical, and a physical shift is either the left or the right
// one, but code (the engine's own GUI included) asks about the generic kKeyShift.
// The typed text of the frame is available without walking the message queue.
void test_typed_text_and_generic_modifiers() {
  InputMessage right_shift;
  right_shift.kind = InputMessage::kKeyboard;
  right_shift.keyboard.key = kKeyRightShift;
  right_shift.keyboard.key_state = 1;
  PushInputMessage(right_shift);

  InputMessage typed;
  typed.kind = InputMessage::kKeyboard;
  typed.keyboard.key = kKeyA;
  typed.keyboard.key_state = 1;
  snprintf(typed.keyboard.characters, sizeof(typed.keyboard.characters),
      "%s", "\xd0\xa4");  // Cyrillic capital ef, typed with shift held
  PushInputMessage(typed);
  ShowFrame();

  TEST_CHECK_(IsKeyDown(kKeyRightShift), "the right shift is not down");
  TEST_CHECK_(IsKeyDown(kKeyShift),
      "a physical shift must also answer for the generic kKeyShift");
  TEST_CHECK_(IsKeyDown(kKeyA), "the physical key of the keystroke is not down");
  TEST_CHECK_(TypedText() == std::string("\xd0\xa4"),
      "TypedText gave '%s'", TypedText().c_str());

  // Holding the other shift keeps the generic one down when the first is let go.
  InputMessage left_shift;
  left_shift.kind = InputMessage::kKeyboard;
  left_shift.keyboard.key = kKeyLeftShift;
  left_shift.keyboard.key_state = 1;
  PushInputMessage(left_shift);
  right_shift.keyboard.key_state = 2;
  PushInputMessage(right_shift);
  ShowFrame();
  TEST_CHECK_(IsKeyDown(kKeyShift),
      "the generic shift went up while the left one is still held");

  left_shift.keyboard.key_state = 2;
  PushInputMessage(left_shift);
  typed.keyboard.key_state = 2;
  typed.keyboard.characters[0] = '\0';
  PushInputMessage(typed);
  ShowFrame();
  TEST_CHECK_(!IsKeyDown(kKeyShift),
      "the generic shift stayed down after both shifts were released");
  TEST_CHECK_(IsKeyUpward(kKeyShift),
      "the release of the last shift was not reported as an upward edge");
  TEST_CHECK_(TypedText().empty(),
      "the typed text survived the frame: '%s'", TypedText().c_str());

  // Keys that are not text are reported with a control byte for a character by
  // one platform or another, and none of that belongs in the typed text.
  InputMessage tab;
  tab.kind = InputMessage::kKeyboard;
  tab.keyboard.key = kKeyTab;
  tab.keyboard.key_state = 1;
  tab.keyboard.characters[0] = '\t';
  PushInputMessage(tab);
  InputMessage control_letter;
  control_letter.kind = InputMessage::kKeyboard;
  control_letter.keyboard.key = kKeyA;
  control_letter.keyboard.key_state = 1;
  control_letter.keyboard.characters[0] = '\x01';  // Control with a letter
  PushInputMessage(control_letter);
  InputMessage letter;
  letter.kind = InputMessage::kKeyboard;
  letter.keyboard.key = kKeyX;
  letter.keyboard.key_state = 1;
  letter.keyboard.characters[0] = 'x';
  PushInputMessage(letter);
  ShowFrame();
  TEST_CHECK_(TypedText() == std::string("x"),
      "the typed text of a frame with Tab and Control+A gave '%s'",
      TypedText().c_str());

  tab.keyboard.key_state = 2;
  PushInputMessage(tab);
  control_letter.keyboard.key_state = 2;
  PushInputMessage(control_letter);
  letter.keyboard.key_state = 2;
  PushInputMessage(letter);
  ShowFrame();
}

// The mouse reaches the game in the pixels the game draws in: backbuffer
// pixels, y upward, clamped to the backbuffer. A position that arrives in
// window pixels, or counted from the top, or unclamped over a letterbox bar,
// is what makes a click land beside the thing it was aimed at.
void test_mouse_arrives_in_backbuffer_pixels() {
  ResizeScreen(320, 200);
  const Vec2Si32 size = ScreenSize();
  TEST_CHECK_(size == Vec2Si32(320, 200), "the backbuffer is %d x %d",
      size.x, size.y);

  // The engine takes a mouse position as a fraction of the window along each
  // axis, y upward, and hands the game backbuffer pixels.
  auto move_mouse_to = [](float x, float y) {
    InputMessage message;
    message.kind = InputMessage::kMouse;
    message.mouse.pos = Vec2F(x, y);
    PushInputMessage(message);
    ShowFrame();
  };

  move_mouse_to(0.0f, 0.0f);
  TEST_CHECK_(MousePos() == Vec2Si32(0, 0),
      "the bottom-left corner of the window came out as (%d, %d)",
      MousePos().x, MousePos().y);

  move_mouse_to(1.0f, 1.0f);
  TEST_CHECK_(MousePos() == size - Vec2Si32(1, 1),
      "the top-right corner of the window came out as (%d, %d) instead of "
      "(%d, %d)", MousePos().x, MousePos().y, size.x - 1, size.y - 1);

  // Y grows upward: the upper half of the window is the upper half of the
  // backbuffer, not the lower one.
  move_mouse_to(0.5f, 0.8f);
  const Si32 high_y = MousePos().y;
  move_mouse_to(0.5f, 0.2f);
  const Si32 low_y = MousePos().y;
  TEST_CHECK_(high_y > size.y / 2,
      "a cursor in the upper part of the window reported y = %d of %d",
      high_y, size.y);
  TEST_CHECK_(low_y < size.y / 2,
      "a cursor in the lower part of the window reported y = %d of %d",
      low_y, size.y);
  TEST_CHECK_(high_y > low_y,
      "y did not grow upward: %d up against %d down", high_y, low_y);

  // A position outside the window, which is what the bars of a letterboxed
  // window and a dragged cursor produce, is clamped to the backbuffer instead
  // of naming a pixel that does not exist.
  move_mouse_to(-0.5f, 1.5f);
  TEST_CHECK_(MousePos().x == 0 && MousePos().y == size.y - 1,
      "a position outside the window came out as (%d, %d), which is not "
      "clamped to the backbuffer", MousePos().x, MousePos().y);

  // The same number is in the message queue, which is where the GUI reads it,
  // so a game handling messages itself sees exactly what MousePos() says.
  move_mouse_to(0.25f, 0.75f);
  Si32 mouse_messages = 0;
  for (Si32 i = 0; i < InputMessageCount(); ++i) {
    const InputMessage &message = GetInputMessage(i);
    if (message.kind != InputMessage::kMouse) {
      continue;
    }
    ++mouse_messages;
    TEST_CHECK_(message.mouse.backbuffer_pos == MousePos(),
        "the message says (%d, %d) while MousePos says (%d, %d)",
        message.mouse.backbuffer_pos.x, message.mouse.backbuffer_pos.y,
        MousePos().x, MousePos().y);
  }
  TEST_CHECK_(mouse_messages == 1,
      "the frame carried %d mouse messages instead of one", mouse_messages);

  // Negative control for the direction of y: the position a game would compute
  // by counting from the top must differ from the one the engine reports,
  // otherwise the checks above hold for both conventions and prove nothing.
  const Si32 from_the_top = size.y - 1 - MousePos().y;
  TEST_CHECK_(from_the_top != MousePos().y,
      "a cursor was placed exactly in the middle of the window, where the two "
      "conventions agree, so this test proves nothing about the direction");
}

// How long a key has been held is a question every game with a charged shot or
// a repeat-after-delay had to answer for itself, remembering the moment of the
// press by hand. The engine remembers it now.
void test_key_down_seconds_measures_the_hold() {
  SetKey(kKeySpace, false);
  ShowFrame();
  TEST_CHECK_(KeyDownSeconds(kKeySpace) == 0.0,
      "a key that is not held reported %f seconds",
      KeyDownSeconds(kKeySpace));

  SetKey(kKeySpace, true);
  const double at_the_press = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(at_the_press >= 0.0 && at_the_press < 0.05,
      "right after the press the hold is %f seconds", at_the_press);

  Sleep(0.05);
  ShowFrame();
  const double after_a_frame = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(after_a_frame >= 0.05,
      "after fifty milliseconds of holding the answer is %f seconds",
      after_a_frame);
  TEST_CHECK_(after_a_frame > at_the_press,
      "the hold did not grow: %f then %f", at_the_press, after_a_frame);
  TEST_CHECK_(IsKeyDown(kKeySpace),
      "the key stopped being down while nothing released it");
  TEST_CHECK_(!IsKeyDownward(kKeySpace),
      "a key held since the previous frame must not be a fresh press");

  Sleep(0.05);
  ShowFrame();
  const double after_two_frames = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(after_two_frames > after_a_frame,
      "the hold stopped growing across frames: %f then %f",
      after_a_frame, after_two_frames);

  SetKey(kKeySpace, false);
  ShowFrame();
  TEST_CHECK_(KeyDownSeconds(kKeySpace) == 0.0,
      "a released key still reports %f seconds", KeyDownSeconds(kKeySpace));

  // Negative control for the reset: a version that kept the old moment of the
  // press would answer with the whole time since the first press, which is
  // more than what has passed since this one.
  SetKey(kKeySpace, true);
  const double second_press = KeyDownSeconds(kKeySpace);
  TEST_CHECK_(second_press < after_two_frames,
      "the second press reports %f seconds, and the first hold was %f, so the "
      "moment of the press was not taken anew", second_press,
      after_two_frames);
  SetKey(kKeySpace, false);
  ShowFrame();

  // The string form asks about several keys at once and answers with the
  // longest hold, the same way IsKeyDown answers about any of them.
  SetKey('a', true);
  Sleep(0.05);
  SetKey('d', true);
  ShowFrame();
  // The clock runs between the calls, so the three readings are compared with
  // a millisecond of slack rather than for equality.
  const double both = KeyDownSeconds("ad");
  const double held_a = KeyDownSeconds('a');
  const double held_d = KeyDownSeconds('d');
  TEST_CHECK_(held_a > held_d,
      "the key pressed earlier is held for %f while the later one is %f",
      held_a, held_d);
  TEST_CHECK_(both > held_d + 0.03 && both <= held_a
          && both > held_a - 0.005,
      "asking about both keys gave %f, and the two holds are %f and %f, so it "
      "is not the longer of them", both, held_a, held_d);
  TEST_CHECK_(KeyDownSeconds("qz") == 0.0,
      "keys nobody pressed reported %f seconds", KeyDownSeconds("qz"));
  SetKey('a', false);
  SetKey('d', false);
  ShowFrame();
}

static void PushKeyEdge(KeyCode key, bool is_down) {
  InputMessage message;
  message.kind = InputMessage::kKeyboard;
  message.keyboard.key = key;
  message.keyboard.key_state = is_down ? 1 : 2;
  message.keyboard.characters[0] = '\0';
  PushInputMessage(message);
}

// IsKeyDownward / IsKeyUpward name the real edges of a key, not every OS
// repeat or a second release of a key that is already up. A latch that steps
// on IsKeyUpward (the global map numpad) otherwise sees a phantom release and
// swallows the next real tap of KP_1 / KP_3.
void test_key_edges_follow_real_transitions() {
  SetKey(kKeyNumpad1, false);
  SetKey(kKeyNumpad3, false);
  ShowFrame();

  PushKeyEdge(kKeyNumpad1, true);
  ShowFrame();
  TEST_CHECK_(IsKeyDown(kKeyNumpad1), "KP_1 did not go down");
  TEST_CHECK_(IsKeyDownward(kKeyNumpad1), "the first press of KP_1 was not a downward edge");

  PushKeyEdge(kKeyNumpad1, true);
  ShowFrame();
  TEST_CHECK_(IsKeyDown(kKeyNumpad1), "KP_1 came up on a repeated down");
  TEST_CHECK_(!IsKeyDownward(kKeyNumpad1),
      "a repeated down of a held key was taken for a fresh press");
  TEST_CHECK_(!IsKeyUpward(kKeyNumpad1),
      "a repeated down of a held key was taken for a release");

  PushKeyEdge(kKeyNumpad1, false);
  ShowFrame();
  TEST_CHECK_(!IsKeyDown(kKeyNumpad1), "KP_1 stayed down after the release");
  TEST_CHECK_(IsKeyUpward(kKeyNumpad1), "the release of KP_1 was not an upward edge");

  PushKeyEdge(kKeyNumpad1, false);
  ShowFrame();
  TEST_CHECK_(!IsKeyDown(kKeyNumpad1), "a duplicate release brought KP_1 down");
  TEST_CHECK_(!IsKeyUpward(kKeyNumpad1),
      "a release of a key that is already up was reported as an upward edge");
  TEST_CHECK_(!IsKeyDownward(kKeyNumpad1),
      "a duplicate release was reported as a press");

  PushKeyEdge(kKeyNumpad1, true);
  PushKeyEdge(kKeyNumpad1, false);
  ShowFrame();
  TEST_CHECK_(IsKeyDownward(kKeyNumpad1),
      "a tap that went down and up in one frame lost the downward edge");
  TEST_CHECK_(IsKeyUpward(kKeyNumpad1),
      "a tap that went down and up in one frame lost the upward edge");
  TEST_CHECK_(!IsKeyDown(kKeyNumpad1),
      "a tap that ended in the same frame left KP_1 down");

  PushKeyEdge(kKeyNumpad3, true);
  ShowFrame();
  PushKeyEdge(kKeyNumpad3, false);
  ShowFrame();
  TEST_CHECK_(IsKeyUpward(kKeyNumpad3), "the first tap of KP_3 was not an upward edge");
  PushKeyEdge(kKeyNumpad3, true);
  ShowFrame();
  PushKeyEdge(kKeyNumpad3, false);
  ShowFrame();
  TEST_CHECK_(IsKeyUpward(kKeyNumpad3),
      "the second tap of KP_3 was swallowed");
  TEST_CHECK_(!IsKeyDown(kKeyNumpad3), "KP_3 stayed down after the second tap");
}
