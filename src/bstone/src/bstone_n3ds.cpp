/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Nintendo 3DS: buttons, Circle Pad and C-stick.

#ifdef __3DS__

#include "bstone_n3ds.h"

#include <cstdio>
#include <cstdlib>
#include <exception>

#include <3ds.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "id_in.h"

// libctru's main thread stack is 32 KiB by default: too small for BStone.
extern "C"
{
	u32 __stacksize__ = 1024 * 1024;
}

namespace bstone {
namespace n3ds {

namespace {

bool is_game_mode_ = false;
bool is_menu_quick_exit_ = false;
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

// An exception that nothing caught (e.g. before the game's own error
// handling is set up): show it instead of closing silently.
[[noreturn]] void on_terminate()
{
	auto message = "Unknown error.";

	try
	{
		const auto exception = std::current_exception();

		if (exception != nullptr)
		{
			std::rethrow_exception(exception);
		}
	}
	catch (const std::exception& exception)
	{
		message = exception.what();
	}
	catch (...)
	{
	}

	show_error(message);
	std::_Exit(1);
}

void initialize()
{
	osSetSpeedupEnable(true);
	SDL_SetMainReady();
	std::set_terminate(on_terminate);
}

void show_error(const char* message)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, nullptr);

	std::printf(
		"\n BStone error:\n\n%s\n\n"
		" Log: /3ds/bstone/bstone_log.txt\n\n"
		" START: exit\n",
		message != nullptr ? message : "");

	while (aptMainLoop())
	{
		hidScanInput();

		if ((hidKeysDown() & KEY_START) != 0)
		{
			break;
		}

		gspWaitForVBlank();
	}

	gfxExit();
}

bool is_game_mode() noexcept
{
	return is_game_mode_;
}

void set_game_mode(bool is_game_mode) noexcept
{
	is_game_mode_ = is_game_mode;
}

bool is_menu_quick_exit() noexcept
{
	return is_menu_quick_exit_;
}

void clear_menu_quick_exit() noexcept
{
	is_menu_quick_exit_ = false;
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

	// START in game opens the menu; in a menu it closes all of them.
	if (is_menu && (pressed & KEY_START) != 0)
	{
		is_menu_quick_exit_ = true;
	}

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

std::uint32_t* get_top_framebuffer()
{
	return reinterpret_cast<std::uint32_t*>(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr));
}

void present_top_framebuffer()
{
	GSPGPU_FlushDataCache(get_top_framebuffer(), 400 * 240 * 4);
	gfxScreenSwapBuffers(GFX_TOP, false);
	gspWaitForVBlank();
}

void wait_for_vblank()
{
	gspWaitForVBlank();
}

std::uint64_t get_milliseconds()
{
	return osGetTime();
}

namespace {

// Bottom screen: 320x240 RGBA8, rotated like the top one.
void fill_bottom(std::uint32_t* framebuffer, int x, int y, int width, int height, std::uint32_t color)
{
	for (auto i = x; i < x + width; ++i)
	{
		for (auto j = y; j < y + height; ++j)
		{
			framebuffer[i * 240 + (239 - j)] = color;
		}
	}
}

// 3x5 digits, as in the n3ds-ports hello world.
void draw_number(std::uint32_t* framebuffer, int number, int x, int y, int scale, std::uint32_t color)
{
	static constexpr std::uint8_t digits[10][5] =
	{
		{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
		{7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
	};

	char text[12];
	std::snprintf(text, sizeof(text), "%d", number);

	for (auto i = 0; text[i] != '\0'; ++i)
	{
		const auto& digit = digits[text[i] - '0'];

		for (auto row = 0; row < 5; ++row)
		{
			for (auto col = 0; col < 3; ++col)
			{
				if ((digit[row] & (4 >> col)) != 0)
				{
					fill_bottom(framebuffer, x + (i * 4 + col) * scale, y + row * scale, scale, scale, color);
				}
			}
		}
	}
}

int frame_count_ = 0;
std::uint64_t fps_time_ = 0;
std::uint64_t present_us_total_ = 0;

} // namespace

std::uint64_t get_microseconds()
{
	return svcGetSystemTick() / CPU_TICKS_PER_USEC;
}

void count_frame(int present_us)
{
	++frame_count_;
	present_us_total_ += static_cast<std::uint64_t>(present_us);

	const auto now = osGetTime();
	const auto elapsed = now - fps_time_;

	if (elapsed < 1000)
	{
		return;
	}

	const auto fps = static_cast<int>((frame_count_ * 1000) / elapsed);
	const auto frame_ms = static_cast<int>(elapsed / frame_count_);
	const auto present_ms = static_cast<int>(present_us_total_ / frame_count_ / 1000);
	frame_count_ = 0;
	present_us_total_ = 0;
	fps_time_ = now;

	// Frames per second (big, white), milliseconds per frame (green) and
	// milliseconds of present() per frame (yellow).
	const auto framebuffer = reinterpret_cast<std::uint32_t*>(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr));
	fill_bottom(framebuffer, 0, 0, 320, 240, 0x000000FFU);
	draw_number(framebuffer, fps, 40, 40, 16, 0xFFFFFFFFU);
	draw_number(framebuffer, frame_ms, 40, 160, 8, 0x80FF80FFU);
	draw_number(framebuffer, present_ms, 200, 160, 8, 0xFFFF40FFU);
	GSPGPU_FlushDataCache(framebuffer, 320 * 240 * 4);
	gfxScreenSwapBuffers(GFX_BOTTOM, false);
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
