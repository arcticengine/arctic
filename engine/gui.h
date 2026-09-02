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

// Panels live in backbuffer pixels, the very same ones the game draws in:
// (0, 0) at the bottom-left corner, y growing upward, a position being the
// bottom-left corner of the panel. There is no separate interface coordinate
// space, so a world position and a panel position may be compared directly.
// See the "Where Zero Is and Which Way Is Up" section of the documentation,
// and FromTopLeft() in engine/easy_util.h for a layout measured from the top.

/// @brief Enumeration of GUI message types.
///
/// What ApplyInput appends to the queue it was given, one entry per event, each
/// naming the panel it happened to.
enum GuiMessageKind {
  kGuiButtonClick,  ///< A button was pressed and released: the click happened
  kGuiButtonDown,  ///< A button went down, the release is still to come
  kGuiScrollChange,  ///< A scrollbar was moved, ask it for the new value
  kGuiPanelLeftDown,  ///< A clickable panel took a left button press
  kGuiButtonHover,  ///< The cursor came onto a button
  kGuiEditboxTextChange,  ///< The text of an edit box changed, by any means
  kGuiEditboxEditDone,  ///< Enter was pressed, or the focus went elsewhere
  kGuiSliderChange,  ///< A slider was moved, ask it for the new value
  kGuiRadioSelect,  ///< A radio button became the selected one of its group
  kGuiListSelectionChange,  ///< The selected item of a list box changed
  kGuiListItemClick,  ///< An item of a list box was clicked, selected or not
  kGuiListItemActivate,  ///< A list item was double-clicked or Enter was pressed
  kGuiDropdownChange,  ///< The chosen item of a dropdown changed
  kGuiTabChange,  ///< Another tab of a tab control was selected
};

/// @brief One visual line of wrapped text, see WrapText().
struct WrappedTextLine {
  std::string text;  ///< The line as drawn: no '\n', trailing spaces kept
  Si32 start = 0;  ///< Byte offset of the first byte of the line in the source
  Si32 end = 0;  ///< Byte offset right after the line, includes a hard '\n'
};

/// @brief Splits text into visual lines.
/// @param font Font that measures the text.
/// @param text UTF-8 text, '\n' always starts a new line.
/// @param max_width Width in pixels a line may take, 0 to break on '\n' only.
/// @return The lines in order; an empty text gives a single empty line.
///
/// Breaks happen after a space run or after a hyphen, never inside a word. A
/// word longer than max_width stays on a line of its own and overflows.
std::vector<WrappedTextLine> WrapText(Font font, const std::string &text,
    Si32 max_width);

/// @brief Enumeration of text selection modes.
enum TextSelectionMode {
  kTextSelectionModeInvert,
  kTextSelectionModeSwapColors,
  /// Alpha-blends selection_color_1 over the pixels under the selection,
  /// producing a translucent highlight rectangle behind the text (the
  /// selection color's alpha channel controls the strength).
  kTextSelectionModeBlend
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

/// @brief How a tooltip looks: the frame around it, the font and the delay.
class GuiThemeTooltip {
 public:
  DecoratedFrame frame_;
  Font font_;
  Rgba color_ = Rgba(255, 255, 255);
  Si32 padding_ = 4;  ///< Space between the frame border and the text
  double delay_seconds_ = 0.5;  ///< How long the cursor rests before it shows
};

/// @brief Slider theme: the track and the thumb in its four states.
class GuiThemeSlider {
 public:
  DecoratedFrame track_;
  DecoratedFrame thumb_normal_;
  DecoratedFrame thumb_hovered_;
  DecoratedFrame thumb_down_;
  DecoratedFrame thumb_disabled_;
  Si32 thumb_length_ = 16;  ///< Thumb extent along the track, in pixels
  bool is_horizontal_ = true;
};

/// @brief Class representing the overall GUI theme.
///
/// Every entry the loader does not find in the XML gets a fallback taken from
/// another entry (radio buttons look like check boxes, list rows like buttons,
/// tab headers like buttons and so on), so a theme written for the older
/// widgets keeps working with the newer ones.
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

  /// Radio buttons: XML nodes radio_clear_normal, radio_checked_normal,
  /// radio_clear_down, ... the same eight names check boxes use; the check box
  /// sprites are the fallback.
  Sprite radio_clear_normal_;
  Sprite radio_checked_normal_;
  Sprite radio_clear_down_;
  Sprite radio_checked_down_;
  Sprite radio_clear_hovered_;
  Sprite radio_checked_hovered_;
  Sprite radio_clear_disabled_;
  Sprite radio_checked_disabled_;

  /// Sliders: XML nodes slider_track, slider_thumb_normal, slider_thumb_hovered,
  /// slider_thumb_down, slider_thumb_disabled (decorated frames) and the
  /// thumb_length attribute of slider_thumb_normal. Fallback: the scrollbar
  /// background for the track, the button frames for the thumb.
  std::shared_ptr<GuiThemeSlider> h_slider_;
  std::shared_ptr<GuiThemeSlider> v_slider_;

  /// List boxes: XML nodes listbox_background, listbox_selection,
  /// listbox_hover (decorated frames). Fallback: the edit box frame for the
  /// background, the down and hovered button frames for the rows.
  DecoratedFrame listbox_background_;
  DecoratedFrame listbox_selection_;
  DecoratedFrame listbox_hover_;

  /// Dropdowns: XML node dropdown_arrow (a sprite); when absent a triangle is
  /// drawn in the text color. The box itself uses the button frames.
  Sprite dropdown_arrow_;

  /// Tab controls: XML nodes tab_normal, tab_selected, tab_hovered, tab_page
  /// (decorated frames). Fallback: the normal, down and hovered button frames
  /// for the headers and the panel background for the page.
  DecoratedFrame tab_normal_;
  DecoratedFrame tab_selected_;
  DecoratedFrame tab_hovered_;
  DecoratedFrame tab_page_;

  /// Tooltips: XML node tooltip_frame (a decorated frame, fallback is the panel
  /// background) with optional delay_seconds and padding attributes; the text
  /// font and the first color of the text palette.
  std::shared_ptr<GuiThemeTooltip> tooltip_;

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
  bool is_clipping_children_ = false;
  std::string tooltip_;
  // Root-only tooltip state: the style, the panel the cursor rests on, since
  // when, and where the cursor was.
  std::shared_ptr<GuiThemeTooltip> tooltip_theme_;
  std::weak_ptr<Panel> tooltip_owner_;
  double tooltip_since_ = 0.0;
  Vec2Si32 tooltip_pos_ = Vec2Si32(0, 0);

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

