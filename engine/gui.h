// The MIT License (MIT)
//
// Copyright (c) 2018 - 2024 Huldra
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

#ifndef ENGINE_GUI_H_
#define ENGINE_GUI_H_

#define _USE_MATH_DEFINES
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/arctic_input.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/font.h"
#include "engine/decorated_frame.h"

namespace arctic {

/// @addtogroup global_gui
/// @{

/// @brief Enumeration of GUI message types.
enum GuiMessageKind {
  kGuiButtonClick,
  kGuiButtonDown,
  kGuiScrollChange,
  kGuiPanelLeftDown,
};

/// @brief Enumeration of text selection modes.
enum TextSelectionMode {
  kTextSelectionModeInvert,
  kTextSelectionModeSwapColors
};

/// @brief Enumeration of anchor types for GUI elements.
enum AnchorKind : Ui8 {
  kAnchorNone = 0,
  kAnchorBottom = 1,
  kAnchorTop = 2,
  kAnchorLeft = 4,
  kAnchorRight = 8
};

/// @brief Bitwise OR operator for AnchorKind.
/// @param a First AnchorKind value.
/// @param b Second AnchorKind value.
/// @return Result of bitwise OR operation.
inline AnchorKind operator|(AnchorKind a, AnchorKind b) {
    return static_cast<AnchorKind>(static_cast<Ui8>(a) | static_cast<Ui8>(b));
}

/// @brief Enumeration of docking types for GUI elements.
enum DockKind : Ui8 {
  kDockNone = 0,
  kDockBottom = 1,
  kDockTop = 2,
  kDockLeft = 4,
  kDockRight = 8
};

/// @brief Bitwise OR operator for DockKind.
/// @param a First DockKind value.
/// @param b Second DockKind value.
/// @return Result of bitwise OR operation.
inline DockKind operator|(DockKind a, DockKind b) {
    return static_cast<DockKind>(static_cast<Ui8>(a) | static_cast<Ui8>(b));
}

class Panel;

/// @brief Empty function that does nothing.
void inline DoNothing() {
  return;
}

/// @brief Class representing text theme for GUI elements.
class GuiThemeText {
 public:
  Font font_;
  std::vector<Rgba> palete_;
  std::vector<Rgba> disabled_palete_;
  TextAlignment alignment_;
  TextOrigin origin_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;
};

/// @brief Class representing button theme for GUI elements.
class GuiThemeButton {
 public:
  DecoratedFrame normal_;
  DecoratedFrame down_;
  DecoratedFrame hovered_;
  DecoratedFrame disabled_;
  Sound down_sound_;
  Sound up_sound_;

  std::shared_ptr<GuiThemeText> text_;
};

/// @brief Class representing scrollbar theme for GUI elements.
class GuiThemeScrollbar {
 public:
  DecoratedFrame normal_background_;
  DecoratedFrame focused_background_;
  DecoratedFrame disabled_background_;
  Sprite normal_button_dec_;
  Sprite focused_button_dec_;
  Sprite down_button_dec_;
  Sprite disabled_button_dec_;
  Sprite normal_button_inc_;
  Sprite focused_button_inc_;
  Sprite down_button_inc_;
  Sprite disabled_button_inc_;
  Sprite normal_button_cur_;
  Sprite focused_button_cur_;
  Sprite down_button_cur_;
  Sprite disabled_button_cur_;
  bool is_horizontal_;
};

/// @brief Class representing the overall GUI theme.
class GuiTheme {
 public:
  DecoratedFrame panel_background_;

  std::shared_ptr<GuiThemeButton> button_;

  std::shared_ptr<GuiThemeText> text_;

  DecoratedFrame progressbar_incomplete_;
  DecoratedFrame progressbar_complete_;

  std::shared_ptr<GuiThemeText> editbox_text_;

  DecoratedFrame editbox_normal_;
  DecoratedFrame editbox_focused_;

  std::shared_ptr<GuiThemeScrollbar> h_scrollbar_;
  std::shared_ptr<GuiThemeScrollbar> v_scrollbar_;

  Sprite checkbox_clear_normal_;
  Sprite checkbox_checked_normal_;
  Sprite checkbox_clear_down_;
  Sprite checkbox_checked_down_;
  Sprite checkbox_clear_hovered_;
  Sprite checkbox_checked_hovered_;
  Sprite checkbox_clear_disabled_;
  Sprite checkbox_checked_disabled_;
  Sound checkbox_down_sound_;
  Sound checkbox_up_sound_;

