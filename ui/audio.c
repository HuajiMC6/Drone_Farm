#include "audio.h"
#include "speaker.h"

void audio_set_volume(uint8_t vol) {
    speaker_set_volume(vol);
}

uint8_t audio_get_volume(void) {
    return speaker_get_volume();
}

// 播放背景音乐
int bgm_music_play(void) {
    return speaker_play_bgm_stream_file(AUDIO_BGM, 1);
}

// 点击音效
int bubble_audio_play(void) {
    return speaker_play_pcm_file(AUDIO_BUBBLE, 0);
}

// 无人机飞行音效
int drone_flying_audio_play(void) {
    return speaker_play_pcm_file(AUDIO_DRONE_FLYING, 1);
}

// 收获音效
int harvest_audio_play(void) {
    return speaker_play_pcm_file(AUDIO_HARVEST, 0);
}

// 停止指定槽位的播放
void audio_stop_slot(uint8_t slot_index) {
    speaker_stop_slot(slot_index);
}