  /// @brief Offers one input message to this panel and everything inside it
  /// @param message Input message to apply, as GetInputMessage() gives it.
  /// @param out_gui_messages Queue to append what happened to, may be nullptr.
  /// @return True when the interface took the message, false when it did not.
  ///
  /// This is the form a game loop calls, once per message, on the root panel:
  /// @code
  /// for (Si32 i = 0; i < InputMessageCount(); ++i) {
  ///   const InputMessage &message = GetInputMessage(i);
  ///   if (gui_root->ApplyInput(message, &gui_messages)) {
  ///     continue;  // the interface took it, the world must not see it
  ///   }
  ///   HandleWorldInput(message);
  /// }
  /// @endcode
  ///
  /// The answer is the thing worth using: a click that landed on a button comes
  /// back as true, and a click that fell past the interface comes back as false,
  /// which is how a game keeps a press on a panel from also being a press on
  /// whatever the panel covers. It says nothing about *which* panel took it --
  /// that is what the queue is for. To ask about a point without offering a
  /// message at all, IsInside() answers the same question geometrically.
  ///
  /// Mouse positions are read from InputMessage::Mouse::backbuffer_pos, so the
  /// message has to be one the engine handed out; a message built by hand needs
  /// that field filled in.
  ///
  /// The queue collects a GuiMessage per event, naming the panel and the kind
  /// (kGuiButtonClick, kGuiPanelLeftDown, kGuiEditboxTextChange and the rest),
  /// which is the way to see what the interface did without wiring a callback
  /// into every element. Panels also call their own callbacks in the middle of
  /// this walk, so neither the callbacks nor the code reading the queue may add
  /// or remove panels while it is going on: remember what to do and do it after
  /// the input loop.
  bool ApplyInput(const InputMessage &message,
      std::deque<GuiMessage> *out_gui_messages);

  /// @brief Applies input to the panel and its children.
  /// @param parent_pos Position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level Flag indicating if this is a top-level panel.
  /// @param in_out_is_applied Input/output flag indicating if input was applied.
  /// @param out_gui_messages Output queue for GUI messages.
  /// @param out_current_tab Output pointer to the current tab panel.
  ///
  /// A hidden or disabled panel takes nothing, and neither does anything inside
  /// it; this is decided here, once, and the panel kinds do not get to forget
  /// it. What a visible and enabled panel does with the message is HandleInput,
  /// which is the function to override.
  void ApplyInput(Vec2Si32 parent_pos, const InputMessage &message,
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

  /// @brief Checks if the panel holds the input focus (is the current tab)
  /// @return True if this very panel is the current tab, false otherwise.
  ///
  /// Handy for a host that has to know whether the user is working in a
  /// particular field: `if (my_editbox->IsFocused()) { ... }` says it without
  /// comparing the pointer FindCurrentTab returned against every field.
  bool IsFocused() const;

  /// @brief Tells whether the keyboard belongs to this panel while it is focused
  /// @return True for a panel that reads keystrokes as text, false otherwise.
  ///
  /// Panels that turn keystrokes into their own state, an Editbox above all,
  /// return true. The base implementation returns false.
  virtual bool IsKeyboardCapturing() const;

  /// @brief Checks whether the focused panel in this subtree eats the keyboard
  /// @return True if the focus is inside a visible panel that reads keystrokes.
  ///
  /// An application usually has hotkeys of its own, and they must not fire while
  /// the user is typing a value into a field. Ask the root panel about it once
  /// per frame instead of keeping a list of the fields that can hold the focus:
  /// @code
  /// if (!gui_root->IsKeyboardCaptured()) {
  ///   if (IsKeyDownward(kKeySpace)) {
  ///     TogglePause();
  ///   }
  /// }
  /// @endcode
  bool IsKeyboardCaptured();

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

  /// @brief Checks if the panel is enabled.
  /// @return True if the panel is enabled, false otherwise.
  virtual bool IsEnabled();

  /// @brief Checks if the panel is visible.
  /// @return True if the panel is visible, false otherwise.
  virtual bool IsVisible();

  /// @brief Checks whether input can reach this panel at all
  /// @return True while this panel and every panel above it are visible.
  ///
  /// Hiding a panel hides its children with it, and the input walk stops at the
  /// hidden panel without ever asking the children about anything, so a visible
  /// field inside a hidden dialog receives nothing.
  bool IsReachableForInput();

  /// @brief Tells whether a point belongs to the interface
  /// @param backbuffer_pos A point in backbuffer pixels, MousePos() for instance
  /// @return True when a click there would be taken by this panel or by
  ///   something inside it, false when it would fall through to the game.
  ///
  /// The question "is this click mine or the interface's?" without pushing a
  /// message through the interface and reading the answer of ApplyInput():
  /// @code
  /// if (IsKeyDownward(kKeyMouseLeft) && !gui_root->IsInside(MousePos())) {
  ///   SelectInTheWorld(MousePos());
  /// }
  /// @endcode
  ///
  /// A hidden panel takes nothing, and neither does anything inside it. A panel
  /// that is not clickable takes nothing either, so a click passes through a
  /// Text or a bare background panel and reaches the world -- which is what the
  /// input walk does as well, and this is meant to agree with it. Buttons,
  /// checkboxes, edit boxes and scrollbars always count.
  ///
  /// Positions are counted from the bottom-left corner of the backbuffer, and
  /// the panel is assumed to be a root drawn at (0, 0), exactly as
  /// ApplyInput(message, queue) assumes.
  bool IsInside(Vec2Si32 backbuffer_pos);

  /// @brief Checks if the panel is mouse transparent at a given position.
  /// @param parent_pos Absolute position of the parent panel.
  /// @param mouse_pos Mouse position in backbuffer pixels, not relative to the parent.
  /// @return True if a click at that point would pass through this panel and
  ///   everything inside it, false if something would take it.
  ///
  /// The inverted form of IsInside, which is the one to call: this one exists
  /// for the panels that override it and for the walk that uses it.
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

  /// @brief Returns the children in drawing order (the last one is on top).
  const std::deque<std::shared_ptr<Panel>> &GetChildren() const;

  /// @brief Returns the parent, nullptr for a root.
  Panel *GetParent() const;

  /// @brief Makes the panel cut off whatever its children draw outside it.
  /// @param is_clipping True to clip, false (the default) to let it spill.
  ///
  /// With clipping on, the children are drawn into the rectangle of the panel
  /// only, which is what a scrolling area or a list of rows needs. Input is not
  /// affected: a child that sticks out still takes clicks there, so move or
  /// hide the children that should not.
  void SetClipChildren(bool is_clipping);

  /// @brief Tells whether the children are clipped to the panel.
  bool IsClippingChildren() const;

  /// @brief Sets the text shown when the cursor rests over the panel.
  /// @param tooltip UTF-8 text, '\n' starts a new line; empty means no tooltip.
  ///
  /// The tooltip is drawn by the root of the tree after everything else, in the
  /// style the root got from its theme or from SetTooltipTheme(). The deepest
  /// visible panel under the cursor that has a tooltip wins, so a tooltip on a
  /// panel is a fallback for the children without one.
  void SetTooltip(std::string tooltip);

  /// @brief Returns the tooltip text, empty when there is none.
  const std::string &GetTooltip() const;

  /// @brief Sets the tooltip style used when this panel is the root.
  /// @param theme The frame, font, color and delay; nullptr turns tooltips off.
  ///
  /// A panel made with a theme takes the style from it; a plain panel that
  /// serves as the root needs this call before any tooltip can appear.
  void SetTooltipTheme(std::shared_ptr<GuiThemeTooltip> theme);

 protected:
  /// @brief Draws what must appear above the whole tree: popups and the like.
  /// @param absolute_pos Absolute position of this panel.
  ///
  /// The root calls this on itself after the ordinary Draw of every panel, so
  /// whatever is drawn here lies above every sibling and every ancestor. The
  /// base implementation asks the visible children; a dropdown draws its
  /// open list here.
  virtual void DrawOverlays(Vec2Si32 absolute_pos);

  /// @brief Offers an input message to the overlays before the ordinary walk.
  /// @param absolute_pos Absolute position of this panel.
  /// @param message Input message to apply.
  /// @param in_out_is_applied Set to true once something took the message.
  /// @param out_gui_messages Queue for what happened, may be nullptr.
  /// @param out_current_tab Set to the panel that is to take the focus.
  ///
  /// The root calls this first, so an open popup takes the click that would
  /// otherwise land on what lies under it. The base implementation asks the
  /// visible and enabled children.
  virtual void HandleOverlayInput(Vec2Si32 absolute_pos,
      const InputMessage &message,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab);

  /// @brief Tells whether no overlay of this subtree takes a point.
  /// @param absolute_pos Absolute position of this panel.
  /// @param mouse_pos Mouse position in backbuffer pixels.
  /// @return False when an open popup would take a click at that point.
  virtual bool IsOverlayTransparentAt(Vec2Si32 absolute_pos,
      Vec2Si32 mouse_pos);

  /// @brief Finds the deepest visible panel under a point that has a tooltip.
  /// @param absolute_pos Absolute position of this panel.
  /// @param mouse_pos Mouse position in backbuffer pixels.
  /// @return The panel, or nullptr when there is none under the point.
  Panel *FindTooltipOwnerAt(Vec2Si32 absolute_pos, Vec2Si32 mouse_pos);

  /// @brief Root only: remembers which panel the cursor rests on and since when.
  void TrackTooltip(Vec2Si32 absolute_pos, const InputMessage &message);

  /// @brief Root only: draws the tooltip once the cursor has rested long enough.
  void DrawTooltip(Vec2Si32 absolute_pos);

  /// @brief Handles one input message for a visible and enabled panel
  /// @param parent_pos Absolute position of the parent panel.
  /// @param message Input message to apply.
  /// @param is_top_level True for the root of the walk, which owns the Tab key.
  /// @param in_out_is_applied Set to true once something took the message.
  /// @param out_gui_messages Queue for what happened, may be nullptr.
  /// @param out_current_tab Set to the panel that is to take the focus.
  ///
  /// ApplyInput has already checked that the panel is visible and enabled and
  /// that the pointers are there. The base implementation walks the children
  /// (front to back, so the panel drawn last is asked first), takes a left press
  /// when the panel is clickable, and, at the top level, moves the focus on Tab.
  /// An override does its own work after calling Panel::HandleInput.
  virtual void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab);

