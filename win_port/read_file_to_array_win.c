/**
 * @file    win_port/read_file_to_array_win.c
 * @brief   Windows 版 FATFS 文件读取 (不卸载卷)
 *
 * 原版 read_file_to_array.c 在读取失败时会调用
 * f_mount(NULL, "", 0) 卸载整个 FATFS 卷, 导致后续所有
 * 文件操作失败 (包括 BGM 播放).
 * Windows 版仅返回错误码, 不卸载卷.
 */

#include "ff.h"
#include <stdint.h>

int read_file_to_array(const char *filename, uint8_t *buffer, uint32_t max_size) {
    FIL file;
    FRESULT res;
    UINT bytes_read;

    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        return -1; /* 不卸载卷! */
    }

    FSIZE_t file_size = f_size(&file);
    if (file_size > max_size) {
        f_close(&file);
        return -1;
    }

    res = f_read(&file, buffer, file_size, &bytes_read);
    f_close(&file);

    if (res != FR_OK || bytes_read != file_size) {
        return -1;
    }

    return (int)bytes_read;
}
