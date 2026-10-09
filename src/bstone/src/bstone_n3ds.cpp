/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Nintendo 3DS: buttons, Circle Pad and C-stick.

#ifdef __3DS__

#include "bstone_n3ds.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <3ds.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "id_in.h"
#include "bstone_cvar.h"
#include "bstone_cvar_mgr.h"

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

void initialize_bottom_screen(); // below

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
	initialize_bottom_screen();
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

namespace {

struct GameChoice
{
	const char* name;
	const char* data_file;
	const char* option;
};

constexpr GameChoice game_choices[] =
{
	{"Aliens of Gold", "sdmc:/3ds/bstone/vswap.bs6", "--aog"},
	{"Aliens of Gold (shareware)", "sdmc:/3ds/bstone/vswap.bs1", "--aog_sw"},
	{"Planet Strike", "sdmc:/3ds/bstone/vswap.vsi", "--ps"},
};

constexpr auto game_choice_count = static_cast<int>(sizeof(game_choices) / sizeof(game_choices[0]));
constexpr auto last_game_path = "sdmc:/3ds/bstone/last-game.txt";

bool has_game_choice_ = false;

bool file_exists(const char* path)
{
	const auto file = std::fopen(path, "rb");

	if (file == nullptr)
	{
		return false;
	}

	std::fclose(file);
	return true;
}

} // namespace

const char* choose_game()
{
	int available[game_choice_count];
	auto count = 0;

	for (auto i = 0; i < game_choice_count; ++i)
	{
		if (file_exists(game_choices[i].data_file))
		{
			available[count++] = i;
		}
	}

	if (count <= 1)
	{
		return nullptr; // BStone finds the game (or reports that there is none)
	}

	has_game_choice_ = true;

	// Start on the last game played.
	auto selected = 0;
	auto last_game = -1;

	if (const auto file = std::fopen(last_game_path, "r"))
	{
		if (std::fscanf(file, "%d", &last_game) != 1)
		{
			last_game = -1;
		}

		std::fclose(file);
	}

	for (auto i = 0; i < count; ++i)
	{
		if (available[i] == last_game)
		{
			selected = i;
		}
	}

	gfxInitDefault();
	consoleInit(GFX_TOP, nullptr);

	auto drawn = -1;
	auto is_chosen = false;

	while (aptMainLoop())
	{
		hidScanInput();
		const auto down = hidKeysDown();

		if ((down & (KEY_DOWN | KEY_CPAD_DOWN)) != 0)
		{
			selected = (selected + 1) % count;
		}

		if ((down & (KEY_UP | KEY_CPAD_UP)) != 0)
		{
			selected = (selected + count - 1) % count;
		}

		if ((down & KEY_A) != 0)
		{
			is_chosen = true;
			break;
		}

		if ((down & KEY_START) != 0)
		{
			break;
		}

		if (selected != drawn)
		{
			consoleClear();
			std::printf("\n  BLAKE STONE\n\n  Choose a game:\n\n");

			for (auto i = 0; i < count; ++i)
			{
				std::printf("  %s %s\n\n", i == selected ? ">" : " ", game_choices[available[i]].name);
			}

			std::printf("\n  Up/Down: choose  A: start\n  START: back to the menu\n");
			drawn = selected;
		}

		gspWaitForVBlank();
	}

	gfxExit(); // SDL sets the screens up again

	if (!is_chosen)
	{
		std::exit(0);
	}

	const auto game = available[selected];

	// Written over in place ("r+"): a new file gets its space on the SD card
	// allocated anew, which can take seconds (the number is always 1 digit).
	auto file = std::fopen(last_game_path, "r+");

	if (file == nullptr)
	{
		file = std::fopen(last_game_path, "w");
	}

	if (file != nullptr)
	{
		std::fprintf(file, "%d\n", game);
		std::fclose(file);
	}

	return game_choices[game].option;
}

