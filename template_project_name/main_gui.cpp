// A screen with a panel of widgets on it and a "world" behind the panel.
//
// Every widget of engine/gui.h is here, each doing the job it exists for, in a
// small dot-drawing toy: a click on the world puts a dot down, and the panel
// says what the dot looks like and keeps the list of dots placed.
//
//   TabControl   two pages, "Dot" and "Scene", so the panel stays short
//   RadioButton  the shape of the dot: one of three
//   Slider       the radius (horizontal) and the brightness (vertical)
//   Dropdown     the color, a choice from a closed list
//   Image        a preview of the dot, a sprite redrawn on every change
//   Editbox      a caption written under the dot; Enter applies it too
//   Checkbox     the grid on or off
//   Scrollbar    the grid step, a number with buttons at both ends
//   ListBox      the dots placed, with its own scrollbar; a row selects a dot
//   Button       remove the selected dot, clear them all
//   Progressbar  how full the scene is, out of a fixed number of dots
//   Text         the status line
//
// The widgets come from GuiFactory with the theme in data/gui_theme.xml, the
// input goes to them through ApplyInput, and every reaction is a named function
// hung on the widget's callback. The one thing a program with a GUI has to
// decide for itself is which clicks are the interface's and which are the
// world's: Panel::IsInside answers it, and a click that lands outside the panel
// is a dot here.
//
// Escape quits.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "engine/easy.h"

using namespace arctic;  // NOLINT

// The world

enum Shape {
  kShapeCircle = 0,
  kShapeSquare,
  kShapeRing,
  kShapeCount
};

// A dot on the screen. Not named Dot: that is the dot product in the engine.
struct Mark {
  Vec2Si32 pos;
  Shape shape = kShapeCircle;
  Si32 radius = 8;
  Rgba color;
  std::string caption;
};

struct NamedColor {
  const char *name;
  Rgba color;
};

const NamedColor kColors[] = {
  {"Amber", Rgba(255, 200, 80)},
  {"Sky", Rgba(90, 180, 255)},
  {"Mint", Rgba(120, 230, 160)},
  {"Rose", Rgba(255, 120, 150)},
  {"Snow", Rgba(240, 240, 250)},
};
const Si32 kColorCount = static_cast<Si32>(sizeof(kColors) / sizeof(kColors[0]));
const Si32 kMaxDots = 50;
const Rgba kBackground(30, 40, 60);

std::vector<Mark> g_dots;
Font g_font;

// The panel

std::shared_ptr<GuiTheme> g_theme;
std::shared_ptr<Panel> g_gui;
std::shared_ptr<Text> g_status;
std::shared_ptr<TabControl> g_tabs;
std::shared_ptr<RadioButton> g_shape[kShapeCount];
std::shared_ptr<Slider> g_radius;
std::shared_ptr<Slider> g_brightness;
std::shared_ptr<Dropdown> g_color;
std::shared_ptr<Image> g_preview;
std::shared_ptr<Editbox> g_caption;
std::shared_ptr<Checkbox> g_grid;
std::shared_ptr<Scrollbar> g_grid_step;
std::shared_ptr<ListBox> g_list;
std::shared_ptr<Progressbar> g_fill;

// What the next dot looks like, read straight from the widgets.
Mark CurrentDot() {
  Mark dot;
  for (Si32 idx = 0; idx < kShapeCount; ++idx) {
    if (g_shape[idx]->IsChecked()) {
      dot.shape = static_cast<Shape>(idx);
    }
  }
  dot.radius = g_radius->GetValue();
  const Si32 color_idx = std::max(0, g_color->GetSelectedIndex());
  const Rgba base = kColors[color_idx].color;
  const Si32 brightness = g_brightness->GetValue();
  dot.color = Rgba(static_cast<Ui8>(base.r * brightness / 100),
      static_cast<Ui8>(base.g * brightness / 100),
      static_cast<Ui8>(base.b * brightness / 100));
  dot.caption = g_caption->GetText();
  return dot;
}

// Draws a dot into a sprite; the preview and the world share it, so the
// preview is always what the next click will put down.
void DrawDot(Sprite to, const Mark &dot, Vec2Si32 at) {
  switch (dot.shape) {
    case kShapeCircle:
      DrawCircle(to, at, dot.radius, dot.color);
      break;
    case kShapeSquare:
      DrawRectangle(to, at - Vec2Si32(dot.radius, dot.radius),
          at + Vec2Si32(dot.radius, dot.radius), dot.color);
      break;
    case kShapeRing:
      DrawCircle(to, at, dot.radius, dot.color);
      DrawCircle(to, at, dot.radius * 2 / 3, kBackground);
      break;
    default:
      break;
  }
}