  /// @brief Tells whether a point is inside the rectangle of this panel
  /// @param relative_pos A point counted from the bottom-left corner of the panel.
  /// @return True for 0 <= x < width and 0 <= y < height.
  bool IsWithin(Vec2Si32 relative_pos) const;

  /// @brief Tells whether a visible panel of this kind takes clicks inside it
  /// @return True when a click inside the rectangle belongs to the panel.
  ///
  /// The base answer is the clickable flag; the kinds whose HandleInput takes
  /// every click inside them (buttons, edit boxes, scrollbars) answer true, so
  /// that IsInside() agrees with the input walk.
  virtual bool TakesMouse() const;

  /// @brief Appends a message about this panel to the queue, if there is one
  /// @param kind What happened.
  /// @param out_gui_messages The queue, may be nullptr.
  void Emit(GuiMessageKind kind, std::deque<GuiMessage> *out_gui_messages);
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
  bool word_wrap_ = false;

  // The text as drawn: text_ itself, or the wrapped lines joined with '\n'.
  std::string ShownText() const;
  // Byte offset in text_ -> byte offset in ShownText().
  Si32 ToShownOffset(Si32 offset) const;

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
  /// @return Size of the text as drawn, wrapped lines included.
  Vec2Si32 EvaluateSize();

  /// @brief Sets the enabled status of the text panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Turns word wrap on or off.
  /// @param word_wrap True to break lines at the width of the panel.
  ///
  /// With wrap on, the text is laid out into lines no wider than the panel
  /// (see WrapText()), EvaluateSize() reports the wrapped size, and a selection
  /// set with Select() still counts bytes of the original text.
  void SetWordWrap(bool word_wrap);

  /// @brief Tells whether word wrap is on.
  bool IsWordWrap() const;
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

  /// @brief Sets the current tab status of the button panel.
  /// @param is_current_tab True if the panel is the current tab, false otherwise.
  void SetCurrentTab(bool is_current_tab) override;

  /// @brief Sets the visibility of the button panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  void SetVisible(bool is_visible) override;

  /// @brief Sets the enabled status of the button panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Checks if the button panel is enabled.
  /// @return True if the panel is enabled, false otherwise.
  bool IsEnabled() override;

  /// @brief Checks if the button panel is visible.
  /// @return True if the panel is visible, false otherwise.
  bool IsVisible() override;

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

  /// @brief Sets the position of the text of the button panel.
  /// @param pos New position of the text.
  void SetTextPos(Vec2Si32 pos);

  std::function<void(void)> OnButtonClick = DoNothing;
  std::function<void(void)> OnButtonDown = DoNothing;

 protected:
  /// @brief Presses, releases, hovers and hotkeys; see Panel::HandleInput.
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief A button takes every click inside it.
  bool TakesMouse() const override;
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

  // Multiline support (single-line by default, like WinForms TextBox.Multiline).
  bool is_multiline_ = false;
  // Wraps overlong multiline text at word / hyphen boundaries when set.
  bool word_wrap_ = false;
  // When false, Enter is left to the host even in multiline mode (the same idea
  // as TextBox.AcceptsReturn in Windows Forms).
  bool accepts_return_ = true;
  // Vertical step between multiline rows; 0 means the font's own line height.
  Si32 line_spacing_ = 0;
  // Inset of multiline text from the box edges, in pixels.
  Si32 multiline_padding_ = 2;
  Si32 first_visible_line_ = 0;
  // Maximum number of bytes allowed in the text (0 = unlimited).
  Si32 max_length_ = 0;
  // True while the left mouse button drags a selection inside the box.
  bool is_dragging_ = false;
  // Time the caret last moved or the text last changed. The caret stays lit
  // instead of blinking for a short while after that, so it stays readable
  // while typing or walking through the text.
  double caret_touch_time_ = 0.0;

  // Coalescing groups for the local undo/redo history.
  enum EditGroup {
    kEditGroupNone = 0,
    kEditGroupType = 1,
    kEditGroupDelete = 2,
    kEditGroupDiscrete = 3
  };
  // A single point in the editbox text history.
  struct EditSnapshot {
    std::string text;
    Si32 cursor = 0;
    Si32 sel_begin = 0;
    Si32 sel_end = 0;
  };
  std::vector<EditSnapshot> undo_;
  std::vector<EditSnapshot> redo_;
  Si32 edit_group_ = kEditGroupNone;