  /// @brief Loads the GUI theme from an XML file.
  /// @param xml_file_path Path to the XML file containing theme data.
  void Load(const char *xml_file_path);
};

/// @brief Class representing a GUI message.
class GuiMessage {
 public:
  std::shared_ptr<Panel> panel;
  GuiMessageKind kind;

  /// @brief Constructor for GuiMessage.
  /// @param panel Panel associated with the message.
  /// @param kind Kind of the GUI message.
  GuiMessage(std::shared_ptr<Panel> panel, GuiMessageKind kind);
};

/// @brief Base class for all GUI panels.
class Panel : public std::enable_shared_from_this<Panel> {
 protected:
  static std::shared_ptr<Panel> invalid_panel_;
  Ui64 tag_;
  Vec2Si32 pos_;
  Vec2Si32 size_;
  Ui32 tab_order_;
  bool is_current_tab_;
  Sprite background_;
  bool do_regenerate_background_;
  std::deque<std::shared_ptr<Panel>> children_;
  Panel *parent_ = nullptr;
  Si32 anchor_bottom_d_ = 0;
  Si32 anchor_top_d_ = 0;
  Si32 anchor_left_d_ = 0;
  Si32 anchor_right_d_ = 0;
  bool is_clickable_;
  bool is_visible_;
  AnchorKind anchor_ = kAnchorNone;
  DockKind dock_ = kDockNone;
  std::shared_ptr<GuiTheme> theme_;

 public:
  /// @brief Constructor for Panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param background Background sprite of the panel.
  /// @param is_clickable Flag indicating if panel is clickable.
  Panel(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order = 0,
    Sprite background = Sprite(), bool is_clickable = false);

  /// @brief Constructor for Panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme Theme for the panel.
  Panel(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Returns an invalid panel instance.
  /// @return Shared pointer to an invalid panel.
  static std::shared_ptr<Panel> Invalid() {
    return invalid_panel_;
  }

  /// @brief Gets the size of the panel.
  /// @return Size of the panel.
  Vec2Si32 GetSize() const;

  /// @brief Sets the size of the panel.
  /// @param size New size of the panel.
  void SetSize(Vec2Si32 size);

  /// @brief Sets the size of the panel.
  /// @param width New width of the panel.
  /// @param height New height of the panel.
  void SetSize(Si32 width, Si32 height);

  /// @brief Handles parent size changes.
  /// @param prev_size Previous size of the parent.
  /// @param cur_size Current size of the parent.
  void ParentSizeChanged(Vec2Si32 prev_size, Vec2Si32 cur_size);

  /// @brief Gets the tab order of the panel
  /// @return The tab order value of the panel, or 0 if the panel does not participate in the tab order.
  Ui32 GetTabOrder() const;

  /// @brief Sets the tab order of the panel
  /// @param [in] tab_order Tab order value.
  /// The panel does not participate in tab order if tab_order is 0.
  void SetTabOrder(Ui32 tab_order);

  /// @brief Gets the tag of the panel.
  /// @return Tag of the panel.
  Ui64 GetTag() const;

  /// @brief Sets the tag of the panel.
  /// @param tag New tag for the panel.
  void SetTag(Ui64 tag);

  /// @brief Gets the position of the panel.
  /// @return Position of the panel.
  Vec2Si32 GetPos() const;

  /// @brief Sets the position of the panel.
  /// @param pos New position for the panel.
  void SetPos(Vec2Si32 pos);

  /// @brief Sets the position of the panel.
  /// @param x New x-coordinate for the panel.
  /// @param y New y-coordinate for the panel.
  void SetPos(Si32 x, Si32 y);

  /// @brief Sets the width of the panel.
  /// @param width New width for the panel.
  void SetWidth(Si32 width);

  /// @brief Sets the height of the panel.
  /// @param height New height for the panel.
  void SetHeight(Si32 height);

  /// @brief Sets the background sprite of the panel.
  /// @param background New background sprite for the panel.
  void SetBackground(const Sprite &background);

  /// @brief Virtual destructor for Panel.
  virtual ~Panel();

  /// @brief Draws the panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  virtual void Draw(Vec2Si32 parent_absolute_pos);

  /// @brief Applies input to the panel.
  /// @param message Input message to apply.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @return True if input was applied, false otherwise.
  virtual bool ApplyInput(const InputMessage &message,
      std::deque<GuiMessage> *out_gui_messages);

  /// @brief Applies input to the panel and its children.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  virtual void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab);

  /// @brief Makes the specified panel the current tab.
  /// @param target Target panel to make the current tab.
  void MakeCurrentTab(const Panel *target);

  /// @brief Switches the current tab.
  /// @param is_forward True to switch to the next tab, false for the previous tab.
  /// @return True if the tab was switched, false otherwise.
  bool SwitchCurrentTab(bool is_forward);

  /// @brief Finds the neighboring panels in the tab order.
  /// @param current_tab_order Current tab order.
  /// @param in_out_prev Output pointer to the previous panel in tab order.
  /// @param in_out_next Output pointer to the next panel in tab order.
  void FindNeighbors(Ui32 current_tab_order,
      Panel **in_out_prev, Panel **in_out_next);

  /// @brief Finds the current tab panel.
  /// @return Pointer to the current tab panel, or nullptr if not found.
  Panel *FindCurrentTab();

  /// @brief Sets the current tab status of the panel.
  /// @param is_current_tab True if the panel is the current tab, false otherwise.
  virtual void SetCurrentTab(bool is_current_tab);

  /// @brief Adds a child panel to this panel.
  /// @param child Shared pointer to the child panel.
  virtual void AddChild(std::shared_ptr<Panel> child);

  /// @brief Removes a child panel from this panel.
  /// @param child Shared pointer to the child panel.
  virtual void RemoveChild(std::shared_ptr<Panel> child);

  /// @brief Sets the visibility of the panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  virtual void SetVisible(bool is_visible);

  /// @brief Sets the enabled status of the panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  virtual void SetEnabled(bool is_enabled);

  /// @brief Checks if the panel is visible.
  /// @return True if the panel is visible, false otherwise.
  virtual bool IsVisible();

  /// @brief Checks if the panel is mouse transparent at a given position.
  /// @param parent_pos Position of the parent panel.
  /// @param mouse_pos Mouse position relative to the parent panel.
  /// @return True if the panel is mouse transparent at the given position, false otherwise.
  virtual bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos);

