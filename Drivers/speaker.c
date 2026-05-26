/**
 * @file    speaker.c
 * @brief   I2S 扬声器驱动 - SPI1 I2S + 软件缓冲播放
 *          支持 PCM 音频播放、流式背景音乐和短音效
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
 *   I2S 中断按采样节奏取数输出
 *   主循环负责补充流式 BGM 的环形缓冲
 *   PCM 文件可先读入 SDRAM，也可以直接流式播放
 */

#include "speaker.h"
#include "drivers.h"
#include "ff.h"
#include "sdram_malloc.h"
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

/* SPI1 I2S 发送相关宏 */
#define SPK_DMA DMA0
#define SPK_DMA_CH DMA_CH6
#define SPK_DMA_REQUEST DMA_REQUEST_SPI1_TX

/* 音频参数 */
#define SPK_SAMPLE_RATE_HZ 44100U /* 44.1kHz, 匹配老师代码 */
#define SPK_BUF_SAMPLES 512U
#define SPK_BUF_TOTAL (SPK_BUF_SAMPLES * 2U)

/* 源切换去爆音: 在播放源切换瞬间做短淡入 */
#define SPK_DECLICK_SAMPLES 32U

/* 流式 BGM 的环形缓冲 */
#define SPK_STREAM_RING_SAMPLES 4096U
#define SPK_STREAM_READ_CHUNK 512U
#define SPK_STREAM_RING_MASK (SPK_STREAM_RING_SAMPLES - 1U)

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

typedef struct {
    FIL fil;
    uint8_t active;
    uint8_t loop;
    uint8_t eof;
    FSIZE_t file_size;
} speaker_stream_file_ctx_t;

/* ============================================================
 *  静态变量
 * ============================================================ */

/* 保留的半区标记，当前实现主要依赖中断取样 */
static volatile uint8_t s_dma_half = 0U;

/* 播放状态 */
static speaker_state_t s_state = SPK_STATE_IDLE;

/* 音量 0~100 -> 幅度缩放 (Q16) */
static uint32_t s_volume = 65536U;

/* 内存 PCM 播放上下文：通常作为背景音乐使用 */
static speaker_pcm_ctx_t s_bgm_ctx;
static uint8_t s_bgm_active = 0U;

/* 内存 PCM 播放上下文：通常作为短音效使用 */
static speaker_pcm_ctx_t s_sfx_ctx;
static uint8_t s_sfx_active = 0U;

/* 播放切换期间的暂停标志 */
static uint8_t s_bgm_paused = 0U;

/* 流式背景音乐状态 */
static speaker_stream_file_ctx_t s_stream_ctx;
static uint16_t s_stream_ring[SPK_STREAM_RING_SAMPLES];
static uint16_t s_stream_chunk[SPK_STREAM_READ_CHUNK];
static volatile uint32_t s_stream_head = 0U;
static volatile uint32_t s_stream_tail = 0U;
static volatile uint32_t s_stream_count = 0U;
static uint8_t s_stream_active = 0U;

/* ============================================================
 *  内部函数声明
 * ============================================================ */

static void speaker_gpio_init(void);
static void speaker_i2s_init(void);
static uint32_t speaker_irq_save(void);
static void speaker_irq_restore(uint32_t primask);
static void speaker_stream_reset(void);
static void speaker_stream_close(void);
static uint32_t speaker_stream_space_locked(void);
static uint32_t speaker_stream_count_locked(void);
static uint32_t speaker_stream_push_locked(const uint16_t *data, uint32_t samples);
static uint8_t speaker_stream_pop_locked(uint16_t *sample);
static void speaker_stream_refill(void);
static uint8_t speaker_pcm_next_sample(speaker_pcm_ctx_t *ctx, uint16_t *sample);
static uint8_t speaker_active_source_id(void);

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
 *  SPI1 I2S 初始化
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
 *  进入/退出临界区 (保护流式环形缓冲)
 * ============================================================ */
