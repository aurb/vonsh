#ifndef AUDIO_H
#define AUDIO_H

void audio_init(void);
void audio_shutdown(void);
void audio_play_idle_music();
void audio_play_gameplay_music();
void audio_play_exp_sound();
void audio_play_die_sound();
void audio_toggle_music(void);
void audio_pause_music(void);
void audio_resume_music(void);

#endif /* AUDIO_H */
