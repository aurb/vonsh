# Vonsh 1.1
Snake-like game for Linux based on libSDL

https://github.com/aurb/vonsh/

## Depedencies
Package dependencies:
+ SDL 2
+ SDL\_image 2
+ SDL\_mixer 2
+ SDL\_ttf 2
+ cJSON

Build dependencies:
+ build-essential
+ debhelper (= 13)
+ libsdl2-dev
+ libsdl2-image-dev
+ libsdl2-mixer-dev
+ libsdl2-ttf-dev
+ libcjson-dev

## Changes from 1.0
+ Added user menu navigated with keyboard and/or mouse
+ Added a support for changing the board size or switching to full screen
+ Added configurable gameplay keys
+ Music and sound effects are now toggled from the user menu
+ Added highscore saving with "Hall of fame"
+ Added storage of user configuration and highscore table
+ Added optional fps counter

## How to build and install
Commands to be executed from the project root directory:

To build the release executable(optimization ON, debug symbols OFF) **./usr/games/vonsh**:
> make release

To build the debug executable(optimization OFF, debug symbols ON) **./usr/games/vonsh**:
> make debug

To clean project:
> make clean

To build Debian installation package:
> make deb_build

To install resulting package:
> sudo dpkg -i ../vonsh\_1.1\_arch.deb

To clean all products of Debian package build:
> make deb_clean

REMARK: in the resulting Debian package name replace "arch" with your architecture suffix

## How to play
If the game is installed from a package - click the "Vonsh" icon in your system menu or type "vonsh" in the terminal.

If the project was built independently, type ./usr/games/vonsh from the project directory.

The game can be configured and started from a (hopefully) self-explanatory menu. The configurable settings are available in the "Options" menu. The minimum allowed board dimensions are 28x28.

During the gameplay control the snake using the directional keys (or other of your choice).

The current game configuration is permanently stored in the ~/.local/share/vonsh/config.json file.

The highscore list is permanently stored in the ~/.local/share/vonsh/hiscore.json file.

## Authors
### Code
+ Andrzej Urbaniak https://github.com/aurb/
+ Sam Lantinga https://github.com/libsdl-org/SDL
+ PCG Random Number Generation http://www.pcg-random.org
+ cJSON Dave Gamble and cJSON contributors https://github.com/DaveGamble/cJSON

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
+ Simple DirectMedia Layer
    + Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>
    + License: ZLib

+ PCG Random Number Generation, Files: inc/pcg_basic.h src/pcg_basic.c
    + Copyright: 2014 Melissa O'Neill <oneill@pcg-random.org>
    + License: Apache License, Version 2.0

+ cJSON Ultralightweight JSON parser in ANSI C
    + Copyright: 2009-2017 Dave Gamble and cJSON contributors
    + License: MIT

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
