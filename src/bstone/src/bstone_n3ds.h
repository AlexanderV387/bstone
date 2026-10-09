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
namespace n3ds {

// What SDL's own 3DS main would do: 804 MHz on the New 3DS.
void initialize();

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

// Counts a shown frame; the bottom screen shows the frames per second, the
// milliseconds per frame and how many of them present() took (composing
// and copying to the screen, without the vsync wait).
void count_frame(int present_us);

// Circle Pad: move and strafe. C-stick: turn.
void poll_analog(int tics, bool is_running, int& control_x, int& control_y, int& strafe);

} // namespace n3ds
} // namespace bstone

#endif // __3DS__

#endif // BSTONE_N3DS_INCLUDED
