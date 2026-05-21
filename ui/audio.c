#include "audio.h"
#include "speaker.h"

// 播放背景音乐
int bgm_music_play(void) {
    return speaker_play_bgm_stream_file(AUDIO_BGM, 1);
}

// 首页悬浮按钮点击音效
void icon_btns_click_audio_play(lv_event_t *e) {
    // speaker_set_volume(20);
    speaker_play_pcm_file(AUDIO_ICON_BTN_CLICK, 0);
}