void on_quit()
{
	// Quit in the installed .cia goes back to the game picker: relaunch this
	// title when it exits. Not when closed from the HOME Menu, nor as a
	// .3dsx (relaunching would restart the Homebrew Launcher).
	if (has_game_choice_ && !envIsHomebrew() && !aptShouldClose())
	{
		aptSetChainloaderToSelf();
	}
}

namespace {

bool is_ui_overlay_fullscreen_ = false;

} // namespace

void set_ui_overlay_fullscreen(bool value) noexcept
{
	is_ui_overlay_fullscreen_ = value;
}

bool is_ui_overlay_fullscreen() noexcept
{
	return is_ui_overlay_fullscreen_;
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

	poll_bottom_screen_toggle();

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

// Closed from the HOME Menu: the screens are no longer ours and a vblank wait
// could block for a long time; skip them so the game shuts down at once.
bool is_closing()
{
	return aptShouldClose();
}

void present_top_framebuffer()
{
	if (is_closing())
	{
		return;
	}

	GSPGPU_FlushDataCache(get_top_framebuffer(), 400 * 240 * 4);
	gfxScreenSwapBuffers(GFX_TOP, false);
	gspWaitForVBlank();
}

void wait_for_vblank()
{
	if (!is_closing())
	{
		gspWaitForVBlank();
	}
}

std::uint64_t get_milliseconds()
{
	return osGetTime();
}

namespace {

bool is_bottom_on_ = true;
bool was_touching_ = false;
aptHookCookie apt_hook_cookie_{};

void set_bottom_backlight(bool is_on)
{
	if (R_FAILED(gspLcdInit()))
	{
		return;
	}

	if (is_on)
	{
		GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTTOM);
	}
	else
	{
		GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);
	}

	gspLcdExit();
}

// The bottom screen is composed in this buffer (row by row) and copied
// rotated to its framebuffer once per frame.
std::uint32_t bottom_buffer_[bottom_width * bottom_height];

bool is_bottom_hud_shown_ = false;
int applied_bottom_mode_ = -1;