  // Pushes the current text state onto the undo stack before a mutation.
  // Consecutive edits sharing a non-discrete group collapse into one step.
  void SnapshotBefore(Si32 group);
  // Restores the previous / next text state from the undo / redo stack.
  void Undo();
  void Redo();
  // Moves the caret to a byte offset, extending the selection when shift is
  // held or collapsing it otherwise. Keeps the engine selection invariant.
  void MoveCursorTo(Si32 target, bool shift);
  // A single visual line of multiline text. `text` is what gets drawn and never
  // contains the '\n'. [start, end) is the source byte range the line owns, so
  // end is where the next visual line begins: the ranges are contiguous and
  // every caret offset maps to exactly one line.
  typedef WrappedTextLine VisualLine;
  // Splits text_ into visual lines, always breaking at '\n' and additionally at
  // word / hyphen boundaries when word_wrap_ is on.
  std::vector<VisualLine> WrapVisualLines();
  // Whether the text can be changed by the user; see SetReadOnly.
  bool is_read_only_ = false;
  // Codepoint every character is shown as, 0 for none; see SetPasswordChar.
  Ui32 password_char_ = 0;
  // Text drawn in placeholder_color_ while text_ is empty.
  std::string placeholder_;
  Rgba placeholder_color_ = Rgba(128, 128, 128);
  // Byte offset in text_ -> byte offset in ShownText() and back. Identity
  // unless a password char is set.
  Si32 ToShownOffset(Si32 offset) const;
  Si32 FromShownOffset(Si32 shown_offset) const;
  // Vertical step between two multiline rows.
  Si32 LineStep() const;
  // Index of the visual line owning the caret offset pos. At a line boundary
  // the later line wins, so a caret right after a break reads as the next line.
  static Si32 VisualLineIndex(const std::vector<VisualLine> &lines, Si32 pos);
  // Pixel width of the first `bytes` bytes of `line`. Keeps the advance of
  // trailing spaces, so the caret stays put after a trailing space instead of
  // snapping back onto the last visible glyph.
  Si32 PrefixWidth(const std::string &line, Si32 bytes);
  // Largest caret offset that still renders on visual line li. A soft-wrapped
  // line ends where the next one starts, so the offset past its trailing space
  // would draw a line lower; this stops before that space instead.
  Si32 VisualLineCaretLimit(const std::vector<VisualLine> &lines, Si32 li) const;
  // Width available for text inside the box in multiline mode.
  Si32 MultilineInnerWidth() const;
  // Byte offset of the start / end of the visual line containing pos.
  Si32 LineStart(Si32 pos);
  Si32 LineEnd(Si32 pos);
  // Byte offset of the previous / next word boundary from pos (Ctrl+Left/Right).
  Si32 PrevWordPos(Si32 pos) const;
  Si32 NextWordPos(Si32 pos) const;
  // Moves the caret one line up (dir=-1) or down (dir=+1) in multiline mode.
  void MoveCursorVertical(Si32 dir, bool shift);
  // Maps a point relative to the box (y-up, 0 at bottom-left) to a caret byte
  // offset, honoring horizontal scroll (single line) or line layout (multiline).
  Si32 CaretFromPoint(Vec2Si32 relative_pos);
  // Draws the text, caret and selection in multiline mode. pos is the absolute
  // bottom-left corner of the box.
  void DrawMultiline(Vec2Si32 pos);
  // Restarts the "caret is solid" cooldown; call it whenever the caret moves.
  void TouchCaret();
  // True when the caret should be drawn: solid during the cooldown after it
  // last moved, blinking afterwards.
  bool IsCaretVisible() const;
  // Deletes the current selection (if any) and collapses the caret to its start.
  void EraseSelection();
  // Applies is_digits_ / allow_list_ filters to a string in place.
  void FilterAllowedInPlace(std::string *s) const;

  /// @brief Focus, caret, selection and editing; see Panel::HandleInput.
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief An edit box takes every click inside it.
  bool TakesMouse() const override;

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

  /// @brief Sets the cursor position in the edit box panel.
  /// @param pos Byte offset in the text string. Clamped to [0, text.length()].
  void SetCursorPos(Si32 pos);

  /// @brief Gets the cursor position as a byte offset in the text.
  Si32 GetCursorPos() const;

  /// @brief Gets the first byte offset of the selection (== end when empty).
  Si32 GetSelectionBegin() const;

  /// @brief Gets the byte offset just past the selection (== begin when empty).
  Si32 GetSelectionEnd() const;

  /// @brief Selects the range between two byte offsets, in any order.
  /// Does not move the caret; call SetCursorPos to place it on either edge.
  /// @param begin One edge of the selection.
  /// @param end The other edge of the selection.
  void SetSelection(Si32 begin, Si32 end);

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

  /// @brief Sets the flag indicating if the edit box only accepts digits.
  /// @param is_digits Flag indicating if the edit box only accepts digits.
  void SetIsDigits(bool is_digits);

  /// @brief Enables or disables multiline editing (like TextBox.Multiline).
  /// In multiline mode Enter inserts a newline, Up/Down move between lines,
  /// Home/End act on the current line and the view scrolls vertically.
  /// @param is_multiline True to allow multiple lines, false for single line.
  void SetMultiline(bool is_multiline);

  /// @brief Returns true when the edit box is in multiline mode.
  bool IsMultiline() const;

  /// @brief Enables or disables word wrapping of overlong multiline text.
  /// With wrapping on, a line too wide for the box continues on the next visual
  /// line, breaking after a space or hyphen; the caret, selection, Up/Down and
  /// Home/End all follow the visual lines. Only meaningful in multiline mode.
  /// @param word_wrap True to wrap long lines, false to let them overflow.
  void SetWordWrap(bool word_wrap);

  /// @brief Returns true when word wrapping is enabled.
  bool IsWordWrap() const;

  /// @brief Chooses who handles Enter in multiline mode (default: the widget).
  /// With false, Enter never inserts a newline and the host can use it to
  /// accept the edit, like TextBox.AcceptsReturn = false in Windows Forms.
  /// @param accepts_return True to insert a newline, false to leave Enter alone.
  void SetAcceptsReturn(bool accepts_return);

  /// @brief Returns true when Enter inserts a newline in multiline mode.
  bool AcceptsReturn() const;

  /// @brief Sets the vertical step between multiline rows.
  /// Use it to match the line spacing of surrounding text drawn by the host,
  /// so switching a field between display and editing does not shift the text.
  /// @param line_spacing Step in pixels, or 0 to use the font's line height.
  void SetLineSpacing(Si32 line_spacing);

  /// @brief Sets the inset of multiline text from the box edges (default 2).
  /// @param padding Inset in pixels, applied on all four sides.
  void SetMultilinePadding(Si32 padding);

  /// @brief Sets the maximum text length in bytes (0 = unlimited).
  /// @param max_length Maximum number of bytes allowed in the text.
  void SetMaxLength(Si32 max_length);

  /// @brief Forbids or allows changes by the user.
  /// @param is_read_only True to make typing, deleting, pasting, cutting and
  ///   undo do nothing; the caret still moves, text can still be selected and
  ///   copied, and SetText() still works.
  void SetReadOnly(bool is_read_only);

  /// @brief Tells whether the box is read-only.
  bool IsReadOnly() const;

  /// @brief Shows every character as one and the same character.
  /// @param password_char The codepoint shown instead of each character, for
  ///   example '*'; 0 shows the text itself. Single-line boxes only: the text
  ///   is still edited as usual and GetText() returns the real text.
  void SetPasswordChar(Ui32 password_char);

  /// @brief Returns the password character, 0 when the text is shown as is.
  Ui32 GetPasswordChar() const;

  /// @brief Returns the text as it is drawn: the text itself, or the password
  /// character repeated once per character.
  std::string ShownText() const;

  /// @brief Sets the hint drawn in the box while its text is empty.
  /// @param placeholder The hint; an empty string removes it.
  void SetPlaceholder(std::string placeholder);

