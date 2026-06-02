/**
 * @file    win_port/gd32h7xx.h
 * @brief   GD32H7xx HAL stub for Windows port
 *
 * 提供嵌入式代码引用的最小类型定义和宏, 使游戏逻辑层
 * 无需修改即可在 Windows 上编译.
 */

#ifndef WIN_PORT_GD32H7XX_H
#define WIN_PORT_GD32H7XX_H

/* ---- 基础类型 (与 CMSIS 兼容) ---- */
#include <stdint.h>

#ifndef __IO
#define __IO  volatile
#endif
#ifndef __I
#define __I   volatile const
#endif
#ifndef __O
#define __O   volatile
#endif

/* ---- 复位模拟 (Windows 上用 exit) ---- */
#include <stdlib.h>
#define NVIC_SystemReset()  exit(0)

/* ---- 常用宏 ---- */
#define RESET  0
#define SET    1

/* ---- TRNG stub (如果需要) ---- */
#define TRNG_FLAG_DRDY  0
static inline uint32_t trng_get_true_random_data(void) { return (uint32_t)rand(); }

/* ---- 系统时钟频率 (不需要) ---- */
#define SystemCoreClock  600000000U

#endif /* WIN_PORT_GD32H7XX_H */
