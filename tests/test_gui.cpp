// The GUI: panels, input routing, every widget of engine/gui.h.
#define TEST_NO_MAIN
#include "test_helpers.h"

// Bug 62: Panel with top+bottom (or left+right) anchoring gets negative
// size when the parent shrinks below the sum of anchor distances.
void test_panel_anchor_no_negative_size() {
  auto parent = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(400, 300));

  auto child = std::make_shared<Panel>(1, Vec2Si32(20, 30), Vec2Si32(360, 240));
  parent->AddChild(child);
  child->SetAnchor(static_cast<AnchorKind>(kAnchorLeft | kAnchorRight |
                                            kAnchorBottom | kAnchorTop));

  // anchor_left_d_ = 20, anchor_right_d_ = 400 - 360 - 20 = 20
  // anchor_bottom_d_ = 30, anchor_top_d_ = 300 - 240 - 30 = 30
  // So sum of horizontal anchors = 40, sum of vertical anchors = 60.

  // Shrink parent below the anchor distances.
  parent->SetSize(Vec2Si32(30, 40));

  Vec2Si32 sz = child->GetSize();
  TEST_CHECK_(sz.x >= 0,
      "Child width must not be negative after parent shrink, got %d", sz.x);
  TEST_CHECK_(sz.y >= 0,
      "Child height must not be negative after parent shrink, got %d", sz.y);
}

// An editbox has to tell its host what happened to it: that the text changed,
// and that the editing is over. Without it the host is left comparing GetText()
// against a remembered copy every frame, which is what deca used to do.
void test_editbox_reports_text_change_and_edit_done() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite normal;
  normal.Create(60, 12);
  Sprite focused;
  focused.Create(60, 12);

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(200, 100));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, normal, focused,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto other = std::make_shared<Editbox>(2, Vec2Si32(10, 40), 2, normal,
      focused, font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(other);

  Si32 changes = 0;
  Si32 dones = 0;
  box->OnTextChange = [&changes]() { ++changes; };
  box->OnEditDone = [&dones]() { ++dones; };

  // Nothing is focused yet, so nothing holds the keyboard.
  TEST_CHECK(!root->IsKeyboardCaptured());
  TEST_CHECK(!box->IsFocused());

  box->SetCurrentTab(true);
  TEST_CHECK(box->IsFocused());
  TEST_CHECK_(root->IsKeyboardCaptured(),
      "a focused editbox must hold the keyboard");

  std::deque<GuiMessage> messages;
  InputMessage typed;
  typed.kind = InputMessage::kKeyboard;
  typed.keyboard.key = kKeyA;
  typed.keyboard.key_state = 1;
  typed.keyboard.characters[0] = 'a';
  bool is_applied = false;
  std::shared_ptr<Panel> current_tab;
  box->ApplyInput(Vec2Si32(0, 0), typed, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string("a"),
      "the keystroke did not reach the text: '%s'", box->GetText().c_str());
  TEST_CHECK_(changes == 1, "OnTextChange fired %d times, expected 1",
      (int)changes);
  TEST_CHECK_(dones == 0, "the editing is not over yet");
  Si32 change_messages = 0;
  for (const GuiMessage &m : messages) {
    if (m.kind == kGuiEditboxTextChange) {
      ++change_messages;
    }
  }
  TEST_CHECK_(change_messages == 1,
      "%d text change messages were queued, expected 1", (int)change_messages);

  // A keystroke that leaves the text alone is not a change.
  InputMessage arrow;
  arrow.kind = InputMessage::kKeyboard;
  arrow.keyboard.key = kKeyLeft;
  arrow.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), arrow, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(changes == 1, "moving the caret counted as a text change");

  // Enter ends the editing in a single line box and is left for the host.
  InputMessage enter;
  enter.kind = InputMessage::kKeyboard;
  enter.keyboard.key = kKeyEnter;
  enter.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), enter, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(dones == 1, "OnEditDone fired %d times on Enter, expected 1",
      (int)dones);
  TEST_CHECK_(!is_applied, "Enter must be left to the host");

  // Losing the focus ends the editing as well.
  root->SwitchCurrentTab(true);
  TEST_CHECK_(!box->IsFocused(), "the focus did not move away");
  TEST_CHECK_(dones == 2,
      "OnEditDone fired %d times after losing the focus, expected 2",
      (int)dones);

  // A hidden field can not be typed into, so it does not hold the keyboard.
  TEST_CHECK(other->IsFocused());
  TEST_CHECK(root->IsKeyboardCaptured());
  other->SetVisible(false);
  TEST_CHECK_(!root->IsKeyboardCaptured(),
      "a hidden editbox still claims the keyboard");

  // A visible field inside a hidden panel can not be typed into either: the walk
  // over the panels stops at the hidden one and never asks the children.
  other->SetVisible(true);
  TEST_CHECK(root->IsKeyboardCaptured());
  root->SetVisible(false);
  TEST_CHECK_(!root->IsKeyboardCaptured(),
      "an editbox inside a hidden panel still claims the keyboard");
  root->SetVisible(true);
}

// The keys report physical positions, so an editbox can not take the letter from
// the key code: it has to use the typed text, or a Cyrillic layout would type
// latin letters. A key that carries no text at all (Windows sends the key and
// the character as two separate messages) must add nothing.
void test_editbox_accepts_any_layout() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite normal;
  normal.Create(60, 12);
  Sprite focused;
  focused.Create(60, 12);

  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, normal, focused,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  box->SetCurrentTab(true);

  std::deque<GuiMessage> messages;
  std::shared_ptr<Panel> current_tab;
  const char *kEf = "\xd1\x84";  // Cyrillic small letter ef, the key of latin A

  InputMessage cyrillic;
  cyrillic.kind = InputMessage::kKeyboard;
  cyrillic.keyboard.key = kKeyA;
  cyrillic.keyboard.key_state = 1;
  snprintf(cyrillic.keyboard.characters, sizeof(cyrillic.keyboard.characters),
      "%s", kEf);
  bool is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), cyrillic, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf),
      "a Cyrillic keystroke gave '%s'", box->GetText().c_str());

  InputMessage latin;
  latin.kind = InputMessage::kKeyboard;
  latin.keyboard.key = kKeyA;
  latin.keyboard.key_state = 1;
  latin.keyboard.characters[0] = 'a';
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), latin, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "a latin keystroke after a Cyrillic one gave '%s'",
      box->GetText().c_str());

  InputMessage bare;
  bare.kind = InputMessage::kKeyboard;
  bare.keyboard.key = kKeyA;
  bare.keyboard.key_state = 1;
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), bare, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "a key without typed text inserted something: '%s'",
      box->GetText().c_str());

  // Tab moves the focus and is not text, even when it comes with a tabulation
  // for a character, and even when this box is the only one that can be focused.
  InputMessage tab;
  tab.kind = InputMessage::kKeyboard;
  tab.keyboard.key = kKeyTab;
  tab.keyboard.key_state = 1;
  tab.keyboard.characters[0] = '\t';
  is_applied = false;
  box->ApplyInput(Vec2Si32(0, 0), tab, true, &is_applied, &messages,
      &current_tab);
  TEST_CHECK_(box->GetText() == std::string(kEf) + "a",
      "Tab put something in the text: '%s'", box->GetText().c_str());
}

// "Is this click mine or the interface's?" is the question a game asks before
// selecting something in the world, and IsInside answers it about a point. The
// answer has to agree with what the input walk actually does, so every case is
// checked against ApplyInput as well.
void test_panel_is_inside_answers_for_the_interface() {
  auto click_at = [](Vec2Si32 at) {
    InputMessage message;
    message.kind = InputMessage::kMouse;
    message.mouse.backbuffer_pos = at;
    message.keyboard.key = kKeyMouseLeft;
    message.keyboard.key_state = 1;
    return message;
  };
  std::deque<GuiMessage> gui_messages;

  auto root = std::make_shared<Panel>(1, Vec2Si32(0, 0), Vec2Si32(320, 200));
  // A bare panel is not clickable, so it takes nothing and a click over it
  // belongs to the world. This is the case that surprises people, and it is the
  // behaviour of the input walk, not an opinion of this function.
  TEST_CHECK_(!root->IsInside(Vec2Si32(10, 10)),
      "an empty non-clickable panel claimed a click");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(10, 10)), &gui_messages),
      "the input walk applied a click on a non-clickable panel, so IsInside "
      "and the walk disagree");

  Sprite face;
  face.Create(40, 20);
  auto button = std::make_shared<Button>(2, Vec2Si32(100, 50), face);
  root->AddChild(button);
  TEST_CHECK_(button->GetSize() == Vec2Si32(40, 20),
      "the button is %d x %d, and the coordinates below assume 40 x 20",
      button->GetSize().x, button->GetSize().y);

  TEST_CHECK_(root->IsInside(Vec2Si32(100, 50)),
      "the lower left pixel of a button is outside it");
  TEST_CHECK_(root->IsInside(Vec2Si32(139, 69)),
      "the upper right pixel of a button is outside it");
  TEST_CHECK_(!root->IsInside(Vec2Si32(140, 70)),
      "the pixel past the upper right corner of a button belongs to it");
  TEST_CHECK_(!root->IsInside(Vec2Si32(99, 50)),
      "the pixel left of a button belongs to it");
  TEST_CHECK_(root->ApplyInput(click_at(Vec2Si32(110, 60)), &gui_messages),
      "a click on a button was not taken by the interface");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(200, 150)), &gui_messages),
      "a click far from every panel was taken by the interface");

  // A clickable panel deep in the tree is found, and its position is the sum of
  // the positions on the way down.
  auto dialog = std::make_shared<Panel>(3, Vec2Si32(20, 20),
      Vec2Si32(100, 100));
  auto slot = std::make_shared<Panel>(4, Vec2Si32(5, 5), Vec2Si32(10, 10), 0,
      Sprite(), true);
  dialog->AddChild(slot);
  root->AddChild(dialog);
  TEST_CHECK_(root->IsInside(Vec2Si32(25, 25)),
      "a clickable panel at 20 plus 5 was not found at 25");
  TEST_CHECK_(root->IsInside(Vec2Si32(34, 34)),
      "the far corner of the nested panel was not found");
  TEST_CHECK_(!root->IsInside(Vec2Si32(35, 35)),
      "the point past the nested panel belongs to it");
  TEST_CHECK_(root->ApplyInput(click_at(Vec2Si32(30, 30)), &gui_messages),
      "a click on the nested panel was not taken");

  // Hiding a panel hides what is inside it, and neither takes clicks any more.
  dialog->SetVisible(false);
  TEST_CHECK_(!root->IsInside(Vec2Si32(28, 28)),
      "a panel inside a hidden one still claims clicks");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(28, 28)), &gui_messages),
      "the input walk went into a hidden panel");
  dialog->SetVisible(true);
  TEST_CHECK_(root->IsInside(Vec2Si32(28, 28)),
      "the nested panel did not come back with the dialog");

  button->SetVisible(false);
  TEST_CHECK_(!root->IsInside(Vec2Si32(110, 60)),
      "a hidden button still claims clicks");
  TEST_CHECK_(!root->ApplyInput(click_at(Vec2Si32(110, 60)), &gui_messages),
      "the input walk clicked a hidden button");
  button->SetVisible(true);

  // Negative control: the obvious hand-written test, "is the point inside the
  // root rectangle", answers yes for points that belong to the world, so the
  // checks above are not something any implementation would pass.
  const Vec2Si32 world_point(200, 150);
  const bool naive_says_interface = world_point.x >= root->GetPos().x
    && world_point.y >= root->GetPos().y
    && world_point.x < root->GetPos().x + root->GetSize().x
    && world_point.y < root->GetPos().y + root->GetSize().y;
  TEST_CHECK_(naive_says_interface && !root->IsInside(world_point),
      "a point inside the root rectangle but on no panel: the rectangle test "
      "says %d and IsInside says %d, and they were supposed to differ",
      naive_says_interface ? 1 : 0, root->IsInside(world_point) ? 1 : 0);
}

