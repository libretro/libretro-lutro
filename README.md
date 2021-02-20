# Lutro [![Build Status](https://travis-ci.org/libretro/libretro-lutro.svg?branch=master)](https://travis-ci.org/libretro/libretro-lutro)

Experimental [Lua](http://lua.org) game framework for [libretro](http://libretro.com), following the [LÖVE](http://love2d.org) API.

[Lutro](https://github.com/libretro/libretro-lutro) is software rendered and implements only a [subset of the LÖVE API](https://github.com/libretro/lutro-status). It targets portability though the libretro API and backed in dependancies.

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

## Build

Compile Lutro by [installing the RetroArch dependencies](https://github.com/libretro/retroarch#dependencies-pc), and running:

    make

## Test

Run the Lutro testing suite by executing:

    make test

To run tests manually, run:

    retroarch -L path/to/lutro_libretro.so test
