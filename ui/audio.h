#ifndef AUDIO_H
#define AUDIO_H
#include "ff.h"
#include "lvgl.h"
#include <stdint.h>

int speaker_play_pcm_file(const TCHAR *path, uint8_t loop);

void icon_btns_click_audio_play(lv_event_t *e);

#endif /* AUDIO_H */