  /// @brief Returns the placeholder text.
  const std::string &GetPlaceholder() const;

  /// @brief Sets the color of the placeholder text (default grey).
  void SetPlaceholderColor(Rgba color);

  /// @brief Keystrokes are text while the box has the focus.
  bool IsKeyboardCapturing() const override;

  /// @brief Notices the loss of focus, which ends the editing.
  /// @param is_current_tab True if the panel becomes the current tab.
  void SetCurrentTab(bool is_current_tab) override;

  /// @brief Called whenever the text changes because of user input
  ///
  /// Typing, deleting, pasting, undo and redo all end up here, so a host can
  /// follow the field instead of comparing GetText() against a remembered copy
  /// every frame. A kGuiEditboxTextChange message is queued as well when the
  /// input is applied through the ApplyInput overload that takes a message
  /// queue. SetText() is a change made by the host itself and does not call it.
  ///
  /// Example:
  /// @code
  /// rate_box->OnTextChange = [&]() { ... };  // or a free function
  /// @endcode
  ///
  /// @warning The callbacks run in the middle of input handling, so they must not
  /// add or remove panels; remember what to do and do it after ApplyInput.
  std::function<void(void)> OnTextChange = DoNothing;

  /// @brief Called when the user finishes editing the text
  ///
  /// Two things end the editing: Enter in a field that does not take newlines
  /// (a single line box, or a multiline one with AcceptsReturn off), and the loss
  /// of focus, by Tab or by a click elsewhere. This is the moment to accept the
  /// value the user typed, or to put the old one back when it makes no sense.
  /// Enter is not consumed, so a host that uses it for something of its own
  /// keeps seeing it. A kGuiEditboxEditDone message is queued for the Enter case,
  /// where a message queue is at hand; the loss of focus only calls the callback.
  std::function<void(void)> OnEditDone = DoNothing;
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
   kIncFast = 7,
   kDisabled = 8
 };

 /// @brief Enumeration of scrollbar kinds.
 enum ScrollKind {
   kScrollHorizontal = 0,
   kScrollVertical = 1
 };

 protected:
  Sprite normal_background_;
  Sprite focused_background_;
  Sprite disabled_background_;
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

  Si32 step_;
  Si32 line_step_;
  Si32 min_value_;
  Si32 max_value_;
  Si32 value_;
  ScrollState state_ = kNormal;
  Si32 start_relative_s_ = 0;
  Si32 start_value_ = 0;
  Si32 dir_ = 0; // 0 for horizontal, 1 for vertical
  std::shared_ptr<GuiThemeScrollbar> theme_;

  Si32 thumb_extent_ = 0;
  enum class ScrollHoverZone {
    kNone,
    kDec,
    kInc,
    kThumb,
    kTrackBefore,
    kTrackAfter
  };
  ScrollHoverZone hover_zone_ = ScrollHoverZone::kNone;
  DecoratedFrame cur_frame_normal_;
  DecoratedFrame cur_frame_hover_;
  DecoratedFrame cur_frame_down_;
  DecoratedFrame cur_frame_disabled_;
  bool cur_hover_is_normal_ = false;
  bool cur_down_is_normal_ = false;
  bool thumb_df_ok_[3] = {false, false, false};
  bool thumb_df_disabled_ok_ = false;
  struct ThumbRasterCache {
    Sprite sprite;
    Si32 along = -1;
    Si32 cross = -1;
  };
  ThumbRasterCache thumb_cache_[3];
  ThumbRasterCache thumb_cache_disabled_;

  void InvalidateThumbCache();
  void InitThumbDecoratedFrames();
  Si32 ThumbTrackInnerPx() const;
  Si32 EffectiveThumbPx() const;
  Vec2Si32 ThumbOuterPixelSize() const;
  DecoratedFrame& ThumbFrameForLayer(Si32 layer_idx);
  void DrawThumbAt(Vec2Si32 cur_pos_abs, Si32 layer_idx);
  void DrawDisabledThumbAt(Vec2Si32 cur_pos_abs);
  void UpdateHoverZone(Vec2Si32 relative_pos, Si32 s1, Si32 s2, Si32 s3, Si32 s4);
  void DrawDecButton(Vec2Si32 absolute_pos, Vec2Si32 button_offset);
  void DrawIncButton(Vec2Si32 inc_pos, Vec2Si32 button_offset);

  /// @brief Wheel, arrows, track and thumb; see Panel::HandleInput.
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief A scrollbar takes every click inside it.
  bool TakesMouse() const override;

  // Clamps the value into range, then tells the queue and the callback.
  void SetValueAndNotify(Si32 value, std::deque<GuiMessage> *out_gui_messages);

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

  /// @brief Replaces theme sprites and rebuilds thumb frames (e.g. after UI scale change).
  void ApplyTheme(std::shared_ptr<GuiThemeScrollbar> theme);

  /// @brief Draws the scrollbar panel.
  /// @param parent_absolute_pos Absolute position of the parent panel.
  void Draw(Vec2Si32 parent_absolute_pos) override;

  /// @brief Sets the page step for track clicks and mouse wheel (not arrow keys).
  /// @param step New step value (clamped to at least 1).
  void SetStep(Si32 step);

  /// @brief Sets the line step for end-arrow clicks and keyboard when focused.
  /// @param line_step New line step (clamped to at least 1).
  void SetLineStep(Si32 line_step);

  /// @brief Gets the line step for end arrows and keyboard.
  /// @return Current line step.
  Si32 GetLineStep() const;

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

  /// @brief Sets the visibility of the scrollbar panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  void SetVisible(bool is_visible) override;

  /// @brief Sets the enabled status of the scrollbar panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Checks if the scrollbar panel is enabled.
  /// @return True if the panel is enabled, false otherwise.
  bool IsEnabled() override;

  /// @brief Regenerates the sprites of the scrollbar panel.
  void RegenerateSprites() override;

  /// @brief Sets pixel extent of the thumb along the scroll direction (0 = use theme sprite size).
  void SetThumbExtent(Si32 extent);
  /// @brief Gets the configured thumb extent (0 means theme default at draw time).
  Si32 GetThumbExtent() const;
  /// @brief True while the user is dragging the thumb.
  bool IsThumbDragging() const;

  void SetSize(Vec2Si32 size);
  void SetSize(Si32 width, Si32 height);

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

  /// @brief Sets the current tab status of the checkbox panel.
  /// @param is_current_tab True if the panel is the current tab, false otherwise.
  void SetCurrentTab(bool is_current_tab) override;

  /// @brief Sets the visibility of the checkbox panel.
  /// @param is_visible True to make the panel visible, false to hide it.
  void SetVisible(bool is_visible) override;

  /// @brief Sets the enabled status of the checkbox panel.
  /// @param is_enabled True to enable the panel, false to disable it.
  void SetEnabled(bool is_enabled) override;

  /// @brief Checks if the checkbox panel is enabled.
  /// @return True if the panel is enabled, false otherwise.
  bool IsEnabled() override;

  /// @brief Checks if the checkbox panel is visible.
  /// @return True if the panel is visible, false otherwise.
  bool IsVisible() override;

  /// @brief Sets the checked status of the checkbox panel.
  /// @param is_checked True to check the checkbox, false to uncheck it.
  virtual void SetChecked(bool is_checked);

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

 protected:
  /// @brief Presses, releases, hovers and hotkeys; see Panel::HandleInput.
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief A checkbox takes every click inside it.
  bool TakesMouse() const override;

  /// @brief What a click or the hotkey does to the value: flips it.
  /// @param out_gui_messages The queue, may be nullptr.
  ///
  /// Called on the release that completes a click; the click message itself is
  /// emitted by the caller afterwards. A radio button overrides this to select
  /// itself instead of flipping.
  virtual void Toggle(std::deque<GuiMessage> *out_gui_messages);
};

