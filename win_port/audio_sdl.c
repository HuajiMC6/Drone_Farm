#include "audio_sdl.h"
#include "compat.h"
#include "ff.h"

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 *  常量
 * ================================================================ */

#define SAMPLE_RATE 44100
#define NUM_SLOTS 4               /* 与嵌入式版一致 */
#define RING_BUFFER_SAMPLES 16384 /* 环形缓冲 ~372ms @ 44.1kHz */

/* ---- 流式播放: 每次从文件读取的采样数 ---- */
#define STREAM_CHUNK_SAMPLES 2048

/* ================================================================
 *  播放槽位结构
 * ================================================================ */

typedef enum {
    SLOT_FREE = 0,
    SLOT_MEMORY, /* 内存中 PCM, 一次性或循环 */
    SLOT_STREAM  /* 文件流式播放 */
} slot_mode_t;

typedef struct {
    slot_mode_t mode;

    /* 内存模式 */
    const uint16_t *pcm_data; /* 源数据指针 */
    uint32_t pcm_total;       /* 总采样数 */
    uint32_t pcm_pos;         /* 当前播放位置 */
    uint8_t loop;             /* 是否循环 */

    /* 流模式 */
    char file_path[256];                   /* 文件路径 */
    FILE *file_handle;                     /* Windows 文件句柄 */
    uint32_t file_size_samples;            /* 文件总采样数 */
    uint32_t file_pos_samples;             /* 当前读取位置 (采样) */
    int16_t ring_buf[RING_BUFFER_SAMPLES]; /* 环形缓冲 */
    uint32_t ring_read;                    /* 音频回调读取位置 */
    uint32_t ring_write;                   /* speaker_update 写入位置 */
    uint32_t ring_count;                   /* 缓冲中可用采样数 */
} audio_slot_t;

/* ================================================================
 *  内部状态
 * ================================================================ */

static audio_slot_t g_slots[NUM_SLOTS];
static SDL_AudioDeviceID g_audio_dev = 0;
static uint8_t g_volume = 100;
static SDL_AudioSpec g_audio_spec;
static int g_audio_channels = 1; /* 实际通道数 (SDL 可能改为 2) */

/* ================================================================
 *  内部辅助
 * ================================================================ */

static int alloc_slot(void) {
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (g_slots[i].mode == SLOT_FREE)
            return i;
    }
    return -1;
}

static void free_slot(int idx) {
    if (idx < 0 || idx >= NUM_SLOTS)
        return;
    audio_slot_t *s = &g_slots[idx];
    if (s->mode == SLOT_STREAM && s->file_handle) {
        fclose(s->file_handle);
        s->file_handle = NULL;
    }
    memset(s, 0, sizeof(*s));
    s->mode = SLOT_FREE;
}

/* ---- 从流槽读取数据填充输出缓冲 ---- */
static int stream_read(audio_slot_t *s, int16_t *dst, int want_samples) {
    int total = 0;
    while (total < want_samples) {
        if (s->ring_count == 0) {
            /* 缓冲区空 — 尝试从文件填充 */
            if (!s->file_handle)
                break;

            int remaining = (int)(s->file_size_samples - s->file_pos_samples);
            int to_read = (STREAM_CHUNK_SAMPLES < remaining) ? STREAM_CHUNK_SAMPLES : remaining;
            if (to_read <= 0) {
                if (s->loop) {
                    /* 循环: 回到文件开头 */
                    fseek(s->file_handle, 0, SEEK_SET);
                    s->file_pos_samples = 0;
                    to_read = (STREAM_CHUNK_SAMPLES < (int)s->file_size_samples) ? STREAM_CHUNK_SAMPLES
                                                                                 : (int)s->file_size_samples;
                } else {
                    break; /* 播放完毕 */
                }
            }

            /* 读取 PCM 数据到环形缓冲 */
            int16_t temp[STREAM_CHUNK_SAMPLES];
            size_t rd = fread(temp, sizeof(int16_t), to_read, s->file_handle);
            if (rd == 0)
                break;
            s->file_pos_samples += (uint32_t)rd;

            /* 写入环形缓冲 */
            for (size_t i = 0; i < rd; i++) {
                s->ring_buf[s->ring_write] = temp[i];
                s->ring_write = (s->ring_write + 1) % RING_BUFFER_SAMPLES;
            }
            s->ring_count += (uint32_t)rd;
        }

        /* 从环形缓冲读取到输出 */
        int avail = (int)s->ring_count;
        int copy = (avail < (want_samples - total)) ? avail : (want_samples - total);
        for (int i = 0; i < copy; i++) {
            dst[total + i] = s->ring_buf[s->ring_read];
            s->ring_read = (s->ring_read + 1) % RING_BUFFER_SAMPLES;
        }
        s->ring_count -= copy;
        total += copy;
    }
    return total;
}