// A window over the world: a panel made clickable takes the clicks on its
// empty parts and on the widgets that take nothing themselves (a Text, a
// Progressbar), so none of them reaches the game under the window; a click
// beside the window still does. IsInside and the input walk agree throughout.
void test_panel_set_clickable_makes_a_window() {
  std::deque<GuiMessage> gui_messages;
  auto window = std::make_shared<Panel>(1, Vec2Si32(100, 50),
      Vec2Si32(100, 100));
  Font font;
  font.CreateEmpty(4, 5);
  auto label = std::make_shared<Text>(2, Vec2Si32(10, 80), Vec2Si32(80, 10), 0,
      font, kTextOriginBottom, Rgba(255, 255, 255), "label");
  window->AddChild(label);
  Sprite bar;
  bar.Create(80, 10);
  auto progress = std::make_shared<Progressbar>(3, Vec2Si32(10, 10), bar, bar,
      std::vector<Rgba>(1, Rgba(255, 255, 255)), font, 100.0f, 50.0f);
  window->AddChild(progress);
  TEST_CHECK_(!window->IsClickable(), "a panel is clickable from the start");

  const Vec2Si32 on_label(120, 132);
  const Vec2Si32 on_progress(120, 62);
  const Vec2Si32 on_empty(150, 100);
  const Vec2Si32 beside(99, 100);
  const Vec2Si32 past(200, 100);

  // Not clickable: everything falls through, as documented.
  TEST_CHECK_(!window->IsInside(on_label) && !window->IsInside(on_progress)
      && !window->IsInside(on_empty),
      "a bare panel took a click on a label, a progress bar or its background");
  TEST_CHECK_(!window->ApplyInput(LeftClickAt(on_empty), &gui_messages),
      "the walk applied a click on a bare panel");

  window->SetClickable(true);
  TEST_CHECK_(window->IsClickable(), "SetClickable(true) did not stick");
  const Vec2Si32 inside[] = {on_label, on_progress, on_empty,
      Vec2Si32(100, 50), Vec2Si32(199, 149)};
  for (const Vec2Si32 &at : inside) {
    TEST_CHECK_(window->IsInside(at),
        "IsInside says (%d, %d) on a clickable window belongs to the world",
        at.x, at.y);
    gui_messages.clear();
    TEST_CHECK_(window->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk let a click at (%d, %d) fall through a clickable window",
        at.x, at.y);
    TEST_CHECK_(gui_messages.size() == 1
        && gui_messages.front().kind == kGuiPanelLeftDown
        && gui_messages.front().panel == window,
        "a click at (%d, %d) on the window did not emit kGuiPanelLeftDown "
        "from the window", at.x, at.y);
  }
  const Vec2Si32 outside[] = {beside, past, Vec2Si32(150, 49),
      Vec2Si32(150, 150)};
  for (const Vec2Si32 &at : outside) {
    TEST_CHECK_(!window->IsInside(at),
        "IsInside claims (%d, %d) beside the window", at.x, at.y);
    TEST_CHECK_(!window->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk took a click beside the window at (%d, %d)", at.x, at.y);
  }

  // The flag can be taken back, and a hidden window takes nothing either way.
  window->SetClickable(false);
  TEST_CHECK_(!window->IsInside(on_empty),
      "SetClickable(false) left the panel clickable");
  window->SetClickable(true);
  window->SetVisible(false);
  TEST_CHECK_(!window->IsInside(on_empty)
      && !window->ApplyInput(LeftClickAt(on_empty), &gui_messages),
      "a hidden clickable window still takes clicks");
}

// A hidden panel is not there: it is not drawn and it takes no input. Button,
// Checkbox and Scrollbar keep that promise through their state, Text checks the
// flag, and Editbox and Progressbar used to draw and (the editbox) take clicks
// and keystrokes while hidden, because only the base class part of them looked
// at the flag.
void test_hidden_editbox_and_progressbar_are_not_there() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  const Rgba kGreen(0, 255, 0, 255);
  Font font;
  font.CreateEmpty(4, 5);
  Sprite red;
  red.Create(60, 12);
  red.Clear(kRed);
  Sprite green;
  green.Create(60, 12);
  green.Clear(kGreen);

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, red, red,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto bar = std::make_shared<Progressbar>(2, Vec2Si32(100, 10), red, green,
      std::vector<Rgba>(1, Rgba(255, 255, 255)), font, 1.0f, 0.0f);
  root->AddChild(bar);
  const Vec2Si32 in_box(20, 15);
  const Vec2Si32 in_bar(110, 15);

  // Positive control: visible, both are drawn where they are.
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(in_box).rgba == kRed.rgba,
      "a visible editbox did not draw its face, so hiding it proves nothing");
  TEST_CHECK_(BackbufferPixel(in_bar).rgba == kRed.rgba,
      "a visible progressbar at zero did not draw its incomplete face");

  box->SetVisible(false);
  bar->SetVisible(false);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(in_box).rgba == kBlack.rgba,
      "a hidden editbox is still drawn");
  TEST_CHECK_(BackbufferPixel(in_bar).rgba == kBlack.rgba,
      "a hidden progressbar is still drawn");

  // A click on a hidden editbox falls through to the world and focuses nothing.
  std::deque<GuiMessage> gui_messages;
  TEST_CHECK_(!root->ApplyInput(LeftClickAt(in_box), &gui_messages),
      "a click on a hidden editbox was taken by the interface");
  TEST_CHECK_(!box->IsFocused(), "a click focused a hidden editbox");
  TEST_CHECK_(!root->IsInside(in_box), "IsInside counts a hidden editbox");

  // The same click on the visible box is taken and focuses it.
  box->SetVisible(true);
  TEST_CHECK_(root->ApplyInput(LeftClickAt(in_box), &gui_messages),
      "a click on a visible editbox was not taken");
  TEST_CHECK_(box->IsFocused(), "a click did not focus a visible editbox");

  // A box hidden while it holds the focus does not type either.
  box->SetVisible(false);
  root->ApplyInput(TypedLetter('a', kKeyA), &gui_messages);
  TEST_CHECK_(box->GetText().empty(),
      "a hidden editbox typed '%s'", box->GetText().c_str());
  box->SetVisible(true);
  root->ApplyInput(TypedLetter('a', kKeyA), &gui_messages);
  TEST_CHECK_(box->GetText() == std::string("a"),
      "the visible editbox did not type, so the hidden check proves nothing");

  // Back in view, both draw again.
  bar->SetVisible(true);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK(BackbufferPixel(in_box).rgba == kRed.rgba);
  TEST_CHECK(BackbufferPixel(in_bar).rgba == kRed.rgba);
}

// IsInside promises to agree with the input walk about every panel. Editbox and
// Scrollbar take clicks in ApplyInput but are not marked clickable, so the base
// answer called them transparent and a click on a field went to the world too.
void test_is_inside_counts_editbox_and_scrollbar() {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite face;
  face.Create(60, 12);
  Sprite track;
  track.Create(12, 100);
  Sprite knob;
  knob.Create(12, 12);
  std::deque<GuiMessage> gui_messages;

  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, face, face,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  root->AddChild(box);
  auto bar = std::make_shared<Scrollbar>(2, Vec2Si32(200, 20), 2,
      track, track, knob, knob, knob, knob, knob, knob, knob, knob, knob,
      0, 10, 0, Scrollbar::kScrollVertical);
  root->AddChild(bar);
  TEST_CHECK_(box->GetSize() == Vec2Si32(60, 12) &&
      bar->GetSize() == Vec2Si32(12, 100),
      "the sizes are %d x %d and %d x %d, the coordinates below assume "
      "60 x 12 and 12 x 100", box->GetSize().x, box->GetSize().y,
      bar->GetSize().x, bar->GetSize().y);

  // Every corner pixel and one past it, the same way the walk sees them.
  const Vec2Si32 box_points[] = {
    Vec2Si32(10, 10), Vec2Si32(69, 21), Vec2Si32(40, 15)
  };
  for (const Vec2Si32 &at : box_points) {
    TEST_CHECK_(root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk did not take a click on the editbox at (%d, %d)", at.x, at.y);
    TEST_CHECK_(root->IsInside(at),
        "IsInside says (%d, %d) on the editbox belongs to the world",
        at.x, at.y);
  }
  const Vec2Si32 bar_points[] = {
    Vec2Si32(200, 20), Vec2Si32(211, 119), Vec2Si32(205, 70)
  };
  for (const Vec2Si32 &at : bar_points) {
    TEST_CHECK_(root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk did not take a click on the scrollbar at (%d, %d)",
        at.x, at.y);
    TEST_CHECK_(root->IsInside(at),
        "IsInside says (%d, %d) on the scrollbar belongs to the world",
        at.x, at.y);
  }
  const Vec2Si32 outside[] = {
    Vec2Si32(9, 10), Vec2Si32(70, 21), Vec2Si32(40, 22),
    Vec2Si32(199, 20), Vec2Si32(212, 119), Vec2Si32(205, 120)
  };
  for (const Vec2Si32 &at : outside) {
    TEST_CHECK_(!root->ApplyInput(LeftClickAt(at), &gui_messages),
        "the walk took a click beside a panel at (%d, %d)", at.x, at.y);
    TEST_CHECK_(!root->IsInside(at),
        "IsInside claims (%d, %d) beside every panel", at.x, at.y);
  }

  // Hidden, both stop counting, and IsInside follows.
  box->SetVisible(false);
  bar->SetVisible(false);
  TEST_CHECK(!root->ApplyInput(LeftClickAt(Vec2Si32(40, 15)), &gui_messages));
  TEST_CHECK_(!root->IsInside(Vec2Si32(40, 15)),
      "IsInside counts a hidden editbox");
  TEST_CHECK(!root->ApplyInput(LeftClickAt(Vec2Si32(205, 70)), &gui_messages));
  TEST_CHECK_(!root->IsInside(Vec2Si32(205, 70)),
      "IsInside counts a hidden scrollbar");
}

// An anchor means "keep these distances to the edges of the parent". The
// distances used to be measured only inside SetAnchor and only when the panel
// already had a parent, so SetAnchor before AddChild measured nothing, and a
// SetPos after SetAnchor was undone by the next resize of the parent. A dock
// used to take effect only at the next resize of the parent as well.
void test_panel_anchor_and_dock_follow_the_calls() {
  // SetAnchor before AddChild: the distances are measured when the panel gets
  // its parent, from where it is at that moment.
  auto parent = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(400, 300));
  auto early = std::make_shared<Panel>(1, Vec2Si32(20, 30), Vec2Si32(100, 50));
  early->SetAnchor(kAnchorTop | kAnchorRight);
  parent->AddChild(early);
  parent->SetSize(Vec2Si32(500, 400));
  TEST_CHECK_(early->GetPos() == Vec2Si32(120, 130),
      "a panel anchored before AddChild went to (%d, %d) instead of "
      "(120, 130) when the parent grew by 100", early->GetPos().x,
      early->GetPos().y);
  TEST_CHECK_(early->GetSize() == Vec2Si32(100, 50),
      "a top-right anchor changed the size to %d x %d",
      early->GetSize().x, early->GetSize().y);

  // SetPos after SetAnchor: the new place is the one to keep.
  auto moved = std::make_shared<Panel>(2, Vec2Si32(20, 30), Vec2Si32(100, 50));
  parent->AddChild(moved);
  moved->SetAnchor(kAnchorTop | kAnchorRight);
  moved->SetPos(50, 60);
  parent->SetSize(Vec2Si32(600, 500));
  TEST_CHECK_(moved->GetPos() == Vec2Si32(150, 160),
      "a panel moved after SetAnchor went to (%d, %d) instead of (150, 160)",
      moved->GetPos().x, moved->GetPos().y);
  // And the one anchored earlier keeps following.
  TEST_CHECK_(early->GetPos() == Vec2Si32(220, 230),
      "the early panel is at (%d, %d) after the second resize",
      early->GetPos().x, early->GetPos().y);

  // Stretching anchors measured at AddChild time keep both distances.
  auto stretched = std::make_shared<Panel>(3, Vec2Si32(10, 20),
      Vec2Si32(580, 460));
  stretched->SetAnchor(kAnchorLeft | kAnchorRight | kAnchorBottom | kAnchorTop);
  parent->AddChild(stretched);
  parent->SetSize(Vec2Si32(300, 200));
  TEST_CHECK_(stretched->GetPos() == Vec2Si32(10, 20) &&
      stretched->GetSize() == Vec2Si32(280, 160),
      "a fully anchored panel is at (%d, %d) sized %d x %d, expected "
      "(10, 20) and 280 x 160", stretched->GetPos().x, stretched->GetPos().y,
      stretched->GetSize().x, stretched->GetSize().y);

  // A dock takes effect at once, whether set after or before AddChild.
  auto dock_parent = std::make_shared<Panel>(10, Vec2Si32(0, 0),
      Vec2Si32(400, 300));
  auto late_dock = std::make_shared<Panel>(11, Vec2Si32(5, 5),
      Vec2Si32(100, 50));
  dock_parent->AddChild(late_dock);
  late_dock->SetDock(kDockTop | kDockRight);
  TEST_CHECK_(late_dock->GetPos() == Vec2Si32(300, 250),
      "SetDock left the panel at (%d, %d), expected (300, 250) at once",
      late_dock->GetPos().x, late_dock->GetPos().y);
  auto early_dock = std::make_shared<Panel>(12, Vec2Si32(5, 5),
      Vec2Si32(100, 50));
  early_dock->SetDock(kDockLeft | kDockRight | kDockBottom);
  dock_parent->AddChild(early_dock);
  TEST_CHECK_(early_dock->GetPos() == Vec2Si32(0, 0) &&
      early_dock->GetSize() == Vec2Si32(400, 50),
      "a panel docked before AddChild is at (%d, %d) sized %d x %d, expected "
      "(0, 0) and 400 x 50", early_dock->GetPos().x, early_dock->GetPos().y,
      early_dock->GetSize().x, early_dock->GetSize().y);
  dock_parent->SetSize(Vec2Si32(200, 100));
  TEST_CHECK(late_dock->GetPos() == Vec2Si32(100, 50));
  TEST_CHECK(early_dock->GetSize() == Vec2Si32(200, 50));

  // Negative control: a panel with no anchor and no dock stays where it was.
  auto plain = std::make_shared<Panel>(13, Vec2Si32(7, 8), Vec2Si32(10, 10));
  dock_parent->AddChild(plain);
  dock_parent->SetSize(Vec2Si32(300, 300));
  TEST_CHECK_(plain->GetPos() == Vec2Si32(7, 8),
      "a plain panel moved to (%d, %d) on a parent resize",
      plain->GetPos().x, plain->GetPos().y);
}

// Tab walks the visible interface. A hidden panel, and everything inside a
// hidden panel, can not be seen or typed into, so the focus must skip it.
void test_tab_skips_hidden_panels() {
  Sprite face;
  face.Create(40, 20);
  Font font;
  font.CreateEmpty(4, 5);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto make_button = [&](Ui64 tag, Ui32 tab_order) {
    return std::make_shared<Button>(tag, Vec2Si32(10, Si32(tag) * 30), face,
        Sprite(), Sprite(), Sound(), Sound(), kKeyNone, tab_order);
  };
  auto first = make_button(1, 1);
  // An editbox rather than a button: a hidden button already drops out of the
  // walk through IsEnabled(), an editbox does not.
  auto hidden = std::make_shared<Editbox>(2, Vec2Si32(10, 60), 2, face, face,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  auto dialog = std::make_shared<Panel>(3, Vec2Si32(100, 0),
      Vec2Si32(100, 100));
  auto inside_hidden_dialog = make_button(4, 3);
  dialog->AddChild(inside_hidden_dialog);
  auto last = make_button(5, 4);
  root->AddChild(first);
  root->AddChild(hidden);
  root->AddChild(dialog);
  root->AddChild(last);

  // Positive control: with everything visible, Tab visits all four in order.
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "the first Tab did not focus tab order 1");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(hidden->IsFocused(), "the second Tab skipped a visible button");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(inside_hidden_dialog->IsFocused(),
      "the third Tab skipped a button inside a visible dialog");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK(last->IsFocused());
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "Tab did not wrap around to the first");

  hidden->SetVisible(false);
  dialog->SetVisible(false);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(!hidden->IsFocused(), "Tab focused a hidden button");
  TEST_CHECK_(!inside_hidden_dialog->IsFocused(),
      "Tab focused a button inside a hidden dialog");
  TEST_CHECK_(last->IsFocused(),
      "Tab from the first visible button did not land on the last one");
  TEST_CHECK(root->SwitchCurrentTab(false));
  TEST_CHECK_(first->IsFocused() && !hidden->IsFocused() &&
      !inside_hidden_dialog->IsFocused(),
      "Shift+Tab went through the hidden panels on the way back");

  // Shown again, they are back in the walk.
  hidden->SetVisible(true);
  dialog->SetVisible(true);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(hidden->IsFocused(), "a button shown again is still skipped");
}

namespace {

// A panel kind of its own, the way a game would write one: it only overrides
// HandleInput and counts the calls.
class CountingPanel : public Panel {
 public:
  Si32 calls = 0;
  bool is_enabled = true;

  CountingPanel(Vec2Si32 pos, Vec2Si32 size)
    : Panel(0, pos, size) {
  }

  bool IsEnabled() override {
    return is_enabled;
  }

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level, bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override {
    ++calls;
    Panel::HandleInput(parent_pos, message, is_top_level, in_out_is_applied,
        out_gui_messages, out_current_tab);
  }
};

}  // namespace

