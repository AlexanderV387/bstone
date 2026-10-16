/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Weapon buttons on the 3DS bottom screen (see bstone_n3ds_weapons.h).

#include "bstone_n3ds_weapons.h"

namespace n3ds_weapons
{
    namespace
    {
        // 5x7 digits 1 to 9: the rows' 5 low bits, the leftmost pixel the
        // highest (the keyboard's letters).
        const unsigned char digits[9][7] =
        {
            {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
            {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
            {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
            {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
            {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
        };

        const unsigned black = 0x000000, dark = 0x303038, gray = 0x808088,
                       white = 0xffffff;
        const int width = 320;

        SendKey sendkey = 0;
        int top = -1;               // where Draw last put the row (-1: hidden)
        int numslots = 0;
        int keydown = 0;            // the key held by the finger, or 0
        bool wastouching = false;
        bool ownstouch = false;
    }

    void SetSendKey (SendKey send)
    {
        sendkey = send;
    }

    void Draw (Fill fill, int y, int slots, unsigned owned, int in_hand)
    {
        const int pitch = width / slots;

        top = y;
        numslots = slots;

        fill (0, y, width, height, black);

        for (int slot = 0; slot < slots && slot < 9; slot++)
        {
            const int x = slot * pitch + 2, w = pitch - 4;
            const bool hand = slot == in_hand;
            const unsigned color = hand ? black : (owned & (1u << slot)) ? white : dark;
            const int scale = 2, dx = x + (w - 5 * scale) / 2, dy = y + (height - 7 * scale) / 2;

            fill (x, y + 2, w, height - 4, gray);
            fill (x + 2, y + 4, w - 4, height - 8, hand ? gray : black);

            for (int row = 0; row < 7; row++)
                for (int col = 0; col < 5; col++)
                    if (digits[slot][row] & (0x10 >> col))
                        fill (dx + col * scale, dy + row * scale, scale, scale, color);
        }
    }

    void Hide ()
    {
        top = -1;
    }

    bool OwnsTouch ()
    {
        return ownstouch;
    }

    bool Update (bool touching, int x, int y)
    {
        if (touching && !wastouching)
        {
            ownstouch = top >= 0 && y >= top && y < top + height && x >= 0 && x < width;

            if (ownstouch && sendkey)
            {
                const int slot = x * numslots / width;
                keydown = '1' + (slot < numslots ? slot : numslots - 1);
                sendkey (keydown, true);
            }
        }
        else if (!touching)
        {
            if (keydown && sendkey)
                sendkey (keydown, false);
            keydown = 0;
            ownstouch = false;
        }

        wastouching = touching;
        return ownstouch;
    }
}
