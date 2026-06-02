#ifndef WIN_PORT_DISKIO_WIN_H
#define WIN_PORT_DISKIO_WIN_H

/* ================================================================
 *  win_port/diskio_win.h — FATFS Windows 磁盘 I/O 层
 *
 *  使用单个二进制文件 "sdcard/fat.img" 模拟 SD 卡。
 *  FATFS 核心 (ff.c) 通过 diskio.h 接口调用此模块的
 *  disk_read / disk_write / disk_ioctl。
 * ================================================================ */

#include "diskio.h"
#include "ff.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 磁盘镜像文件路径 (相对于可执行文件)
 *
 * 默认: "sdcard/fat.img". 可在此修改路径.
 */
#ifndef WIN_DISK_IMAGE_PATH
#define WIN_DISK_IMAGE_PATH   "sdcard\\fat.img"
#endif

/**
 * @brief 虚拟磁盘大小 (字节)
 *
 * 默认 64MB — 足够存储存档和少量音频资源.
 */
#ifndef WIN_DISK_SIZE
#define WIN_DISK_SIZE         (64ULL * 1024 * 1024)
#endif

/**
 * @brief 扇区大小 (必须为 512, 与 FATFS FF_MIN_SS / FF_MAX_SS 一致)
 */
#define WIN_DISK_SECTOR_SIZE  512

/* ---- FATFS diskio.h 接口实现声明 ---- */

DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status     (BYTE pdrv);
DRESULT disk_read       (BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_write      (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_ioctl      (BYTE pdrv, BYTE cmd, void *buff);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_DISKIO_WIN_H */
