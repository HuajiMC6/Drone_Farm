#ifndef __SPEAKER_H
#define __SPEAKER_H

#include "gd32h7xx.h"
#include <stdint.h>

/* ============================================================
 *  PCM 音频格式说明
 *
 *  本驱动通过 SAI2 I2S 输出 16-bit 有符号 PCM 数据。
 *  请确保 PCM 数据参数匹配以下配置:
 *    - 采样率: 44100 Hz (SPI1 I2S 主模式)
 *    - 位深:   16-bit signed (int16_t)
 *    - 声道:   单声道 (Mono)
 * ============================================================ */

/* 初始化扬声器 (SAI2 + DMA) */
void speaker_init(void);

/* 每帧在主循环调用, 推进播放状态机, 填充 DMA 缓冲区 */
void speaker_update(void);

/* 播放 PCM 音频数据
 *
 * pcm_data:     16-bit signed PCM 采样数组
 *               类型为 uint16_t*, 便于直接写入 SAI DATA 寄存器
 * sample_count: 采样点个数 (不是字节数!)
 * loop:         0 = 播放一次后停止 (SFX 音效)
 *               1 = 循环播放 (BGM 背景音乐)
 *
 * 调用示例:
 *   // 播放一次性音效
 *   speaker_play_pcm(click_sfx, click_sfx_sample_count, 0);
 *   // 播放循环背景音乐
 *   speaker_play_pcm(bgm_data, bgm_data_sample_count, 1);
 */
void speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop);

/* 停止所有播放 */
void speaker_stop(void);

/* 设置音量 (0~100), 0=静音, 100=最大 (默认100) */
void speaker_set_volume(uint8_t vol);

/* 查询是否正在播放 */
uint8_t speaker_is_playing(void);

/* I2S 中断喂数据 (由 SPI1_IRQHandler 调用, 用户勿调) */
void speaker_i2s_feed(void);

#endif /* __SPEAKER_H */
