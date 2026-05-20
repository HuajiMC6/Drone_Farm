/**
 * @file    speaker.c
 * @brief   I2S 扬声器驱动 - SPI1 I2S + DMA 双缓冲
 *          支持 PCM 音频播放 (BGM 循环 / SFX 一次性)
 *
 * 硬件引脚 (板载 MAX98357A, SD 已硬接 5V):
 *   PC1  - I2S_SD   (SPI1_MOSI,  AF5)
 *   PB12 - I2S_WS   (SPI1_NSS,   AF5)
 *   PB13 - I2S_CK   (SPI1_SCK,   AF5)
 *
 * 音频参数:
 *   时钟源: PLL0Q (200MHz), SPI1 I2S 主模式
 *   采样率: 44100Hz
 *   格式: 16-bit signed 单声道, I2S Philips 标准
 *
 * 架构:
 *   DMA 双缓冲循环模式 (2 x 512 采样点)
 *   主循环每帧检查 DMA 半区切换 -> 从 PCM 源复制数据到空闲半区
 *   BGM 循环播放, SFX 优先 (SFX 期间暂停 BGM)
 */

#include "speaker.h"
#include "drivers.h"
#include <stddef.h>
#include <string.h>

/* ============================================================
 *  硬件配置宏
 * ============================================================ */

/* SPI1 I2S 外设与引脚 */
#define SPK_SPI SPI1
#define SPK_SPI_INDEX IDX_SPI1
#define SPK_SPI_CLK RCU_SPI1
#define SPK_SPI_CLK_SRC RCU_SPISRC_PLL0Q

#define SPK_GPIO_PORT_DATA GPIOC
#define SPK_GPIO_PIN_DATA GPIO_PIN_1 /* PC1: SPI1_MOSI / I2S_SD */
#define SPK_GPIO_PORT_LRCLK GPIOB
#define SPK_GPIO_PIN_LRCLK GPIO_PIN_12 /* PB12: SPI1_NSS / I2S_WS */
#define SPK_GPIO_PORT_BCLK GPIOB
#define SPK_GPIO_PIN_BCLK GPIO_PIN_13 /* PB13: SPI1_SCK / I2S_CK */
#define SPK_GPIO_AF GPIO_AF_5         /* SPI1 使用 AF5 */

/* DMA 配置 */
#define SPK_DMA DMA0
#define SPK_DMA_CH DMA_CH6
#define SPK_DMA_REQUEST DMA_REQUEST_SPI1_TX

/* 音频参数 */
#define SPK_SAMPLE_RATE_HZ 44100U /* 44.1kHz, 匹配老师代码 */
#define SPK_BUF_SAMPLES 512U
#define SPK_BUF_TOTAL (SPK_BUF_SAMPLES * 2U)

/* ============================================================
 *  播放状态枚举 & PCM 上下文 (保持不变)
 * ============================================================ */
typedef enum {
    SPK_STATE_IDLE = 0,
    SPK_STATE_PLAYING,
} speaker_state_t;

/* ============================================================
 *  PCM 播放上下文
 * ============================================================ */
typedef struct {
    const uint16_t *data;   /* PCM 数据指针 */
    uint32_t total_samples; /* 总采样数 */
    uint32_t position;      /* 当前播放位置 (采样索引) */
    uint8_t loop;           /* 0=单次, 1=循环 */
} speaker_pcm_ctx_t;

/* ============================================================
 *  静态变量
 * ============================================================ */

/* 双缓冲: 连续存储, 每个元素是 32-bit (低16位=采样, 高16位=0) */
static uint32_t s_buf[SPK_BUF_TOTAL];

/* DMA 当前所在的半区 (0 或 1) */
static volatile uint8_t s_dma_half = 0U;

/* 播放状态 */
static speaker_state_t s_state = SPK_STATE_IDLE;

/* 音量 0~100 -> 幅度缩放 (Q16) */
static uint32_t s_volume = 65536U;

/* BGM 播放上下文 (循环) */
static speaker_pcm_ctx_t s_bgm_ctx;
static uint8_t s_bgm_active = 0U;

/* SFX 播放上下文 (一次性, 优先于 BGM) */
static speaker_pcm_ctx_t s_sfx_ctx;
static uint8_t s_sfx_active = 0U;

/* BGM 暂停标志 (SFX 播放期间) */
static uint8_t s_bgm_paused = 0U;

/* ============================================================
 *  内部函数声明
 * ============================================================ */

static void speaker_gpio_init(void);
static void speaker_i2s_init(void);
static void speaker_dma_init(void);
static void speaker_start_dma(void);
static void speaker_dma_poll(void);
static uint32_t speaker_fill_pcm(speaker_pcm_ctx_t *ctx, uint32_t *buf, uint32_t count);