std::string DescribeDot(const Mark &dot) {
  static const char *kShapeNames[kShapeCount] = {"circle", "square", "ring"};
  char text[128];
  snprintf(text, sizeof(text), "%s r=%d at %d,%d", kShapeNames[dot.shape],
      dot.radius, dot.pos.x, dot.pos.y);
  return text;
}

// Reactions. Each is a plain function set as the widget's callback; the
// widgets take std::function, so a member function bound to an object would
// do as well, but a named function is the easiest to find and to test.

// The preview follows the shape, the radius, the brightness and the color.
void OnLookChange() {
  Sprite preview;
  preview.Create(100, 100);
  preview.Clear(Rgba(0, 0, 0, 0));
  DrawDot(preview, CurrentDot(), Vec2Si32(50, 50));
  g_preview->SetSprite(preview);
}

void UpdateSceneWidgets() {
  g_fill->SetCurrentValue(static_cast<float>(g_dots.size()));
  char text[64];
  snprintf(text, sizeof(text), "%d of %d dots", static_cast<int>(g_dots.size()),
      static_cast<int>(kMaxDots));
  g_status->SetText(text);
}

void AddDot(Vec2Si32 at) {
  if (static_cast<Si32>(g_dots.size()) >= kMaxDots) {
    return;
  }
  Mark dot = CurrentDot();
  dot.pos = at;
  g_dots.push_back(dot);
  g_list->AddItem(DescribeDot(dot));
  g_list->SetSelectedIndex(static_cast<Si32>(g_dots.size()) - 1);
  UpdateSceneWidgets();
}

void OnRemoveSelected() {
  const Si32 idx = g_list->GetSelectedIndex();
  if (idx < 0 || idx >= static_cast<Si32>(g_dots.size())) {
    return;
  }
  g_dots.erase(g_dots.begin() + idx);
  g_list->RemoveItem(idx);
  UpdateSceneWidgets();
}

void OnClearAll() {
  g_dots.clear();
  g_list->ClearItems();
  UpdateSceneWidgets();
}

// Enter in the caption box is as good as a click on the world would be:
// the caption is part of the look, so the preview shows it too.
void OnCaptionDone() {
  OnLookChange();
}

// Building

// The "Dot" page: what the next dot looks like.
void BuildDotPage(GuiFactory *gf, std::shared_ptr<Panel> page) {
  std::shared_ptr<Text> shape_label = gf->MakeText();
  shape_label->SetText("Shape");
  shape_label->SetPos(Vec2Si32(10, 435));
  shape_label->SetSize(Vec2Si32(200, 24));
  page->AddChild(shape_label);

  // Radio buttons of one group: checking one clears the others.
  static const char *kShapeTitles[kShapeCount] = {" Circle", " Square", " Ring"};
  for (Si32 idx = 0; idx < kShapeCount; ++idx) {
    g_shape[idx] = gf->MakeRadioButton(0);
    g_shape[idx]->SetText(kShapeTitles[idx]);
    g_shape[idx]->SetPos(Vec2Si32(10 + idx * 130, 395));
    g_shape[idx]->OnSelect = OnLookChange;
    page->AddChild(g_shape[idx]);
  }
  g_shape[kShapeCircle]->SetChecked(true);

  std::shared_ptr<Text> radius_label = gf->MakeText();
  radius_label->SetText("Radius");
  radius_label->SetPos(Vec2Si32(10, 355));
  radius_label->SetSize(Vec2Si32(200, 24));
  page->AddChild(radius_label);

  // A slider is a value on a track: no buttons, the thumb is the value.
  g_radius = gf->MakeHorizontalSlider();
  g_radius->SetPos(Vec2Si32(10, 325));
  g_radius->SetWidth(300);
  g_radius->SetRange(2, 40);
  g_radius->SetStep(1);
  g_radius->SetValue(8);
  g_radius->SetTooltip("Dot radius, 2 to 40 pixels");
  g_radius->OnSliderChange = OnLookChange;
  page->AddChild(g_radius);

  g_brightness = gf->MakeVerticalSlider();
  g_brightness->SetPos(Vec2Si32(340, 100));
  g_brightness->SetSize(Vec2Si32(24, 255));
  g_brightness->SetRange(20, 100);
  g_brightness->SetStep(5);
  g_brightness->SetValue(100);
  g_brightness->SetTooltip("Brightness, percent");
  g_brightness->OnSliderChange = OnLookChange;
  page->AddChild(g_brightness);

  std::shared_ptr<Text> color_label = gf->MakeText();
  color_label->SetText("Color");
  color_label->SetPos(Vec2Si32(10, 285));
  color_label->SetSize(Vec2Si32(200, 24));
  page->AddChild(color_label);

  // A dropdown opens its list above everything else in the panel and
  // closes it on a choice or on a click elsewhere.
  g_color = gf->MakeDropdown();
  g_color->SetPos(Vec2Si32(10, 248));
  g_color->SetWidth(200);
  for (Si32 idx = 0; idx < kColorCount; ++idx) {
    g_color->AddItem(kColors[idx].name);
  }
  g_color->SetSelectedIndex(0);
  g_color->OnChange = OnLookChange;
  page->AddChild(g_color);

  // An image panel shows a sprite; kScaleCenter keeps the sprite's own
  // pixels and centers it in the panel.
  Sprite empty;
  empty.Create(100, 100);
  g_preview = gf->MakeImage(empty, Image::kScaleCenter);
  g_preview->SetPos(Vec2Si32(230, 100));
  g_preview->SetSize(Vec2Si32(100, 100));
  g_preview->SetTooltip("The next dot");
  page->AddChild(g_preview);

  std::shared_ptr<Text> caption_label = gf->MakeText();
  caption_label->SetText("Caption");
  caption_label->SetPos(Vec2Si32(10, 208));
  caption_label->SetSize(Vec2Si32(200, 24));
  page->AddChild(caption_label);

  g_caption = gf->MakeEditbox();
  g_caption->SetPos(Vec2Si32(10, 155));
  g_caption->SetWidth(200);
  g_caption->SetTooltip("Written under every new dot. Enter applies it.");
  g_caption->OnEditDone = OnCaptionDone;
  page->AddChild(g_caption);
}