/// @brief A check box of which only one per group is checked at a time.
///
/// Radio buttons that share a parent and a group number form a group: checking
/// one clears the others, and clicking the checked one leaves it checked.
/// Two groups may live in one parent under different group numbers.
class RadioButton : public Checkbox {
 protected:
  Si32 group_ = 0;

  // Checks this one and clears the rest of its group, without messages.
  void SelectQuietly();

 public:
  /// @brief Constructor with explicit sprites, the same set a Checkbox takes.
  RadioButton(Ui64 tag, Vec2Si32 pos, Ui32 tab_order,
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
    Si32 group = 0);

  /// @brief Constructor using a theme (the radio_* sprites, or the check box
  /// sprites when the theme has none).
  RadioButton(Ui64 tag, std::shared_ptr<GuiTheme> theme, Si32 group = 0);

  /// @brief Sets the group number; siblings with the same number are one group.
  void SetGroup(Si32 group);

  /// @brief Returns the group number.
  Si32 GetGroup() const;

  /// @brief Checks this button and clears the rest of its group; false is
  /// allowed and simply clears this one.
  void SetChecked(bool is_checked) override;

  /// @brief Called when this button becomes the selected one by a click or the
  /// hotkey; not called when it was selected already.
  std::function<void(void)> OnSelect = DoNothing;

 protected:
  /// @brief Selects this button; emits kGuiRadioSelect when that is a change.
  void Toggle(std::deque<GuiMessage> *out_gui_messages) override;
};

/// @brief A panel that shows a sprite.
///
/// A plain Panel stretches its background over its whole rectangle; an Image
/// offers the other ways to fit a picture into a box and clips what does not
/// fit.
class Image : public Panel {
 public:
  /// @brief How the sprite is placed into the rectangle of the panel.
  enum ScaleMode {
    kScaleStretch = 0,  ///< Fill the rectangle, aspect ratio not kept
    kScaleFit = 1,  ///< As large as fits, aspect ratio kept, centered
    kScaleNone = 2,  ///< Own size, bottom-left corner at the panel's
    kScaleCenter = 3  ///< Own size, centered in the panel
  };

 protected:
  Sprite sprite_;
  ScaleMode scale_mode_ = kScaleStretch;
  DrawBlendingMode blending_mode_ = kDrawBlendingModeAlphaBlend;
  DrawFilterMode filter_mode_ = kFilterNearest;
  Rgba color_ = Rgba(255, 255, 255, 255);

 public:
  /// @brief Constructor for Image.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel.
  /// @param sprite The picture.
  /// @param scale_mode How the picture is placed.
  Image(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Sprite sprite,
      ScaleMode scale_mode = kScaleStretch);

  /// @brief Replaces the picture.
  void SetSprite(Sprite sprite);

  /// @brief Returns the picture.
  Sprite GetSprite() const;

  /// @brief Sets how the picture is placed into the panel.
  void SetScaleMode(ScaleMode scale_mode);

  /// @brief Returns how the picture is placed into the panel.
  ScaleMode GetScaleMode() const;

  /// @brief Sets the blending and the filter used to draw the picture.
  void SetDrawMode(DrawBlendingMode blending_mode, DrawFilterMode filter_mode);

  /// @brief Sets the color the picture is tinted with in colorize blending;
  /// ignored by the other blending modes.
  void SetColor(Rgba color);

  /// @brief Returns the rectangle the picture is drawn into, relative to the
  /// panel, for the current scale mode.
  void GetPictureRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const;

  /// @brief Draws the picture, clipped to the panel, then the children.
  void Draw(Vec2Si32 parent_absolute_pos) override;
};

/// @brief A slider: a thumb dragged along a track to pick a value in a range.
///
/// Horizontal sliders grow to the right, vertical ones grow upward. A click on
/// the track moves the thumb there, the thumb can be dragged, the wheel and,
/// when focused, the arrow keys, Home and End move it by steps. Each change
/// emits kGuiSliderChange and calls OnSliderChange.
class Slider : public Panel {
 public:
  /// @brief Enumeration of slider states.
  enum SliderState {
    kNormal = 0,
    kHovered = 1,
    kDown = 2,
    kDisabled = 3
  };

 protected:
  DecoratedFrame track_frame_;
  DecoratedFrame thumb_frame_[4];
  Sprite track_;
  Sprite thumb_[4];
  Si32 thumb_length_ = 16;
  bool is_horizontal_ = true;
  Si32 min_value_ = 0;
  Si32 max_value_ = 100;
  Si32 value_ = 0;
  Si32 step_ = 1;
  SliderState state_ = kNormal;
  Si32 drag_offset_ = 0;  // grab point minus thumb start, along the track
  std::shared_ptr<GuiThemeSlider> theme_;

  Si32 Along(Vec2Si32 v) const;  // the coordinate along the track
  Si32 TrackLength() const;  // room the thumb start can move in
  Si32 ThumbStart() const;  // thumb start for value_, along the track
  Si32 ValueAt(Si32 thumb_start) const;  // inverse of ThumbStart
  void SetValueAndNotify(Si32 value,
      std::deque<GuiMessage> *out_gui_messages);

 public:
  /// @brief Constructor with explicit frames.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel; the long side is the track.
  /// @param tab_order Tab order of the panel, 0 keeps it out of Tab.
  /// @param track Frame stretched over the whole panel.
  /// @param thumb_normal Thumb when nothing happens.
  /// @param thumb_hovered Thumb under the cursor.
  /// @param thumb_down Thumb being dragged.
  /// @param thumb_disabled Thumb of a disabled slider.
  /// @param thumb_length Thumb extent along the track, in pixels.
  /// @param is_horizontal True for a slider that grows to the right.
  Slider(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      DecoratedFrame track, DecoratedFrame thumb_normal,
      DecoratedFrame thumb_hovered, DecoratedFrame thumb_down,
      DecoratedFrame thumb_disabled, Si32 thumb_length, bool is_horizontal);

  /// @brief Constructor using a theme.
  Slider(Ui64 tag, std::shared_ptr<GuiThemeSlider> theme);

  /// @brief Sets the range; the value is clamped into it.
  /// @param min_value Value at the left or bottom end.
  /// @param max_value Value at the right or top end, at least min_value.
  void SetRange(Si32 min_value, Si32 max_value);

  /// @brief Returns the lowest value.
  Si32 GetMinValue() const;

  /// @brief Returns the highest value.
  Si32 GetMaxValue() const;

  /// @brief Sets the value, clamped into the range, without messages.
  void SetValue(Si32 value);

  /// @brief Returns the value.
  Si32 GetValue() const;

  /// @brief Sets how far one wheel notch or one key press moves the value.
  void SetStep(Si32 step);

  /// @brief Returns the step.
  Si32 GetStep() const;

  /// @brief Tells whether the slider grows to the right rather than upward.
  bool IsHorizontal() const;

