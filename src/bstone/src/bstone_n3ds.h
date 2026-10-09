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

// While a binding is being assigned, A and B are not Enter and Escape.
void set_assigning(bool is_assigning) noexcept;

// Turns button presses and releases into key events. Called after the SDL
// events are polled (SDL reads the buttons with hidScanInput).
void handle_buttons();

// Circle Pad: move and strafe. C-stick: turn.
void poll_analog(int tics, bool is_running, int& control_x, int& control_y, int& strafe);

} // namespace n3ds
} // namespace bstone

#endif // __3DS__

#endif // BSTONE_N3DS_INCLUDED
