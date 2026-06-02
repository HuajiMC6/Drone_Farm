/**
 * @file    win_port/resource_embed.h
 * @brief   嵌入式资源声明
 *
 * 构建时将静态资源 (图片/音频) 转换为 C 字节数组,
 * 运行时首次启动将其写入 SD 卡虚拟磁盘镜像.
 *
 * 使用方法:
 *   1. 将资源文件放入 resources/ 目录
 *   2. CMake 构建时自动生成 resources_embed.c (包含所有字节数组)
 *   3. 运行时调用 resource_install_all() 安装到 SD 镜像
 */

#ifndef WIN_PORT_RESOURCE_EMBED_H
#define WIN_PORT_RESOURCE_EMBED_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 单个嵌入资源的描述
 */
typedef struct {
    const char    *name;       /* 文件名 (含路径, 如 "images/img_load_bg.bin") */
    const uint8_t *data;
    size_t         size;
} resource_entry_t;

/**
 * @brief 所有嵌入资源列表 (以 NULL name 结尾)
 *
 * 由 CMake 生成的 resources_embed.c 定义.
 */
extern const resource_entry_t g_resources[];

/**
 * @brief 将所有嵌入资源解压到 FATFS 虚拟磁盘 (仅首次启动)
 *
 * 遍历 g_resources, 检查 "0:/xxx" 是否已存在,
 * 不存在则通过 FATFS f_open/f_write 写入.
 *
 * @return 成功写入的文件数
 */
int resource_install_all(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_RESOURCE_EMBED_H */
