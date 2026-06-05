#include "display_sdl.h"
#include "compat.h"

#include <SDL2/SDL.h>
#include <stdio.h>

/* ================================================================
 *  内部状态
 * ================================================================ */

#define DISP_HOR_RES 1024
#define DISP_VER_RES 600

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_texture = NULL;

static lv_disp_drv_t *s_disp_drv = NULL;

/* 双缓冲 (与嵌入式版一致: 2×100 行) */
#define BUF_ROWS 100
static lv_color_t buf_2_1[DISP_HOR_RES * BUF_ROWS];
static lv_color_t buf_2_2[DISP_HOR_RES * BUF_ROWS];

/* ================================================================
 *  LVGL flush 回调 — 将渲染结果通过 SDL 纹理显示
 * ================================================================ */

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    /* 更新纹理的脏区域 */
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;

    SDL_Rect rect = {area->x1, area->y1, w, h};
    SDL_UpdateTexture(g_texture, &rect, color_p, w * sizeof(lv_color_t));

    /* ★ 关键: 必须通知 LVGL flush 完成, 否则 LVGL 会阻塞等待 */
    lv_disp_flush_ready(disp_drv);
}

/* ================================================================
 *  公开接口
 * ================================================================ */

void lv_port_disp_init(void) {
    /* 创建窗口 (SDL 由 main_win.c 统一初始化) */
    g_window = SDL_CreateWindow("Drone Farm (Windows Port)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                DISP_HOR_RES, DISP_VER_RES, SDL_WINDOW_SHOWN);
    if (!g_window) {
        fprintf(stderr, "[FATAL] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    /* 创建渲染器 (软件渲染, 避免 VSYNC 兼容性问题) */
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_renderer) {
        fprintf(stderr, "[FATAL] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return;
    }

    /* 创建纹理 (RGB565 — 与原嵌入式硬件一致) */
    g_texture =
        SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, DISP_HOR_RES, DISP_VER_RES);
    if (!g_texture) {
        fprintf(stderr, "[FATAL] SDL_CreateTexture failed: %s\n", SDL_GetError());
        return;
    }

    /* 初始化 LVGL 显示缓冲 (双缓冲 100 行) */
    static lv_disp_draw_buf_t draw_buf_dsc;
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_2_1, buf_2_2, DISP_HOR_RES * BUF_ROWS);

    /* 注册 LVGL 显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;
    lv_disp_drv_register(&disp_drv);
    s_disp_drv = &disp_drv;

    printf("[INFO] SDL display initialized: %dx%d RGB565\n", DISP_HOR_RES, DISP_VER_RES);
}

void lv_port_disp_flush_ready(void) {
    if (s_disp_drv) {
        lv_disp_flush_ready(s_disp_drv);
    }
}

void *lv_port_disp_get_renderer(void) {
    return g_renderer;
}

void *lv_port_disp_get_window(void) {
    return g_window;
}

/* ================================================================
 *  SDL 渲染循环辅助 — 供 main_win.c 调用
 * ================================================================ */

/**
 * @brief 将 LVGL 渲染结果提交到屏幕
 *
 * 每帧在 lv_timer_handler() 之后调用一次。
 */
void lv_port_disp_render(void) {
    if (!g_renderer || !g_texture)
        return;

    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}
