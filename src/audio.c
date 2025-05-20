#include <SDL2/SDL_mixer.h>

#include "types.h"
#include "audio.h"
#include "error_handling.h"

static Audio g_audio;

void audio_init(void) {
    if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096 ) == -1 ) {
        set_error("Error opening audio device: %s.", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(4);

    if ((g_audio.exp_chunk = Mix_LoadWAV(RES_DIR"exp_sound.wav")) == NULL) {
        set_error("Error loading sample '%s': %s", RES_DIR"exp_sound.wav", Mix_GetError());
    }

    if ((g_audio.die_chunk = Mix_LoadWAV(RES_DIR"die_sound.wav")) == NULL) {
        set_error("Error loading sample '%s': %s", RES_DIR"die_sound.wav", Mix_GetError());
    }

    if ((g_audio.idle_music = Mix_LoadMUS(RES_DIR"idle_tune.mp3")) == NULL) {
        set_error("Error loading music '%s': %s", RES_DIR"idle_tune.mp3", Mix_GetError());
    }

    if ((g_audio.gameplay_music = Mix_LoadMUS(RES_DIR"play_tune.mp3")) == NULL) {
        set_error("Error loading music '%s': %s", RES_DIR"play_tune.mp3", Mix_GetError());
    }
    
    Mix_VolumeChunk(g_audio.die_chunk, MIX_MAX_VOLUME/3);
    Mix_VolumeChunk(g_audio.exp_chunk, MIX_MAX_VOLUME/3);
}

void audio_shutdown(void) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    if (g_audio.idle_music) Mix_FreeMusic(g_audio.idle_music);
    if (g_audio.gameplay_music) Mix_FreeMusic(g_audio.gameplay_music);
    if (g_audio.exp_chunk) Mix_FreeChunk(g_audio.exp_chunk);
    if (g_audio.die_chunk) Mix_FreeChunk(g_audio.die_chunk);
    Mix_CloseAudio();
}

static void play_music(Mix_Music* music) {
    if (g_game.music_on && music != NULL) {
        if (Mix_PlayMusic(music, -1) == -1) {
            set_error("Failed to play music: %s", Mix_GetError());
        }
    }
}

void audio_play_idle_music() {
    play_music(g_audio.idle_music);
}

void audio_play_gameplay_music() {
    play_music(g_audio.gameplay_music);
}

void audio_play_exp_sound() {
    if (Mix_PlayChannel(-1, g_audio.exp_chunk, 0) == -1) {
        set_error("Failed to play sound effect: %s", Mix_GetError());
    }
}

void audio_play_die_sound() {
    if (Mix_PlayChannel(-1, g_audio.die_chunk, 0) == -1) {
        set_error("Failed to play sound effect: %s", Mix_GetError());
    }
}

void audio_toggle_music(void) {
    g_game.music_on = !g_game.music_on;
    if (g_game.music_on) {
        play_music(g_audio.idle_music);
    } else {
        Mix_HaltMusic();
    }
}

void audio_pause_music(void) {
    if (g_game.music_on) {
        Mix_PauseMusic();
    }
}

void audio_resume_music(void) {
    if (g_game.music_on) {
        Mix_ResumeMusic();
    }
}