// The "Scene" page: the grid and the dots already placed.
void BuildScenePage(GuiFactory *gf, std::shared_ptr<Panel> page) {
  g_grid = gf->MakeCheckbox();
  g_grid->SetText(" Show grid");
  g_grid->SetPos(Vec2Si32(10, 435));
  g_grid->SetTooltip("A grid behind the dots, drawn in the backbuffer.");
  page->AddChild(g_grid);

  std::shared_ptr<Text> step_label = gf->MakeText();
  step_label->SetText("Grid step");
  step_label->SetPos(Vec2Si32(10, 400));
  step_label->SetSize(Vec2Si32(200, 24));
  page->AddChild(step_label);

  // A scrollbar is a value with a button at each end and a thumb between;
  // the wheel over it moves the value by the step as well.
  g_grid_step = gf->MakeHorizontalScrollbar();
  g_grid_step->SetPos(Vec2Si32(10, 368));
  g_grid_step->SetWidth(360);
  g_grid_step->SetMinValue(10);
  g_grid_step->SetMaxValue(100);
  g_grid_step->SetStep(10);
  g_grid_step->SetValue(40);
  g_grid_step->SetTooltip("Grid step, 10 to 100 pixels");
  page->AddChild(g_grid_step);

  // A list box scrolls by itself once the rows outgrow it; a selected row
  // is the dot the Remove button acts on and the world highlights.
  g_list = gf->MakeListBox();
  g_list->SetPos(Vec2Si32(10, 190));
  g_list->SetSize(Vec2Si32(360, 168));
  g_list->SetTooltip("The dots, newest at the bottom");
  page->AddChild(g_list);

  // A button is as wide as it is told and its text is not shrunk to fit, so
  // the width comes from the longest text it may show.
  std::shared_ptr<Button> remove = gf->MakeButton();
  remove->SetText("Remove selected");
  remove->SetPos(Vec2Si32(10, 128));
  remove->SetWidth(360);
  remove->OnButtonClick = OnRemoveSelected;
  page->AddChild(remove);

  std::shared_ptr<Button> clear = gf->MakeButton();
  clear->SetText("Clear all");
  clear->SetPos(Vec2Si32(10, 66));
  clear->SetWidth(360);
  clear->OnButtonClick = OnClearAll;
  page->AddChild(clear);

  // A progress bar is a read-only value out of a total.
  g_fill = gf->MakeProgressbar();
  g_fill->SetPos(Vec2Si32(10, 4));
  g_fill->SetWidth(360);
  g_fill->SetTotalValue(static_cast<float>(kMaxDots));
  g_fill->SetCurrentValue(0.0f);
  g_fill->SetTooltip("How full the scene is");
  page->AddChild(g_fill);
}

