/**
 * @file    win_port/resource_embed.c
 * @brief   首次启动时将嵌入资源写入 SD 卡虚拟磁盘
 */

#include "resource_embed.h"
#include "ff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 版本化标记: 改版本号会触发重新安装 */
#define INSTALL_MARKER "0:/_res_ver_2"

/**
 * @brief 递归创建 FATFS 目录 (类似 mkdir -p)
 *
 * 例如 path="audio/sub" → 先 mkdir "audio", 再 mkdir "audio/sub"
 */
static void resource_mkdirs(const char *vfs_path) {
    char dir[256];
    strncpy(dir, vfs_path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';

    /* 去掉文件名, 只保留目录部分 */
    char *slash = strrchr(dir, '/');
    if (!slash)
        return; /* 无子目录 */
    *slash = '\0';

    /* 逐级创建 */
    char *p = dir;
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            f_mkdir(dir);
            *p = '/';
        }
        p++;
    }
    f_mkdir(dir); /* 最后一级 */
}

int resource_install_all(void) {
    /* 空资源 — 快速返回 */
    if (g_resources[0].name == NULL)
        return 0;

    /* 检查标记文件, 已安装则跳过 */
    FIL tmp;
    if (f_open(&tmp, INSTALL_MARKER, FA_READ) == FR_OK) {
        f_close(&tmp);
        return 0;
    }

    printf("[INFO] First run detected — installing embedded resources...\n");

    int count = 0;
    for (int i = 0; g_resources[i].name != NULL; i++) {
        const resource_entry_t *r = &g_resources[i];

        /* 检查目标是否已存在 */
        if (f_open(&tmp, r->name, FA_READ) == FR_OK) {
            f_close(&tmp);
            continue;
        }

        /* 确保目录存在 */
        resource_mkdirs(r->name);

        /* 创建目标文件并写入 */
        FIL fw;
        FRESULT res = f_open(&fw, r->name, FA_WRITE | FA_CREATE_ALWAYS);
        if (res != FR_OK) {
            fprintf(stderr, "[WARN] resource_install: cannot create '%s' (%d)\n", r->name, res);
            continue;
        }

        UINT bw;
        res = f_write(&fw, r->data, (UINT)r->size, &bw);
        f_close(&fw);

        if (res == FR_OK && bw == r->size) {
            printf("[INFO]   installed: %s (%zu bytes)\n", r->name, r->size);
            count++;
        } else {
            fprintf(stderr, "[WARN] resource_install: write failed for '%s'\n", r->name);
        }
    }

    /* 写入标记文件 */
    FIL fw;
    if (f_open(&fw, INSTALL_MARKER, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        f_close(&fw);
    }

    printf("[INFO] Resource installation complete: %d files.\n", count);
    return count;
}
