# C8Emu
A Chip-8 emulator. 

## Usage
`c8emu.exe <path-to-ROM>`

For example, from the root directory: 

`c8emu.exe games/MAZE`

## Progress
Runs all the ROMs I've tried, albeit with bugs and issues with the update rate being too fast. 

TODOs:
* clock speed control - games which don't use the delay timer really need this (KALEID)
* edit memory - dependency on flow control being implemented
* fix issue with key input going to game while trying to use debug tools - check focus on game window
* Sound timer doesn't make sound
* Dockable windows
