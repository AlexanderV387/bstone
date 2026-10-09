/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Nintendo 3DS: buttons, Circle Pad and C-stick.

#ifdef __3DS__

#include "bstone_n3ds.h"

#include <cstdlib>

#include <3ds.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "id_in.h"

namespace bstone {
namespace n3ds {

namespace {

bool is_game_mode_ = false;
bool is_assigning_ = false;
u32 last_held_ = 0;

struct ButtonMap
{
	u32 mask;
	ScanCode own; // always
	ScanCode menu[2]; // only in menus
};

const ButtonMap button_maps[] =
{
	{KEY_A, ScanCode::sc_n3ds_a, {ScanCode::sc_return, ScanCode::sc_y}},
	{KEY_B, ScanCode::sc_n3ds_b, {ScanCode::sc_escape, ScanCode::sc_n}},
	{KEY_X, ScanCode::sc_n3ds_x, {ScanCode::sc_delete, ScanCode::sc_none}},
	{KEY_Y, ScanCode::sc_n3ds_y, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_L, ScanCode::sc_n3ds_l, {ScanCode::sc_page_up, ScanCode::sc_none}},
	{KEY_R, ScanCode::sc_n3ds_r, {ScanCode::sc_page_down, ScanCode::sc_none}},
	{KEY_ZL, ScanCode::sc_n3ds_zl, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_ZR, ScanCode::sc_n3ds_zr, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_SELECT, ScanCode::sc_n3ds_select, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_START, ScanCode::sc_escape, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_DUP, ScanCode::sc_up_arrow, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_DDOWN, ScanCode::sc_down_arrow, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_DLEFT, ScanCode::sc_left_arrow, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_DRIGHT, ScanCode::sc_right_arrow, {ScanCode::sc_none, ScanCode::sc_none}},
	{KEY_CPAD_UP, ScanCode::sc_none, {ScanCode::sc_up_arrow, ScanCode::sc_none}},
	{KEY_CPAD_DOWN, ScanCode::sc_none, {ScanCode::sc_down_arrow, ScanCode::sc_none}},
	{KEY_CPAD_LEFT, ScanCode::sc_none, {ScanCode::sc_left_arrow, ScanCode::sc_none}},
	{KEY_CPAD_RIGHT, ScanCode::sc_none, {ScanCode::sc_right_arrow, ScanCode::sc_none}},
};

void press_key(ScanCode key)
{
	if (key == ScanCode::sc_none)
	{
		return;
	}

	Keyboard[key] = true;
	LastScan = key;
}

void release_key(ScanCode key)
{
	if (key == ScanCode::sc_none)
	{
		return;
	}

	Keyboard[key] = false;
}

// Removes the dead zone and limits the range: returns -range..range.
int apply_dead_zone(int value, int dead_zone, int max_value)
{
	if (std::abs(value) <= dead_zone)
	{
		return 0;
	}

	value += (value > 0 ? -dead_zone : dead_zone);
	const auto range = max_value - dead_zone;

	if (value > range)
	{
		return range;
	}

	if (value < -range)
	{
		return -range;
	}

	return value;
}

// From 3d_play.cpp.
constexpr auto base_move = 35;
constexpr auto run_move = 70;

constexpr auto circle_pad_dead_zone = 32;
constexpr auto circle_pad_max = 150;
constexpr auto c_stick_dead_zone = 16;
constexpr auto c_stick_max = 146;

} // namespace

void initialize()
{
	osSetSpeedupEnable(true);
	SDL_SetMainReady();
}

bool is_game_mode() noexcept
{
	return is_game_mode_;
}

void set_game_mode(bool is_game_mode) noexcept
{
	is_game_mode_ = is_game_mode;
}

void set_assigning(bool is_assigning) noexcept
{
	is_assigning_ = is_assigning;
}

void handle_buttons()
{
	const auto held = hidKeysHeld();
	const auto pressed = held & ~last_held_;
	const auto released = last_held_ & ~held;
	last_held_ = held;

	const auto is_menu = !is_game_mode_ && !is_assigning_;

	for (const auto& map : button_maps)
	{
		// A release clears every key the button can give, so switching
		// between menu and game mode never leaves a key stuck.
		if ((released & map.mask) != 0)
		{
			release_key(map.own);
			release_key(map.menu[0]);
			release_key(map.menu[1]);
		}

		if ((pressed & map.mask) != 0)
		{
			press_key(map.own);

			if (is_menu)
			{
				press_key(map.menu[1]);
				press_key(map.menu[0]);
			}
		}
	}
}

void poll_analog(int tics, bool is_running, int& control_x, int& control_y, int& strafe)
{
	// Proportional to the tilt and to the time: the stick fully pushed
	// moves as fast as the D-pad (walking or running).
	const auto speed = tics * (is_running ? run_move : base_move);
	const auto pad_range = circle_pad_max - circle_pad_dead_zone;

	auto pad = circlePosition{};
	hidCircleRead(&pad);

	const auto pad_x = apply_dead_zone(pad.dx, circle_pad_dead_zone, circle_pad_max);
	const auto pad_y = apply_dead_zone(pad.dy, circle_pad_dead_zone, circle_pad_max);

	control_y -= (pad_y * speed) / pad_range;
	strafe += (pad_x * speed) / pad_range;

	// C-stick fully pushed turns at the running turn speed.
	auto stick = circlePosition{};
	hidCstickRead(&stick);

	const auto stick_range = c_stick_max - c_stick_dead_zone;
	const auto stick_x = apply_dead_zone(stick.dx, c_stick_dead_zone, c_stick_max);

	control_x += (stick_x * tics * run_move) / stick_range;
}

} // namespace n3ds
} // namespace bstone

#endif // __3DS__
