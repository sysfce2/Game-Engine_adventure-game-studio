//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_GUIMAIN_H
#define __AC_GUIMAIN_H

#include <vector>
#include "ac/common_defines.h" // TODO: split out gui drawing helpers
#include "gfx/gfx_def.h" // TODO: split out gui drawing helpers
#include "gui/guidefines.h"
#include "util/error.h"
#include "util/geometry.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

class SplitLines;
namespace AGS
{
namespace Common
{

class Bitmap;
class GUIObject;


class GUIMain
{
public:
    GUIMain();

    void    InitDefaults();

    // Tells if GUI will react on clicking on it
    bool    IsClickable() const { return (_flags & kGUIMain_Clickable) != 0; }
    // Tells if GUI's visibility is overridden and it won't be displayed on
    // screen regardless of Visible property (until concealed mode is off).
    bool    IsConcealed() const { return (_flags & kGUIMain_Concealed) != 0; }
    // Tells if gui is actually meant to be displayed on screen.
    // Normally Visible property determines whether GUI is allowed to be seen,
    // but there may be other settings that override it.
    bool    IsDisplayed() const { return IsVisible() && !IsConcealed(); }
    // Tells if given coordinates are within interactable area of gui
    // NOTE: this currently tests for actual visibility and Clickable property
    bool    IsInteractableAt(int x, int y) const;
    // Tells if gui is a text window
    bool    IsTextWindow() const { return (_flags & kGUIMain_TextWindow) != 0; }
    // Tells if GUI is *allowed* to be displayed and interacted with.
    // This does not necessarily mean that it is displayed right now, because
    // GUI may be hidden for other reasons, including overriding behavior.
    // For example GUI with kGUIPopupMouseY style will not be shown unless
    // mouse cursor is at certain position on screen.
    bool    IsVisible() const { return (_flags & kGUIMain_Visible) != 0; }

    // Returns GUI's graphic space params
    inline const GraphicSpace &GetGraphicSpace() const { return _gs; }

    // Tells if GUI has graphically changed recently
    bool    HasChanged() const { return _hasChanged; }
    bool    HasControlsChanged() const { return _hasControlsChanged; }
    // Manually marks GUI as graphically changed.
    // NOTE: this only matters if GUI's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    void    MarkChanged();
    // Marks GUI as having any of its controls changed its looks.
    void    MarkControlChanged();
    // Clears changed flag
    void    ClearChanged();
    // Notify GUI about any of its controls changing its location.
    void    NotifyControlPosition();
    // Notify GUI about one of its controls changing its interactive state.
    void    NotifyControlState(int objid, bool mark_changed);
    // Resets control-under-mouse detection.
    void    ResetOverControl();

    // Finds a control under given screen coordinates, returns control's child ID.
    // Optionally allows extra leeway (offset in all directions) to let the user grab tiny controls.
    // Optionally only allows clickable controls, ignoring non-clickable ones.
    int32_t FindControlAt(int atx, int aty, int leeway = 0, bool must_be_clickable = true) const;
    // Gets the number of the GUI child controls
    int32_t GetControlCount() const;
    // Gets control by its child's index
    GUIObject *GetControl(int index) const;
    // Gets child control's type, looks up with child's index
    GUIControlType GetControlType(int index) const;
    // Gets child control's global ID, looks up with child's index
    int32_t GetControlID(int index) const;
    // Gets an array of child control indexes in the z-order, from bottom to top
    const std::vector<int> &GetControlsDrawOrder() const;

    // Child control management
    // Note that currently GUIMain does not own controls (should not delete them)
    void    AddControl(GUIControlType type, int id, GUIObject *control);
    void    RemoveAllControls();

