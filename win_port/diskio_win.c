#include "diskio_win.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ================================================================
 *  内部状态
 * ================================================================ */

static FILE *g_disk_file = NULL;   /* 磁盘镜像文件句柄 */
static int g_disk_initialized = 0; /* 是否已初始化 */

/* 获取可执行文件所在目录 (用于定位 fat.img 的默认路径) */
static void get_exe_dir(char *buf, size_t bufsize) {
    GetModuleFileNameA(NULL, buf, (DWORD)bufsize);
    char *last = strrchr(buf, '\\');
    if (last)
        *(last + 1) = '\0';
}

/* ================================================================
 *  FATFS diskio.h 接口实现
 * ================================================================ */

DSTATUS disk_initialize(BYTE pdrv) {
    (void)pdrv;

    if (g_disk_initialized)
        return 0;

    char path[1024];
    get_exe_dir(path, sizeof(path));
    strcat_s(path, sizeof(path), WIN_DISK_IMAGE_PATH);

    /* 确保目录存在 */
    char dir[1024];
    strcpy_s(dir, sizeof(dir), path);
    char *slash = strrchr(dir, '\\');
    if (slash) {
        *slash = '\0';
        CreateDirectoryA(dir, NULL); /* 递归创建需 SHCreateDirectoryEx, 简单场景一层够用 */
    }

    /* 尝试打开已有镜像 */
    g_disk_file = fopen(path, "r+b");

    if (!g_disk_file) {
        /* 镜像不存在 — 创建并格式化 */
        g_disk_file = fopen(path, "w+b");
        if (!g_disk_file)
            return STA_NOINIT;

        /* 扩展到 WIN_DISK_SIZE */
        if (_fseeki64(g_disk_file, WIN_DISK_SIZE - 1, SEEK_SET) != 0) {
            fclose(g_disk_file);
            g_disk_file = NULL;
            return STA_NOINIT;
        }
        fputc(0, g_disk_file);
        fflush(g_disk_file);
    }

    g_disk_initialized = 1;
    return 0;
}

DSTATUS disk_status(BYTE pdrv) {
    (void)pdrv;
    return g_disk_file ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (!g_disk_file)
        return RES_NOTRDY;
    if (count == 0)
        return RES_PARERR;

    long long offset = (long long)sector * WIN_DISK_SECTOR_SIZE;
    if (_fseeki64(g_disk_file, offset, SEEK_SET) != 0)
        return RES_ERROR;

    size_t bytes = (size_t)count * WIN_DISK_SECTOR_SIZE;
    size_t read = fread(buff, 1, bytes, g_disk_file);
    return (read == bytes) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (!g_disk_file)
        return RES_NOTRDY;
    if (count == 0)
        return RES_PARERR;

    long long offset = (long long)sector * WIN_DISK_SECTOR_SIZE;
    if (_fseeki64(g_disk_file, offset, SEEK_SET) != 0)
        return RES_ERROR;

    size_t bytes = (size_t)count * WIN_DISK_SECTOR_SIZE;
    size_t wrote = fwrite(buff, 1, bytes, g_disk_file);
    if (wrote != bytes)
        return RES_ERROR;

    fflush(g_disk_file);
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    (void)pdrv;

    switch (cmd) {
        case CTRL_SYNC:
            if (g_disk_file)
                fflush(g_disk_file);
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = (DWORD)(WIN_DISK_SIZE / WIN_DISK_SECTOR_SIZE);
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = WIN_DISK_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1; /* 擦除块大小 = 1 扇区 */
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
