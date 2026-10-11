/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Keyboard on the 3DS bottom screen, hidden behind a tab in its top left
// corner: save names, cheats and debug keys, as on a PC keyboard. The
// system's keyboard is an applet that stops the game and takes the screens.
//
// The same module is in the Wolfenstein 3D and Blake Stone ports: the game
// gives it the touches, turns its keys into key presses and lends it a way
// to fill rectangles on the bottom screen.

#ifndef BSTONE_N3DS_KEYBOARD_INCLUDED
#define BSTONE_N3DS_KEYBOARD_INCLUDED

#ifdef __3DS__

namespace n3ds_keyboard
{
	// Keys: letters (uppercase), digits, ' ', '-' and '.' are themselves.
	enum
	{
		key_backspace = 0x100,
		key_enter,
		key_tab,
		key_shift,
		key_ctrl,
		key_alt,
		key_f10,
	};

	// Presses (down) and releases (!down) a key in the game.
	typedef void (*SendKey) (int key, bool down);

	// Fills a rectangle of the bottom screen (320x240) with 0xRRGGBB.
	typedef void (*Fill) (int x, int y, int w, int h, unsigned rgb);

	void SetSendKey (SendKey send);

	// Every time the buttons are read: the touch screen, held or not, and
	// where. Returns true when the touch is the keyboard's (it began on the
	// tab or the keyboard is open): the game must not use it.
	bool Update (bool touching, int x, int y);

	// The touch being held is the keyboard's (see Update).
	bool OwnsTouch ();

	bool IsOpen ();
	void Open (bool open);

	// The open keyboard covers the whole bottom screen; closed, only the
	// tab is drawn, at the left edge, tab_y rows from the top.
	void Draw (Fill fill, int tab_y);
}

#endif // __3DS__

#endif // BSTONE_N3DS_KEYBOARD_INCLUDED