/* ============================================================
 *  GPIO 初始化
 * ============================================================ */
static void speaker_gpio_init(void) {
    /* 使能 GPIO 时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* DATA: PC1 */
    gpio_af_set(SPK_GPIO_PORT_DATA, SPK_GPIO_AF, SPK_GPIO_PIN_DATA);
    gpio_mode_set(SPK_GPIO_PORT_DATA, GPIO_MODE_AF, GPIO_PUPD_NONE, SPK_GPIO_PIN_DATA);
    gpio_output_options_set(SPK_GPIO_PORT_DATA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SPK_GPIO_PIN_DATA);

    /* LRCLK: PB12 */
    gpio_af_set(SPK_GPIO_PORT_LRCLK, SPK_GPIO_AF, SPK_GPIO_PIN_LRCLK);
    gpio_mode_set(SPK_GPIO_PORT_LRCLK, GPIO_MODE_AF, GPIO_PUPD_NONE, SPK_GPIO_PIN_LRCLK);
    gpio_output_options_set(SPK_GPIO_PORT_LRCLK, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SPK_GPIO_PIN_LRCLK);

    /* BCLK: PB13 */
    gpio_af_set(SPK_GPIO_PORT_BCLK, SPK_GPIO_AF, SPK_GPIO_PIN_BCLK);
    gpio_mode_set(SPK_GPIO_PORT_BCLK, GPIO_MODE_AF, GPIO_PUPD_NONE, SPK_GPIO_PIN_BCLK);
    gpio_output_options_set(SPK_GPIO_PORT_BCLK, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, SPK_GPIO_PIN_BCLK);

    /* MAX98357A SD 已硬接 5V, 无需 GPIO 控制 */
}

/* ============================================================
 *  SPI1 I2S 初始化 (参考老师代码)
 * ============================================================ */
static void speaker_i2s_init(void) {
    /* 使能 SPI1 时钟 */
    rcu_periph_clock_enable(SPK_SPI_CLK);

    /* SPI1 I2S 时钟源: PLL0Q (200MHz) */
    rcu_spi_clock_config(SPK_SPI_INDEX, SPK_SPI_CLK_SRC);

    /* 复位 SPI1 */
    spi_i2s_deinit(SPK_SPI);

    /* I2S 时钟分频: 44.1kHz, 16-bit 数据, 16-bit 通道, 无 MCLK */
    i2s_psc_config(SPK_SPI, I2S_AUDIOSAMPLE_44K, I2S_FRAMEFORMAT_DT16B_CH16B, I2S_MCKOUT_DISABLE);

    /* I2S 模式: 主发送, Philips 标准, 时钟极性低 */
    i2s_init(SPK_SPI, I2S_MODE_MASTERTX, I2S_STD_PHILIPS, I2S_CKPL_LOW);
}

/* ============================================================
 *  DMA 初始化 (双缓冲循环模式)
 * ============================================================ */
static void speaker_dma_init(void) {
    dma_single_data_parameter_struct dma_para;

    /* 使能 DMA 时钟 */
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);

    /* 先禁用通道以便配置 */
    dma_channel_disable(SPK_DMA, SPK_DMA_CH);

    dma_single_data_para_struct_init(&dma_para);
    dma_para.request = SPK_DMA_REQUEST;
    dma_para.direction = DMA_MEMORY_TO_PERIPH;
    dma_para.periph_addr = (uint32_t)(SPI1 + 0x00000020U); /* SPI_TDATA 寄存器 */
    dma_para.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_para.memory0_addr = (uint32_t)s_buf;
    dma_para.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_para.periph_memory_width = DMA_PERIPH_WIDTH_32BIT; /* SPI_TDATA 是 32 位寄存器 */
    dma_para.number = SPK_BUF_TOTAL;                       /* 2 × 512 = 1024 个半字 */
    dma_para.priority = DMA_PRIORITY_HIGH;

    dma_single_data_mode_init(SPK_DMA, SPK_DMA_CH, &dma_para);

    /* 循环模式: DMA 自动回绕 */
    dma_circulation_enable(SPK_DMA, SPK_DMA_CH);
}

/* ============================================================
 *  从 PCM 上下文填充缓冲区 (立体声: 每个采样复制为 L+R 对)
 * ============================================================ */