  /// @brief Returns the rectangle of the thumb relative to the panel.
  void GetThumbRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const;

  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetEnabled(bool is_enabled) override;
  bool IsEnabled() override;
  void RegenerateSprites() override;

  std::function<void(void)> OnSliderChange = DoNothing;

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief A slider takes every click inside it.
  bool TakesMouse() const override;
};

/// @brief A list of text rows with one selected, scrolled by a scrollbar, the
/// wheel or the keys.
///
/// Rows are laid out from the top of the list. A click selects the row and
/// emits kGuiListItemClick; a change of the selection emits
/// kGuiListSelectionChange; a double click or Enter emits kGuiListItemActivate.
/// When focused, the arrow keys, Home, End, Page Up and Page Down move the
/// selection and keep it in view.
class ListBox : public Panel {
 protected:
  std::vector<std::string> items_;
  Si32 selected_ = -1;
  Si32 hovered_ = -1;
  Si32 first_visible_ = 0;
  Si32 row_height_ = 1;
  Si32 padding_ = 2;  // between the frame border and the rows
  Vec2Si32 border_ = Vec2Si32(0, 0);  // frame border, inner rect = size - 2 * border
  Font font_;
  std::vector<Rgba> palete_;
  std::vector<Rgba> disabled_palete_;
  DecoratedFrame background_frame_;
  DecoratedFrame selection_frame_;
  DecoratedFrame hover_frame_;
  Sprite selection_row_;
  Sprite hover_row_;
  std::shared_ptr<Scrollbar> scrollbar_;
  bool is_enabled_ = true;
  double last_click_time_ = -1.0;
  Si32 last_click_item_ = -1;
  std::shared_ptr<GuiTheme> theme_;

  Si32 InnerLeft() const;
  Si32 InnerTop() const;
  Si32 RowsWidth() const;  // inner width minus the scrollbar
  Si32 VisibleRows() const;
  Si32 MaxFirstVisible() const;
  Si32 RowAt(Vec2Si32 relative_pos) const;  // -1 when not on a row
  void ClampScroll();
  void SyncScrollbar();
  void EnsureVisible(Si32 item);
  void SelectAndNotify(Si32 item, std::deque<GuiMessage> *out_gui_messages);
  void LayoutScrollbar();

 public:
  /// @brief Constructor with explicit frames and no scrollbar (wheel and keys
  /// still scroll).
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel.
  /// @param tab_order Tab order of the panel, 0 keeps it out of Tab.
  /// @param font Font of the rows.
  /// @param color Color of the text.
  /// @param background Frame around the whole list.
  /// @param selection Frame drawn under the selected row.
  /// @param hover Frame drawn under the row beneath the cursor.
  ListBox(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, Rgba color, DecoratedFrame background,
      DecoratedFrame selection, DecoratedFrame hover);

  /// @brief Constructor using a theme, with a vertical scrollbar of the theme.
  ListBox(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Replaces all items; the selection is dropped.
  void SetItems(const std::vector<std::string> &items);

  /// @brief Appends an item.
  void AddItem(const std::string &item);

  /// @brief Removes an item; the selection follows the item it was on.
  void RemoveItem(Si32 index);

  /// @brief Removes every item.
  void ClearItems();

  /// @brief Returns the number of items.
  Si32 GetItemCount() const;

  /// @brief Returns an item, or an empty string for an index out of range.
  const std::string &GetItem(Si32 index) const;

  /// @brief Selects an item without messages; -1 clears the selection.
  void SetSelectedIndex(Si32 index);

  /// @brief Returns the selected index, -1 when nothing is selected.
  Si32 GetSelectedIndex() const;

  /// @brief Returns the selected item, or an empty string.
  const std::string &GetSelectedItem() const;

  /// @brief Sets the height of a row; the default is the line height of the
  /// font plus twice the padding.
  void SetRowHeight(Si32 row_height);

  /// @brief Returns the height of a row.
  Si32 GetRowHeight() const;

  /// @brief Scrolls so that the given item is the top row, as far as the
  /// list allows.
  void SetFirstVisible(Si32 index);

  /// @brief Returns the index of the top row.
  Si32 GetFirstVisible() const;

  /// @brief Returns how many rows fit into the list.
  Si32 GetVisibleRows() const;

  /// @brief Returns the height a list of this style needs to show the given
  /// number of rows, frame and padding included.
  Si32 HeightForRows(Si32 rows) const;

  /// @brief Returns the row under a point, -1 for none.
  /// @param relative_pos A point counted from the bottom-left corner of the list.
  Si32 ItemAt(Vec2Si32 relative_pos) const;

  /// @brief Returns the rectangle of a row relative to the list, false when
  /// the row is not on screen.
  bool GetRowRect(Si32 index, Vec2Si32 *out_pos, Vec2Si32 *out_size) const;

  /// @brief Sets the font of the rows.
  void SetFont(Font font);

  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetEnabled(bool is_enabled) override;
  bool IsEnabled() override;
  void RegenerateSprites() override;

  std::function<void(void)> OnSelectionChange = DoNothing;
  std::function<void(void)> OnItemClick = DoNothing;
  std::function<void(void)> OnItemActivate = DoNothing;

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief A list box takes every click inside it.
  bool TakesMouse() const override;
};

/// @brief A box showing the chosen item that opens a list of items to choose
/// from.
///
/// The list opens below the box (above it when there is no room below) and is
/// drawn over everything else in the tree; a click outside closes it. A choice
/// emits kGuiDropdownChange and calls OnChange when the item changed. When the
/// list is closed and the box is focused, the arrow keys change the item
/// directly, Enter or Space open the list; Escape closes it.
class Dropdown : public Panel {
 public:
  /// @brief Enumeration of dropdown states.
  enum DropdownState {
    kNormal = 0,
    kHovered = 1,
    kDown = 2,
    kDisabled = 3
  };

 protected:
  std::vector<std::string> items_;
  Si32 selected_ = -1;
  bool is_open_ = false;
  DropdownState state_ = kNormal;
  DecoratedFrame frame_[4];
  Sprite box_[4];
  Sprite arrow_;
  Font font_;
  std::vector<Rgba> palete_;
  std::vector<Rgba> disabled_palete_;
  std::shared_ptr<ListBox> list_;
  Si32 max_visible_items_ = 8;
  std::shared_ptr<GuiTheme> theme_;

  void ChooseAndNotify(Si32 index, std::deque<GuiMessage> *out_gui_messages);
  void OpenList(Vec2Si32 absolute_pos);
  void CloseList();
  Vec2Si32 ListParentPos(Vec2Si32 absolute_pos) const;
  void RefreshList();

 public:
  /// @brief Constructor with explicit frames and a list that has no scrollbar.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the box (the list takes the same width).
  /// @param tab_order Tab order of the panel, 0 keeps it out of Tab.
  /// @param font Font of the text.
  /// @param color Color of the text.
  /// @param normal Frame of the box when nothing happens.
  /// @param hovered Frame of the box under the cursor.
  /// @param down Frame of the box while the list is open.
  /// @param disabled Frame of a disabled box.
  /// @param list_background Frame around the list.
  /// @param list_selection Frame under the selected row.
  /// @param list_hover Frame under the row beneath the cursor.
  Dropdown(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, Rgba color,
      DecoratedFrame normal, DecoratedFrame hovered,
      DecoratedFrame down, DecoratedFrame disabled,
      DecoratedFrame list_background, DecoratedFrame list_selection,
      DecoratedFrame list_hover);