  /// @brief Sets the enabled status of a panel with a specific tag.
  /// @param tag Tag of the panel to enable or disable.
  /// @param is_enabled True to enable the panel, false to disable it.
  virtual void SetEnabledByTag(Ui64 tag, bool is_enabled);

  /// @brief Regenerates the sprites of the panel.
  virtual void RegenerateSprites();

  /// @brief Makes this panel a child of another panel.
  /// @param parent Pointer to the parent panel.
  virtual void BecomeChild(Panel *parent);

  /// @brief Sets the anchor of the panel.
  /// @param anchor Anchor kind to set.
  virtual void SetAnchor(AnchorKind anchor);

  /// @brief Sets the dock of the panel.
  /// @param dock Dock kind to set.
  virtual void SetDock(DockKind dock);

  std::function<void(void)> OnPanelLeftDown = DoNothing;
};

/// @brief Class representing a text panel.
class Text : public Panel {
 protected:
  Font font_;
  TextOrigin origin_;
  Rgba color_;
  std::vector<Rgba> palete_;
  std::string text_;
  TextAlignment alignment_;
  Si32 selection_begin_;
  Si32 selection_end_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;
  std::shared_ptr<GuiThemeText> theme_;
  bool is_enabled_ = true;

 public:
  /// @brief Constructor for Text panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param font Font for the text.
  /// @param origin Text origin.
  /// @param color Text color.
  /// @param text Text content.
  /// @param alignment Text alignment.
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, Rgba color, std::string text,
      TextAlignment alignment = kTextAlignmentLeft);

  /// @brief Constructor for Text panel with color palette.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param font Font for the text.
  /// @param origin Text origin.
  /// @param palete Color palette for the text.
  /// @param text Text content.
  /// @param alignment Text alignment.
  Text(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, TextOrigin origin, std::vector<Rgba> palete, std::string text,
      TextAlignment alignment = kTextAlignmentLeft);

  /// @brief Constructor for Text panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme Text theme for the panel.
  Text(Ui64 tag, std::shared_ptr<GuiThemeText> theme);

  /// @brief Sets the text color.
  /// @param rgba New text color.
  void SetColor(Rgba rgba);

  /// @brief Sets the text content.
  /// @param text New text content.
  void SetText(std::string text);