static uint32_t speaker_fill_pcm(speaker_pcm_ctx_t *ctx, uint32_t *buf, uint32_t count) {
    uint32_t i, pair_count;
    uint32_t remaining;
    uint32_t to_copy_pairs;
    int32_t sample;
    uint32_t packed;

    /* count 是 32-bit 字数, 立体声每对=2字(L+R), 源采样数=count/2 */
    pair_count = count / 2U;

    if (ctx == NULL || ctx->data == NULL || ctx->total_samples == 0U) {
        memset(buf, 0, count * sizeof(uint32_t));
        return 0U;
    }

    remaining = ctx->total_samples - ctx->position;
    to_copy_pairs = (pair_count < remaining) ? pair_count : remaining;

    /* 每个源采样写两次: buf[2i]=L, buf[2i+1]=R (同 L, 单声道) */
    for (i = 0U; i < to_copy_pairs; i++) {
        sample = (int32_t)((int16_t)ctx->data[ctx->position + i]);
        sample = (sample * (int32_t)s_volume) >> 16;
        if (sample > 32767)
            sample = 32767;
        else if (sample < -32768)
            sample = -32768;
        packed = (uint32_t)(uint16_t)(int16_t)sample;
        buf[i * 2U] = packed;
        buf[i * 2U + 1U] = packed;
    }

    ctx->position += to_copy_pairs;

    if (to_copy_pairs < pair_count) {
        if (ctx->loop) {
            ctx->position = 0U;
            for (i = to_copy_pairs; i < pair_count; i++) {
                if (ctx->position >= ctx->total_samples)
                    ctx->position = 0U;
                sample = (int32_t)((int16_t)ctx->data[ctx->position]);
                sample = (sample * (int32_t)s_volume) >> 16;
                if (sample > 32767)
                    sample = 32767;
                else if (sample < -32768)
                    sample = -32768;
                packed = (uint32_t)(uint16_t)(int16_t)sample;
                buf[i * 2U] = packed;
                buf[i * 2U + 1U] = packed;
                ctx->position++;
            }
        } else {
            memset(&buf[to_copy_pairs * 2U], 0, (count - to_copy_pairs * 2U) * sizeof(uint32_t));
        }
    }

    return to_copy_pairs * 2U;
}

/* ============================================================
 *  检查 DMA 半区切换, 从 PCM 源填充空闲半区
 * ============================================================ */
static void speaker_dma_poll(void) {
    uint32_t cnt;
    uint8_t current_half;
    uint32_t *fill_buf;
    speaker_pcm_ctx_t *active_ctx = NULL;

    /* 确定当前活跃的 PCM 上下文 (SFX 优先) */
    if (s_sfx_active) {
        active_ctx = &s_sfx_ctx;
    } else if (s_bgm_active && !s_bgm_paused) {
        active_ctx = &s_bgm_ctx;
    }

    /* 读取 DMA 剩余传输计数 */
    cnt = DMA_CHCNT(SPK_DMA, SPK_DMA_CH);

    /* CHCNT > SPK_BUF_SAMPLES -> DMA 还在前半 (buf0)
     * CHCNT <= SPK_BUF_SAMPLES -> DMA 已进入后半 (buf1) */
    current_half = (cnt > SPK_BUF_SAMPLES) ? 0U : 1U;

    if (current_half != s_dma_half) {
        if (s_dma_half == 0U) {
            fill_buf = s_buf;
        } else {
            fill_buf = &s_buf[SPK_BUF_SAMPLES];
        }

        if (active_ctx != NULL) {
            uint32_t written = speaker_fill_pcm(active_ctx, fill_buf, SPK_BUF_SAMPLES);

            /* 非循环模式下, 返回数 < 请求数 表示 PCM 已播完 */
            if (!active_ctx->loop && written < SPK_BUF_SAMPLES) {
                if (s_sfx_active) {
                    s_sfx_active = 0U;
                    s_sfx_ctx.data = NULL;
                    /* 恢复 BGM */
                    if (s_bgm_active) {
                        s_bgm_paused = 0U;
                    }
                }
                if (s_bgm_active && !s_bgm_ctx.loop) {
                    s_bgm_active = 0U;
                    s_bgm_ctx.data = NULL;
                }
                if (!s_sfx_active && !s_bgm_active) {
                    s_state = SPK_STATE_IDLE;
                }
            }
        } else {
            memset(fill_buf, 0, SPK_BUF_SAMPLES * sizeof(uint32_t));
        }

        s_dma_half = current_half;
    }
}

/* ============================================================
 *  启动 DMA
 * ============================================================ */
static void speaker_start_dma(void) {
    dma_channel_enable(SPK_DMA, SPK_DMA_CH);
}

/* ============================================================
 *  API: 初始化扬声器
 * ============================================================ */
