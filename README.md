# Vonsh
Snake-like game for Linux based on libSDL

https://github.com/aurb/vonsh/

## Depedencies
Package dependencies:
+ libc6 (>= 2.2.5)
+ libsdl2-2.0-0 (>= 2.0.9)
+ libsdl2-image-2.0-0 (>= 2.0.2)
+ libsdl2-mixer-2.0-0 (>= 2.0.2)

Build dependencies:
+ libc6-dev (>=2.28)
+ gcc (>=4:8.3.0)
+ g++ (>=4:8.3.0)
+ make (>=4.2.1)
+ dpkg-dev (>=1.19.7)
+ debhelper (>= 12)
+ libsdl2-dev (>=2.0.9)
+ libsdl2-image-dev (>=2.0.4)
+ libsdl2-mixer-dev (>=2.0.4)

## How to build and install
Commands to be executed from project root directory:

To build development release executable(optimization ON, debug symbols OFF) **./usr/games/vonsh**:
> make release

To build development debug executable(optimization OFF, debug symbols ON) **./usr/games/vonsh**:
> make debug

To clean project:
> make clean

To build Debian full installation package **../vonsh\_1.0\_arch.deb**:
> dpkg-buildpackage -b -uc

To install that package:
> sudo dpkg -i ../vonsh\_1.0\_arch.deb

## How to play
If game is installed from package - click "Vonsh" icon in your system menu or type "vonsh" in console.

If only executable is compiled - from project directory type "./usr/games/vonsh".

Use arrow keys to move snake. Eat food, avoid obstacles and screen edges.

SPACE to pause/unpause, ESC to quit.

Music and sound effects can be toggled with Z and X.

## Authors
### Code
+ Andrzej Urbaniak https://github.com/aurb/
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
+ PCG Random Number Generation, Files: inc/pcg_basic.h src/pcg_basic.c
    + Copyright: 2014 Melissa O'Neill <oneill@pcg-random.org>
    + License: Apache License, Version 2.0

+ File: usr/share/games/vonsh/board_tiles.png
    + Copyright: 2018 Beast
    + License: CC0 1.0

+ File: usr/share/games/vonsh/food_tiles.png
    + Copyright: 2016 Henry Software
    + License: CC0 1.0

+ File: usr/share/games/vonsh/character_tiles.png
    + Copyright: 2017 Fleurman
    + License: CC0 1.0

+ File: usr/share/games/vonsh/good_neighbors.png
    + Copyright: 2015 Clint Bellanger
    + License: CC0 1.0

+ File: usr/share/games/vonsh/trophy-bronze.png
    + Copyright: 2014 Yevhen Danchenko
    + License: CC0 1.0

+ Files: usr/share/games/vonsh/die_sound.wav usr/share/games/vonsh/exp_sound.wav 
    + Copyright: 2014 Damaged Panda
    + License: CC BY 3.0

+ File: usr/share/games/vonsh/idle_tune.mp3
    + Copyright: 2016 Juhani Junkala 
    + License: CC0 1.0

+ Files: usr/share/games/vonsh/play_tune.mp3
    + Copyright: 2013 Przemyslaw Sikorski
    + License: CC BY 3.0

+ All files not mentioned above:
    + Copyright: 2019 Andrzej Urbaniak
    + License: MIT