/* ---- SDL 音频回调: 混音所有活跃槽 ---- */
static void audio_callback(void *userdata, Uint8 *stream, int len_bytes) {
    (void)userdata;
    int ch = g_audio_channels;
    int frames = len_bytes / (int)sizeof(int16_t) / ch;
    int16_t *out = (int16_t *)stream;

    memset(out, 0, (size_t)len_bytes);

    for (int i = 0; i < NUM_SLOTS; i++) {
        audio_slot_t *s = &g_slots[i];
        if (s->mode == SLOT_FREE)
            continue;

        if (s->mode == SLOT_MEMORY) {
            uint32_t remaining = s->pcm_total - s->pcm_pos;
            if (remaining == 0) {
                if (s->loop) {
                    s->pcm_pos = 0;
                    remaining = s->pcm_total;
                } else {
                    free_slot(i);
                    continue;
                }
            }
            int copy = (int)((remaining < (uint32_t)frames) ? remaining : (uint32_t)frames);
            for (int j = 0; j < copy; j++) {
                int16_t sample = (int16_t)s->pcm_data[s->pcm_pos + j];
                /* 单声道 → 多声道 (写入所有通道) */
                for (int c = 0; c < ch; c++) {
                    int idx = j * ch + c;
                    int32_t mixed = (int32_t)out[idx] + (int32_t)sample;
                    if (mixed > 32767)
                        mixed = 32767;
                    if (mixed < -32768)
                        mixed = -32768;
                    out[idx] = (int16_t)mixed;
                }
            }
            s->pcm_pos += (uint32_t)copy;
        } else if (s->mode == SLOT_STREAM) {
            int16_t temp[4096];
            int rd = stream_read(s, temp, frames);
            for (int j = 0; j < rd; j++) {
                int16_t sample = temp[j];
                for (int c = 0; c < ch; c++) {
                    int idx = j * ch + c;
                    int32_t mixed = (int32_t)out[idx] + (int32_t)sample;
                    if (mixed > 32767)
                        mixed = 32767;
                    if (mixed < -32768)
                        mixed = -32768;
                    out[idx] = (int16_t)mixed;
                }
            }
        }
    }

    /* 应用主音量 */
    if (g_volume < 100) {
        float scale = g_volume / 100.0f;
        int total = frames * ch;
        for (int i = 0; i < total; i++) {
            out[i] = (int16_t)(out[i] * scale);
        }
    }
}

/* ================================================================
 *  公开接口
 * ================================================================ */

void speaker_init(void) {
    memset(g_slots, 0, sizeof(g_slots));

    SDL_AudioSpec want;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;

    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &g_audio_spec, 0);
    if (g_audio_dev == 0) {
        /* 若严格匹配失败, 尝试允许 SDL 调整参数 */
        g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &g_audio_spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    }
    if (g_audio_dev == 0) {
        fprintf(stderr, "[AUDIO] SDL_OpenAudioDevice FAILED: %s\n", SDL_GetError());
        return;
    }

    g_audio_channels = g_audio_spec.channels; /* ★ 记录实际通道数 */
    printf("[AUDIO] SDL audio: %dHz %dch fmt=0x%X dev=%d\n", g_audio_spec.freq, g_audio_channels, g_audio_spec.format,
           g_audio_dev);

    SDL_PauseAudioDevice(g_audio_dev, 0); /* 开始播放 */
}

void speaker_update(void) {
    /* 清理已播放完毕的非循环内存槽 */
    for (int i = 0; i < NUM_SLOTS; i++) {
        audio_slot_t *s = &g_slots[i];
        if (s->mode == SLOT_MEMORY && !s->loop && s->pcm_pos >= s->pcm_total) {
            free_slot(i);
        }
        if (s->mode == SLOT_STREAM && !s->loop && s->ring_count == 0 && s->file_pos_samples >= s->file_size_samples) {
            free_slot(i);
        }
    }
}

