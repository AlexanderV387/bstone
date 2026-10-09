/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Nintendo 3DS: buttons, Circle Pad and C-stick.
//
// The 3DS buttons are scan codes (sc_n3ds_*), so the bindings menu assigns
// them and the configuration file saves them like keyboard keys.

#ifndef BSTONE_N3DS_INCLUDED
#define BSTONE_N3DS_INCLUDED

#ifdef __3DS__

#include <cstdint>

namespace bstone {

class CVarMgr;

namespace n3ds {

// What SDL's own 3DS main would do: 804 MHz on the New 3DS.
void initialize();

// When the data folder holds more than one game (*.BS1, *.BS6, *.VSI), a
// text menu on the top screen chooses one before SDL starts (BStone asks
// with a message box, which the 3DS does not have). Returns the option for
// it (--aog_sw, --aog or --ps), or nullptr when there is nothing to choose.
// START in the menu exits.
const char* choose_game();

// Appends a timed line to sdmc:/3ds/bstone/bstone_exit.txt (closing steps:
// the log is buffered and lost when the console is turned off).
void trace(const char* step);

// After a normal quit: with more than one game, the .cia relaunches itself
// to show the picker again (as in the Wolfenstein 3D port).
void on_quit();

// Menus (the default): A is Enter and Yes, B is Escape and No, X is Delete,
// L and R are Page Up and Page Down, the Circle Pad moves like the D-pad.
// In game every button is only its own bindable scan code.
// START is always Escape and the D-pad always gives the arrow keys.
bool is_game_mode() noexcept;
void set_game_mode(bool is_game_mode) noexcept;

// START pressed in a menu: leave the menus and go back to the game, from any
// submenu (as in the Wolfenstein 3D port). Cleared by the main menu.
bool is_menu_quick_exit() noexcept;
void clear_menu_quick_exit() noexcept;

// While a binding is being assigned, A and B are not Enter and Escape.
void set_assigning(bool is_assigning) noexcept;

// Shows a fatal error on the top screen until START is pressed: the 3DS has
// no message boxes, so without it the game would just close.
void show_error(const char* message);

// Turns button presses and releases into key events. Called after the SDL
// events are polled (SDL reads the buttons with hidScanInput).
void handle_buttons();

// Top screen framebuffer: 400x240 RGBA8 pixels (0xRRGGBBAA), stored rotated:
// screen pixel (x, y) is at [x * 240 + (239 - y)].
std::uint32_t* get_top_framebuffer();

// Shows the top framebuffer and waits for the vertical blank (SDL's
// software renderer on the 3DS ignores vsync).
void present_top_framebuffer();

void wait_for_vblank();

std::uint64_t get_milliseconds();
std::uint64_t get_microseconds();

// Time spent per frame in the parts of the 3D view, shown on the bottom
// screen: where the frame time goes.
enum ProfileSlot
{
	profile_walls,
	profile_planes, // floors and ceilings
	profile_sprites,
	profile_slot_count,
};

void add_profile_time(int slot, int us);

class ProfileScope
{
public:
	explicit ProfileScope(int slot) : slot_{slot}, start_{get_microseconds()} {}
	ProfileScope(const ProfileScope&) = delete;
	ProfileScope& operator=(const ProfileScope&) = delete;
	~ProfileScope() { add_profile_time(slot_, static_cast<int>(get_microseconds() - start_)); }

private:
	int slot_;
	std::uint64_t start_;
};

// Bottom screen: 320x240 pixels (0xRRGGBBAA), row by row. present_bottom()
// adds the frame counter, copies it rotated to the screen and swaps; without
// the HUD the buffer is cleared first. Called once per frame by the video.
constexpr auto bottom_width = 320;
constexpr auto bottom_height = 240;

std::uint32_t* get_bottom_buffer() noexcept;
void present_bottom(bool is_hud);

// Fills the bottom screen buffer with the HUD: the top bar (location and
// messages), the map of what was explored and the status bar, taken from
// the 320x200 UI buffer (palette indices, colors in framebuffer format).
void draw_bottom_hud(const std::uint8_t* ui, const std::uint32_t* colors);

// While true (the fizzle effects), the UI over the 3D view is stretched to
// the whole top screen instead of keeping its original place.
void set_ui_overlay_fullscreen(bool value) noexcept;
bool is_ui_overlay_fullscreen() noexcept;

// Counts a shown frame; the bottom screen shows the frames per second, the
// milliseconds per frame and how many of them present() took (composing
// and copying to the screen, without the vsync wait).
void count_frame(int present_us);

// Settings, saved in the configuration file (n3ds_* cvars).
void initialize_cvars(CVarMgr& cvar_mgr);

constexpr auto min_sensitivity = 1;
constexpr auto max_sensitivity = 10;

enum RunMode
{
	run_mode_stick, // the stick fully pushed runs; the button too
	run_mode_hold, // while the run button is held (default)
	run_mode_toggle, // press once: run until the player stops moving
};

bool is_dual_stick() noexcept;
void set_dual_stick(bool value);
int get_stick_sensitivity() noexcept; // C-stick turn speed, 1-10
void set_stick_sensitivity(int value);
int get_run_mode() noexcept;
void set_run_mode(int value);
bool is_touch_turning() noexcept; // drag on the touch screen to turn
void set_touch_turning(bool value);
int get_touch_speed() noexcept; // 1-10
void set_touch_speed(int value);
bool is_fps_shown() noexcept; // small frame counter on the bottom screen
void set_fps_shown(bool value);
bool is_hud_on_bottom() noexcept; // 3D view on the whole top screen
void set_hud_on_bottom(bool value);
bool is_status_bar_on_top() noexcept; // bottom screen HUD: status bar above the map
void set_status_bar_on_top(bool value);

// Call once per frame with the run binding (already inverted by "always
// run"); returns whether the player runs, for the D-pad too.
bool update_running(bool run_button);
bool is_running() noexcept;

// A tap on the touch screen turns the bottom screen off or on (in game with
// touch turning, a drag turns instead). Called by handle_buttons().
void poll_bottom_screen_toggle();

// Circle Pad: move and strafe (dual stick) or move and turn (classic).
// C-stick: turn. Touch screen drag: turn (optional).
void poll_analog(int tics, int& control_x, int& control_y, int& strafe);

} // namespace n3ds
} // namespace bstone

#endif // __3DS__

#endif // BSTONE_N3DS_INCLUDED
