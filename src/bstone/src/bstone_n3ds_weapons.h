/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Weapon buttons on the 3DS bottom screen: a row of numbers at its bottom,
// as the PC's number keys. A touch on one holds that number's key while
// the finger is on it. The weapons held are white, the one in hand on
// gray, the others dark.
//
// The same module is in the Wolfenstein 3D and Blake Stone ports, as the
// keyboard: the game gives it the touches, turns its keys into key presses
// and lends it a way to fill rectangles on the bottom screen.

#ifndef BSTONE_N3DS_WEAPONS_INCLUDED
#define BSTONE_N3DS_WEAPONS_INCLUDED

namespace n3ds_weapons
{
    // Rows the row takes, at the bottom of the bottom screen.
    const int height = 24;

    // Presses (down) and releases (!down) a key ('1' to '9') in the game.
    typedef void (*SendKey) (int key, bool down);

    // Fills a rectangle of the bottom screen (320x240) with 0xRRGGBB.
    typedef void (*Fill) (int x, int y, int w, int h, unsigned rgb);

    void SetSendKey (SendKey send);

    // Draws the row at row y: numbers 1 to slots, white those in the bits
    // of owned, in_hand's (0 based, or -1) on gray. Where it is drawn is
    // where Update looks for touches.
    void Draw (Fill fill, int y, int slots, unsigned owned, int in_hand);

    // Not on the bottom screen (no game being played): no touch is its.
    void Hide ();

    // Every time the buttons are read: the touch screen, held or not, and
    // where. Returns true when the touch is the row's: the game must not
    // use it.
    bool Update (bool touching, int x, int y);

    // The touch being held is the row's (see Update).
    bool OwnsTouch ();
}

#endif
