# BStone for New 3DS

Downloads: [releases](../../releases). Port of [BStone](https://github.com/bibendovsky/bstone) (Blake Stone: Aliens of Gold and Planet Strike) to the New Nintendo 3DS. 

## Install

1. Install `bstone.cia` (FBI) or copy `bstone.3dsx` to `/3ds/`.
2. Copy the game data to `/3ds/bstone/` on the SD card:

| Game | Files |
|---|---|
| Aliens of Gold, shareware (free) | `*.BS1` |
| Aliens of Gold, full (Steam/GOG) | `*.BS6` |
| Planet Strike (Steam/GOG) | `*.VSI` |

Configuration, saved games and the log (`bstone_log.txt`) go to the same folder.

## Controls

| Button | In game | In menus |
|---|---|---|
| Circle Pad | Move and strafe | Move |
| C-stick | Turn | |
| D-pad | Move and turn | Move |
| R / ZR | Fire | Page down (R) |
| A | Use / open | Accept |
| B / ZL | Run | Back |
| L | Strafe | Page up |
| X / Y | Next / previous weapon | X: clear binding |
| SELECT | Pause | |
| START | Menu | Exit menu |

The buttons can be reassigned in **Options > Controls > Customize controls**.

**Options > Controls > 3DS controls** (saved in the configuration file):

| Option | Values |
|---|---|
| Dual stick | On: Circle Pad moves and strafes, C-stick turns. Off (classic): Circle Pad moves and turns |
| Stick sensitivity | 1-10 (C-stick turn speed) |
| Run | Stick pushed (fully pushed runs), hold button (default), press once (runs until you stop) |
| Touch turning | Drag on the touch screen to turn, for consoles without C-stick |
| Touch speed | 1-10 |
| FPS counter | Small counter on the bottom screen: frames per second and milliseconds per frame (off by default) |
| HUD | Bottom screen (default): the 3D view fills the top screen; the bottom screen shows the status bar, a map of what you explored and the location bar. Top (original): everything on the top screen, as on PC, and the bottom screen off |
| Status bar | Above the map (default) or below it |

A tap on the touch screen turns the bottom screen off or on (not while it shows the HUD; in game with touch turning, a drag turns instead).

## 3DS changes

- 60 FPS: the frame is composed in one pass straight into the top screen's framebuffer and waits for the vertical blank; the game no longer also sleeps until its next 70 Hz tic (the two waits drifted apart: about 40 FPS).

- Software renderer at 400x240 on the top screen; no OpenGL, Vulkan or OpenAL.
- The game timer is computed from the elapsed time instead of a ticker thread: 3DS threads on the same core do not preempt each other.
- No file locks, shared libraries or child processes (the 3DS has none).

## Building

With devkitPro (devkitARM, libctru):

```sh
cmake -S . -B build-3ds -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake \
  -DCMAKE_BUILD_TYPE=Release -DBSTONE_INTERNAL_SDL2=ON
cmake --build build-3ds
```

GitHub Actions (`.github/workflows/n3ds.yml`) also builds the `.cia`.

## Credits

- BStone: Boris I. Bendovsky and contributors.
- Blake Stone: JAM Productions / Apogee.
- 3DS port: AlexanderV387, with help from Claude (Anthropic).