static uint32_t speaker_irq_save(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void speaker_irq_restore(uint32_t primask) {
    if (primask == 0U) {
        __enable_irq();
    }
}

/* ============================================================
 *  流式音频环形缓冲管理
 * ============================================================ */
static void speaker_stream_reset(void) {
    uint32_t primask = speaker_irq_save();
    s_stream_head = 0U;
    s_stream_tail = 0U;
    s_stream_count = 0U;
    speaker_irq_restore(primask);
}

static void speaker_stream_close(void) {
    if (s_stream_ctx.active) {
        f_close(&s_stream_ctx.fil);
    }

    s_stream_ctx.active = 0U;
    s_stream_ctx.loop = 0U;
    s_stream_ctx.eof = 0U;
    s_stream_ctx.file_size = 0U;
    s_stream_active = 0U;
    speaker_stream_reset();
}

static uint32_t speaker_stream_space_locked(void) {
    return SPK_STREAM_RING_SAMPLES - s_stream_count;
}

static uint32_t speaker_stream_count_locked(void) {
    return s_stream_count;
}

static uint32_t speaker_stream_push_locked(const uint16_t *data, uint32_t samples) {
    uint32_t pushed = 0U;

    while (pushed < samples && s_stream_count < SPK_STREAM_RING_SAMPLES) {
        s_stream_ring[s_stream_head] = data[pushed];
        s_stream_head = (s_stream_head + 1U) & SPK_STREAM_RING_MASK;
        s_stream_count++;
        pushed++;
    }

    return pushed;
}

static uint8_t speaker_stream_pop_locked(uint16_t *sample) {
    if (s_stream_count == 0U) {
        return 0U;
    }

    *sample = s_stream_ring[s_stream_tail];
    s_stream_tail = (s_stream_tail + 1U) & SPK_STREAM_RING_MASK;
    s_stream_count--;
    return 1U;
}

static void speaker_stream_refill(void) {
    if (!s_stream_active || s_bgm_paused || !s_stream_ctx.active) {
        return;
    }

    while (1) {
        uint32_t space;
        uint32_t to_read_samples;
        UINT bytes_read = 0U;
        FRESULT res;

        {
            uint32_t primask = speaker_irq_save();
            space = speaker_stream_space_locked();
            speaker_irq_restore(primask);
        }

        if (space == 0U) {
            break;
        }

        if (space > SPK_STREAM_READ_CHUNK) {
            to_read_samples = SPK_STREAM_READ_CHUNK;
        } else {
            to_read_samples = space;
        }

        res = f_read(&s_stream_ctx.fil, s_stream_chunk, (UINT)(to_read_samples * sizeof(uint16_t)), &bytes_read);
        if (res != FR_OK) {
            speaker_stream_close();
            return;
        }

        if (bytes_read == 0U) {
            if (s_stream_ctx.loop) {
                if (f_lseek(&s_stream_ctx.fil, 0U) != FR_OK) {
                    speaker_stream_close();
                    return;
                }
                continue;
            }

            s_stream_ctx.eof = 1U;
            break;
        }

        uint32_t read_samples = (uint32_t)(bytes_read / sizeof(uint16_t));
        if (read_samples > 0U) {
            uint32_t primask = speaker_irq_save();
            (void)speaker_stream_push_locked(s_stream_chunk, read_samples);
            speaker_irq_restore(primask);
        }

        if (bytes_read < (UINT)(to_read_samples * sizeof(uint16_t))) {
            if (s_stream_ctx.loop) {
                if (f_lseek(&s_stream_ctx.fil, 0U) != FR_OK) {
                    speaker_stream_close();
                    return;
                }
                continue;
            }

            s_stream_ctx.eof = 1U;
            break;
        }
    }

    if (s_stream_ctx.eof) {
        uint32_t primask = speaker_irq_save();
        uint32_t remaining = speaker_stream_count_locked();
        speaker_irq_restore(primask);

        if (remaining == 0U) {
            speaker_stream_close();
            if (!s_sfx_active && !s_bgm_active) {
                s_state = SPK_STATE_IDLE;
            }
        }
    }
}

static uint8_t speaker_pcm_next_sample(speaker_pcm_ctx_t *ctx, uint16_t *sample) {
    int32_t value;

    if (ctx == NULL || sample == NULL || ctx->data == NULL || ctx->total_samples == 0U) {
        return 0U;
    }

    if (ctx->position >= ctx->total_samples) {
        if (ctx->loop) {
            ctx->position = 0U;
        } else {
            return 0U;
        }
    }

    value = (int32_t)((int16_t)ctx->data[ctx->position]);
    value = (value * (int32_t)s_volume) >> 16;
    if (value > 32767) {
        value = 32767;
    } else if (value < -32768) {
        value = -32768;
    }

    *sample = (uint16_t)(int16_t)value;
    ctx->position++;
    return 1U;
}

static uint8_t speaker_stream_next_sample(uint16_t *sample) {
    uint32_t primask;
    uint8_t ok;

    if (!s_stream_active || s_bgm_paused || sample == NULL) {
        return 0U;
    }

    primask = speaker_irq_save();
    ok = speaker_stream_pop_locked(sample);
    speaker_irq_restore(primask);

    if (ok) {
        int32_t value = (int32_t)(int16_t)(*sample);
        value = (value * (int32_t)s_volume) >> 16;
        if (value > 32767) {
            value = 32767;
        } else if (value < -32768) {
            value = -32768;
        }
        *sample = (uint16_t)(int16_t)value;
    }

    return ok;
}

static uint8_t speaker_next_sample(uint16_t *sample) {
    if (sample == NULL) {
        return 0U;
    }

    if (s_sfx_active) {
        if (speaker_pcm_next_sample(&s_sfx_ctx, sample)) {
            return 1U;
        }

        s_sfx_active = 0U;
        s_sfx_ctx.data = NULL;
        if (s_bgm_active || s_stream_active) {
            s_bgm_paused = 0U;
        }
        if (!s_bgm_active && !s_stream_active) {
            s_state = SPK_STATE_IDLE;
        }
        return 0U;
    }

    if (s_bgm_active && !s_bgm_paused) {
        if (speaker_pcm_next_sample(&s_bgm_ctx, sample)) {
            return 1U;
        }

        s_bgm_active = 0U;
        s_bgm_ctx.data = NULL;
        if (!s_sfx_active && !s_stream_active) {
            s_state = SPK_STATE_IDLE;
        }
        return 0U;
    }

    if (s_stream_active && !s_bgm_paused) {
        return speaker_stream_next_sample(sample);
    }

    return 0U;
}

static uint8_t speaker_active_source_id(void) {
    uint8_t mask = 0U;

    if (s_sfx_active) {
        mask |= 0x01U;
    }

    if (s_bgm_active) {
        mask |= 0x02U;
    }

    if (s_stream_active) {
        mask |= 0x04U;
    }

    return mask;
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
    /* 流式播放需要主循环补充环形缓冲 */
    speaker_stream_refill();
}

/* ============================================================
 *  API: 播放 PCM 音频
 * ============================================================ */
void speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop) {
    if (pcm_data == NULL || sample_count == 0U) {
        return;
    }

    if (loop) {
        /* 切换到内存 PCM 前, 关闭可能存在的流式播放 */
        speaker_stream_close();

        /* loop=1 时通常作为背景音乐 */
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
        /* loop=0 时通常作为短音效 */
        if (s_bgm_active || s_stream_active) {
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
    speaker_stream_close();
    s_state = SPK_STATE_IDLE;
    s_bgm_active = 0U;
    s_sfx_active = 0U;
    s_bgm_paused = 0U;
    memset(&s_bgm_ctx, 0, sizeof(s_bgm_ctx));
    memset(&s_sfx_ctx, 0, sizeof(s_sfx_ctx));
}

/* ============================================================
 *  API: 从 SD 卡加载 PCM 到 SDRAM 后播放
 *  适合短音效，播放结束后由调用者决定是否释放内存
 * ============================================================ */
int speaker_play_pcm_file(const TCHAR *path, uint8_t loop) {
    FIL fil;
    FRESULT res;
    UINT br;
    FSIZE_t file_size;
    uint32_t sample_count;
    uint16_t *pcm_buf;

    if (path == NULL) {
        return -1;
    }

    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) {
        return -1;
    }

    file_size = f_size(&fil);
    if (file_size < 2U || (file_size & 1U) != 0U) {
        f_close(&fil);
        return -1;
    }
    sample_count = (uint32_t)(file_size / 2U);

    pcm_buf = (uint16_t *)sdram_malloc((size_t)file_size);
    if (pcm_buf == NULL) {
        f_close(&fil);
        return -1;
    }

    res = f_read(&fil, pcm_buf, (UINT)file_size, &br);
    f_close(&fil);

    if (res != FR_OK || br != (UINT)file_size) {
        sdram_free(pcm_buf);
        return -1;
    }

    speaker_play_pcm(pcm_buf, sample_count, loop);
    return 0;
}

/* ============================================================
 *  API: 从 SD 卡流式播放背景音乐
 *  适合长音频，只保留小缓冲区
 * ============================================================ */
int speaker_play_bgm_stream_file(const TCHAR *path, uint8_t loop) {
    FRESULT res;

    if (path == NULL) {
        return -1;
    }

    speaker_stop();

    res = f_open(&s_stream_ctx.fil, path, FA_READ);
    if (res != FR_OK) {
        return -1;
    }

    s_stream_ctx.active = 1U;
    s_stream_ctx.loop = (loop != 0U) ? 1U : 0U;
    s_stream_ctx.eof = 0U;
    s_stream_ctx.file_size = f_size(&s_stream_ctx.fil);

    if (s_stream_ctx.file_size < 2U || (s_stream_ctx.file_size & 1U) != 0U) {
        speaker_stream_close();
        return -1;
    }

    speaker_stream_reset();
    s_stream_active = 1U;
    s_state = SPK_STATE_PLAYING;

    speaker_stream_refill();

    if (!s_stream_active) {
        return -1;
    }

    return 0;
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
    uint32_t stream_pending;

    {
        uint32_t primask = speaker_irq_save();
        stream_pending = speaker_stream_count_locked();
        speaker_irq_restore(primask);
    }

    if (s_state != SPK_STATE_IDLE || s_stream_active || stream_pending > 0U) {
        return 1U;
    }

    return 0U;
}

/* ============================================================
 *  I2S 中断喂数据 (由 SPI1_IRQHandler 调用)
 *  单声道 PCM 通过重复采样方式输出到 I2S
 * ============================================================ */
void speaker_i2s_feed(void) {
    static uint8_t isr_phase = 0U; /* 0=需要新采样, 1=重复上次 */
    static uint16_t last_sample = 0U;
    static uint8_t prev_source = 0U;
    static uint8_t declick_left = 0U;
    uint16_t sample = 0U;
    uint8_t source;
    int32_t s32;
    uint32_t step;

    if (isr_phase == 0U) {
        source = speaker_active_source_id();

        if (!speaker_next_sample(&sample)) {
            spi_i2s_data_transmit(SPK_SPI, 0U);
            isr_phase = 0U;
            prev_source = 0U;
            declick_left = 0U;
            return;
        }

        if (source != prev_source) {
            prev_source = source;
            declick_left = SPK_DECLICK_SAMPLES;
        }

        if (declick_left > 0U) {
            step = SPK_DECLICK_SAMPLES - declick_left + 1U;
            s32 = (int32_t)(int16_t)sample;
            s32 = (s32 * (int32_t)step) / (int32_t)SPK_DECLICK_SAMPLES;
            sample = (uint16_t)(int16_t)s32;
            declick_left--;
        }

        last_sample = sample;
    }

    spi_i2s_data_transmit(SPK_SPI, last_sample);
    isr_phase ^= 1U;
}
