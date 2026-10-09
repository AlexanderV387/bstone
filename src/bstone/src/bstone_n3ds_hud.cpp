/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2026 Alexander Vinueza and Contributors
SPDX-License-Identifier: MIT
*/

// Nintendo 3DS: the HUD on the bottom screen.
//
// The 320x240 bottom screen is as wide as the game's 320x200 UI, so the top
// bar (location and messages, 16 rows) and the status bar (48 rows) are
// copied 1:1. Between them, a map of what was explored, centered on the
// player.

#ifdef __3DS__

#include <cmath>

#include "3d_def.h"
#include "bstone_n3ds.h"
#include "bstone_ref_values.h"

namespace bstone {
namespace n3ds {

namespace {

constexpr auto map_top = ref_top_bar_height;
constexpr auto map_height = bottom_height - ref_top_bar_height - ref_bottom_bar_height;
constexpr auto map_bottom = map_top + map_height;
constexpr auto tile_size = 4; // pixels per map tile

constexpr std::uint32_t map_background_color = 0x080810FFU;
constexpr std::uint32_t map_floor_color = 0x283850FFU;
constexpr std::uint32_t map_wall_color = 0xA8A8B8FFU;
constexpr std::uint32_t map_door_color = 0xE0B030FFU;
constexpr std::uint32_t map_player_color = 0xFF4040FFU;

void copy_ui_rows(
	std::uint32_t* buffer,
	const std::uint8_t* ui,
	const std::uint32_t* colors,
	int src_y,
	int dst_y,
	int rows)
{
	for (auto row = 0; row < rows; ++row)
	{
		const auto src = &ui[(src_y + row) * vga_ref_width];
		auto dst = &buffer[(dst_y + row) * bottom_width];

		for (auto x = 0; x < vga_ref_width; ++x)
		{
			dst[x] = colors[src[x]];
		}
	}
}

// Clipped to the map area.
void fill_map(std::uint32_t* buffer, int x, int y, int width, int height, std::uint32_t color)
{
	const auto x0 = std::max(x, 0);
	const auto y0 = std::max(y, map_top);
	const auto x1 = std::min(x + width, bottom_width);
	const auto y1 = std::min(y + height, map_bottom);

	for (auto j = y0; j < y1; ++j)
	{
		for (auto i = x0; i < x1; ++i)
		{
			buffer[j * bottom_width + i] = color;
		}
	}
}

bool is_seen(int x, int y)
{
	return
		x >= 0 && x < MAPSIZE && y >= 0 && y < MAPSIZE &&
		(travel_table_[x][y] & TT_TRAVELED) != 0;
}

void draw_map(std::uint32_t* buffer)
{
	fill_map(buffer, 0, map_top, bottom_width, map_height, map_background_color);

	if (player == nullptr)
	{
		return;
	}

	const auto center_x = bottom_width / 2.0;
	const auto center_y = map_top + (map_height / 2.0);

	for (auto tx = 0; tx < MAPSIZE; ++tx)
	{
		for (auto ty = 0; ty < MAPSIZE; ++ty)
		{
			const auto tile = tilemap[tx][ty];
			auto color = map_floor_color;

			if (tile == 0)
			{
				if (!is_seen(tx, ty))
				{
					continue;
				}
			}
			else
			{
				// Walls and doors are shown once a tile next to them was seen.
				if (!is_seen(tx, ty) &&
					!is_seen(tx - 1, ty) && !is_seen(tx + 1, ty) &&
					!is_seen(tx, ty - 1) && !is_seen(tx, ty + 1))
				{
					continue;
				}

				color = (tile & 0x80) != 0 ? map_door_color : map_wall_color;
			}

			const auto x = static_cast<int>(std::floor(center_x + (tx - player->x) * tile_size));
			const auto y = static_cast<int>(std::floor(center_y + (ty - player->y) * tile_size));
			fill_map(buffer, x, y, tile_size, tile_size, color);
		}
	}

	// The player: a dot and a line toward where they look (angle in degrees,
	// 0 is east, counterclockwise).
	const auto angle = player->angle * 3.14159265358979323846 / 180.0;
	const auto dx = std::cos(angle);
	const auto dy = -std::sin(angle);

	for (auto i = 0; i <= 8; ++i)
	{
		const auto x = static_cast<int>(center_x + (dx * i));
		const auto y = static_cast<int>(center_y + (dy * i));
		fill_map(buffer, x, y, 1, 1, map_player_color);
	}

	fill_map(buffer, static_cast<int>(center_x) - 1, static_cast<int>(center_y) - 1, 3, 3, map_player_color);
}

} // namespace

void draw_bottom_hud(const std::uint8_t* ui, const std::uint32_t* colors)
{
	const auto buffer = get_bottom_buffer();

	copy_ui_rows(buffer, ui, colors, 0, 0, ref_top_bar_height);
	draw_map(buffer);
	copy_ui_rows(
		buffer,
		ui,
		colors,
		vga_ref_height - ref_bottom_bar_height,
		bottom_height - ref_bottom_bar_height,
		ref_bottom_bar_height);
}

} // namespace n3ds
} // namespace bstone

#endif // __3DS__