    // Operations
    bool    BringControlToFront(int index);
    void    DrawSelf(Bitmap *ds);
    void    DrawWithControls(Bitmap *ds);
    // Polls GUI state, providing current cursor (mouse) coordinates
    void    Poll(int mx, int my);
    HError  RebuildArray();
    void    ResortZOrder();
    bool    SendControlToBack(int index);
    // Sets GUI position
    void    SetAt(int x, int y);
    // Sets whether GUI should react to player clicking on it
    void    SetClickable(bool on);
    // Override GUI visibility; when in concealed mode GUI won't show up
    // even if Visible = true
    void    SetConceal(bool on);
    // Attempts to change control's zorder; returns if zorder changed
    bool    SetControlZOrder(int index, int zorder);
    // Sets GUI rotation, in degrees
    void    SetRotation(float degrees);
    // Sets GUI size
    void    SetSize(int w, int h);
    // Changes GUI style to the text window or back
    void    SetTextWindow(bool on);
    // Sets GUI transparency as a percentage (0 - 100) where 100 = invisible
    void    SetTransparencyAsPercentage(int percent);
    // Sets whether GUI is allowed to be displayed on screen
    void    SetVisible(bool on);

    // Events
    void    OnMouseButtonDown(int mx, int my);
    void    OnMouseButtonUp();
  
    // Serialization
    void    ReadFromFile(Stream *in, GuiVersion gui_version);
    void    WriteToFile(Stream *out) const;
    // TODO: move to engine, into gui savegame component unit
    // (should read/write GUI properties accessing them by interface)
    void    ReadFromSavegame(Stream *in, GuiSvgVersion svg_version);
    void    WriteToSavegame(Stream *out) const;

private:
    void    DrawBlob(Bitmap *ds, int x, int y, color_t draw_color);
    // Same as FindControlAt but expects local space coordinates
    int32_t FindControlAtLocal(int atx, int aty, int leeway, bool must_be_clickable) const;

    // Recalculate graphic space using current object properties
    void    UpdateGraphicSpace();

    // TODO: all members are currently public; hide them later
public:
    int32_t ID;             // GUI identifier
    String  Name;           // the name of the GUI

    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
    color_t BgColor;        // background color
    int32_t BgImage;        // background sprite index
    color_t FgColor;        // foreground color (used as border color in normal GUIs,
                            // and text color in text windows)
    int32_t Padding;        // padding surrounding a GUI text window
    GUIPopupStyle PopupStyle; // GUI popup behavior
    int32_t PopupAtMouseY;  // popup when mousey < this
    int32_t Transparency;   // "incorrect" alpha (in legacy 255-range units)
    Common::BlendMode BlendMode; // render blend mode
    float Rotation;         // rotation, in degrees
    int32_t ZOrder;

    int32_t FocusCtrl;      // which control has the focus
    int32_t HighlightCtrl;  // which control has the bounding selection rect
    int32_t MouseOverCtrl;  // which control has the mouse cursor over it
    int32_t MouseDownCtrl;  // which control has the mouse button pressed on it
    Point   MouseWasAt;     // last mouse cursor position

    String  OnClickHandler; // script function name

private:
    int32_t _flags;         // style and behavior flags
    GraphicSpace _gs;
    bool    _hasChanged;    // flag tells whether GUI has graphically changed recently
    bool    _hasControlsChanged;
    bool    _polling;       // inside the polling process

    // Array of types and control indexes in global GUI object arrays;
    // maps GUI child slots to actual controls and used for rebuilding Controls array
    typedef std::pair<GUIControlType, int32_t> ControlRef;
    std::vector<ControlRef> _ctrlRefs;
    // Array of child control references (not exclusively owned!)
    std::vector<GUIObject*> _controls;
    // Sorted array of controls in z-order.
    std::vector<int32_t>    _ctrlDrawOrder;
};


namespace GUI
{
    extern GuiVersion GameGuiVersion;
    extern GuiOptions Options;

