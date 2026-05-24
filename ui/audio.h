#ifndef AUDIO_H
#define AUDIO_H
#include "ff.h"
#include "lvgl.h"
#include <stdint.h>

/* 音频文件路径定义 */
#define AUDIO_BGM "0:/audio/bgm.pcm"
#define AUDIO_ICON_BTN_CLICK "0:/audio/icon_btns_click.pcm"

void audio_set_volume(uint8_t vol);

void bgm_music_play(void);
void icon_btns_click_audio_play(lv_event_t *e);

#endif /* AUDIO_H */
