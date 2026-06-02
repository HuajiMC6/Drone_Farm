#ifndef WIN_PORT_SYSTEM_WIN_H
#define WIN_PORT_SYSTEM_WIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Windows 平台的 sys_init() 桩实现
 *
 * 嵌入式原版 sys_init() 负责:
 *   - 系统时钟配置 (HXTAL/PLL/SysTick)
 *   - SDRAM 初始化
 *   - LCD / Touch / DMA 初始化
 *   - SD 卡挂载 (FATFS f_mount)
 *
 * Windows 版仅需: 挂载文件系统 (FATFS 通过 diskio_win 访问磁盘镜像).
 */
void sys_init(void);

/**
 * @brief 微秒级延迟
 *
 * 嵌入式版使用 SysTick 硬件计时。Windows 版使用 Sleep().
 * 注意: Sleep(0) ≈ 15ms 精度, 高精度需求建议使用
 * SDL_Delay 或 timeBeginPeriod + Sleep.
 *
 * @param nus  延迟微秒数
 */
void delay_us(uint32_t nus);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_SYSTEM_WIN_H */