  /// @brief Sets the font for the text.
  /// @param font New font for the text.
  void SetFont(Font font);

  /// @brief Sets the text origin.
  /// @param origin New text origin.
  void SetOrigin(TextOrigin origin);

  /// @brief Sets the text alignment.
  /// @param alignment New text alignment.
  void SetAlignment(TextAlignment alignment);

  /// @brief Draws the text panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Selects a range of text.
  /// @param selection_begin Start index of the selected text.
  /// @param selection_end End index of the selected text.
  void Select(Si32 selection_begin, Si32 selection_end);

  /// @brief Sets the text selection mode.
  /// @param selection_mode Text selection mode.
  /// @param selection_color_1 First color for text selection.
  /// @param selection_color_2 Second color for text selection.
  void SetSelectionMode(
      TextSelectionMode selection_mode = kTextSelectionModeInvert,
      Rgba selection_color_1 = Rgba(0, 0, 0),
      Rgba selection_color_2 = Rgba(255, 255, 255));

  /// @brief Evaluates the size of the text panel.
  /// @return Size of the text panel.
  Vec2Si32 EvaluateSize();

  /// @brief Sets the enabled status of the text panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;
};

/// @brief Class representing a button panel.
class Button : public Panel {
 public:
  /// @brief Enumeration of button states.
  enum ButtonState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kDown = 3,
    kDisabled = 4
  };

 protected:
  Sprite normal_;
  Sprite down_;
  Sprite hovered_;
  Sprite disabled_;
  Sound down_sound_;
  Sound up_sound_;
  KeyCode hotkey_;
  ButtonState state_ = kNormal;
  std::shared_ptr<Text> text_;
  std::shared_ptr<GuiThemeButton> theme_;

 public:
  /// @brief Constructor for Button panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param normal Normal state sprite.
  /// @param down Down state sprite.
  /// @param hovered Hovered state sprite.
  /// @param down_sound Sound played when button is pressed.
  /// @param up_sound Sound played when button is released.
  /// @param hotkey Hotkey for the button.
  /// @param tab_order Tab order of the panel.
  /// @param disabled Disabled state sprite.
  Button(Ui64 tag, Vec2Si32 pos,
    Sprite normal,
    Sprite down = Sprite(),
    Sprite hovered = Sprite(),
    Sound down_sound = Sound(),
    Sound up_sound = Sound(),
    KeyCode hotkey = kKeyNone,
    Ui32 tab_order = 0,
    Sprite disabled = Sprite());

  /// @brief Constructor for Button panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme Button theme for the panel.
  Button(Ui64 tag, std::shared_ptr<GuiThemeButton> theme);

  /// @brief Draws the button panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Applies input to the button panel.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief Sets the current tab status of the button panel.
  /// @param is_current_tab True if the panel is the current tab, false otherwise.
  void SetCurrentTab(bool is_current_tab) override;

  /// @brief Sets the visibility of the button panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  void SetVisible(bool is_visible) override;

  /// @brief Sets the enabled status of the button panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Checks if the button panel is visible.
  /// @return True if the panel is visible, false otherwise.
  bool IsVisible() override;

  /// @brief Checks if the button panel is mouse transparent at a given position.
  /// @param parent_pos Position of the parent panel.
  /// @param mouse_pos Mouse position relative to the parent panel.
  /// @return True if the panel is mouse transparent at the given position, false otherwise.
  bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) override;

  /// @brief Sets the text color of the button panel.
  /// @param rgba New text color.
  void SetTextColor(Rgba rgba);

  /// @brief Sets the text content of the button panel.
  /// @param text New text content.
  void SetText(std::string text);

  /// @brief Sets the font for the text of the button panel.
  /// @param font New font for the text.
  void SetFont(Font font);

  /// @brief Regenerates the sprites of the button panel.
  void RegenerateSprites() override;

  /// @brief Sets the hotkey for the button panel.
  /// @param hotkey New hotkey for the button.
  void SetHotkey(KeyCode hotkey);

  std::function<void(void)> OnButtonClick = DoNothing;
  std::function<void(void)> OnButtonDown = DoNothing;
};

/// @brief Class representing a progress bar panel.
class Progressbar: public Panel {
 protected:
  Sprite incomplete_;
  Sprite complete_;
  float total_value_;
  float current_value_;
  std::shared_ptr<Text> text_;
  std::shared_ptr<GuiTheme> theme_;