  /// @brief Constructor using a theme.
  Dropdown(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Replaces all items; the choice is dropped.
  void SetItems(const std::vector<std::string> &items);

  /// @brief Appends an item.
  void AddItem(const std::string &item);

  /// @brief Removes every item.
  void ClearItems();

  /// @brief Returns the number of items.
  Si32 GetItemCount() const;

  /// @brief Returns an item, or an empty string for an index out of range.
  const std::string &GetItem(Si32 index) const;

  /// @brief Chooses an item without messages; -1 means nothing chosen.
  void SetSelectedIndex(Si32 index);

  /// @brief Returns the chosen index, -1 when nothing is chosen.
  Si32 GetSelectedIndex() const;

  /// @brief Returns the chosen item, or an empty string.
  const std::string &GetSelectedItem() const;

  /// @brief Sets how many rows the open list shows before it scrolls.
  void SetMaxVisibleItems(Si32 max_visible_items);

  /// @brief Tells whether the list is open.
  bool IsOpen() const;

  /// @brief Closes the list if it is open.
  void Close();

  /// @brief Returns the list panel the dropdown opens; it is not a child of
  /// the tree and is positioned relative to the dropdown.
  std::shared_ptr<ListBox> GetList() const;

  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetEnabled(bool is_enabled) override;
  bool IsEnabled() override;
  void SetCurrentTab(bool is_current_tab) override;
  void RegenerateSprites() override;

  std::function<void(void)> OnChange = DoNothing;

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;
  bool TakesMouse() const override;
  void DrawOverlays(Vec2Si32 absolute_pos) override;
  void HandleOverlayInput(Vec2Si32 absolute_pos,
      const InputMessage &message,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;
  bool IsOverlayTransparentAt(Vec2Si32 absolute_pos,
      Vec2Si32 mouse_pos) override;
};

/// @brief A row of tab headers over a stack of pages, one page shown at a time.
///
/// AddTab() gives back the page, a panel that fills the control below the
/// headers; put the widgets of the tab into it. A click on a header or, when
/// focused, the Left and Right keys select a tab, which emits kGuiTabChange
/// and calls OnTabChange.
class TabControl : public Panel {
 protected:
  struct Tab {
    std::string title;
    std::shared_ptr<Panel> page;
    Si32 width = 0;
    Sprite header[3];  // normal, selected, hovered
  };
  std::vector<Tab> tabs_;
  Si32 selected_ = -1;
  Si32 hovered_ = -1;
  Si32 header_height_ = 24;
  Si32 header_padding_ = 8;  // text to header edge, horizontally
  Font font_;
  std::vector<Rgba> palete_;
  DecoratedFrame header_frame_[3];
  DecoratedFrame page_frame_;
  bool is_enabled_ = true;
  Ui64 next_page_tag_ = 0;
  std::shared_ptr<GuiTheme> theme_;

  Si32 HeaderAt(Vec2Si32 relative_pos) const;  // -1 when not on a header
  void LayoutTabs();
  void SelectAndNotify(Si32 index, std::deque<GuiMessage> *out_gui_messages);

 public:
  /// @brief Constructor with explicit frames.
  /// @param tag Unique tag for the panel.
  /// @param pos Position of the panel.
  /// @param size Size of the panel, headers included.
  /// @param tab_order Tab order of the panel, 0 keeps it out of Tab.
  /// @param font Font of the titles.
  /// @param color Color of the titles.
  /// @param header_normal Frame of a header that is not selected.
  /// @param header_selected Frame of the selected header.
  /// @param header_hovered Frame of a header under the cursor.
  /// @param page Frame of the page below the headers.
  /// @param header_height Height of the header row.
  TabControl(Ui64 tag, Vec2Si32 pos, Vec2Si32 size, Ui32 tab_order,
      Font font, Rgba color,
      DecoratedFrame header_normal, DecoratedFrame header_selected,
      DecoratedFrame header_hovered, DecoratedFrame page,
      Si32 header_height);

  /// @brief Constructor using a theme.
  TabControl(Ui64 tag, std::shared_ptr<GuiTheme> theme);

  /// @brief Adds a tab and returns its page; the first tab added is selected.
  /// @param title Text of the header.
  /// @param tag Tag for the page panel, 0 to derive one from the control's tag.
  std::shared_ptr<Panel> AddTab(const std::string &title, Ui64 tag = 0);

  /// @brief Removes a tab and its page; the selection moves to a neighbor.
  void RemoveTab(Si32 index);

  /// @brief Returns the number of tabs.
  Si32 GetTabCount() const;

  /// @brief Returns the page of a tab, or Panel::Invalid() for a bad index.
  std::shared_ptr<Panel> GetPage(Si32 index) const;

  /// @brief Returns the title of a tab, or an empty string.
  const std::string &GetTitle(Si32 index) const;

  /// @brief Changes the title of a tab.
  void SetTitle(Si32 index, const std::string &title);

  /// @brief Selects a tab without messages.
  void SetSelectedIndex(Si32 index);

  /// @brief Returns the selected tab, -1 when there are no tabs.
  Si32 GetSelectedIndex() const;

  /// @brief Returns the rectangle of a header relative to the control, false
  /// for a bad index.
  bool GetHeaderRect(Si32 index, Vec2Si32 *out_pos, Vec2Si32 *out_size) const;

  /// @brief Returns the rectangle the pages occupy, relative to the control.
  void GetPageRect(Vec2Si32 *out_pos, Vec2Si32 *out_size) const;

  /// @brief Sets the height of the header row.
  void SetHeaderHeight(Si32 header_height);

  void Draw(Vec2Si32 parent_absolute_pos) override;
  void SetEnabled(bool is_enabled) override;
  bool IsEnabled() override;
  void RegenerateSprites() override;

  std::function<void(void)> OnTabChange = DoNothing;

 protected:
  void HandleInput(Vec2Si32 parent_pos, const InputMessage &message,
      bool is_top_level,
      bool *in_out_is_applied,
      std::deque<GuiMessage> *out_gui_messages,
      std::shared_ptr<Panel> *out_current_tab) override;

  /// @brief The header row takes clicks; the page area is up to the pages.
  bool IsMouseTransparentAt(Vec2Si32 parent_pos, Vec2Si32 mouse_pos) override;
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

  /// @brief Creates a radio button in the given group.
  std::shared_ptr<RadioButton> MakeRadioButton(Si32 group = 0);

  /// @brief Creates an image panel the size of the sprite.
  std::shared_ptr<Image> MakeImage(Sprite sprite,
      Image::ScaleMode scale_mode = Image::kScaleStretch);

  /// @brief Creates a slider that grows to the right.
  std::shared_ptr<Slider> MakeHorizontalSlider();

  /// @brief Creates a slider that grows upward.
  std::shared_ptr<Slider> MakeVerticalSlider();

  /// @brief Creates a list box with a vertical scrollbar.
  std::shared_ptr<ListBox> MakeListBox();

  /// @brief Creates a dropdown.
  std::shared_ptr<Dropdown> MakeDropdown();

  /// @brief Creates a tab control without tabs.
  std::shared_ptr<TabControl> MakeTabControl();
};

/// @}

}  // namespace arctic


#endif  // ENGINE_GUI_H_