void BuildGui() {
  GuiFactory gf;
  gf.theme_ = g_theme;

  const Vec2Si32 panel_size(400, 600);
  g_gui = gf.MakePanel();
  g_gui->SetSize(panel_size);
  g_gui->SetPos(ScreenSize() - panel_size - Vec2Si32(16, 16));
  // The panel is a window, not a group of widgets floating over the world:
  // a click on its empty part, on a label or on the progress bar is a click
  // on the window and must not reach the world behind it. A bare panel lets
  // such clicks through; a clickable one takes them, and IsInside says so.
  g_gui->SetClickable(true);

  // Everything below is placed relative to its parent, y upward from the
  // parent's bottom edge, like everything else on the screen.
  g_status = gf.MakeText();
  g_status->SetPos(Vec2Si32(10, 550));
  g_status->SetSize(Vec2Si32(380, 24));
  g_gui->AddChild(g_status);

  // A tab control owns one page panel per tab and shows the selected one;
  // AddTab returns the page, and the widgets of the page are its children.
  g_tabs = gf.MakeTabControl();
  g_tabs->SetPos(Vec2Si32(10, 10));
  g_tabs->SetSize(Vec2Si32(380, 525));
  g_gui->AddChild(g_tabs);
  BuildDotPage(&gf, g_tabs->AddTab("Dot"));
  BuildScenePage(&gf, g_tabs->AddTab("Scene"));

  OnLookChange();
  UpdateSceneWidgets();
}

// The frame

void ProcessInput() {
  // The whole queue goes to the GUI, which handles the widgets, the focus and
  // the tooltips. The GUI takes what it needs from a message and leaves the
  // rest alone, so a click that lands on the world is still a click for us;
  // IsInside is the test that tells the two apart, on the same backbuffer
  // pixels the panel is positioned in.
  for (Si32 i = 0; i < InputMessageCount(); ++i) {
    const InputMessage &message = GetInputMessage(i);
    g_gui->ApplyInput(message, nullptr);
    if (message.kind == InputMessage::kMouse
        && message.keyboard.key == kKeyMouseLeft
        && message.keyboard.key_state == 1
        && !g_gui->IsInside(message.mouse.backbuffer_pos)) {
      AddDot(message.mouse.backbuffer_pos);
    }
  }
}

void DrawWorld() {
  Clear(kBackground);
  if (g_grid->IsChecked()) {
    const Si32 step = g_grid_step->GetValue();
    const Rgba line(50, 60, 90);
    for (Si32 x = 0; x < ScreenSize().x; x += step) {
      DrawLine(Vec2Si32(x, 0), Vec2Si32(x, ScreenSize().y), line);
    }
    for (Si32 y = 0; y < ScreenSize().y; y += step) {
      DrawLine(Vec2Si32(0, y), Vec2Si32(ScreenSize().x, y), line);
    }
  }
  Sprite backbuffer = GetEngine()->GetBackbuffer();
  const Si32 selected = g_list->GetSelectedIndex();
  for (size_t idx = 0; idx < g_dots.size(); ++idx) {
    const Mark &dot = g_dots[idx];
    if (static_cast<Si32>(idx) == selected) {
      DrawCircle(dot.pos, dot.radius + 4, Rgba(255, 255, 255));
      DrawCircle(dot.pos, dot.radius + 2, kBackground);
    }
    DrawDot(backbuffer, dot, dot.pos);
    if (!dot.caption.empty()) {
      g_font.Draw(dot.caption.c_str(), dot.pos.x, dot.pos.y - dot.radius - 4,
          kTextOriginTop, kTextAlignmentCenter);
    }
  }
  g_font.Draw("Click outside the panel to put a dot. Esc quits.",
      16, 16, kTextOriginBottom);
}

void EasyMain() {
  ResizeScreen(1280, 720);
  g_font.Load("data/arctic_one_bmf.fnt");
  g_theme = std::make_shared<GuiTheme>();
  g_theme->Load("data/gui_theme.xml");
  BuildGui();

  while (!IsKeyDownward(kKeyEscape)) {
    ProcessInput();
    DrawWorld();
    // The GUI is drawn last: it is on top, and its tooltips and the open
    // dropdown list above all of it.
    g_gui->Draw(Vec2Si32(0, 0));
    ShowFrame();
  }
}
