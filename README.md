# Lutro

Experimental [Lua](http://lua.org) game framework for [libretro](http://libretro.com), following the [LÖVE](http://love2d.org) API.

## Sample Games


 * https://github.com/libretro/lutro-platformer
 * https://github.com/libretro/lutro-game-of-life
 * https://github.com/libretro/lutro-snake
 * https://github.com/libretro/lutro-tetris
 * https://github.com/libretro/lutro-iyfct
 * https://github.com/libretro/lutro-pong

## Usage

Through RetroArch, use the Lutro core to load the game's source directory:

    retroarch -L libretro_lutro.so path/to/gamedir/

Alternatively, you can load a compressed `.lutro` file:

    retroarch -L libretro_lutro.so game.lutro
