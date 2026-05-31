#ifndef AUDIO_H
#define AUDIO_H
#include "ff.h"
#include <stdint.h>

/* 音频文件路径定义 */
#define AUDIO_BGM "0:/audio/bgm.pcm"
#define AUDIO_BUBBLE "0:/audio/bubble.pcm"
#define AUDIO_DRONE_FLYING "0:/audio/drone_flying.pcm"
#define AUDIO_HARVEST "0:/audio/harvest.pcm"

void audio_set_volume(uint8_t vol);
uint8_t audio_get_volume(void);
void audio_stop_slot(uint8_t slot_index);

int bgm_music_play(void);
int bubble_audio_play(void);
int drone_flying_audio_play(void);
int harvest_audio_play(void);

#endif /* AUDIO_H */
