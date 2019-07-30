# Vonsh
Snake game for Linux using SDL library.

https://github.com/aurb/vonsh

## Depedencies
Package dependencies:
+ libc6 (>= 2.2.5)
+ libsdl2-2.0-0 (>= 2.0.9)
+ libsdl2-image-2.0-0 (>= 2.0.2)
+ libsdl2-mixer-2.0-0 (>= 2.0.2)

Build dependencies:
+ build-essential
+ debhelper (>= 9)
+ libsdl2-dev (>=2.0.9)
+ libsdl2-image-dev (>=2.0.4)
+ libsdl2-mixer-dev (>=2.0.4)

## How to build and install
Commands to be executed from project root directory:

To build release executable(optimization ON, debug symbols OFF) **usr/games/vonsh**:
> make release

To build debug executable(optimization OFF, debug symbols ON) **usr/games/vonsh**:
> make debug

To clean project:
> make clean

To build Debian installation package **../vonsh_x.y-z_arch.deb**:
> dpkg-buildpackage -b -uc

## How to play
If game is installed from package - type "vonsh". If game is only compiled - run vonsh_0.0/usr/games/vonsh binary.
Use arrow keys to move snake. Eat food, avoid obstacles and screen edges.
SPACE to pause/unpause, ESC to quit.

Music and sound effects can be toggled with Z and X.

## Authors
### Code
+ Andrzej Urbaniak https://github.com/aurb
+ PCG Random Number Generation http://www.pcg-random.org
### Graphics
+ Beast https://opengameart.org/content/overworld-grass-biome
+ Henry Software https://henrysoftware.itch.io/pixel-food
+ Fleurman https://opengameart.org/content/tiny-characters-set
+ Good Neighbors font by Clint Bellanger
+ Yevhen Danchenko https://opengameart.org/content/trophies-goblets
+ Andrzej Urbaniak
### Sound Effects
+ Damaged Panda https://opengameart.org/content/100-plus-game-sound-effects-wavoggm4a
### Music
+ Juhani Junkala https://opengameart.org/content/5-chiptunes-action
+ Przemyslaw Sikorski https://opengameart.org/content/puzzle-tune-1
Music licensed under CC-BY 3.0 https://creativecommons.org/licenses/by/3.0/

## License
For licenses see LICENSE file.