// Whether a panel is there to take input is decided once, in ApplyInput, and
// not by every panel kind for itself. A kind that overrides HandleInput never
// hears about a message while it is hidden or disabled, and neither do the
// kinds the engine ships with.
void test_hidden_and_disabled_panels_never_handle_input() {
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto counting = std::make_shared<CountingPanel>(Vec2Si32(10, 10),
      Vec2Si32(50, 50));
  root->AddChild(counting);
  std::deque<GuiMessage> gui_messages;
  const InputMessage click = LeftClickAt(Vec2Si32(20, 20));

  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1,
      "a visible enabled panel handled the message %d times, expected 1",
      (int)counting->calls);

  counting->SetVisible(false);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1, "a hidden panel handled a message");
  counting->SetVisible(true);

  counting->is_enabled = false;
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1, "a disabled panel handled a message");
  counting->is_enabled = true;

  // A hidden parent stops the walk before the child is asked.
  root->SetVisible(false);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 1,
      "a panel inside a hidden one handled a message");
  root->SetVisible(true);
  root->ApplyInput(click, &gui_messages);
  TEST_CHECK_(counting->calls == 2,
      "the panel did not come back with its parent: %d calls",
      (int)counting->calls);

  // The engine kinds: a disabled checkbox does not toggle, a disabled
  // scrollbar does not move, and neither says anything.
  Sprite face;
  face.Create(20, 20);
  Sprite track;
  track.Create(12, 100);
  Sprite knob;
  knob.Create(12, 12);
  auto checkbox = std::make_shared<Checkbox>(3, Vec2Si32(100, 10), 2,
      face, face);
  root->AddChild(checkbox);
  auto scrollbar = std::make_shared<Scrollbar>(4, Vec2Si32(200, 20), 3,
      track, track, knob, knob, knob, knob, knob, knob, knob, knob, knob,
      0, 10, 5, Scrollbar::kScrollVertical);
  root->AddChild(scrollbar);
  Si32 checkbox_downs = 0;
  Si32 scroll_changes = 0;
  checkbox->OnButtonDown = [&checkbox_downs]() { ++checkbox_downs; };
  scrollbar->OnScrollChange = [&scroll_changes]() { ++scroll_changes; };

  // Positive control: enabled, a press is heard and the arrow moves the value.
  gui_messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(110, 20)), &gui_messages);
  TEST_CHECK_(checkbox_downs == 1, "a press on an enabled checkbox was lost");
  TEST_CHECK_(!gui_messages.empty() && gui_messages.back().kind == kGuiButtonDown,
      "no kGuiButtonDown was queued for an enabled checkbox");
  // The press lands on the dec arrow of a vertical scrollbar, which is at the
  // bottom of the track.
  root->ApplyInput(LeftClickAt(Vec2Si32(205, 25)), &gui_messages);
  TEST_CHECK_(scroll_changes == 1 && scrollbar->GetValue() == 4,
      "a press on the arrow of an enabled scrollbar changed the value %d "
      "times, value is %d", (int)scroll_changes, (int)scrollbar->GetValue());

  checkbox->SetEnabled(false);
  scrollbar->SetEnabled(false);
  gui_messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(110, 20)), &gui_messages);
  root->ApplyInput(LeftClickAt(Vec2Si32(205, 25)), &gui_messages);
  TEST_CHECK_(checkbox_downs == 1, "a disabled checkbox heard a press");
  TEST_CHECK_(scroll_changes == 1 && scrollbar->GetValue() == 4,
      "a disabled scrollbar moved to %d", (int)scrollbar->GetValue());
  TEST_CHECK_(gui_messages.empty(),
      "%d messages were queued by disabled panels", (int)gui_messages.size());

  // Tab does not stop on a disabled panel either. The press above gave the
  // scrollbar the focus, so the walk is started from nothing.
  auto first = std::make_shared<Button>(5, Vec2Si32(10, 100), face, Sprite(),
      Sprite(), Sound(), Sound(), kKeyNone, 1);
  auto last = std::make_shared<Button>(6, Vec2Si32(50, 100), face, Sprite(),
      Sprite(), Sound(), Sound(), kKeyNone, 4);
  root->AddChild(first);
  root->AddChild(last);
  root->MakeCurrentTab(nullptr);
  TEST_CHECK_(root->FindCurrentTab() == nullptr,
      "the focus was not cleared, so the Tab walk below starts from the wrong "
      "place");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(first->IsFocused(), "the first Tab did not land on tab order 1");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(last->IsFocused() && !checkbox->IsFocused() &&
      !scrollbar->IsFocused(),
      "Tab stopped on a disabled checkbox or scrollbar");

  // Enabled again, they are back in the walk: this is the negative control for
  // the check above.
  checkbox->SetEnabled(true);
  scrollbar->SetEnabled(true);
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK(first->IsFocused());
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(checkbox->IsFocused(), "an enabled checkbox is skipped by Tab");
  TEST_CHECK(root->SwitchCurrentTab(true));
  TEST_CHECK_(scrollbar->IsFocused(), "an enabled scrollbar is skipped by Tab");
}

namespace {

Sprite SolidSprite(Si32 width, Si32 height, Rgba color) {
  Sprite sprite;
  sprite.Create(width, height);
  sprite.Clear(color);
  return sprite;
}

// A frame that paints any rectangle in one color, with a 1 pixel border.
DecoratedFrame SolidFrame(Rgba color) {
  DecoratedFrame frame;
  frame.Split(SolidSprite(3, 3, color), 1, true, true);
  return frame;
}

// Every glyph is a solid 4x5 block advancing 4 pixels; the space is blank.
// Only the listed characters exist, so a text of other characters draws
// nothing at all.
Font BlockFont(const char *characters) {
  Font font;
  font.CreateEmpty(4, 5);
  Sprite block = SolidSprite(4, 5, Rgba(255, 255, 255, 255));
  Sprite blank;
  blank.Create(4, 5);
  blank.Clear();
  for (const char *c = characters; *c; ++c) {
    font.AddGlyph(static_cast<Ui32>(*c), 4, *c == ' ' ? blank : block);
  }
  return font;
}

Font BlockFont() {
  return BlockFont("abcdefghijklmnopqrstuvwxyz0123456789 -*");
}

InputMessage LeftReleaseAt(Vec2Si32 at) {
  InputMessage message;
  message.kind = InputMessage::kMouse;
  message.mouse.backbuffer_pos = at;
  message.keyboard.key = kKeyMouseLeft;
  message.keyboard.key_state = 2;
  return message;
}

InputMessage MouseMoveTo(Vec2Si32 at, bool is_left_held = false) {
  InputMessage message;
  message.kind = InputMessage::kMouse;
  message.mouse.backbuffer_pos = at;
  message.keyboard.key = kKeyNone;
  message.keyboard.key_state = 0;
  message.keyboard.state[kKeyMouseLeft] = is_left_held ? 1 : 0;
  return message;
}

InputMessage WheelAt(Vec2Si32 at, Si32 wheel_delta) {
  InputMessage message = MouseMoveTo(at);
  message.mouse.wheel_delta = wheel_delta;
  return message;
}

InputMessage KeyPress(KeyCode key) {
  InputMessage message;
  message.kind = InputMessage::kKeyboard;
  message.keyboard.key = key;
  message.keyboard.key_state = 1;
  return message;
}

InputMessage KeyRelease(KeyCode key) {
  InputMessage message = KeyPress(key);
  message.keyboard.key_state = 2;
  return message;
}

void Click(Panel *root, Vec2Si32 at, std::deque<GuiMessage> *messages) {
  root->ApplyInput(LeftClickAt(at), messages);
  root->ApplyInput(LeftReleaseAt(at), messages);
}

Si32 CountMessages(const std::deque<GuiMessage> &messages, GuiMessageKind kind,
                   const Panel *panel = nullptr) {
  Si32 count = 0;
  for (auto it = messages.begin(); it != messages.end(); ++it) {
    if (it->kind == kind && (panel == nullptr || it->panel.get() == panel)) {
      ++count;
    }
  }
  return count;
}

// Number of backbuffer pixels of the given color inside a rectangle.
Si32 CountPixels(Vec2Si32 low, Vec2Si32 size, Rgba color) {
  Si32 count = 0;
  for (Si32 y = low.y; y < low.y + size.y; ++y) {
    for (Si32 x = low.x; x < low.x + size.x; ++x) {
      if (BackbufferPixel(Vec2Si32(x, y)).rgba == color.rgba) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

// A panel that clips draws its children inside its own rectangle only, and the
// clip rectangles of nested panels intersect. Input is not the subject here.
void test_panel_clips_children() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto box = std::make_shared<Panel>(1, Vec2Si32(50, 50), Vec2Si32(40, 40));
  auto spill = std::make_shared<Panel>(2, Vec2Si32(-20, -20), Vec2Si32(80, 80),
      0, SolidSprite(4, 4, kRed));
  box->AddChild(spill);
  root->AddChild(box);

  // Negative control: without clipping the child spills over the panel.
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(45, 45)).rgba == kRed.rgba,
      "an unclipped child did not draw outside its parent, so the clip check "
      "below proves nothing");
  TEST_CHECK(!box->IsClippingChildren());

  box->SetClipChildren(true);
  TEST_CHECK(box->IsClippingChildren());
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(55, 55)).rgba == kRed.rgba,
      "the part of the child inside the clipping panel was not drawn");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(89, 89)).rgba == kRed.rgba,
      "the last pixel inside the clipping panel was not drawn");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(45, 45)).rgba == kBlack.rgba,
      "a child was drawn below and left of its clipping parent");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(90, 90)).rgba == kBlack.rgba,
      "a child was drawn right past the far corner of its clipping parent");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(70, 95)).rgba == kBlack.rgba,
      "a child was drawn above its clipping parent");
  TEST_CHECK_(CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 40 * 40,
      "the clipped child covers %d pixels, expected exactly the 40x40 panel",
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));

  // Nested clipping panels: the visible region is the intersection, and the
  // grandchild lands at its true absolute position inside it.
  auto outer = std::make_shared<Panel>(3, Vec2Si32(100, 100),
      Vec2Si32(50, 50));
  outer->SetClipChildren(true);
  auto inner = std::make_shared<Panel>(4, Vec2Si32(30, 30), Vec2Si32(40, 40));
  inner->SetClipChildren(true);
  auto deep = std::make_shared<Panel>(5, Vec2Si32(-10, -10), Vec2Si32(60, 60),
      0, SolidSprite(4, 4, kRed));
  inner->AddChild(deep);
  outer->AddChild(inner);
  root->AddChild(outer);
  box->SetVisible(false);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  // outer covers [100, 150), inner covers [130, 170), deep covers [120, 180).
  TEST_CHECK_(BackbufferPixel(Vec2Si32(135, 135)).rgba == kRed.rgba,
      "a grandchild inside both clipping panels was not drawn");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(125, 125)).rgba == kBlack.rgba,
      "a grandchild was drawn inside the outer panel but outside the inner");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(155, 155)).rgba == kBlack.rgba,
      "a grandchild was drawn inside the inner panel but outside the outer");
  TEST_CHECK_(CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 20 * 20,
      "nested clipping shows %d pixels, expected the 20x20 intersection",
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));

  // A clipping panel partly off screen: the child is still placed correctly.
  outer->SetVisible(false);
  auto edge = std::make_shared<Panel>(6, Vec2Si32(-10, -10), Vec2Si32(40, 40));
  edge->SetClipChildren(true);
  auto wide = std::make_shared<Panel>(7, Vec2Si32(0, 0), Vec2Si32(100, 100),
      0, SolidSprite(4, 4, kRed));
  edge->AddChild(wide);
  root->AddChild(edge);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(5, 5)).rgba == kRed.rgba,
      "a child of a panel that starts off screen was not drawn where it is");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(29, 29)).rgba == kRed.rgba,
      "the last visible pixel of an off screen clipping panel is missing");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(30, 30)).rgba == kBlack.rgba &&
      BackbufferPixel(Vec2Si32(35, 5)).rgba == kBlack.rgba,
      "a child of a panel that starts off screen was drawn past the panel");

  // The backbuffer is whole again after every Draw.
  Sprite backbuffer = GetEngine()->GetBackbuffer();
  TEST_CHECK_(backbuffer.Size() == Vec2Si32(320, 200),
      "Draw left the backbuffer as a %dx%d view",
      (int)backbuffer.Size().x, (int)backbuffer.Size().y);
}