void speaker_init(void) {
    speaker_gpio_init();
    speaker_i2s_init();

    /* 使能 SPI1 中断 (SPI1_IRQHandler 调用 speaker_i2s_feed) */
    nvic_irq_enable(SPI1_IRQn, 1, 0);

    /* 使能 I2S 并启动传输 */
    i2s_enable(SPK_SPI);
    spi_master_transfer_start(SPK_SPI, SPI_TRANS_START);

    /* 使能 I2S TX 空中断 */
    spi_i2s_interrupt_enable(SPK_SPI, SPI_I2S_INT_TP);

    s_state = SPK_STATE_IDLE;
    s_bgm_active = 0U;
    s_sfx_active = 0U;
    s_bgm_paused = 0U;
    memset(&s_bgm_ctx, 0, sizeof(s_bgm_ctx));
    memset(&s_sfx_ctx, 0, sizeof(s_sfx_ctx));
}

/* ============================================================
 *  API: 每帧更新 (主循环中调用)
 * ============================================================ */
void speaker_update(void) {
    /* 中断驱动, 无需轮询 */
}

/* ============================================================
 *  API: 播放 PCM 音频
 * ============================================================ */
void speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop) {
    if (pcm_data == NULL || sample_count == 0U) {
        return;
    }

    if (loop) {
        /* 循环播放 = BGM */
        s_bgm_ctx.data = pcm_data;
        s_bgm_ctx.total_samples = sample_count;
        s_bgm_ctx.position = 0U;
        s_bgm_ctx.loop = 1U;
        s_bgm_active = 1U;
        s_bgm_paused = 0U;

        if (!s_sfx_active) {
            s_state = SPK_STATE_PLAYING;
        }
    } else {
        /* 一次性播放 = SFX (优先于 BGM) */
        if (s_bgm_active) {
            s_bgm_paused = 1U;
        }

        s_sfx_ctx.data = pcm_data;
        s_sfx_ctx.total_samples = sample_count;
        s_sfx_ctx.position = 0U;
        s_sfx_ctx.loop = 0U;
        s_sfx_active = 1U;

        s_state = SPK_STATE_PLAYING;
    }
}

/* ============================================================
 *  API: 停止所有播放
 * ============================================================ */
void speaker_stop(void) {
    s_state = SPK_STATE_IDLE;
    s_bgm_active = 0U;
    s_sfx_active = 0U;
    s_bgm_paused = 0U;
    memset(&s_bgm_ctx, 0, sizeof(s_bgm_ctx));
    memset(&s_sfx_ctx, 0, sizeof(s_sfx_ctx));
}

/* ============================================================
 *  API: 设置音量
 * ============================================================ */
void speaker_set_volume(uint8_t vol) {
    if (vol > 100U) {
        vol = 100U;
    }
    s_volume = ((uint32_t)vol * 65536U) / 100U;
}

/* ============================================================
 *  API: 查询是否正在播放
 * ============================================================ */
uint8_t speaker_is_playing(void) {
    return (s_state != SPK_STATE_IDLE) ? 1U : 0U;
}

/* ============================================================
 *  I2S 中断喂数据 (由 SPI1_IRQHandler 调用)
 *  立体声: 每两次 ISR 发送同一采样 (L + R)
 * ============================================================ */
void speaker_i2s_feed(void) {
    speaker_pcm_ctx_t *ctx = NULL;
    static uint8_t isr_phase = 0U; /* 0=需要新采样, 1=重复上次 */
    static uint16_t last_sample = 0U;
    int32_t sample;
    uint16_t out;

    /* 选活跃上下文: SFX 优先 */
    if (s_sfx_active) {
        ctx = &s_sfx_ctx;
    } else if (s_bgm_active && !s_bgm_paused) {
        ctx = &s_bgm_ctx;
    }

    if (ctx == NULL || ctx->data == NULL || ctx->total_samples == 0U) {
        spi_i2s_data_transmit(SPK_SPI, 0);
        isr_phase = 0U;
        return;
    }

    if (isr_phase == 0U) {
        /* 取新采样 */
        if (ctx->position >= ctx->total_samples) {
            if (ctx->loop) {
                ctx->position = 0U;
            } else {
                /* 播放完毕 */
                spi_i2s_data_transmit(SPK_SPI, 0);
                if (s_sfx_active) {
                    s_sfx_active = 0U;
                    s_sfx_ctx.data = NULL;
                    if (s_bgm_active)
                        s_bgm_paused = 0U;
                } else {
                    s_bgm_active = 0U;
                    s_bgm_ctx.data = NULL;
                }
                if (!s_sfx_active && !s_bgm_active)
                    s_state = SPK_STATE_IDLE;
                isr_phase = 0U;
                return;
            }
        }

        sample = (int32_t)((int16_t)ctx->data[ctx->position]);
        sample = (sample * (int32_t)s_volume) >> 16;
        if (sample > 32767)
            sample = 32767;
        else if (sample < -32768)
            sample = -32768;
        last_sample = (uint16_t)(int16_t)sample;
        ctx->position++;
    }

    spi_i2s_data_transmit(SPK_SPI, last_sample);
    isr_phase ^= 1U;
}
