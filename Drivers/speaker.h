#ifndef __SPEAKER_H
#define __SPEAKER_H

#include "ff.h"
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
/* 返回分配到的槽位索引 (0~3), 可用于 speaker_stop_slot() 单独停止;
   -1 表示分配失败 (槽位已满) */
int speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop);

/* 停止指定槽位的播放 (slot_index 由 speaker_play_pcm 返回) */
void speaker_stop_slot(uint8_t slot_index);

/* 停止所有播放 */
void speaker_stop(void);

/* 设置音量 (0~100), 0=静音, 100=最大 (默认100) */
void speaker_set_volume(uint8_t vol);

/* 查询当前音量 (0~100) */
uint8_t speaker_get_volume(void);

/* 查询是否正在播放 */
uint8_t speaker_is_playing(void);

/* I2S 中断喂数据 (由 SPI1_IRQHandler 调用, 用户勿调) */
void speaker_i2s_feed(void);

/* 从 SD 卡加载 PCM 到 SDRAM 后播放 (适合短音效)
 *
 * path:  音频文件路径, 例如 "0:/audio/click.pcm"
 * loop:  0 = 单次播放, 1 = 循环播放
 * 返回:  槽位索引 (0~3) 用于 speaker_stop_slot(), -1 表示失败
 */
int speaker_play_pcm_file(const TCHAR *path, uint8_t loop);

/* 从 SD 卡直接流式播放背景音乐 (适合长音频)
 *
 * path:  音频文件路径, 例如 "0:/audio/bgm.pcm"
 * loop:  0 = 播放到末尾停止, 1 = 播放完自动从头循环
 */
int speaker_play_bgm_stream_file(const TCHAR *path, uint8_t loop);

#endif /* __SPEAKER_H */