// WrapText breaks after spaces and hyphens, always at '\n', and reports byte
// ranges that tile the source; Text with word wrap on draws and measures the
// wrapped lines.
void test_text_word_wrap() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  Font font = BlockFont();

  // Each glyph is 4 pixels wide, so "aaaa bbbb cccc" is 56 pixels.
  std::vector<WrappedTextLine> lines = WrapText(font, "aaaa bbbb cccc", 30);
  TEST_CHECK_(lines.size() == 3, "%d lines, expected 3", (int)lines.size());
  if (lines.size() == 3) {
    TEST_CHECK_(lines[0].text == "aaaa " && lines[0].start == 0 &&
        lines[0].end == 5, "line 0 is '%s' [%d, %d)", lines[0].text.c_str(),
        (int)lines[0].start, (int)lines[0].end);
    TEST_CHECK_(lines[1].text == "bbbb " && lines[1].start == 5 &&
        lines[1].end == 10, "line 1 is '%s' [%d, %d)", lines[1].text.c_str(),
        (int)lines[1].start, (int)lines[1].end);
    TEST_CHECK_(lines[2].text == "cccc" && lines[2].start == 10 &&
        lines[2].end == 14, "line 2 is '%s' [%d, %d)", lines[2].text.c_str(),
        (int)lines[2].start, (int)lines[2].end);
  }
  // Two words fit on one line when the width allows it.
  lines = WrapText(font, "aaaa bbbb cccc", 40);
  TEST_CHECK_(lines.size() == 2 && lines[0].text == "aaaa bbbb ",
      "at 40 pixels: %d lines, first is '%s'", (int)lines.size(),
      lines.empty() ? "" : lines[0].text.c_str());
  // A hyphen is a break point that stays on the first line.
  lines = WrapText(font, "aaaa-bbbb", 20);
  TEST_CHECK_(lines.size() == 2 && lines[0].text == "aaaa-" &&
      lines[1].text == "bbbb", "hyphen: %d lines, '%s' / '%s'",
      (int)lines.size(), lines.empty() ? "" : lines[0].text.c_str(),
      lines.size() < 2 ? "" : lines[1].text.c_str());
  // A newline breaks even when there is room, and its byte belongs to the
  // range of the line it ends.
  lines = WrapText(font, "aa\nbb", 100);
  TEST_CHECK_(lines.size() == 2 && lines[0].text == "aa" && lines[0].end == 3
      && lines[1].start == 3 && lines[1].end == 5,
      "newline: %d lines", (int)lines.size());
  // A word longer than the width overflows on a line of its own.
  lines = WrapText(font, "aaaaaaaaaa bb", 20);
  TEST_CHECK_(lines.size() == 2 && lines[0].text == "aaaaaaaaaa ",
      "long word: %d lines, first '%s'", (int)lines.size(),
      lines.empty() ? "" : lines[0].text.c_str());
  // Zero width means no soft breaks at all.
  lines = WrapText(font, "aaaa bbbb cccc", 0);
  TEST_CHECK_(lines.size() == 1, "width 0 wrapped into %d lines",
      (int)lines.size());
  lines = WrapText(font, "", 30);
  TEST_CHECK_(lines.size() == 1 && lines[0].text.empty(),
      "an empty text should be one empty line");

  // The Text panel: measured and drawn size follow the wrap flag.
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto text = std::make_shared<Text>(1, Vec2Si32(10, 100), Vec2Si32(30, 50), 0,
      font, kTextOriginTop, kRed, "aaaa bbbb cccc");
  root->AddChild(text);
  Vec2Si32 plain_size = text->EvaluateSize();
  TEST_CHECK_(plain_size.x == 56 && plain_size.y == 5,
      "unwrapped size is %dx%d, expected 56x5",
      (int)plain_size.x, (int)plain_size.y);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  Si32 plain_right = 0;
  Si32 plain_top = 0;
  Si32 plain_bottom = 200;
  for (Si32 y = 0; y < 200; ++y) {
    for (Si32 x = 0; x < 320; ++x) {
      if (BackbufferPixel(Vec2Si32(x, y)).rgba == kRed.rgba) {
        plain_right = std::max(plain_right, x);
        plain_top = std::max(plain_top, y);
        plain_bottom = std::min(plain_bottom, y);
      }
    }
  }
  TEST_CHECK_(plain_right >= 10 + 48 && plain_top - plain_bottom < 5,
      "the unwrapped text did not draw as one long line (right %d, rows %d), "
      "so the wrapped check proves nothing", (int)plain_right,
      (int)(plain_top - plain_bottom + 1));

  text->SetWordWrap(true);
  TEST_CHECK(text->IsWordWrap());
  Vec2Si32 wrapped_size = text->EvaluateSize();
  TEST_CHECK_(wrapped_size.x <= 30 && wrapped_size.y == 15,
      "wrapped size is %dx%d, expected 3 lines of 5 no wider than 30",
      (int)wrapped_size.x, (int)wrapped_size.y);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  Si32 wrapped_right = 0;
  Si32 wrapped_top = 0;
  Si32 wrapped_bottom = 200;
  for (Si32 y = 0; y < 200; ++y) {
    for (Si32 x = 0; x < 320; ++x) {
      if (BackbufferPixel(Vec2Si32(x, y)).rgba == kRed.rgba) {
        wrapped_right = std::max(wrapped_right, x);
        wrapped_top = std::max(wrapped_top, y);
        wrapped_bottom = std::min(wrapped_bottom, y);
      }
    }
  }
  TEST_CHECK_(wrapped_right < 10 + 30,
      "wrapped text still reaches x = %d past the 30 pixel panel",
      (int)wrapped_right);
  TEST_CHECK_(wrapped_top - wrapped_bottom + 1 >= 15,
      "wrapped text spans %d rows, expected three lines",
      (int)(wrapped_top - wrapped_bottom + 1));
  TEST_CHECK_(CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 12 * 4 * 5,
      "wrapped text has %d red pixels, expected 12 glyphs of 4x5",
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));
  // A selection over the second word is drawn without going out of bounds, and
  // the wrap goes away when turned off.
  text->Select(5, 9);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  text->Select(0, 0);
  text->SetWordWrap(false);
  TEST_CHECK(text->EvaluateSize() == plain_size);
}

// A read-only edit box keeps the caret and the selection but not a single key
// changes the text; a password char masks what is drawn and what the caret
// counts; a placeholder shows while the text is empty.
void test_editbox_read_only_password_placeholder() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kGrey(128, 128, 128, 255);
  const Rgba kNavy(0, 0, 64, 255);
  Font font = BlockFont();
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  Sprite face = SolidSprite(100, 13, kNavy);
  auto box = std::make_shared<Editbox>(1, Vec2Si32(10, 10), 1, face, face,
      font, kTextOriginBottom, kWhite, std::string());
  root->AddChild(box);
  std::deque<GuiMessage> messages;

  box->SetText("abc");
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 100 - 2, 15)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 100 - 2, 15)), &messages);
  TEST_CHECK_(box->IsFocused() && box->GetCursorPos() == 3,
      "a click at the right end put the caret at %d, expected 3",
      (int)box->GetCursorPos());

  // Positive control: editable, a key changes the text.
  root->ApplyInput(TypedLetter('x', kKeyX), &messages);
  TEST_CHECK_(box->GetText() == "abcx",
      "typing into an editable box gave '%s'", box->GetText().c_str());
  TEST_CHECK(CountMessages(messages, kGuiEditboxTextChange) == 1);

  box->SetReadOnly(true);
  TEST_CHECK(box->IsReadOnly());
  messages.clear();
  bool typed_taken = root->ApplyInput(TypedLetter('y', kKeyY), &messages);
  root->ApplyInput(KeyPress(kKeyBackspace), &messages);
  root->ApplyInput(KeyPress(kKeyDelete), &messages);
  InputMessage undo = KeyPress(kKeyZ);
  undo.keyboard.state[kKeyControl] = 1;
  root->ApplyInput(undo, &messages);
  InputMessage cut = KeyPress(kKeyX);
  cut.keyboard.state[kKeyControl] = 1;
  root->ApplyInput(cut, &messages);
  root->ApplyInput(KeyPress(kKeyEnter), &messages);
  TEST_CHECK_(box->GetText() == "abcx",
      "a read-only box changed its text to '%s'", box->GetText().c_str());
  TEST_CHECK_(!typed_taken,
      "a read-only box took a letter it did not use; the host never sees it");
  TEST_CHECK_(CountMessages(messages, kGuiEditboxTextChange) == 0,
      "a read-only box reported a text change");
  TEST_CHECK_(CountMessages(messages, kGuiEditboxEditDone) == 1,
      "Enter in a read-only box did not end the edit");
  // The caret and the selection still move.
  TEST_CHECK(root->ApplyInput(KeyPress(kKeyLeft), &messages));
  TEST_CHECK_(box->GetCursorPos() == 3,
      "Left in a read-only box left the caret at %d", (int)box->GetCursorPos());
  InputMessage select_all = KeyPress(kKeyA);
  select_all.keyboard.state[kKeyControl] = 1;
  root->ApplyInput(select_all, &messages);
  TEST_CHECK_(box->GetSelectionBegin() == 0 && box->GetSelectionEnd() == 4,
      "Ctrl+A in a read-only box selected [%d, %d)",
      (int)box->GetSelectionBegin(), (int)box->GetSelectionEnd());
  // With the selection in place a letter still changes nothing.
  root->ApplyInput(TypedLetter('q', kKeyQ), &messages);
  TEST_CHECK(box->GetText() == "abcx");
  // SetText is the host's own doing and is allowed.
  box->SetText("abc");
  TEST_CHECK(box->GetText() == "abc");
  box->SetReadOnly(false);
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  root->ApplyInput(TypedLetter('d', kKeyD), &messages);
  TEST_CHECK_(box->GetText() == "abcd",
      "a box made editable again did not type: '%s'", box->GetText().c_str());

  // Password: the shown text is the mask, the real text stays. This box has a
  // font that knows only the star, so the letters themselves draw nothing.
  Font stars_only = BlockFont("*");
  auto pass = std::make_shared<Editbox>(2, Vec2Si32(10, 60), 2, face, face,
      stars_only, kTextOriginBottom, kWhite, std::string());
  root->AddChild(pass);
  pass->SetText("abc");
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 100 - 2, 65)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 100 - 2, 65)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(10, 60), Vec2Si32(100, 13), kWhite) <= 2 * 13,
      "letters without glyphs drew %d white pixels (the caret is allowed), so "
      "the password check proves nothing",
      (int)CountPixels(Vec2Si32(10, 60), Vec2Si32(100, 13), kWhite));
  pass->SetPasswordChar('*');
  TEST_CHECK(pass->GetPasswordChar() == '*');
  TEST_CHECK_(pass->ShownText() == "***", "shown text is '%s'",
      pass->ShownText().c_str());
  TEST_CHECK_(pass->GetText() == "abc", "the password mask changed the text");
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(10, 60), Vec2Si32(100, 13), kWhite) >= 3 * 4 * 5,
      "three masked characters drew %d white pixels, expected three 4x5 stars",
      (int)CountPixels(Vec2Si32(10, 60), Vec2Si32(100, 13), kWhite));
  // A multi-byte mask over multi-byte text: offsets are counted in characters.
  pass->SetText("\xD0\xB0\xD0\xB1\xD0\xB2");  // three Cyrillic letters
  pass->SetPasswordChar(0x2022);  // a bullet, three bytes in UTF-8
  TEST_CHECK_(pass->ShownText() == "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2",
      "a multi-byte mask gave %d bytes", (int)pass->ShownText().length());
  pass->SetPasswordChar('*');
  // The caret from a click lands on character boundaries of the real text:
  // each star is 4 pixels wide, the box border is (13 - 5) / 2 = 4.
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 4 + 8, 65)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 4 + 8, 65)), &messages);
  TEST_CHECK_(pass->GetCursorPos() == 4,
      "a click after two masked letters put the caret at byte %d, expected 4",
      (int)pass->GetCursorPos());
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 4 + 2, 65)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 4 + 2, 65)), &messages);
  TEST_CHECK_(pass->GetCursorPos() == 0 || pass->GetCursorPos() == 2,
      "a click near the start put the caret at byte %d, not on a boundary",
      (int)pass->GetCursorPos());
  pass->SetPasswordChar(0);
  TEST_CHECK(pass->ShownText() == pass->GetText());
  pass->SetVisible(false);

  // Placeholder: drawn in its own color while the text is empty, gone after.
  box->SetText("");
  box->SetPlaceholder("hint");
  TEST_CHECK(box->GetPlaceholder() == "hint");
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(10, 10), Vec2Si32(100, 13), kGrey) == 4 * 4 * 5,
      "the placeholder drew %d grey pixels, expected four 4x5 glyphs",
      (int)CountPixels(Vec2Si32(10, 10), Vec2Si32(100, 13), kGrey));
  box->SetPlaceholderColor(kWhite);
  box->SetText("a");
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(10, 10), Vec2Si32(100, 13), kGrey) == 0,
      "the placeholder is still drawn over a text");
  // Multiline boxes show it too.
  box->SetText("");
  box->SetMultiline(true);
  box->SetSize(Vec2Si32(100, 40));
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(10, 10), Vec2Si32(100, 40), kWhite) >= 4 * 4 * 5,
      "a multiline box drew %d placeholder pixels",
      (int)CountPixels(Vec2Si32(10, 10), Vec2Si32(100, 40), kWhite));
}

// An Image places its sprite into its rectangle in four ways and never draws
// outside the rectangle.
void test_image_scale_modes() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  Sprite picture = SolidSprite(8, 4, kRed);
  auto image = std::make_shared<Image>(1, Vec2Si32(100, 100), Vec2Si32(40, 20),
      picture, Image::kScaleStretch);
  root->AddChild(image);
  Vec2Si32 rect_pos;
  Vec2Si32 rect_size;

  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  image->GetPictureRect(&rect_pos, &rect_size);
  TEST_CHECK_(rect_pos == Vec2Si32(0, 0) && rect_size == Vec2Si32(40, 20),
      "stretch rect is (%d, %d) %dx%d", (int)rect_pos.x, (int)rect_pos.y,
      (int)rect_size.x, (int)rect_size.y);
  TEST_CHECK_(CountPixels(Vec2Si32(100, 100), Vec2Si32(40, 20), kRed) == 800,
      "stretch filled %d of 800 pixels",
      (int)CountPixels(Vec2Si32(100, 100), Vec2Si32(40, 20), kRed));

  image->SetScaleMode(Image::kScaleNone);
  TEST_CHECK(image->GetScaleMode() == Image::kScaleNone);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(100, 100), Vec2Si32(8, 4), kRed) == 32 &&
      CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 32,
      "own size at the corner: %d pixels in place, %d in total",
      (int)CountPixels(Vec2Si32(100, 100), Vec2Si32(8, 4), kRed),
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));

  image->SetScaleMode(Image::kScaleCenter);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  image->GetPictureRect(&rect_pos, &rect_size);
  TEST_CHECK_(rect_pos == Vec2Si32(16, 8) && rect_size == Vec2Si32(8, 4),
      "center rect is (%d, %d) %dx%d", (int)rect_pos.x, (int)rect_pos.y,
      (int)rect_size.x, (int)rect_size.y);
  TEST_CHECK_(CountPixels(Vec2Si32(116, 108), Vec2Si32(8, 4), kRed) == 32 &&
      CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 32,
      "centered picture is not at (116, 108)");

  // Fit into a 40x40 box: the 2:1 picture becomes 40x20 in the middle.
  image->SetSize(Vec2Si32(40, 40));
  image->SetScaleMode(Image::kScaleFit);
  image->GetPictureRect(&rect_pos, &rect_size);
  TEST_CHECK_(rect_pos == Vec2Si32(0, 10) && rect_size == Vec2Si32(40, 20),
      "fit rect is (%d, %d) %dx%d", (int)rect_pos.x, (int)rect_pos.y,
      (int)rect_size.x, (int)rect_size.y);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(100, 110), Vec2Si32(40, 20), kRed) == 800 &&
      CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 800,
      "fit drew %d pixels in the band and %d in total",
      (int)CountPixels(Vec2Si32(100, 110), Vec2Si32(40, 20), kRed),
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));
  // A tall box: the width is the limit the other way round.
  image->SetSize(Vec2Si32(16, 40));
  image->GetPictureRect(&rect_pos, &rect_size);
  TEST_CHECK_(rect_pos == Vec2Si32(0, 16) && rect_size == Vec2Si32(16, 8),
      "fit into a tall box is (%d, %d) %dx%d", (int)rect_pos.x,
      (int)rect_pos.y, (int)rect_size.x, (int)rect_size.y);

  // A picture larger than the panel is clipped to the panel.
  image->SetSprite(SolidSprite(60, 60, kRed));
  image->SetSize(Vec2Si32(20, 20));
  image->SetScaleMode(Image::kScaleNone);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed) == 400,
      "an oversized picture drew %d pixels, expected the 20x20 panel",
      (int)CountPixels(Vec2Si32(0, 0), Vec2Si32(320, 200), kRed));
  TEST_CHECK(image->GetSprite().Size() == Vec2Si32(60, 60));
}

