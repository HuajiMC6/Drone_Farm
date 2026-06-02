#include "system_win.h"

#include "diskio_win.h"
#include "ff.h"

#include <windows.h>

/* ----------------------------------------------------------------
 *  sys_init() — Windows 版本
 *
 *  仅负责:
 *   1. 初始化 FATFS 虚拟磁盘 (diskio_win)
 *   2. 挂载逻辑驱动器 "0:"
 * ---------------------------------------------------------------- */
FATFS g_fs; /* 全局 FATFS 对象, 对应原版 Drivers/sys.c 中的 fs */

void sys_init(void) {
    FRESULT res;

    /* 初始化虚拟磁盘 */
    if (disk_initialize(0) != 0) {
        /* 虚拟磁盘初始化失败 — 游戏仍可运行, 只是不能存档 */
    }

    /* 挂载 FATFS */
    res = f_mount(&g_fs, "0:", 1);
    if (res != FR_OK) {
        /* 挂载失败 — 首次运行可能尚无磁盘镜像, 尝试格式化 */
        BYTE work[FF_MAX_SS];
        res = f_mkfs("0:", 0, work, sizeof(work));
        if (res == FR_OK) {
            f_mount(&g_fs, "0:", 1);
        }
    }
}

/* ----------------------------------------------------------------
 *  delay_us() — Windows 版本
 * ---------------------------------------------------------------- */
void delay_us(uint32_t nus) {
    if (nus == 0)
        return;

    /* Sleep() 精度约 1-15ms (取决于系统定时器分辨率).
     * 主循环中 delay_us(2000) ≈ 2ms, 对于 LVGL 渲染节奏足够. */
    DWORD ms = (nus + 999) / 1000;
    if (ms < 1)
        ms = 1;
    Sleep(ms);
}
