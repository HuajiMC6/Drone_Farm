#ifndef SAVE_UTILS_H
#define SAVE_UTILS_H

#include "ff.h"
#include <stdbool.h>

// f_write/f_read 包装，失败自动关文件返回 false
// 要求当前函数里有 FIL fil 局部变量

static inline bool save_write_int(FIL *fil, int value) {
    UINT bw;
    return f_write(fil, &value, sizeof(int), &bw) == FR_OK && bw == sizeof(int);
}

static inline bool save_read_int(FIL *fil, int *value) {
    UINT br;
    return f_read(fil, value, sizeof(int), &br) == FR_OK && br == sizeof(int);
}

static inline bool save_write_double(FIL *fil, double value) {
    UINT bw;
    return f_write(fil, &value, sizeof(double), &bw) == FR_OK && bw == sizeof(double);
}

static inline bool save_read_double(FIL *fil, double *value) {
    UINT br;
    return f_read(fil, value, sizeof(double), &br) == FR_OK && br == sizeof(double);
}

#define SAVE_CHECK(expr)                                                                                               \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            f_close(&fil);                                                                                             \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#endif