    // Applies current text direction setting (may depend on multiple factors)
    String ApplyTextDirection(const String &text);
    // Calculates the text's draw position, given the alignment
    // optionally returns the real graphical rect that the text would occupy
    Point CalcTextPosition(const char *text, int font, const Rect &frame, FrameAlignment align, Rect *gr_rect = nullptr);
    // Calculates the text's draw position and horizontal extent,
    // using strictly horizontal alignment
    Line CalcTextPositionHor(const char *text, int font, int x1, int x2, int y, FrameAlignment align);
    // Calculates the graphical rect that the text would occupy
    // if drawn at the given coordinates
    Rect CalcTextGraphicalRect(const char *text, int font, const Point &at);
    // Calculates the graphical rect that the text would occupy
    // if drawn aligned to the given frame
    Rect CalcTextGraphicalRect(const char *text, int font, const Rect &frame, FrameAlignment align);
    // Calculates a vertical graphical extent for a given font,
    // which is a top and bottom offsets in zero-based coordinates.
    // NOTE: this applies font size fixups.
    Line CalcFontGraphicalVExtent(int font);
    // Draw standart "shading" effect over rectangle
    void DrawDisabledEffect(Bitmap *ds, const Rect &rc);
    // Draw text aligned inside rectangle
    void DrawTextAligned(Bitmap *ds, const char *text, int font, color_t text_color, const Rect &frame, FrameAlignment align);
    // Draw text aligned horizontally inside given bounds
    void DrawTextAlignedHor(Bitmap *ds, const char *text, int font, color_t text_color, int x1, int x2, int y, FrameAlignment align);

    // Parses the string and returns combination of label macro flags
    GUILabelMacro FindLabelMacros(const String &text);
    // Applies text transformation necessary for rendering, in accordance to the
    // current game settings, such as right-to-left render, and anything else
    String TransformTextForDrawing(const String &text, bool translate, bool apply_direction);
    // Wraps given text to make it fit into width, stores it in the lines;
    // apply_direction param tells whether text direction setting should be applied
    size_t SplitLinesForDrawing(const char *text, bool apply_direction, SplitLines &lines, int font, int width, size_t max_lines = -1);

    // Mark all existing GUI for redraw
    void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl);
    // Mark all translatable GUI controls for redraw
    void MarkForTranslationUpdate();
    // Mark all GUI which use the given font for recalculate/redraw;
    // pass -1 to update all the textual controls together
    void MarkForFontUpdate(int font);
    // Mark labels that acts as special text placeholders for redraw
    void MarkSpecialLabelsForUpdate(GUILabelMacro macro);
    // Mark inventory windows for redraw, optionally only ones linked to given character;
    // also marks buttons with inventory icon mode
    void MarkInventoryForUpdate(int char_id, bool is_player);

    // Reads all GUIs and their controls.
    // WARNING: the data is read into the global arrays (guis, guibuts, and so on)
    HError ReadGUI(Stream *in);
    // Writes all GUIs and their controls.
    // WARNING: the data is written from the global arrays (guis, guibuts, and so on)
    void WriteGUI(Stream *out);

    // Rebuilds GUIs, connecting them to the child controls in memory.
    // WARNING: the data is processed in the global arrays (guis, guibuts, and so on)
    HError RebuildGUI();
}

} // namespace Common
} // namespace AGS

extern std::vector<Common::GUIMain> guis;
// Tells if all controls are disabled
// TODO: investigate how this variable works, and if this is at all needed
extern int all_buttons_disabled;
extern int gui_inv_pic;

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);

// This function has distinct implementations in Engine and Editor
extern void draw_gui_sprite(Common::Bitmap *ds, int spr, int x, int y,
                            Common::BlendMode blend_mode = Common::kBlend_Normal);
extern void draw_gui_sprite(Common::Bitmap *ds, int x, int y,
                            Common::Bitmap *image,
                            Common::BlendMode blend_mode, int alpha);

extern void draw_gui_sprite_flipped(Common::Bitmap *ds, int pic, int x, int y, Common::BlendMode blend_mode, bool is_flipped);
extern void draw_gui_sprite_flipped(Common::Bitmap *ds, int x, int y,
    Common::Bitmap *image, Common::BlendMode blend_mode, int alpha, bool is_flipped);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx);

extern void set_our_eip(int eip);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

#endif // __AC_GUIMAIN_H