// A slider maps the thumb position to a value and back: a click on the track
// jumps there, a drag follows the cursor, the wheel and the keys step, and each
// change is one message.
void test_slider() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kGrey(64, 64, 64, 255);
  const Rgba kGreen(0, 255, 0, 255);
  const Rgba kBlue(0, 0, 255, 255);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  // 110 pixels long with a 10 pixel thumb: the thumb start moves over 100
  // pixels, one per unit of the 0..100 range.
  auto slider = std::make_shared<Slider>(1, Vec2Si32(10, 10), Vec2Si32(110, 10),
      1, SolidFrame(kGrey), SolidFrame(kGreen), SolidFrame(kGreen),
      SolidFrame(kBlue), SolidFrame(kGrey), 10, true);
  root->AddChild(slider);
  std::deque<GuiMessage> messages;
  Si32 changes = 0;
  slider->OnSliderChange = [&changes]() { ++changes; };
  TEST_CHECK(slider->GetMinValue() == 0 && slider->GetMaxValue() == 100);
  TEST_CHECK(slider->GetValue() == 0 && slider->IsHorizontal());

  // A click on the track centers the thumb under the cursor.
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 60, 15)), &messages);
  TEST_CHECK_(slider->GetValue() == 55,
      "a click at x = 60 gave %d, expected 55 (thumb centered)",
      (int)slider->GetValue());
  TEST_CHECK_(slider->IsFocused(), "a click did not focus the slider");
  TEST_CHECK(changes == 1 && CountMessages(messages, kGuiSliderChange) == 1);
  // Dragging with the button held follows the cursor.
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 80, 15), true), &messages);
  TEST_CHECK_(slider->GetValue() == 75, "a drag to x = 80 gave %d, expected 75",
      (int)slider->GetValue());
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 80, 40), true), &messages);
  TEST_CHECK_(slider->GetValue() == 75,
      "a drag straight up changed the value to %d", (int)slider->GetValue());
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 300, 40), true), &messages);
  TEST_CHECK_(slider->GetValue() == 100, "a drag past the end gave %d",
      (int)slider->GetValue());
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 300, 40)), &messages);
  TEST_CHECK_(changes == 3 && CountMessages(messages, kGuiSliderChange) == 3,
      "%d changes and %d messages after three moves", (int)changes,
      (int)CountMessages(messages, kGuiSliderChange));
  // After the release a move without the button does nothing.
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 20, 15)), &messages);
  TEST_CHECK(slider->GetValue() == 100);

  // A press on the thumb grabs it where it is: no jump, then a relative drag.
  slider->SetValue(75);
  Vec2Si32 thumb_pos;
  Vec2Si32 thumb_size;
  slider->GetThumbRect(&thumb_pos, &thumb_size);
  TEST_CHECK_(thumb_pos == Vec2Si32(75, 0) && thumb_size == Vec2Si32(10, 10),
      "thumb at 75 is (%d, %d) %dx%d", (int)thumb_pos.x, (int)thumb_pos.y,
      (int)thumb_size.x, (int)thumb_size.y);
  messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 77, 15)), &messages);
  TEST_CHECK_(slider->GetValue() == 75 && messages.empty(),
      "a press on the thumb moved it to %d", (int)slider->GetValue());
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 97, 15), true), &messages);
  TEST_CHECK_(slider->GetValue() == 95,
      "a drag of 20 pixels from the thumb gave %d, expected 95",
      (int)slider->GetValue());
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 97, 15)), &messages);

  // The wheel steps by the step, up is more.
  root->ApplyInput(WheelAt(Vec2Si32(10 + 50, 15), 120), &messages);
  TEST_CHECK_(slider->GetValue() == 96, "wheel up gave %d, expected 96",
      (int)slider->GetValue());
  slider->SetStep(5);
  TEST_CHECK(slider->GetStep() == 5);
  root->ApplyInput(WheelAt(Vec2Si32(10 + 50, 15), -120), &messages);
  TEST_CHECK_(slider->GetValue() == 91, "wheel down by 5 gave %d",
      (int)slider->GetValue());
  root->ApplyInput(WheelAt(Vec2Si32(200, 150), -120), &messages);
  TEST_CHECK_(slider->GetValue() == 91, "the wheel away from the slider moved it");

  // The keys, while focused.
  messages.clear();
  changes = 0;
  root->ApplyInput(KeyPress(kKeyRight), &messages);
  TEST_CHECK_(slider->GetValue() == 96, "Right gave %d", (int)slider->GetValue());
  root->ApplyInput(KeyPress(kKeyLeft), &messages);
  TEST_CHECK_(slider->GetValue() == 91, "Left gave %d", (int)slider->GetValue());
  root->ApplyInput(KeyPress(kKeyHome), &messages);
  TEST_CHECK_(slider->GetValue() == 0, "Home gave %d", (int)slider->GetValue());
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  TEST_CHECK_(slider->GetValue() == 100, "End gave %d", (int)slider->GetValue());
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  TEST_CHECK_(changes == 4 && CountMessages(messages, kGuiSliderChange) == 4,
      "a key that changes nothing still reported a change (%d changes)",
      (int)changes);
  root->ApplyInput(KeyPress(kKeyUp), &messages);
  TEST_CHECK_(slider->GetValue() == 100,
      "Up moved a horizontal slider to %d", (int)slider->GetValue());

  // Drawing: the thumb is green where the value is, the track grey elsewhere.
  slider->SetValue(0);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 5, 15)).rgba == kGreen.rgba,
      "the thumb at value 0 is not drawn at the left end");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 50, 15)).rgba == kGrey.rgba,
      "the track is not drawn in the middle");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 109, 15)).rgba == kGrey.rgba,
      "the track does not reach the right end");
  slider->SetValue(100);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 105, 15)).rgba == kGreen.rgba &&
      BackbufferPixel(Vec2Si32(10 + 5, 15)).rgba == kGrey.rgba,
      "the thumb at value 100 is not drawn at the right end");

  // Range changes clamp the value; a disabled slider takes nothing.
  slider->SetRange(0, 10);
  TEST_CHECK_(slider->GetValue() == 10, "SetRange left the value at %d",
      (int)slider->GetValue());
  slider->SetRange(20, 5);
  TEST_CHECK_(slider->GetMinValue() == 20 && slider->GetMaxValue() == 20 &&
      slider->GetValue() == 20, "a reversed range was not repaired");
  slider->SetRange(0, 100);
  slider->SetValue(50);
  slider->SetEnabled(false);
  TEST_CHECK(!slider->IsEnabled());
  messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 90, 15)), &messages);
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  TEST_CHECK_(slider->GetValue() == 50 && messages.empty(),
      "a disabled slider moved to %d", (int)slider->GetValue());
  slider->SetEnabled(true);
  root->ApplyInput(LeftClickAt(Vec2Si32(10 + 90, 15)), &messages);
  TEST_CHECK_(slider->GetValue() == 85, "an enabled slider did not move");
  root->ApplyInput(LeftReleaseAt(Vec2Si32(10 + 90, 15)), &messages);

  // A vertical slider grows upward and listens to Up and Down.
  auto vertical = std::make_shared<Slider>(2, Vec2Si32(200, 10),
      Vec2Si32(10, 110), 2, SolidFrame(kGrey), SolidFrame(kGreen),
      SolidFrame(kGreen), SolidFrame(kBlue), SolidFrame(kGrey), 10, false);
  root->AddChild(vertical);
  root->ApplyInput(LeftClickAt(Vec2Si32(205, 10 + 60)), &messages);
  TEST_CHECK_(vertical->GetValue() == 55, "a click at y = 60 gave %d",
      (int)vertical->GetValue());
  root->ApplyInput(LeftReleaseAt(Vec2Si32(205, 10 + 60)), &messages);
  root->ApplyInput(KeyPress(kKeyUp), &messages);
  TEST_CHECK_(vertical->GetValue() == 56, "Up gave %d", (int)vertical->GetValue());
  root->ApplyInput(KeyPress(kKeyDown), &messages);
  root->ApplyInput(KeyPress(kKeyDown), &messages);
  TEST_CHECK_(vertical->GetValue() == 54, "Down gave %d", (int)vertical->GetValue());
  vertical->GetThumbRect(&thumb_pos, &thumb_size);
  TEST_CHECK_(thumb_pos == Vec2Si32(0, 54) && thumb_size == Vec2Si32(10, 10),
      "vertical thumb is (%d, %d) %dx%d", (int)thumb_pos.x, (int)thumb_pos.y,
      (int)thumb_size.x, (int)thumb_size.y);
  TEST_CHECK_(root->IsInside(Vec2Si32(205, 100)) &&
      !root->IsInside(Vec2Si32(205, 130)),
      "IsInside does not agree with the slider rectangle");
}

// Radio buttons in one parent with one group number select one of them at a
// time; another group number in the same parent is another group.
void test_radio_buttons() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  const Rgba kBlue(0, 0, 255, 255);
  Sprite clear = SolidSprite(20, 20, kBlue);
  Sprite checked = SolidSprite(20, 20, kRed);
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto make = [&](Ui64 tag, Si32 x, Si32 group) {
    auto radio = std::make_shared<RadioButton>(tag, Vec2Si32(x, 10), (Ui32)tag,
        clear, checked, clear, checked, clear, checked, clear, checked,
        Sound(), Sound(), kKeyNone, group);
    root->AddChild(radio);
    return radio;
  };
  auto a = make(1, 10, 0);
  auto b = make(2, 40, 0);
  auto c = make(3, 70, 0);
  auto d = make(4, 110, 1);
  auto e = make(5, 140, 1);
  TEST_CHECK(a->GetGroup() == 0 && d->GetGroup() == 1);
  std::deque<GuiMessage> messages;
  Si32 a_selects = 0;
  a->OnSelect = [&a_selects]() { ++a_selects; };

  Click(root.get(), Vec2Si32(20, 20), &messages);
  TEST_CHECK_(a->IsChecked() && !b->IsChecked() && !c->IsChecked(),
      "a click did not check the first radio button alone");
  TEST_CHECK_(!d->IsChecked() && !e->IsChecked(),
      "a click in group 0 touched group 1");
  TEST_CHECK_(a_selects == 1 &&
      CountMessages(messages, kGuiRadioSelect, a.get()) == 1 &&
      CountMessages(messages, kGuiButtonClick, a.get()) == 1,
      "selecting gave %d OnSelect calls, %d kGuiRadioSelect and %d "
      "kGuiButtonClick", (int)a_selects,
      (int)CountMessages(messages, kGuiRadioSelect, a.get()),
      (int)CountMessages(messages, kGuiButtonClick, a.get()));

  // A click on the checked one keeps it checked and is not a new selection.
  messages.clear();
  Click(root.get(), Vec2Si32(20, 20), &messages);
  TEST_CHECK_(a->IsChecked(), "a click cleared the checked radio button");
  TEST_CHECK_(a_selects == 1 && CountMessages(messages, kGuiRadioSelect) == 0,
      "re-clicking the checked radio button reported a selection");
  TEST_CHECK_(CountMessages(messages, kGuiButtonClick, a.get()) == 1,
      "the click itself was not reported");

  Click(root.get(), Vec2Si32(50, 20), &messages);
  TEST_CHECK_(b->IsChecked() && !a->IsChecked() && !c->IsChecked(),
      "selecting the second did not clear the first");
  Click(root.get(), Vec2Si32(120, 20), &messages);
  TEST_CHECK_(d->IsChecked() && !e->IsChecked() && b->IsChecked(),
      "selecting in group 1 disturbed group 0 or checked both of group 1");
  Click(root.get(), Vec2Si32(150, 20), &messages);
  TEST_CHECK_(e->IsChecked() && !d->IsChecked() && b->IsChecked(),
      "the second of group 1 did not take over from the first");

  // Programmatic checks follow the same rule; clearing is allowed.
  messages.clear();
  c->SetChecked(true);
  TEST_CHECK_(c->IsChecked() && !b->IsChecked() && !a->IsChecked() &&
      e->IsChecked(), "SetChecked(true) did not clear the rest of its group");
  TEST_CHECK_(messages.empty() && a_selects == 1,
      "SetChecked reported through messages or callbacks");
  c->SetChecked(false);
  TEST_CHECK_(!a->IsChecked() && !b->IsChecked() && !c->IsChecked(),
      "SetChecked(false) left something checked in group 0");

  // The hotkey selects as a click does; Space while focused as well.
  a->SetHotkey(kKey1);
  root->ApplyInput(KeyPress(kKey1), &messages);
  root->ApplyInput(KeyRelease(kKey1), &messages);
  TEST_CHECK_(a->IsChecked() && a_selects == 2, "the hotkey did not select");
  root->MakeCurrentTab(nullptr);
  b->SetCurrentTab(true);
  root->ApplyInput(KeyPress(kKeySpace), &messages);
  root->ApplyInput(KeyRelease(kKeySpace), &messages);
  TEST_CHECK_(b->IsChecked() && !a->IsChecked(),
      "Space on the focused radio button did not select it");

  // Moving a button to another group changes what it clears.
  c->SetGroup(1);
  c->SetChecked(true);
  TEST_CHECK_(c->IsChecked() && !e->IsChecked() && b->IsChecked(),
      "a button moved to group 1 did not clear group 1 or cleared group 0");

  // Drawing follows the value.
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(50, 20)).rgba == kRed.rgba,
      "the checked radio button is not drawn checked");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(20, 20)).rgba == kBlue.rgba,
      "a clear radio button is not drawn clear");

  // A disabled radio button is not selected by a click.
  d->SetEnabled(false);
  Click(root.get(), Vec2Si32(120, 20), &messages);
  TEST_CHECK_(!d->IsChecked() && c->IsChecked(),
      "a disabled radio button was selected");
}