 public:
  /// @brief Constructor for Progressbar panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param incomplete Incomplete part sprite.
  /// @param complete Complete part sprite.
  /// @param palete Color palette for the text.
  /// @param font Font for the text.
  /// @param total_value Total value of the progress bar.
  /// @param current_value Current value of the progress bar.
  Progressbar(Ui64 tag, Vec2Si32 pos,
    Sprite incomplete, Sprite complete,
    std::vector<Rgba> palete, Font font,
    float total_value = 1.0f, float current_value = 0.0f);

  /// @brief Constructor for Progressbar panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme GUI theme for the progress bar.
  Progressbar(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Draws the progress bar panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Updates the text of the progress bar.
  void UpdateText();

  /// @brief Sets the total value of the progress bar.
  /// @param total_value New total value.
  void SetTotalValue(float total_value);

  /// @brief Sets the current value of the progress bar.
  /// @param current_value New current value.
  void SetCurrentValue(float current_value);

  /// @brief Regenerates the sprites of the progress bar panel.
  void RegenerateSprites() override;
};

/// @brief Class representing an edit box panel.
class Editbox: public Panel {
 protected:
  Font font_;
  TextOrigin origin_;
  Rgba color_;
  std::string text_;
  TextAlignment alignment_;
  Sprite normal_;
  Sprite focused_;
  Si32 cursor_pos_;
  Si32 display_pos_;
  Si32 selection_begin_;
  Si32 selection_end_;
  TextSelectionMode selection_mode_;
  Rgba selection_color_1_;
  Rgba selection_color_2_;
  bool is_digits_;
  std::unordered_set<Ui32> allow_list_;
  std::shared_ptr<GuiTheme> theme_;

 public:
  /// @brief Constructor for Editbox panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param normal Normal state sprite.
  /// @param focused Focused state sprite.
  /// @param font Font for the text.
  /// @param origin Text origin.
  /// @param color Text color.
  /// @param text Text content.
  /// @param alignment Text alignment.
  /// @param is_digits Flag indicating if the edit box only accepts digits.
  /// @param allow_list Set of allowed characters.
  Editbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite normal, Sprite focused,
    Font font, TextOrigin origin, Rgba color, std::string text,
    TextAlignment alignment = kTextAlignmentLeft, bool is_digits = false,
    std::unordered_set<Ui32> allow_list = std::unordered_set<Ui32>());

  /// @brief Constructor for Editbox panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme GUI theme for the edit box.
  Editbox(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Applies input to the edit box panel.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief Sets the text content of the edit box panel.
  /// @param text New text content.
  void SetText(std::string text);

  /// @brief Draws the edit box panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Gets the text content of the edit box panel.
  /// @return Text content of the edit box panel.
  std::string GetText();

  /// @brief Selects all the text in the edit box panel.
  void SelectAll();

  /// @brief Sets the text selection mode of the edit box panel.
  /// @param selection_mode Text selection mode.
  /// @param selection_color_1 First color for text selection.
  /// @param selection_color_2 Second color for text selection.
  void SetSelectionMode(
      TextSelectionMode selection_mode = kTextSelectionModeInvert,
      Rgba selection_color_1 = Rgba(0, 0, 0),
      Rgba selection_color_2 = Rgba(255, 255, 255));

  /// @brief Regenerates the sprites of the edit box panel.
  void RegenerateSprites() override;
};

/// @brief Class representing a scrollbar panel.
class Scrollbar : public Panel {
public:
 /// @brief Enumeration of scrollbar states.
 enum ScrollState {
   kHidden = 0,
   kNormal = 1,
   kHovered = 2,
   kDecDown = 3,
   kIncDown = 4,
   kMiddleDragged = 5,
   kDecFast = 6,
   kIncFast = 7
 };

 /// @brief Enumeration of scrollbar kinds.
 enum ScrollKind {
   kScrollHorizontal = 0,
   kScrollVertical = 1
 };

 protected:
  Sprite normal_background_;
  Sprite focused_background_;
  Sprite normal_button_dec_;
  Sprite focused_button_dec_;
  Sprite down_button_dec_;
  Sprite normal_button_inc_;
  Sprite focused_button_inc_;
  Sprite down_button_inc_;
  Sprite normal_button_cur_;
  Sprite focused_button_cur_;
  Sprite down_button_cur_;