void fill_bottom(std::uint32_t* buffer, int x, int y, int width, int height, std::uint32_t color)
{
	for (auto j = y; j < y + height; ++j)
	{
		for (auto i = x; i < x + width; ++i)
		{
			buffer[j * bottom_width + i] = color;
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
int fps_ = 0;
int frame_ms_ = 0;
int present_ms_ = 0;
int profile_ms_[profile_slot_count] = {};
std::uint64_t present_us_total_ = 0;
std::uint64_t profile_us_totals_[profile_slot_count] = {};

} // namespace

std::uint64_t get_microseconds()
{
	return svcGetSystemTick() / CPU_TICKS_PER_USEC;
}

void add_profile_time(int slot, int us)
{
	profile_us_totals_[slot] += static_cast<std::uint64_t>(us);
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

	if (!is_fps_shown())
	{
		frame_count_ = 0;
		present_us_total_ = 0;

		for (auto& total : profile_us_totals_)
		{
			total = 0;
		}

		fps_time_ = now;
		return;
	}

	fps_ = static_cast<int>((frame_count_ * 1000) / elapsed);
	frame_ms_ = static_cast<int>(elapsed / frame_count_);
	present_ms_ = static_cast<int>(present_us_total_ / frame_count_ / 1000);

	for (auto i = 0; i < profile_slot_count; ++i)
	{
		profile_ms_[i] = static_cast<int>(profile_us_totals_[i] / frame_count_ / 1000);
		profile_us_totals_[i] = 0;
	}

	frame_count_ = 0;
	present_us_total_ = 0;
	fps_time_ = now;
}

std::uint32_t* get_bottom_buffer() noexcept
{
	return bottom_buffer_;
}

void present_bottom(bool is_hud)
{
	if (is_closing())
	{
		return;
	}

	is_bottom_hud_shown_ = is_hud;

	// With the original HUD (everything on the top screen) the bottom screen
	// has nothing to show: turn it off, unless the FPS counter is on. Applied
	// when the setting changes (and at startup); a tap still turns it on.
	const auto bottom_mode = is_hud_on_bottom() ? 1 : (is_fps_shown() ? 2 : 0);

	if (bottom_mode != applied_bottom_mode_)
	{
		applied_bottom_mode_ = bottom_mode;
		const auto is_on = bottom_mode != 0;

		if (is_on != is_bottom_on_)
		{
			is_bottom_on_ = is_on;
			set_bottom_backlight(is_on);
		}
	}

	if (!is_hud)
	{
		fill_bottom(bottom_buffer_, 0, 0, bottom_width, bottom_height, 0x000000FFU);
	}

	if (is_fps_shown())
	{
		// Small, in the right corner of the map area (between the bars):
		// frames per second (white) and milliseconds per frame (green).
		const auto y = is_status_bar_on_top() ? 50 : 18;
		fill_bottom(bottom_buffer_, 256, y, 62, 13, 0x000000FFU);
		draw_number(bottom_buffer_, fps_, 259, y + 2, 2, 0xFFFFFFFFU);
		draw_number(bottom_buffer_, frame_ms_, 289, y + 2, 2, 0x80FF80FFU);
	}

	// The HUD keeps the bottom screen on.
	if (is_hud && !is_bottom_on_)
	{
		is_bottom_on_ = true;
		set_bottom_backlight(true);
	}

	// Rotated copy in 8x8 blocks, as for the top screen.
	const auto framebuffer = reinterpret_cast<std::uint32_t*>(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr));
	constexpr auto block = 8;

	for (auto bx = 0; bx < bottom_width; bx += block)
	{
		for (auto by = 0; by < bottom_height; by += block)
		{
			for (auto x = bx; x < bx + block; ++x)
			{
				auto dst = &framebuffer[x * bottom_height + (bottom_height - 1 - by)];
				auto src = &bottom_buffer_[by * bottom_width + x];

				for (auto y = 0; y < block; ++y)
				{
					*dst-- = *src;
					src += bottom_width;
				}
			}
		}
	}

	GSPGPU_FlushDataCache(framebuffer, bottom_width * bottom_height * 4);
	gfxScreenSwapBuffers(GFX_BOTTOM, false);
}

// ==========================================================================
// Settings (saved in the configuration file like the rest of BStone's)

namespace {

auto dual_stick_cvar = CVar{CVarBoolTag{}, StringView{"n3ds_dual_stick"}, CVarFlags::archive, true};

auto stick_sensitivity_cvar = CVar{
	CVarInt32Tag{}, StringView{"n3ds_stick_sensitivity"}, CVarFlags::archive,
	5, min_sensitivity, max_sensitivity};

auto run_mode_cvar = CVar{
	CVarInt32Tag{}, StringView{"n3ds_run_mode"}, CVarFlags::archive,
	run_mode_hold, run_mode_stick, run_mode_toggle};

auto touch_turning_cvar = CVar{CVarBoolTag{}, StringView{"n3ds_touch_turning"}, CVarFlags::archive, false};

auto touch_speed_cvar = CVar{
	CVarInt32Tag{}, StringView{"n3ds_touch_speed"}, CVarFlags::archive,
	4, min_sensitivity, max_sensitivity};

auto show_fps_cvar = CVar{CVarBoolTag{}, StringView{"n3ds_fps_counter"}, CVarFlags::archive, false};

auto status_bar_on_top_cvar = CVar{CVarBoolTag{}, StringView{"n3ds_status_bar_on_top"}, CVarFlags::archive, true};

auto hud_on_bottom_cvar = CVar{CVarBoolTag{}, StringView{"n3ds_hud_on_bottom"}, CVarFlags::archive, true};

} // namespace

void initialize_cvars(CVarMgr& cvar_mgr)
{
	cvar_mgr.add(dual_stick_cvar);
	cvar_mgr.add(stick_sensitivity_cvar);
	cvar_mgr.add(run_mode_cvar);
	cvar_mgr.add(touch_turning_cvar);
	cvar_mgr.add(touch_speed_cvar);
	cvar_mgr.add(show_fps_cvar);
	cvar_mgr.add(hud_on_bottom_cvar);
	cvar_mgr.add(status_bar_on_top_cvar);
}

bool is_dual_stick() noexcept { return dual_stick_cvar.get_bool(); }
void set_dual_stick(bool value) { dual_stick_cvar.set_bool(value); }
int get_stick_sensitivity() noexcept { return stick_sensitivity_cvar.get_int32(); }
void set_stick_sensitivity(int value) { stick_sensitivity_cvar.set_int32(value); }
int get_run_mode() noexcept { return run_mode_cvar.get_int32(); }
void set_run_mode(int value) { run_mode_cvar.set_int32(value); }
bool is_touch_turning() noexcept { return touch_turning_cvar.get_bool(); }
void set_touch_turning(bool value) { touch_turning_cvar.set_bool(value); }
int get_touch_speed() noexcept { return touch_speed_cvar.get_int32(); }
void set_touch_speed(int value) { touch_speed_cvar.set_int32(value); }
bool is_fps_shown() noexcept { return show_fps_cvar.get_bool(); }
bool is_hud_on_bottom() noexcept { return hud_on_bottom_cvar.get_bool(); }
void set_hud_on_bottom(bool value) { hud_on_bottom_cvar.set_bool(value); }
bool is_status_bar_on_top() noexcept { return status_bar_on_top_cvar.get_bool(); }
void set_status_bar_on_top(bool value) { status_bar_on_top_cvar.set_bool(value); }
void set_fps_shown(bool value) { show_fps_cvar.set_bool(value); }

// ==========================================================================
// Running

namespace {

bool is_running_ = false;
bool was_run_button_ = false;
bool is_run_latched_ = false;
bool was_moving_since_latch_ = false;

bool is_moving()
{
	if ((hidKeysHeld() & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT)) != 0)
	{
		return true;
	}

	auto pad = circlePosition{};
	hidCircleRead(&pad);
	return std::abs(pad.dx) > circle_pad_dead_zone || std::abs(pad.dy) > circle_pad_dead_zone;
}

} // namespace

