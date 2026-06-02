#ifndef WIN_PORT_COMPAT_H
#define WIN_PORT_COMPAT_H

/* ================================================================
 *  win_port/compat.h — MSVC / MinGW 编译器兼容性宏
 *
 *  嵌入式代码使用了 ARMCC / GCC-ARM 特有的语法。这里桥接到
 *  Windows 编译器 (MinGW GCC / MSVC)。
 * ================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* --- CMSIS 兼容宏 (仅当未定义时) --- */
#ifndef __IO
#define __IO volatile
#endif
/* 注意: 不定义 __I 和 __O, 它们可能与 GCC 内置头文件冲突 */

/* --- ARMCC section attribute → compiler-neutral (仅 MSVC 需要) --- */
#ifdef _MSC_VER
#define __attribute__(x)
#endif

/* --- FATFS 类型 (由 FATFS/ff.h 自行定义, 这里不覆盖) --- */
/* ff.h 会定义 BYTE, WORD, DWORD, UINT, LBA_t 等类型, 不要在 compat.h 中重复定义 */

/* --- 嵌入式通常不包含的 C99 特性 --- */
/* math.h 需要显式链接 -lm (Unix) 或不需 (MinGW) */

/* --- MSVC missing stdbool workaround (MinGW already has it) --- */
#ifdef _MSC_VER
  /* MSVC 2013+ has stdbool.h, but just in case */
  #ifndef __bool_true_false_are_defined
    #define bool  int
    #define true  1
    #define false 0
  #endif
  /* MSVC 不支持 VLA, 使用 malloc 替代 */
  #define __STDC_NO_VLA__ 1
  /* 抑制不安全函数警告 */
  #define _CRT_SECURE_NO_WARNINGS
#endif

/* --- Windows 平台通用 --- */
#ifdef _WIN32
  /* 禁用 min/max 宏冲突 */
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_COMPAT_H */