// A list box selects with a click, scrolls with the wheel, the keys and its
// scrollbar, keeps the selection in view and clips long rows.
void test_listbox() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kGrey(64, 64, 64, 255);
  const Rgba kGreen(0, 255, 0, 255);
  const Rgba kBlue(0, 0, 255, 255);
  Font font = BlockFont();
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  // Rows are 5 + 2 * 2 = 9 pixels; a 1 pixel frame and 2 pixels of padding
  // around 4 rows make the list 42 pixels high.
  auto list = std::make_shared<ListBox>(1, Vec2Si32(10, 10), Vec2Si32(100, 42),
      1, font, kWhite, SolidFrame(kGrey), SolidFrame(kGreen), SolidFrame(kBlue));
  root->AddChild(list);
  std::vector<std::string> items;
  for (Si32 i = 0; i < 10; ++i) {
    items.push_back(std::string("item") + static_cast<char>('0' + i));
  }
  list->SetItems(items);
  TEST_CHECK(list->GetItemCount() == 10 && list->GetRowHeight() == 9);
  TEST_CHECK_(list->GetVisibleRows() == 4, "%d visible rows, expected 4",
      (int)list->GetVisibleRows());
  TEST_CHECK(list->GetSelectedIndex() == -1 && list->GetSelectedItem().empty());
  TEST_CHECK(list->HeightForRows(4) == 42);
  std::deque<GuiMessage> messages;
  Si32 selection_changes = 0;
  Si32 activations = 0;
  list->OnSelectionChange = [&selection_changes]() { ++selection_changes; };
  list->OnItemActivate = [&activations]() { ++activations; };

  // Row r (from the top) spans y in [39 - 9 * (r + 1), 39 - 9 * r) of the list.
  Vec2Si32 row_pos;
  Vec2Si32 row_size;
  TEST_CHECK(list->GetRowRect(1, &row_pos, &row_size));
  TEST_CHECK_(row_pos == Vec2Si32(3, 21) && row_size == Vec2Si32(94, 9),
      "row 1 is (%d, %d) %dx%d", (int)row_pos.x, (int)row_pos.y,
      (int)row_size.x, (int)row_size.y);
  TEST_CHECK_(list->ItemAt(Vec2Si32(20, 25)) == 1 &&
      list->ItemAt(Vec2Si32(20, 38)) == 0 && list->ItemAt(Vec2Si32(20, 3)) == 3
      && list->ItemAt(Vec2Si32(20, 2)) == -1 && list->ItemAt(Vec2Si32(1, 25)) == -1,
      "ItemAt does not agree with the row rectangles");
  TEST_CHECK(!list->GetRowRect(4, &row_pos, &row_size));

  root->ApplyInput(LeftClickAt(Vec2Si32(30, 35)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(30, 35)), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 1 && list->GetSelectedItem() == "item1",
      "a click on row 1 selected %d", (int)list->GetSelectedIndex());
  TEST_CHECK_(list->IsFocused(), "a click did not focus the list");
  TEST_CHECK_(selection_changes == 1 &&
      CountMessages(messages, kGuiListSelectionChange) == 1 &&
      CountMessages(messages, kGuiListItemClick) == 1 &&
      CountMessages(messages, kGuiListItemActivate) == 0,
      "one click gave %d selection changes, %d click and %d activate messages",
      (int)selection_changes, (int)CountMessages(messages, kGuiListItemClick),
      (int)CountMessages(messages, kGuiListItemActivate));
  // A second click right away on the same row is a double click.
  root->ApplyInput(LeftClickAt(Vec2Si32(30, 35)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(30, 35)), &messages);
  TEST_CHECK_(activations == 1 &&
      CountMessages(messages, kGuiListItemActivate) == 1 &&
      selection_changes == 1,
      "a double click gave %d activations and %d selection changes",
      (int)activations, (int)selection_changes);
  // A click on the empty part below the rows changes nothing.
  list->SetItems(std::vector<std::string>(items.begin(), items.begin() + 2));
  messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(30, 15)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(30, 15)), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == -1 && messages.empty(),
      "a click below the last row selected %d", (int)list->GetSelectedIndex());
  list->SetItems(items);

  // The wheel scrolls by rows; the row under the cursor changes with it.
  root->ApplyInput(WheelAt(Vec2Si32(30, 35), -120), &messages);
  TEST_CHECK_(list->GetFirstVisible() == 1, "wheel down gave first row %d",
      (int)list->GetFirstVisible());
  TEST_CHECK(list->ItemAt(Vec2Si32(20, 25)) == 2);
  root->ApplyInput(WheelAt(Vec2Si32(30, 35), -120 * 10), &messages);
  TEST_CHECK_(list->GetFirstVisible() == 6, "wheel past the end gave %d",
      (int)list->GetFirstVisible());
  root->ApplyInput(WheelAt(Vec2Si32(30, 35), 120), &messages);
  TEST_CHECK_(list->GetFirstVisible() == 5, "wheel up gave %d",
      (int)list->GetFirstVisible());

  // The keys move the selection and keep it in view.
  selection_changes = 0;
  root->ApplyInput(LeftClickAt(Vec2Si32(30, 35)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(30, 35)), &messages);
  TEST_CHECK(list->GetSelectedIndex() == 6);
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 9 && list->GetFirstVisible() == 6,
      "End gave selection %d with first row %d", (int)list->GetSelectedIndex(),
      (int)list->GetFirstVisible());
  root->ApplyInput(KeyPress(kKeyHome), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 0 && list->GetFirstVisible() == 0,
      "Home gave selection %d with first row %d", (int)list->GetSelectedIndex(),
      (int)list->GetFirstVisible());
  for (Si32 i = 0; i < 4; ++i) {
    root->ApplyInput(KeyPress(kKeyDown), &messages);
  }
  TEST_CHECK_(list->GetSelectedIndex() == 4 && list->GetFirstVisible() == 1,
      "four Downs gave selection %d with first row %d",
      (int)list->GetSelectedIndex(), (int)list->GetFirstVisible());
  root->ApplyInput(KeyPress(kKeyPageDown), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 8 && list->GetFirstVisible() == 5,
      "PageDown gave selection %d with first row %d",
      (int)list->GetSelectedIndex(), (int)list->GetFirstVisible());
  root->ApplyInput(KeyPress(kKeyPageUp), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 4 && list->GetFirstVisible() == 4,
      "PageUp gave selection %d with first row %d",
      (int)list->GetSelectedIndex(), (int)list->GetFirstVisible());
  root->ApplyInput(KeyPress(kKeyUp), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 3 && list->GetFirstVisible() == 3,
      "Up gave selection %d with first row %d",
      (int)list->GetSelectedIndex(), (int)list->GetFirstVisible());
  TEST_CHECK_(selection_changes == 10, "%d selection changes, expected 10",
      (int)selection_changes);
  activations = 0;
  root->ApplyInput(KeyPress(kKeyEnter), &messages);
  TEST_CHECK_(activations == 1, "Enter did not activate the selected item");
  // Unfocused, the keys do nothing.
  root->MakeCurrentTab(nullptr);
  root->ApplyInput(KeyPress(kKeyDown), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 3, "an unfocused list moved on Down");

  // Removing an item ahead of the selection shifts it; removing the selected
  // one clears it.
  list->RemoveItem(0);
  TEST_CHECK_(list->GetSelectedIndex() == 2 && list->GetSelectedItem() == "item3",
      "after removing item 0 the selection is %d", (int)list->GetSelectedIndex());
  list->RemoveItem(2);
  TEST_CHECK_(list->GetSelectedIndex() == -1 && list->GetItemCount() == 8,
      "removing the selected item left the selection at %d",
      (int)list->GetSelectedIndex());
  list->SetSelectedIndex(7);
  TEST_CHECK(list->GetSelectedItem() == "item9");
  list->SetSelectedIndex(8);
  TEST_CHECK_(list->GetSelectedIndex() == -1, "an index past the end selected");
  list->SetItems(items);

  // Drawing: the selected row is green, the hovered one blue, the text white,
  // and a long row does not leak past the list.
  list->SetFirstVisible(0);
  list->SetSelectedIndex(0);
  root->ApplyInput(MouseMoveTo(Vec2Si32(30, 25)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  // Row 0 is at y in [30, 39), row 1 in [21, 30), row 2 in [12, 21).
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 96, 10 + 35)).rgba == kGreen.rgba,
      "the selected row is not drawn green at its right end");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 96, 10 + 15)).rgba == kBlue.rgba,
      "the hovered row is not drawn blue");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 96, 10 + 25)).rgba == kGrey.rgba,
      "a plain row is not the background");
  TEST_CHECK_(CountPixels(Vec2Si32(13, 40), Vec2Si32(94, 9), kWhite) == 5 * 4 * 5,
      "row 0 has %d white pixels, expected 'item0' as five 4x5 glyphs",
      (int)CountPixels(Vec2Si32(13, 40), Vec2Si32(94, 9), kWhite));
  items[0] = std::string(60, 'a');  // 240 pixels, far wider than the list
  list->SetItems(items);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(Vec2Si32(110, 10), Vec2Si32(210, 42), kWhite) == 0,
      "%d white pixels leaked to the right of the list",
      (int)CountPixels(Vec2Si32(110, 10), Vec2Si32(210, 42), kWhite));
  TEST_CHECK_(CountPixels(Vec2Si32(13, 40), Vec2Si32(94, 9), kWhite) > 5 * 4 * 5,
      "the long row was not drawn inside the list");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 99, 10 + 35)).rgba == kGrey.rgba,
      "the long row covered the frame of the list");

  // Disabled: no clicks, no keys, no hover color.
  list->SetEnabled(false);
  messages.clear();
  root->ApplyInput(LeftClickAt(Vec2Si32(30, 35)), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == -1 && messages.empty(),
      "a disabled list selected %d", (int)list->GetSelectedIndex());
  list->SetEnabled(true);

  // With a theme the list gets a scrollbar that follows it and drives it.
  auto theme = std::make_shared<GuiTheme>();
  theme->text_ = std::make_shared<GuiThemeText>();
  theme->text_->font_ = font;
  theme->text_->palete_.push_back(kWhite);
  theme->text_->disabled_palete_.push_back(kGrey);
  theme->v_scrollbar_ = std::make_shared<GuiThemeScrollbar>();
  Sprite knob = SolidSprite(12, 12, kBlue);
  theme->v_scrollbar_->normal_button_dec_ = knob;
  theme->v_scrollbar_->focused_button_dec_ = knob;
  theme->v_scrollbar_->down_button_dec_ = knob;
  theme->v_scrollbar_->disabled_button_dec_ = knob;
  theme->v_scrollbar_->normal_button_inc_ = knob;
  theme->v_scrollbar_->focused_button_inc_ = knob;
  theme->v_scrollbar_->down_button_inc_ = knob;
  theme->v_scrollbar_->disabled_button_inc_ = knob;
  theme->v_scrollbar_->normal_button_cur_ = knob;
  theme->v_scrollbar_->focused_button_cur_ = knob;
  theme->v_scrollbar_->down_button_cur_ = knob;
  theme->v_scrollbar_->disabled_button_cur_ = knob;
  theme->v_scrollbar_->is_horizontal_ = false;
  theme->listbox_background_ = SolidFrame(kGrey);
  theme->listbox_selection_ = SolidFrame(kGreen);
  theme->listbox_hover_ = SolidFrame(kBlue);
  theme->panel_background_ = SolidFrame(kBlack);
  auto themed = std::make_shared<ListBox>(2, theme);
  themed->SetPos(Vec2Si32(150, 10));
  themed->SetSize(Vec2Si32(120, 42));
  root->AddChild(themed);
  themed->SetItems(items);
  TEST_CHECK_(themed->GetVisibleRows() == 4, "themed list shows %d rows",
      (int)themed->GetVisibleRows());
  const std::deque<std::shared_ptr<Panel>> &children = themed->GetChildren();
  TEST_CHECK_(children.size() == 1, "the themed list has %d children, "
      "expected the scrollbar", (int)children.size());
  std::shared_ptr<Scrollbar> scrollbar = children.empty() ?
      std::shared_ptr<Scrollbar>() :
      std::dynamic_pointer_cast<Scrollbar>(children.front());
  TEST_CHECK_(!!scrollbar, "the child of the themed list is not a scrollbar");
  if (scrollbar) {
    TEST_CHECK_(scrollbar->GetMaxValue() == 6 && scrollbar->GetValue() == 6,
        "scrollbar is %d of %d at the top, expected 6 of 6",
        (int)scrollbar->GetValue(), (int)scrollbar->GetMaxValue());
    Vec2Si32 bar_pos = scrollbar->GetPos();
    Vec2Si32 bar_size = scrollbar->GetSize();
    TEST_CHECK_(bar_pos.x + bar_size.x == 120 - 1 && bar_pos.y == 1 &&
        bar_size.y == 40, "the scrollbar is at (%d, %d) %dx%d", (int)bar_pos.x,
        (int)bar_pos.y, (int)bar_size.x, (int)bar_size.y);
    TEST_CHECK_(themed->ItemAt(Vec2Si32(bar_pos.x + 3, 25)) == -1,
        "a point on the scrollbar counts as a row");
    root->ApplyInput(WheelAt(Vec2Si32(160, 30), -120 * 2), &messages);
    TEST_CHECK_(themed->GetFirstVisible() == 2 && scrollbar->GetValue() == 4,
        "after two rows down the scrollbar is at %d", (int)scrollbar->GetValue());
    // The dec arrow of a vertical scrollbar is at its bottom; a press there
    // lowers the value by one, which is one more row down for the list.
    Vec2Si32 dec_arrow(150 + bar_pos.x + 6, 10 + bar_pos.y + 6);
    root->ApplyInput(LeftClickAt(dec_arrow), &messages);
    root->ApplyInput(LeftReleaseAt(dec_arrow), &messages);
    TEST_CHECK_(scrollbar->GetValue() == 3 && themed->GetFirstVisible() == 3,
        "the scrollbar arrow gave value %d and first row %d",
        (int)scrollbar->GetValue(), (int)themed->GetFirstVisible());
    themed->SetItems(std::vector<std::string>(items.begin(), items.begin() + 3));
    TEST_CHECK_(scrollbar->GetMaxValue() == 0 && !scrollbar->IsEnabled(),
        "a list that fits keeps an enabled scrollbar with max %d",
        (int)scrollbar->GetMaxValue());
  }
}