bool update_running(bool run_button)
{
	const auto is_pressed = run_button && !was_run_button_;
	was_run_button_ = run_button;

	if (get_run_mode() != run_mode_toggle)
	{
		// Stick fully pushed: the tilt itself reaches the running speed
		// (poll_analog); the button also runs. Hold: while held.
		is_run_latched_ = false;
		is_running_ = run_button;
		return is_running_;
	}

	// Press once: run at full speed until the player stops moving, like the
	// sprint of modern shooters.
	if (is_pressed)
	{
		is_run_latched_ = true;
		was_moving_since_latch_ = false;
	}

	if (is_run_latched_)
	{
		if (is_moving())
		{
			was_moving_since_latch_ = true;
		}
		else if (was_moving_since_latch_)
		{
			is_run_latched_ = false;
		}
	}

	is_running_ = is_run_latched_;
	return is_running_;
}

bool is_running() noexcept
{
	return is_running_;
}

// ==========================================================================
// Bottom screen: a tap turns its backlight off (to save battery) or on.
//
// The gsp::Lcd session is opened only around each call: kept open, it froze
// the HOME Menu in the Wolfenstein 3D port.

namespace {

void (*on_suspend_)() = nullptr;

// Never leave for the HOME Menu or sleep mode with the bottom screen off.
void on_apt_hook(APT_HookType hook, void*)
{
	// Leaving for the HOME Menu, from where the game may be closed.
	if (hook == APTHOOK_ONSUSPEND && on_suspend_ != nullptr)
	{
		on_suspend_();
	}

	if (is_bottom_on_)
	{
		return;
	}

	switch (hook)
	{
		case APTHOOK_ONSUSPEND:
		case APTHOOK_ONSLEEP:
		case APTHOOK_ONEXIT:
			set_bottom_backlight(true);
			break;

		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			set_bottom_backlight(false);
			break;

		default:
			break;
	}
}

void restore_bottom_screen()
{
	if (!is_bottom_on_)
	{
		set_bottom_backlight(true);
	}
}

} // namespace

