Blake Stone: Aliens of Gold and Planet Strike for the New Nintendo 3DS, based on [BStone](https://github.com/bibendovsky/bstone) 1.3.4. 60 FPS, the 3D view on the whole top screen and the HUD with a map on the bottom screen.

## New in 1.2.0

- **Weapon buttons on the bottom screen** (option "Weapon buttons" in 3DS Controls): a row of numbers under the map, five in Aliens of Gold and seven in Planet Strike; tap one to take that weapon.
- The HUD is always on the bottom screen; the option to put everything on the top screen is gone. The status bar still goes above or below the map.

## New in 1.1.0

- **Keyboard on the bottom screen:** a tab in the top left corner of the map opens it (it also opens when you name a saved game or a high score). Cheats such as J-A-M + ENTER work as on a PC keyboard; **HOLD** keeps tapped keys down for combinations. CLOSE hides it.

## New in 1.0.4

- Fix: closing the game from the HOME Menu hung forever (1.0.3 did not fix it). BStone ignored the quit event SDL sends when the HOME Menu closes the game, so the game kept running in the background and the HOME Menu waited for it. The settings and high scores are now saved when you press HOME, so closing from there is instant, and the HOME Menu's sound no longer cuts out.
- Fix: the elevator and teleporter panels ignored the buttons, so you could not change floors. They now use the menu controls: A selects, B leaves, D-pad or Circle Pad moves.
- Fix: startup, pressing HOME and saving sometimes froze for 8 seconds. On the SD card, writing a file that had just been created or emptied could take that long; files are now written over in place.
- Fix: small stutters when a sound played for the first time. The audio thread is back on the New 3DS's third core.

## New in 1.0.3

- Fix: closing the game from the HOME Menu hung forever. The DSP is paused while the HOME Menu is open, and SDL's audio thread waited for a sound buffer that never finished; it now checks every 50 ms whether it must stop.
- Fix: dying showed nothing for a moment and jumped to the end. The smooth death effect exists only in the OpenGL/Vulkan renderers; the 3DS now uses the original red dots, over the whole top screen.

## New in 1.0.2

- Fix: the `.cia` showed "No message system available" with more than one game in the folder. A `.cia` gets no arguments (a `.3dsx` gets its path), so the game chosen in the picker was lost.

## New in 1.0.1

- HUD "Top (original)": everything on the top screen and the bottom screen turns off (it turns on for the FPS counter, or with a tap).

## Downloads

| File | Use |
|---|---|
| `bstone.cia` | Install with FBI; it appears on the HOME Menu |
| `bstone.3dsx` | Copy to `/3ds/` and open it from the Homebrew Launcher |

Copy the game data to `/3ds/bstone/` (game data is not included: use your own copy):

| Game | Files |
|---|---|
| Aliens of Gold, shareware (free) | `*.BS1` |
| Aliens of Gold, full (Steam, GOG) | `*.BS6` |
| Planet Strike (Steam, GOG) | `*.VSI` |

With more than one game in the folder, a menu chooses one at startup; in the `.cia`, Quit goes back to it.

## Features

- 60 FPS on New 3DS: the frame is composed straight into the screen and paced by the vertical blank.
- HUD on the bottom screen: status bar, location bar and a map of what you explored; the 3D view fills the top screen.
- Dual stick: Circle Pad moves and strafes, C-stick turns (classic mode available), full speed in any direction.
- Three run modes: stick fully pushed, hold the button, or press once.
- Touch turning for consoles without C-stick, with its own speed.
- Rebindable buttons (R fires by default) and Nintendo-style menus: A accepts, B goes back, START leaves the menu.
- Tap the touch screen to turn the bottom screen off or on (never while it shows the HUD and the map).
- Optional FPS counter.

## Tested

Aliens of Gold shareware and Planet Strike on a New Nintendo 3DS with Luma3DS 13.4.

Developed with the help of an AI assistant (Claude); the icon and banner were made with AI tools.
