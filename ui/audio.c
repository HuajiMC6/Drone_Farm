#include "audio.h"
#include "speaker.h"

void audio_set_volume(uint8_t vol) {
    speaker_set_volume(vol);
}

// 播放背景音乐
void bgm_music_play(void) {
    speaker_play_bgm_stream_file(AUDIO_BGM, 1);
}

// 首页悬浮按钮点击音效
void icon_btns_click_audio_play(lv_event_t *e) {
    speaker_play_pcm_file(AUDIO_ICON_BTN_CLICK, 0);
}