int speaker_play_pcm(const uint16_t *pcm_data, uint32_t sample_count, uint8_t loop) {
    int idx = alloc_slot();
    if (idx < 0) {
        fprintf(stderr, "[AUDIO] play_pcm: no free slot!\n");
        return -1;
    }

    audio_slot_t *s = &g_slots[idx];
    s->mode = SLOT_MEMORY;
    s->pcm_data = pcm_data;
    s->pcm_total = sample_count;
    s->pcm_pos = 0;
    s->loop = loop;

    printf("[AUDIO] play_pcm: slot=%d samples=%u loop=%d\n", idx, sample_count, loop);
    return idx;
}

int speaker_play_pcm_file(const char *path, uint8_t loop) {
    uint16_t *buf = NULL;
    uint32_t samples = 0;

    /* 方案 1: FATFS 路径 ("0:/...") → f_open */
    FIL fil;
    if (f_open(&fil, path, FA_READ) == FR_OK) {
        uint32_t fsize = (uint32_t)f_size(&fil);
        samples = fsize / sizeof(int16_t);
        buf = (uint16_t *)malloc(fsize);
        if (buf) {
            UINT br;
            f_read(&fil, buf, fsize, &br);
        }
        f_close(&fil);
        if (buf) {
            int idx = speaker_play_pcm(buf, samples, loop);
            return idx;
        }
    }

    /* 方案 2: Windows 本地路径 → fopen */
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        samples = (uint32_t)(size / sizeof(int16_t));
        buf = (uint16_t *)malloc(size);
        if (buf) {
            fread(buf, 1, size, f);
        }
        fclose(f);
        if (buf) {
            int idx = speaker_play_pcm(buf, samples, loop);
            return idx;
        }
    }

    return -1;
}

int speaker_play_bgm_stream_file(const char *path, uint8_t loop) {
    /* BGM 流式播放 — 在 Windows 上直接加载整个文件到内存播放
     * (12MB BGM 在 PC 上完全可以全载入, 无需模拟嵌入式流式) */

    printf("[AUDIO] play_bgm_stream: '%s' loop=%d\n", path, loop);

    /* 方案 1: FATFS f_open */
    FIL fil;
    FRESULT fr = f_open(&fil, path, FA_READ);
    printf("[AUDIO]   FATFS f_open → %d\n", fr);
    if (fr == FR_OK) {
        uint32_t fsize = (uint32_t)f_size(&fil);
        uint32_t samples = fsize / sizeof(int16_t);
        printf("[AUDIO]   FATFS size=%u samples=%u\n", fsize, samples);
        uint16_t *buf = (uint16_t *)malloc(fsize);
        if (buf) {
            UINT br;
            f_read(&fil, buf, fsize, &br);
            f_close(&fil);
            int slot = speaker_play_pcm(buf, samples, loop);
            printf("[AUDIO]   FATFS → slot %d\n", slot);
            return slot;
        }
        f_close(&fil);
    }

    /* 方案 2: Windows 本地路径 */
    FILE *f = fopen(path, "rb");
    printf("[AUDIO]   fopen → %p\n", (void *)f);
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint32_t samples = (uint32_t)(fsize / sizeof(int16_t));
        uint16_t *buf = (uint16_t *)malloc(fsize);
        if (buf) {
            fread(buf, 1, fsize, f);
            fclose(f);
            int slot = speaker_play_pcm(buf, samples, loop);
            printf("[AUDIO]   Win32 → slot %d\n", slot);
            return slot;
        }
        fclose(f);
    }

    fprintf(stderr, "[AUDIO]   FAILED to open '%s'\n", path);
    return -1;
}

void speaker_stop_slot(uint8_t slot_index) {
    free_slot(slot_index);
}

void speaker_stop(void) {
    for (int i = 0; i < NUM_SLOTS; i++) free_slot(i);
}

void speaker_set_volume(uint8_t vol) {
    if (vol > 100)
        vol = 100;
    g_volume = vol;
}

uint8_t speaker_get_volume(void) {
    return g_volume;
}

uint8_t speaker_is_playing(void) {
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (g_slots[i].mode != SLOT_FREE)
            return 1;
    }
    return 0;
}