// A dropdown opens its list over everything, chooses with a click or Enter,
// closes on a click outside or Escape, and steps with the arrow keys while
// closed.
void test_dropdown() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kGrey(64, 64, 64, 255);
  const Rgba kDark(32, 32, 32, 255);
  const Rgba kGreen(0, 255, 0, 255);
  const Rgba kBlue(0, 0, 255, 255);
  const Rgba kRed(255, 0, 0, 255);
  Font font = BlockFont();
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto button = std::make_shared<Button>(9, Vec2Si32(60, 70),
      SolidSprite(40, 20, kRed), Sprite(), Sprite(), Sound(), Sound(),
      kKeyNone, 2);
  Si32 button_downs = 0;
  button->OnButtonDown = [&button_downs]() { ++button_downs; };
  root->AddChild(button);
  auto dropdown = std::make_shared<Dropdown>(1, Vec2Si32(50, 100),
      Vec2Si32(80, 20), 1, font, kWhite, SolidFrame(kGrey), SolidFrame(kGrey),
      SolidFrame(kDark), SolidFrame(kDark), SolidFrame(kGrey),
      SolidFrame(kGreen), SolidFrame(kBlue));
  root->AddChild(dropdown);
  std::vector<std::string> items;
  items.push_back("aa");
  items.push_back("bb");
  items.push_back("cc");
  dropdown->SetItems(items);
  TEST_CHECK(dropdown->GetItemCount() == 3 && dropdown->GetSelectedIndex() == -1);
  std::deque<GuiMessage> messages;
  Si32 changes = 0;
  dropdown->OnChange = [&changes]() { ++changes; };

  // Closed: the button under the future list is visible and clickable.
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK(!dropdown->IsOpen());
  TEST_CHECK_(BackbufferPixel(Vec2Si32(70, 80)).rgba == kRed.rgba,
      "the button under the closed dropdown is not drawn");
  TEST_CHECK(root->IsInside(Vec2Si32(70, 80)));

  // A click on the box opens the list below it: 3 rows of 9 plus the frame and
  // padding, 33 pixels, from y = 67 to 100.
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 110)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 110)), &messages);
  TEST_CHECK_(dropdown->IsOpen(), "a click on the box did not open the list");
  TEST_CHECK_(dropdown->IsFocused(), "a click did not focus the dropdown");
  std::shared_ptr<ListBox> list = dropdown->GetList();
  TEST_CHECK_(list->GetPos() == Vec2Si32(0, -33) &&
      list->GetSize() == Vec2Si32(80, 33), "the list is at (%d, %d) %dx%d",
      (int)list->GetPos().x, (int)list->GetPos().y, (int)list->GetSize().x,
      (int)list->GetSize().y);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(70, 80)).rgba != kRed.rgba,
      "the open list is drawn under the button instead of over it");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(70, 80)).rgba == kGrey.rgba ||
      BackbufferPixel(Vec2Si32(70, 80)).rgba == kWhite.rgba,
      "the pixel where the list should be is not the list");
  TEST_CHECK_(root->IsInside(Vec2Si32(120, 70)),
      "IsInside does not count the open list");
  // The list rows: row 1 spans y in [79, 88) on screen.
  root->ApplyInput(LeftClickAt(Vec2Si32(70, 83)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(70, 83)), &messages);
  TEST_CHECK_(dropdown->GetSelectedIndex() == 1 &&
      dropdown->GetSelectedItem() == "bb",
      "a click on the second row chose %d", (int)dropdown->GetSelectedIndex());
  TEST_CHECK_(!dropdown->IsOpen(), "the list stayed open after a choice");
  TEST_CHECK_(changes == 1 && CountMessages(messages, kGuiDropdownChange,
      dropdown.get()) == 1, "%d OnChange calls and %d messages", (int)changes,
      (int)CountMessages(messages, kGuiDropdownChange, dropdown.get()));
  TEST_CHECK_(button_downs == 0, "the click on the list reached the button");
  for (auto it = messages.begin(); it != messages.end(); ++it) {
    TEST_CHECK_(it->panel.get() != list.get(),
        "a message of the inner list leaked to the host");
  }

  // Re-open, click far away: the list closes and the click is used up.
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 110)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 110)), &messages);
  TEST_CHECK(dropdown->IsOpen());
  TEST_CHECK_(root->ApplyInput(LeftClickAt(Vec2Si32(250, 150)), &messages),
      "a click outside the open list was not taken");
  TEST_CHECK_(!dropdown->IsOpen(), "a click outside did not close the list");
  TEST_CHECK_(dropdown->GetSelectedIndex() == 1, "a click outside changed the choice");
  root->ApplyInput(LeftReleaseAt(Vec2Si32(250, 150)), &messages);
  TEST_CHECK_(!root->ApplyInput(LeftClickAt(Vec2Si32(250, 150)), &messages),
      "with the list closed a click far away was taken by something");
  root->ApplyInput(LeftReleaseAt(Vec2Si32(250, 150)), &messages);

  // The box itself toggles the list closed again.
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 110)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 110)), &messages);
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 110)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 110)), &messages);
  TEST_CHECK_(!dropdown->IsOpen(), "a second click on the box left the list open");

  // Keyboard: Space opens, Down moves in the list, Enter chooses, Escape closes.
  root->ApplyInput(KeyPress(kKeySpace), &messages);
  TEST_CHECK_(dropdown->IsOpen(), "Space did not open the list");
  TEST_CHECK_(list->GetSelectedIndex() == 1, "the open list does not start on "
      "the chosen item, it is on %d", (int)list->GetSelectedIndex());
  root->ApplyInput(KeyPress(kKeyDown), &messages);
  TEST_CHECK_(list->GetSelectedIndex() == 2 && dropdown->GetSelectedIndex() == 1,
      "Down in the open list changed the choice early or did not move");
  root->ApplyInput(KeyPress(kKeyEnter), &messages);
  TEST_CHECK_(dropdown->GetSelectedIndex() == 2 && !dropdown->IsOpen() &&
      changes == 2, "Enter did not choose the highlighted item");
  root->ApplyInput(KeyPress(kKeyEnter), &messages);
  TEST_CHECK(dropdown->IsOpen());
  root->ApplyInput(KeyPress(kKeyEscape), &messages);
  TEST_CHECK_(!dropdown->IsOpen() && dropdown->GetSelectedIndex() == 2,
      "Escape did not close the list or changed the choice");
  // Closed and focused, the arrows step through the items.
  root->ApplyInput(KeyPress(kKeyUp), &messages);
  TEST_CHECK_(dropdown->GetSelectedIndex() == 1 && changes == 3,
      "Up on a closed dropdown gave %d", (int)dropdown->GetSelectedIndex());
  root->ApplyInput(KeyPress(kKeyHome), &messages);
  root->ApplyInput(KeyPress(kKeyUp), &messages);
  TEST_CHECK_(dropdown->GetSelectedIndex() == 0 && changes == 4,
      "Up at the first item moved or reported a change");
  root->ApplyInput(KeyPress(kKeyEnd), &messages);
  TEST_CHECK(dropdown->GetSelectedIndex() == 2);

  // Losing the focus closes the list.
  root->ApplyInput(KeyPress(kKeySpace), &messages);
  TEST_CHECK(dropdown->IsOpen());
  root->MakeCurrentTab(nullptr);
  dropdown->SetCurrentTab(false);
  TEST_CHECK_(!dropdown->IsOpen(), "the list survived the loss of the focus");

  // No room below: the list opens above the box.
  dropdown->SetPos(Vec2Si32(50, 10));
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 20)), &messages);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 20)), &messages);
  TEST_CHECK_(dropdown->IsOpen() && list->GetPos() == Vec2Si32(0, 20),
      "near the bottom the list opened at (%d, %d), expected above the box",
      (int)list->GetPos().x, (int)list->GetPos().y);
  dropdown->Close();
  TEST_CHECK(!dropdown->IsOpen());

  // Many items: the list shows max_visible rows and scrolls to the choice.
  std::vector<std::string> many;
  for (Si32 i = 0; i < 20; ++i) {
    many.push_back(std::string("n") + static_cast<char>('a' + i));
  }
  dropdown->SetItems(many);
  TEST_CHECK_(dropdown->GetSelectedIndex() == -1, "SetItems kept the choice");
  dropdown->SetMaxVisibleItems(5);
  dropdown->SetSelectedIndex(15);
  root->ApplyInput(KeyPress(kKeySpace), &messages);
  TEST_CHECK_(list->GetVisibleRows() == 5 && list->GetSize().y == 5 * 9 + 6,
      "the list shows %d rows in %d pixels", (int)list->GetVisibleRows(),
      (int)list->GetSize().y);
  TEST_CHECK_(list->GetFirstVisible() == 15 && list->GetSelectedIndex() == 15,
      "the list did not scroll to the chosen item: first row %d",
      (int)list->GetFirstVisible());
  dropdown->Close();

  // Disabled: nothing opens.
  dropdown->SetEnabled(false);
  root->ApplyInput(LeftClickAt(Vec2Si32(90, 20)), &messages);
  TEST_CHECK_(!dropdown->IsOpen(), "a disabled dropdown opened");
  dropdown->SetEnabled(true);
  root->ApplyInput(LeftReleaseAt(Vec2Si32(90, 20)), &messages);
}

// A tab control shows one page at a time, switches with a click on a header or
// with Left and Right, and only the shown page takes input.
void test_tab_control() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kGrey(64, 64, 64, 255);
  const Rgba kDark(32, 32, 32, 255);
  const Rgba kGreen(0, 255, 0, 255);
  const Rgba kBlue(0, 0, 255, 255);
  const Rgba kRed(255, 0, 0, 255);
  Font font = BlockFont();
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto tabs = std::make_shared<TabControl>(1, Vec2Si32(10, 10),
      Vec2Si32(200, 100), 1, font, kWhite, SolidFrame(kGrey),
      SolidFrame(kGreen), SolidFrame(kBlue), SolidFrame(kDark), 20);
  root->AddChild(tabs);
  TEST_CHECK(tabs->GetTabCount() == 0 && tabs->GetSelectedIndex() == -1);
  std::shared_ptr<Panel> page0 = tabs->AddTab("aa");
  std::shared_ptr<Panel> page1 = tabs->AddTab("bbb");
  TEST_CHECK(tabs->GetTabCount() == 2 && tabs->GetSelectedIndex() == 0);
  TEST_CHECK(tabs->GetPage(0) == page0 && tabs->GetPage(1) == page1);
  TEST_CHECK(tabs->GetPage(2) == Panel::Invalid());
  TEST_CHECK(tabs->GetTitle(1) == "bbb" && tabs->GetTitle(5).empty());
  TEST_CHECK_(page0->IsVisible() && !page1->IsVisible(),
      "only the first page should be visible after AddTab");
  Vec2Si32 rect_pos;
  Vec2Si32 rect_size;
  tabs->GetPageRect(&rect_pos, &rect_size);
  TEST_CHECK_(rect_pos == Vec2Si32(0, 0) && rect_size == Vec2Si32(200, 80) &&
      page1->GetSize() == rect_size, "the page rect is (%d, %d) %dx%d and the "
      "page is %dx%d", (int)rect_pos.x, (int)rect_pos.y, (int)rect_size.x,
      (int)rect_size.y, (int)page1->GetSize().x, (int)page1->GetSize().y);
  // Header widths are the title width plus 8 pixels on each side.
  TEST_CHECK(tabs->GetHeaderRect(0, &rect_pos, &rect_size));
  TEST_CHECK_(rect_pos == Vec2Si32(0, 80) && rect_size == Vec2Si32(24, 20),
      "header 0 is (%d, %d) %dx%d", (int)rect_pos.x, (int)rect_pos.y,
      (int)rect_size.x, (int)rect_size.y);
  TEST_CHECK(tabs->GetHeaderRect(1, &rect_pos, &rect_size));
  TEST_CHECK_(rect_pos == Vec2Si32(24, 80) && rect_size == Vec2Si32(28, 20),
      "header 1 is (%d, %d) %dx%d", (int)rect_pos.x, (int)rect_pos.y,
      (int)rect_size.x, (int)rect_size.y);
  TEST_CHECK(!tabs->GetHeaderRect(2, &rect_pos, &rect_size));

  auto button = std::make_shared<Button>(9, Vec2Si32(5, 5),
      SolidSprite(30, 10, kRed), Sprite(), Sprite(), Sound(), Sound(),
      kKeyNone, 3);
  Si32 button_downs = 0;
  button->OnButtonDown = [&button_downs]() { ++button_downs; };
  page1->AddChild(button);
  std::deque<GuiMessage> messages;
  Si32 tab_changes = 0;
  tabs->OnTabChange = [&tab_changes]() { ++tab_changes; };

  // The button lives on the hidden page: it is neither drawn nor clickable.
  const Vec2Si32 on_button(10 + 5 + 10, 10 + 5 + 5);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(on_button).rgba == kDark.rgba,
      "a button on a hidden page is drawn, or the page is not");
  root->ApplyInput(LeftClickAt(on_button), &messages);
  root->ApplyInput(LeftReleaseAt(on_button), &messages);
  TEST_CHECK_(button_downs == 0, "a button on a hidden page took a click");
  TEST_CHECK_(!root->IsInside(on_button),
      "IsInside counts a button on a hidden page");

  // A click on the second header switches pages.
  const Vec2Si32 on_header1(10 + 24 + 5, 10 + 85);
  TEST_CHECK_(root->IsInside(on_header1), "IsInside does not count a header");
  root->ApplyInput(LeftClickAt(on_header1), &messages);
  root->ApplyInput(LeftReleaseAt(on_header1), &messages);
  TEST_CHECK_(tabs->GetSelectedIndex() == 1 && tab_changes == 1 &&
      CountMessages(messages, kGuiTabChange, tabs.get()) == 1,
      "a click on header 1 gave tab %d, %d changes, %d messages",
      (int)tabs->GetSelectedIndex(), (int)tab_changes,
      (int)CountMessages(messages, kGuiTabChange, tabs.get()));
  TEST_CHECK_(tabs->IsFocused(), "a click on a header did not focus the control");
  TEST_CHECK_(page1->IsVisible() && !page0->IsVisible(),
      "the pages did not swap visibility");
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(on_button).rgba == kRed.rgba,
      "the button on the shown page is not drawn");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 24 + 2, 10 + 82)).rgba == kGreen.rgba,
      "the selected header is not drawn selected");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 2, 10 + 82)).rgba == kGrey.rgba,
      "the other header is not drawn normal");
  TEST_CHECK_(CountPixels(Vec2Si32(10, 90), Vec2Si32(52, 20), kWhite) == 5 * 4 * 5,
      "the titles have %d white pixels, expected 'aa' and 'bbb' as 4x5 glyphs",
      (int)CountPixels(Vec2Si32(10, 90), Vec2Si32(52, 20), kWhite));
  root->ApplyInput(LeftClickAt(on_button), &messages);
  root->ApplyInput(LeftReleaseAt(on_button), &messages);
  TEST_CHECK_(button_downs == 1, "the button on the shown page took no click");
  // Clicking the selected header again is not a change.
  root->ApplyInput(LeftClickAt(on_header1), &messages);
  root->ApplyInput(LeftReleaseAt(on_header1), &messages);
  TEST_CHECK_(tab_changes == 1, "re-clicking the selected header reported a change");
  // Hovering the other header colors it.
  root->ApplyInput(MouseMoveTo(Vec2Si32(10 + 5, 10 + 85)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(10 + 2, 10 + 82)).rgba == kBlue.rgba,
      "the hovered header is not drawn hovered");

  // Left and Right while focused.
  root->ApplyInput(KeyPress(kKeyLeft), &messages);
  TEST_CHECK_(tabs->GetSelectedIndex() == 0 && tab_changes == 2 &&
      page0->IsVisible() && !page1->IsVisible(), "Left did not select tab 0");
  root->ApplyInput(KeyPress(kKeyLeft), &messages);
  TEST_CHECK_(tabs->GetSelectedIndex() == 0 && tab_changes == 2,
      "Left at the first tab moved or reported a change");
  root->ApplyInput(KeyPress(kKeyRight), &messages);
  TEST_CHECK_(tabs->GetSelectedIndex() == 1 && tab_changes == 3,
      "Right did not select tab 1");
  // Not focused: nothing.
  root->MakeCurrentTab(nullptr);
  root->ApplyInput(KeyPress(kKeyLeft), &messages);
  TEST_CHECK_(tabs->GetSelectedIndex() == 1, "an unfocused control moved on Left");

  // A resize reaches the pages and the header row.
  tabs->SetSize(Vec2Si32(300, 150));
  TEST_CHECK_(page0->GetSize() == Vec2Si32(300, 130) &&
      page1->GetSize() == Vec2Si32(300, 130), "after a resize the pages are "
      "%dx%d", (int)page1->GetSize().x, (int)page1->GetSize().y);
  TEST_CHECK(tabs->GetHeaderRect(0, &rect_pos, &rect_size));
  TEST_CHECK_(rect_pos.y == 130, "after a resize the headers are at y = %d",
      (int)rect_pos.y);
  tabs->SetTitle(0, "aaaa");
  TEST_CHECK(tabs->GetHeaderRect(1, &rect_pos, &rect_size));
  TEST_CHECK_(rect_pos.x == 32, "after a longer title header 1 starts at %d",
      (int)rect_pos.x);

  // Removing the selected tab selects its neighbor; programmatic selection is
  // silent.
  std::shared_ptr<Panel> page2 = tabs->AddTab("c");
  tabs->SetSelectedIndex(2);
  TEST_CHECK_(tabs->GetSelectedIndex() == 2 && page2->IsVisible() &&
      !page1->IsVisible() && tab_changes == 3,
      "SetSelectedIndex did not switch pages or reported a change");
  tabs->RemoveTab(2);
  TEST_CHECK_(tabs->GetTabCount() == 2 && tabs->GetSelectedIndex() == 1 &&
      page1->IsVisible(), "removing the last selected tab left tab %d",
      (int)tabs->GetSelectedIndex());
  tabs->RemoveTab(0);
  TEST_CHECK_(tabs->GetSelectedIndex() == 0 && tabs->GetPage(0) == page1 &&
      page1->IsVisible(), "removing a tab before the selected one moved it to "
      "%d", (int)tabs->GetSelectedIndex());
  TEST_CHECK_(page0->GetParent() == nullptr, "a removed page still has a parent");
  tabs->RemoveTab(0);
  TEST_CHECK(tabs->GetTabCount() == 0 && tabs->GetSelectedIndex() == -1);
}