  Si32 step_;
  Si32 min_value_;
  Si32 max_value_;
  Si32 value_;
  ScrollState state_ = kNormal;
  Si32 start_relative_s_ = 0;
  Si32 start_value_ = 0;
  Si32 dir_ = 0; // 0 for horizontal, 1 for vertical
  std::shared_ptr<GuiThemeScrollbar> theme_;

 public:
  /// @brief Constructor for Scrollbar panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param normal_background Normal state background sprite.
  /// @param focused_background Focused state background sprite.
  /// @param normal_button_left Normal state left button sprite.
  /// @param focused_button_left Focused state left button sprite.
  /// @param down_button_left Down state left button sprite.
  /// @param normal_button_right Normal state right button sprite.
  /// @param focused_button_right Focused state right button sprite.
  /// @param down_button_right Down state right button sprite.
  /// @param normal_button_cur Normal state cursor button sprite.
  /// @param focused_button_cur Focused state cursor button sprite.
  /// @param down_button_cur Down state cursor button sprite.
  /// @param min_value Minimum value of the scrollbar.
  /// @param max_value Maximum value of the scrollbar.
  /// @param value Current value of the scrollbar.
  /// @param kind Scrollbar kind.
  Scrollbar(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite normal_background,
    Sprite focused_background, Sprite normal_button_left,
    Sprite focused_button_left, Sprite down_button_left,
    Sprite normal_button_right, Sprite focused_button_right,
    Sprite down_button_right, Sprite normal_button_cur,
    Sprite focused_button_cur, Sprite down_button_cur,
    Si32 min_value, Si32 max_value, Si32 value,
    ScrollKind kind);

  /// @brief Constructor for Scrollbar panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme Scrollbar theme for the panel.
  Scrollbar(Ui64 tag, std::shared_ptr<GuiThemeScrollbar> theme);

  /// @brief Applies input to the scrollbar panel.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
    bool is_top_level,
    bool *in_out_is_applied,
    std::deque<GuiMessage> *out_gui_messages,
    std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief Draws the scrollbar panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Sets the step value of the scrollbar.
  /// @param step New step value.
  void SetStep(Si32 step);

  /// @brief Sets the current value of the scrollbar.
  /// @param value New current value.
  void SetValue(Si32 value);

  /// @brief Gets the current value of the scrollbar.
  /// @return Current value of the scrollbar.
  Si32 GetValue() const;

  /// @brief Sets the minimum value of the scrollbar.
  /// @param min_value New minimum value.
  void SetMinValue(Si32 min_value);

  /// @brief Gets the minimum value of the scrollbar.
  /// @return Minimum value of the scrollbar.
  Si32 GetMinValue() const;

  /// @brief Sets the maximum value of the scrollbar.
  /// @param max_value New maximum value.
  void SetMaxValue(Si32 max_value);

  /// @brief Gets the maximum value of the scrollbar.
  /// @return Maximum value of the scrollbar.
  Si32 GetMaxValue() const;

  /// @brief Regenerates the sprites of the scrollbar panel.
  void RegenerateSprites() override;

  std::function<void(void)> OnScrollChange = DoNothing;
};

/// @brief Class representing a checkbox panel.
class Checkbox : public Panel {
 public:
  /// @brief Enumeration of checkbox states.
  enum CheckboxState {
    kHidden = 0,
    kNormal = 1,
    kHovered = 2,
    kDown = 3,
    kDisabled = 4
  };

  /// @brief Enumeration of checkbox values.
  enum CheckboxValue {
    kValueClear = 0,
    kValueChecked = 1
  };

 protected:
  Sprite normal_[2];
  Sprite down_[2];
  Sprite hovered_[2];
  Sprite disabled_[2];
  Sound down_sound_;
  Sound up_sound_;
  KeyCode hotkey_;
  CheckboxState state_ = kNormal;
  CheckboxValue value_ = kValueClear;
  std::shared_ptr<Text> text_;
  std::shared_ptr<GuiTheme> theme_;