void set_on_suspend(void (*callback)())
{
	on_suspend_ = callback;
}

void initialize_bottom_screen()
{
	aptHook(&apt_hook_cookie_, on_apt_hook, nullptr);
	std::atexit(restore_bottom_screen);
}

void poll_bottom_screen_toggle()
{
	const auto is_touching = (hidKeysHeld() & KEY_TOUCH) != 0;

	// With touch turning, a drag in game turns instead; with the HUD on the
	// bottom screen, a tap does not turn it off.
	if (is_touching && !was_touching_ &&
		!(is_touch_turning() && is_game_mode_) &&
		!(is_bottom_hud_shown_ && is_bottom_on_))
	{
		is_bottom_on_ = !is_bottom_on_;
		set_bottom_backlight(is_bottom_on_);
	}

	was_touching_ = is_touching;
}

// ==========================================================================
// Analog movement

namespace {

bool was_touch_turning_ = false;
int last_touch_x_ = 0;

} // namespace

void poll_analog(int tics, int& control_x, int& control_y, int& strafe)
{
	// Proportional to the tilt and to the time. Fully pushed: the walking or
	// running speed of the D-pad; in the "stick" run mode, always running.
	const auto is_fast = is_running_ || get_run_mode() == run_mode_stick;
	const auto speed = tics * (is_fast ? run_move : base_move);
	const auto pad_range = circle_pad_max - circle_pad_dead_zone;

	auto pad = circlePosition{};
	hidCircleRead(&pad);

	// Radial dead zone and range: the Circle Pad moves in a circle, so in a
	// diagonal each axis only reaches ~70%. Scale the direction by the tilt
	// so that fully pushed is full speed at any angle.
	auto pad_x = 0;
	auto pad_y = 0;
	const auto pad_length = std::sqrt(static_cast<float>(pad.dx * pad.dx + pad.dy * pad.dy));

	if (pad_length > circle_pad_dead_zone)
	{
		const auto tilt = std::min(pad_length - circle_pad_dead_zone, static_cast<float>(pad_range));
		pad_x = static_cast<int>((pad.dx * tilt) / pad_length);
		pad_y = static_cast<int>((pad.dy * tilt) / pad_length);
	}

	control_y -= (pad_y * speed) / pad_range;

	// C-stick sensitivity 5: fully pushed turns at the running turn speed.
	const auto turn_speed = (tics * run_move * get_stick_sensitivity()) / 5;

	if (is_dual_stick())
	{
		// Circle Pad moves and strafes, C-stick turns.
		strafe += (pad_x * speed) / pad_range;

		auto stick = circlePosition{};
		hidCstickRead(&stick);

		const auto stick_range = c_stick_max - c_stick_dead_zone;
		const auto stick_x = apply_dead_zone(stick.dx, c_stick_dead_zone, c_stick_max);

		control_x += (stick_x * turn_speed) / stick_range;
	}
	else
	{
		// Classic: the Circle Pad moves and turns, like the original.
		control_x += (pad_x * turn_speed) / pad_range;
	}

	// Touch turning: a horizontal drag turns, with its own speed.
	if (is_touch_turning() && (hidKeysHeld() & KEY_TOUCH) != 0)
	{
		auto touch = touchPosition{};
		hidTouchRead(&touch);

		if (was_touch_turning_)
		{
			control_x += (touch.px - last_touch_x_) * 3 * get_touch_speed();
		}

		last_touch_x_ = touch.px;
		was_touch_turning_ = true;
	}
	else
	{
		was_touch_turning_ = false;
	}
}

} // namespace n3ds
} // namespace bstone

#endif // __3DS__
