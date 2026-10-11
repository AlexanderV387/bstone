/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Keyboard on the 3DS bottom screen (see bstone_n3ds_keyboard.h).

#ifdef __3DS__

#include "bstone_n3ds_keyboard.h"

#include <string.h>

namespace n3ds_keyboard
{
	namespace
	{
		// 5x7 letters: the rows' 5 low bits, the leftmost pixel the highest.
		struct Glyph
		{
			char ch;
			unsigned char rows[7];
		};

		const Glyph font[] =
		{
			{'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}}, {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
			{'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}}, {'3', {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},
			{'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}}, {'5', {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
			{'6', {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}}, {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
			{'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}}, {'9', {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
			{'A', {0x0E,0x11,0x11,0x11,0x1F,0x11,0x11}}, {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
			{'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}}, {'D', {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}},
			{'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}}, {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
			{'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}}, {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
			{'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}}, {'J', {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
			{'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}}, {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
			{'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}}, {'N', {0x11,0x11,0x19,0x15,0x13,0x11,0x11}},
			{'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
			{'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}}, {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
			{'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}}, {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
			{'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
			{'W', {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}}, {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
			{'Y', {0x11,0x11,0x11,0x0A,0x04,0x04,0x04}}, {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
			{'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}}, {'.', {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}},
		};

		const unsigned black = 0x000000, dark = 0x303038, gray = 0x808088,
					   white = 0xffffff, gold = 0xe0b030;

		// Keys of 28x30 pixels every 31x33, 10 per row, in 6 rows.
		const int key_w = 28, key_h = 30, pitch_x = 31, pitch_y = 33;
		const int keys_x = 6, keys_y = 39;

		const int close_key = -1, hold_key = -2;

		struct VKey
		{
			int x, y, w;            // in keys
			int key;                // what it presses, or close_key / hold_key
			char label[6];
		};

		VKey vkeys[64];
		int numvkeys = 0;
		bool down[64];              // held by the finger or by HOLD

		SendKey sendkey = 0;
		bool isopen = false;
		bool hold = false;          // tapped keys stay down until tapped again
		bool wastouching = false;
		bool ownstouch = false;
		int fingerkey = -1;         // the key under the finger (without HOLD)

		// The tab: a small keyboard in the top left corner.
		const int tab_x = 2, tab_size = 24;
		int tabtop = 4;             // where Draw last put it

		void AddKey (int x, int y, int w, int key, const char *label)
		{
			VKey &k = vkeys[numvkeys++];
			k.x = x;
			k.y = y;
			k.w = w;
			k.key = key;
			strncpy (k.label, label, sizeof (k.label) - 1);
			k.label[sizeof (k.label) - 1] = '\0';
		}

		void SetUpKeys ()
		{
			static const char *const rows[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM."};

			for (int row = 0; row < 4; row++)
			{
				for (int i = 0; rows[row][i] != '\0'; i++)
				{
					const char label[2] = {rows[row][i], '\0'};
					AddKey (i, row, 1, rows[row][i], label);
				}
			}

			AddKey (8, 3, 2, key_backspace, "DEL");
			AddKey (0, 4, 2, key_tab, "TAB");
			AddKey (2, 4, 2, key_shift, "SHIFT");
			AddKey (4, 4, 2, key_ctrl, "CTRL");
			AddKey (6, 4, 2, key_alt, "ALT");
			AddKey (8, 4, 2, key_f10, "F10");
			AddKey (0, 5, 2, close_key, "CLOSE");
			AddKey (2, 5, 2, hold_key, "HOLD");
			AddKey (4, 5, 3, ' ', "SPACE");
			AddKey (7, 5, 3, key_enter, "ENTER");
		}

		void KeyRect (const VKey &k, int &x, int &y, int &w)
		{
			x = keys_x + k.x * pitch_x;
			y = keys_y + k.y * pitch_y;
			w = k.w * pitch_x - (pitch_x - key_w);
		}

		void Send (int i, bool isdown)
		{
			if (down[i] != isdown)
			{
				down[i] = isdown;
				if (sendkey)
					sendkey (vkeys[i].key, isdown);
			}
		}

		void ReleaseAll ()
		{
			for (int i = 0; i < numvkeys; i++)
				if (vkeys[i].key > 0)
					Send (i, false);
			fingerkey = -1;
		}

		int KeyAt (int x, int y)
		{
			for (int i = 0; i < numvkeys; i++)
			{
				int kx, ky, kw;
				KeyRect (vkeys[i], kx, ky, kw);
				if (x >= kx && x < kx + kw && y >= ky && y < ky + key_h)
					return i;
			}
			return -1;
		}

		void Tap (int i)
		{
			const int key = vkeys[i].key;

			if (key == close_key)
				Open (false);
			else if (key == hold_key)
			{
				hold = !hold;
				if (!hold)
					ReleaseAll ();
			}
			else if (hold)
				Send (i, !down[i]);
			else
			{
				Send (i, true);
				fingerkey = i;
			}
		}

		void DrawChar (Fill fill, int x, int y, char ch, int scale, unsigned color)
		{
			for (unsigned i = 0; i < sizeof (font) / sizeof (font[0]); i++)
			{
				if (font[i].ch != ch)
					continue;

				for (int row = 0; row < 7; row++)
					for (int col = 0; col < 5; col++)
						if (font[i].rows[row] & (0x10 >> col))
							fill (x + col * scale, y + row * scale, scale, scale, color);
				return;
			}
		}

		void DrawText (Fill fill, int x, int y, const char *text, int scale, unsigned color)
		{
			for (; *text != '\0'; text++, x += 6 * scale)
				DrawChar (fill, x, y, *text, scale, color);
		}

		// Three rows of keys and the space bar, in a frame.
		void DrawTab (Fill fill, int y)
		{
			fill (tab_x, y, tab_size, tab_size, gray);
			fill (tab_x + 2, y + 2, tab_size - 4, tab_size - 4, black);

			for (int row = 0; row < 3; row++)
				for (int key = 0; key < 4 - (row == 2); key++)
					fill (tab_x + 5 + row + key * 4, y + 5 + row * 4, 2, 2, white);

			fill (tab_x + 8, y + 17, 8, 2, white);
		}

		bool OnTab (int x, int y)
		{
			// A little larger than drawn: easier to hit.
			return x < tab_x + tab_size + 6 && y >= tabtop - 4 && y < tabtop + tab_size + 6;
		}
	}

	void SetSendKey (SendKey send)
	{
		sendkey = send;
	}

	bool IsOpen ()
	{
		return isopen;
	}

	void Open (bool open)
	{
		if (numvkeys == 0)
			SetUpKeys ();
		if (!open)
		{
			ReleaseAll ();
			hold = false;
		}
		isopen = open;
	}

	bool OwnsTouch ()
	{
		return ownstouch;
	}

	bool Update (bool touching, int x, int y)
	{
		if (touching && !wastouching)
		{
			ownstouch = isopen || OnTab (x, y);

			if (isopen)
			{
				const int i = KeyAt (x, y);
				if (i >= 0)
					Tap (i);
			}
			else if (ownstouch)
				Open (true);
		}
		else if (!touching)
		{
			// Without HOLD a key is down while the finger is on it.
			if (fingerkey >= 0)
			{
				Send (fingerkey, false);
				fingerkey = -1;
			}
			ownstouch = false;
		}

		wastouching = touching;
		return ownstouch;
	}

	void Draw (Fill fill, int tab_y)
	{
		tabtop = tab_y;

		if (!isopen)
		{
			DrawTab (fill, tab_y);
			return;
		}

		fill (0, 0, 320, 240, black);
		DrawText (fill, 8, 10, "KEYBOARD", 2, gray);
		if (hold)
			DrawText (fill, 120, 14, "HOLD - KEYS STAY DOWN", 1, gold);

		for (int i = 0; i < numvkeys; i++)
		{
			const VKey &k = vkeys[i];
			const bool lit = k.key == hold_key ? hold : (k.key > 0 && down[i]);
			const int len = (int) strlen (k.label);
			const int scale = len == 1 ? 2 : 1;
			int x, y, w;

			KeyRect (k, x, y, w);
			fill (x, y, w, key_h, lit ? gold : gray);
			fill (x + 1, y + 1, w - 2, key_h - 2, lit ? 0x6a5420 : dark);
			DrawText (fill, x + (w - len * 6 * scale + scale) / 2,
					  y + (key_h - 7 * scale) / 2, k.label, scale, white);
		}
	}
}

#endif // __3DS__