 public:
  /// @brief Constructor for Checkbox panel.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param tab_order Tab order of the panel.
  /// @param clear_normal Normal state sprite for unchecked checkbox.
  /// @param checked_normal Normal state sprite for checked checkbox.
  /// @param clear_down Down state sprite for unchecked checkbox.
  /// @param checked_down Down state sprite for checked checkbox.
  /// @param clear_hovered Hovered state sprite for unchecked checkbox.
  /// @param checked_hovered Hovered state sprite for checked checkbox.
  /// @param clear_disabled Disabled state sprite for unchecked checkbox.
  /// @param checked_disabled Disabled state sprite for checked checkbox.
  /// @param down_sound Sound played when checkbox is pressed.
  /// @param up_sound Sound played when checkbox is released.
  /// @param hotkey Hotkey for the checkbox.
  /// @param value Initial checkbox value.
  Checkbox(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
    Sprite clear_normal,
    Sprite checked_normal,
    Sprite clear_down = Sprite(),
    Sprite checked_down = Sprite(),
    Sprite clear_hovered = Sprite(),
    Sprite checked_hovered = Sprite(),
    Sprite clear_disabled = Sprite(),
    Sprite checked_disabled = Sprite(),
    Sound down_sound = Sound(),
    Sound up_sound = Sound(),
    KeyCode hotkey = kKeyNone,
    CheckboxValue value = kValueClear);

  /// @brief Constructor for Checkbox panel using a theme.
  /// @param tag Unique tag for the panel.
  /// @param theme GUI theme for the checkbox.
  Checkbox(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Draws the checkbox panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos)
    override;

  /// @brief Applies input to the checkbox panel.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief Sets the current tab status of the checkbox panel.
  /// @param is_current_tab True if the panel is the current tab, false otherwise.
  void SetCurrentTab(bool is_current_tab) override;

  /// @brief Sets the visibility of the checkbox panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  void SetVisible(bool is_visible) override;

  /// @brief Sets the enabled status of the checkbox panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Checks if the checkbox panel is visible.
  /// @return True if the panel is visible, false otherwise.
  bool IsVisible() override;

  /// @brief Checks if the checkbox panel is mouse transparent at a given position.
  /// @param parent_pos Position of the parent panel.
  /// @param mouse_pos Mouse position relative to the parent panel.
  /// @return True if the panel is mouse transparent at the given position, false otherwise.
  bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) override;

  /// @brief Sets the checked status of the checkbox panel.
  /// @param is_checked True to check the checkbox, false to uncheck it.
  void SetChecked(bool is_checked);

  /// @brief Checks if the checkbox panel is checked.
  /// @return True if the checkbox is checked, false otherwise.
  bool IsChecked();

  /// @brief Sets the text content of the checkbox panel.
  /// @param text New text content.
  void SetText(std::string text);

  /// @brief Sets the hotkey for the checkbox panel.
  /// @param hotkey New hotkey for the checkbox.
  void SetHotkey(KeyCode hotkey);

  std::function<void(void)> OnButtonClick = DoNothing;
  std::function<void(void)> OnButtonDown = DoNothing;
};

/// @brief Class for creating GUI panels.
class GuiFactory {
protected:
  Ui64 last_tag_ = 0;
 public:
  std::shared_ptr<GuiTheme> theme_;

  /// @brief Creates a new panel.
  /// @return Shared pointer to the created panel.
  std::shared_ptr<Panel> MakePanel();

  /// @brief Creates a new transparent panel.
  /// @return Shared pointer to the created panel.
  std::shared_ptr<Panel> MakeTransparentPanel();

  /// @brief Creates a new button panel.
  /// @return Shared pointer to the created button panel.
  std::shared_ptr<Button> MakeButton();

  /// @brief Creates a new text panel.
  /// @return Shared pointer to the created text panel.
  std::shared_ptr<Text> MakeText();

  /// @brief Creates a new progress bar panel.
  /// @return Shared pointer to the created progress bar panel.
  std::shared_ptr<Progressbar> MakeProgressbar();

  /// @brief Creates a new horizontal scrollbar panel.
  /// @return Shared pointer to the created scrollbar panel.
  std::shared_ptr<Scrollbar> MakeHorizontalScrollbar();

  /// @brief Creates a new vertical scrollbar panel.
  /// @return Shared pointer to the created scrollbar panel.
  std::shared_ptr<Scrollbar> MakeVerticalScrollbar();

  /// @brief Creates a new checkbox panel.
  /// @return Shared pointer to the created checkbox panel.
  std::shared_ptr<Checkbox> MakeCheckbox();

  /// @brief Creates a new edit box panel.
  /// @return Shared pointer to the created edit box panel.
  std::shared_ptr<Editbox> MakeEditbox();
};

/// @}

}  // namespace arctic


#endif  // ENGINE_GUI_H_
