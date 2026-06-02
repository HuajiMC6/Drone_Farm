#ifndef WIN_PORT_AUDIO_SDL_H
#define WIN_PORT_AUDIO_SDL_H

#include <stdint.h>

/* ================================================================
 *  win_port/audio_sdl.h — SDL2 音频驱动
 *
 *  实现与 Drivers/speaker.h 完全相同的 API, 但后端使用 SDL2
 *  音频子系统替代 SAI2 I2S + DMA.
 *
 *  支持:
 *    - 4 个并发播放槽 (与嵌入式版一致)
 *    - 内存 PCM 播放 (短音效)
 *    - 文件流式播放 (长 BGM)
 *    - 循环播放
 *    - 音量控制 (0-100)
 * ================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化音频系统
 *
 * 打开 SDL 音频设备: 44100 Hz, Mono, 16-bit signed PCM.
 */
void speaker_init(void);

/**
 * @brief 每帧调用, 填充音频缓冲
 *
 * 嵌入式版在主循环首尾各调用一次以防止欠载。
 * SDL 版使用队列回调, 此函数主要负责:
 *   - 检查流式播放缓冲, 从文件补充数据
 *   - 清理已结束的非循环槽
 */
void speaker_update(void);

/**
 * @brief 播放内存中的 PCM 数据
 *
 * @param pcm_data      uint16_t* 格式的 16-bit 采样
 * @param sample_count  采样点数 (不是字节)
 * @param loop          0=播放一次, 1=循环
 * @return  槽位索引 (0~3), -1 表示失败
 */
int speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop);

/**
 * @brief 从文件加载 PCM 后播放 (适合短音效)
 *
 * @param path 文件路径 (支持 FATFS 格式如 "0:/audio/click.pcm")
 * @param loop 0=单次, 1=循环
 * @return 槽位索引 (0~3), -1 表示失败
 */
int speaker_play_pcm_file(const char *path, uint8_t loop);

/**
 * @brief 从文件流式播放 (适合长 BGM)
 *
 * 不一次性加载到内存, 而是按需从文件读取.
 *
 * @param path 文件路径
 * @param loop 0=播放到末尾停止, 1=自动循环
 * @return 槽位索引 (0~3), -1 表示失败
 */
int speaker_play_bgm_stream_file(const char *path, uint8_t loop);

/**
 * @brief 停止指定槽位
 */
void speaker_stop_slot(uint8_t slot_index);

/**
 * @brief 停止所有播放
 */
void speaker_stop(void);

/**
 * @brief 设置/获取音量
 *
 * @param vol  0=静音, 100=最大 (默认 100)
 */
void    speaker_set_volume(uint8_t vol);
uint8_t speaker_get_volume(void);

/**
 * @brief 查询是否正在播放
 */
uint8_t speaker_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_AUDIO_SDL_H */