// A tooltip appears after the cursor has rested on a panel that has one, is
// drawn by the root above everything, and goes away with the cursor.
void test_tooltip() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kYellow(255, 255, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  Font font = BlockFont();
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(320, 200));
  auto style = std::make_shared<GuiThemeTooltip>();
  style->frame_ = SolidFrame(kYellow);
  style->font_ = font;
  style->color_ = kWhite;
  style->delay_seconds_ = 0.0;
  auto button = std::make_shared<Button>(1, Vec2Si32(50, 50),
      SolidSprite(40, 20, kRed), SolidSprite(40, 20, kRed),
      SolidSprite(40, 20, kRed), Sound(), Sound(),
      kKeyNone, 1);
  button->SetTooltip("hi");
  root->AddChild(button);
  auto other = std::make_shared<Button>(2, Vec2Si32(150, 50),
      SolidSprite(40, 20, kRed), SolidSprite(40, 20, kRed),
      SolidSprite(40, 20, kRed), Sound(), Sound(),
      kKeyNone, 2);
  root->AddChild(other);
  TEST_CHECK(button->GetTooltip() == "hi" && other->GetTooltip().empty());
  std::deque<GuiMessage> messages;
  const Vec2Si32 whole(0, 0);
  const Vec2Si32 whole_size(320, 200);

  // Without a style the root draws no tooltip at all.
  root->ApplyInput(MouseMoveTo(Vec2Si32(60, 60)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "a root without a tooltip style drew a tooltip");

  root->SetTooltipTheme(style);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  // The frame is the 8x5 text plus 4 of padding and 1 of border on each side:
  // 18x15 at the cursor plus (12, 16).
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 18 * 15 - 2 * 4 * 5,
      "the tooltip frame has %d yellow pixels, expected an 18x15 frame minus "
      "two 4x5 glyphs", (int)CountPixels(whole, whole_size, kYellow));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(60 + 12, 60 + 16)).rgba == kYellow.rgba,
      "the tooltip does not start at the cursor plus (12, 16)");
  TEST_CHECK_(CountPixels(Vec2Si32(72, 76), Vec2Si32(18, 15), kWhite) == 2 * 4 * 5,
      "the tooltip text has %d white pixels, expected 'hi' as two 4x5 glyphs",
      (int)CountPixels(Vec2Si32(72, 76), Vec2Si32(18, 15), kWhite));
  TEST_CHECK_(BackbufferPixel(Vec2Si32(55, 55)).rgba == kRed.rgba,
      "the button under the cursor is no longer drawn");

  // Off the button, the tooltip is gone; on a panel without one, nothing.
  root->ApplyInput(MouseMoveTo(Vec2Si32(250, 150)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "the tooltip stayed after the cursor left");
  root->ApplyInput(MouseMoveTo(Vec2Si32(160, 60)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "a button without a tooltip showed one");

  // The delay is honored: with a long delay nothing shows right away.
  style->delay_seconds_ = 1000.0;
  root->ApplyInput(MouseMoveTo(Vec2Si32(60, 60)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "the tooltip showed before its delay");
  style->delay_seconds_ = 0.0;
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) > 0,
      "the tooltip did not show once the delay was over");

  // A key press hides it until the cursor moves again; a move brings it back.
  root->ApplyInput(KeyPress(kKeyA), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "a key press did not hide the tooltip");
  root->ApplyInput(MouseMoveTo(Vec2Si32(61, 60)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) > 0,
      "the tooltip did not come back after a move");

  // The tooltip follows the cursor and stays inside the root near its edge.
  button->SetPos(Vec2Si32(270, 170));
  root->ApplyInput(MouseMoveTo(Vec2Si32(300, 185)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 18 * 15 - 2 * 4 * 5,
      "near the corner the tooltip has %d yellow pixels, so part of it is off "
      "screen", (int)CountPixels(whole, whole_size, kYellow));
  TEST_CHECK_(CountPixels(Vec2Si32(300, 185), Vec2Si32(20, 15), kYellow) == 0,
      "the tooltip was drawn over the cursor instead of beside it");

  // A child without a tooltip inside a panel with one shows the parent's.
  auto group = std::make_shared<Panel>(3, Vec2Si32(10, 100), Vec2Si32(100, 50));
  group->SetTooltip("g");
  auto inner = std::make_shared<Button>(4, Vec2Si32(10, 10),
      SolidSprite(30, 10, kRed), SolidSprite(30, 10, kRed),
      SolidSprite(30, 10, kRed), Sound(), Sound(),
      kKeyNone, 3);
  group->AddChild(inner);
  root->AddChild(group);
  root->ApplyInput(MouseMoveTo(Vec2Si32(25, 115)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 14 * 15 - 4 * 5,
      "the parent's one-letter tooltip has %d yellow pixels, expected a 14x15 "
      "frame minus one glyph", (int)CountPixels(whole, whole_size, kYellow));
  // A hidden owner shows nothing.
  group->SetVisible(false);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == 0,
      "a hidden panel still shows its tooltip");
}

// A root that is a small window in a corner of the screen with a tooltip
// wider than the window: the tooltip is kept on the screen, not in the root,
// so it hangs out of the window to the left rather than off the screen to the
// right, and it lies over the world beside the window.
void test_tooltip_stays_on_screen() {
  ResizeScreen(320, 200);
  const Rgba kBlack(0, 0, 0, 255);
  const Rgba kWhite(255, 255, 255, 255);
  const Rgba kYellow(255, 255, 0, 255);
  const Rgba kRed(255, 0, 0, 255);
  Font font = BlockFont();
  auto style = std::make_shared<GuiThemeTooltip>();
  style->frame_ = SolidFrame(kYellow);
  style->font_ = font;
  style->color_ = kWhite;
  style->delay_seconds_ = 0.0;
  // The window is the top-right 60x60 of the screen.
  auto root = std::make_shared<Panel>(0, Vec2Si32(260, 140), Vec2Si32(60, 60));
  root->SetTooltipTheme(style);
  auto button = std::make_shared<Button>(1, Vec2Si32(10, 10),
      SolidSprite(40, 20, kRed), SolidSprite(40, 20, kRed),
      SolidSprite(40, 20, kRed), Sound(), Sound(),
      kKeyNone, 1);
  // 17 glyphs of 4 pixels are 68 pixels of text, 78 with the padding and the
  // border: wider than the 60 pixel window.
  button->SetTooltip("a tooltip of text");
  root->AddChild(button);
  std::deque<GuiMessage> messages;
  const Vec2Si32 whole(0, 0);
  const Vec2Si32 whole_size(320, 200);
  // The frame less the 14 letters; the 3 spaces are blank and leave it yellow.
  const Si32 frame_pixels = 78 * 15 - 14 * 4 * 5;

  root->ApplyInput(MouseMoveTo(Vec2Si32(280, 160)), &messages);
  GetEngine()->GetBackbuffer().Clear(kBlack);
  root->Draw(Vec2Si32(0, 0));
  TEST_CHECK_(CountPixels(whole, whole_size, kYellow) == frame_pixels,
      "the tooltip has %d yellow pixels on the screen, expected %d: part of "
      "it is off the screen", (int)CountPixels(whole, whole_size, kYellow),
      (int)frame_pixels);
  TEST_CHECK_(BackbufferPixel(Vec2Si32(319, 185)).rgba == kYellow.rgba,
      "the tooltip does not reach the right edge of the screen it was pushed "
      "back to");
  // Pushed back to x = 320 - 78 = 242, the frame runs from 242 to 319 and
  // from 176 to 190; row 177 is padding, so it is yellow all along.
  TEST_CHECK_(BackbufferPixel(Vec2Si32(250, 177)).rgba == kYellow.rgba,
      "the tooltip does not hang out of the window to the left");
  TEST_CHECK_(BackbufferPixel(Vec2Si32(280, 165)).rgba == kRed.rgba,
      "the tooltip covers the button under the cursor");
}

void PressAndRelease(Panel *root, KeyCode key,
    std::deque<GuiMessage> *messages, bool ctrl) {
  InputMessage down = KeyPress(key);
  down.keyboard.state[kKeyControl] = ctrl ? 1 : 0;
  InputMessage up = KeyRelease(key);
  up.keyboard.state[kKeyControl] = ctrl ? 1 : 0;
  root->ApplyInput(down, messages);
  root->ApplyInput(up, messages);
}

// A Save-dialog-like tree: a focused editbox next to Save (S) and Cancel (C).
// Typing those letters, or holding Control for a shortcut, must not fire the
// buttons, and moving the mouse over a button must not steal the keyboard.
void test_focused_editbox_keeps_button_hotkeys_quiet() {
  Font font = BlockFont();
  Sprite face = SolidSprite(120, 16, Rgba(32, 32, 32, 255));
  Sprite btn_face = SolidSprite(40, 20, Rgba(255, 0, 0, 255));
  auto root = std::make_shared<Panel>(0, Vec2Si32(0, 0), Vec2Si32(400, 200));
  auto box = std::make_shared<Editbox>(5, Vec2Si32(10, 80), 1, face, face,
      font, kTextOriginBottom, Rgba(255, 255, 255), std::string());
  auto save = std::make_shared<Button>(1, Vec2Si32(10, 10),
      btn_face, btn_face, btn_face, Sound(), Sound(), kKeyS, 10);
  auto cancel = std::make_shared<Button>(2, Vec2Si32(60, 10),
      btn_face, btn_face, btn_face, Sound(), Sound(), kKeyC, 11);
  root->AddChild(save);
  root->AddChild(cancel);
  root->AddChild(box);

  Si32 save_clicks = 0;
  Si32 cancel_clicks = 0;
  save->OnButtonClick = [&save_clicks]() { ++save_clicks; };
  cancel->OnButtonClick = [&cancel_clicks]() { ++cancel_clicks; };

  box->SetCurrentTab(true);
  TEST_CHECK(box->IsFocused());
  TEST_CHECK(root->IsKeyboardCaptured());

  std::deque<GuiMessage> messages;

  // Without touching the mouse, S and C are text, not the buttons.
  root->ApplyInput(TypedLetter('s', kKeyS), &messages);
  root->ApplyInput(KeyRelease(kKeyS), &messages);
  root->ApplyInput(TypedLetter('c', kKeyC), &messages);
  root->ApplyInput(KeyRelease(kKeyC), &messages);
  TEST_CHECK_(box->GetText() == "sc",
      "S and C did not reach the focused field: '%s'", box->GetText().c_str());
  TEST_CHECK_(save_clicks == 0 && cancel_clicks == 0,
      "a letter hotkey clicked Save %d times and Cancel %d times while typing",
      (int)save_clicks, (int)cancel_clicks);

  // Ctrl+S is not a letter and the box does not consume it as text; the Save
  // hotkey still must not fire while the field holds the keyboard.
  Si32 save_before = save_clicks;
  PressAndRelease(root.get(), kKeyS, &messages, true);
  TEST_CHECK_(save_clicks == save_before,
      "Ctrl+S clicked Save while the field held the keyboard");
  TEST_CHECK_(box->GetText() == "sc",
      "Ctrl+S changed the text: '%s'", box->GetText().c_str());

  // Ctrl+A selects all; Ctrl+C copies and does not press Cancel.
  PressAndRelease(root.get(), kKeyA, &messages, true);
  TEST_CHECK_(box->GetSelectionBegin() == 0 && box->GetSelectionEnd() == 2,
      "Ctrl+A selected [%d, %d), expected [0, 2)",
      (int)box->GetSelectionBegin(), (int)box->GetSelectionEnd());
  Si32 cancel_before = cancel_clicks;
  PressAndRelease(root.get(), kKeyC, &messages, true);
  TEST_CHECK_(cancel_clicks == cancel_before,
      "Ctrl+C clicked Cancel %d times instead of copying",
      (int)cancel_clicks);

  // Hovering Save must not take the keyboard away from the field.
  const Vec2Si32 on_save(10 + 20, 10 + 10);
  root->ApplyInput(MouseMoveTo(on_save), &messages);
  TEST_CHECK_(box->IsFocused(),
      "a mouse move over Save stole focus from the field");
  TEST_CHECK_(root->IsKeyboardCaptured(),
      "a mouse move over Save released the keyboard");
  save_before = save_clicks;
  root->ApplyInput(TypedLetter('t', kKeyT), &messages);
  root->ApplyInput(KeyRelease(kKeyT), &messages);
  TEST_CHECK_(box->GetText() == "t",
      "after hovering Save, T did not replace the selected text: '%s'",
      box->GetText().c_str());
  TEST_CHECK_(save_clicks == save_before,
      "after hovering Save, a letter clicked the button");

  PressAndRelease(root.get(), kKeyA, &messages, true);
  TEST_CHECK_(box->GetSelectionBegin() == 0 &&
      box->GetSelectionEnd() == (Si32)box->GetText().size(),
      "after hovering Save, Ctrl+A selected [%d, %d)",
      (int)box->GetSelectionBegin(), (int)box->GetSelectionEnd());
  cancel_before = cancel_clicks;
  PressAndRelease(root.get(), kKeyC, &messages, true);
  TEST_CHECK_(cancel_clicks == cancel_before,
      "after hovering Save, Ctrl+C clicked Cancel");

  // A real click on Save does take the focus, and with the field unfocused
  // the S hotkey still activates the button.
  save_before = save_clicks;
  Click(root.get(), on_save, &messages);
  TEST_CHECK_(save_clicks == save_before + 1, "a click on Save did not click it");
  TEST_CHECK_(!box->IsFocused(),
      "a click on Save left the field focused");

  box->SetCurrentTab(false);
  save->SetCurrentTab(false);
  messages.clear();
  const Si32 clicks_before_hotkey = save_clicks;
  PressAndRelease(root.get(), kKeyS, &messages, false);
  TEST_CHECK_(save_clicks == clicks_before_hotkey + 1,
      "S did not click Save when no field held the keyboard");
}
