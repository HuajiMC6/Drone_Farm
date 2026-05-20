#include "audio.h"
#include "ff.h"
#include "sdram_malloc.h"
#include "speaker.h"

/**
 * 从 SD 卡加载 .pcm 文件到 SDRAM 并播放
 *
 * path:         文件路径, 例如 "0:/bgm.pcm"
 * loop:         0=单次, 1=循环
 * 返回:         0=成功, -1=失败
 *
 * 注意: 分配的内存在 speaker_stop() 时不会自动释放,
 *       调用者需要在合适的时机 sdram_free(pcm_buf)
 */
int speaker_play_pcm_file(const TCHAR *path, uint8_t loop) {
    FIL fil;
    FRESULT res;
    UINT br;
    FSIZE_t file_size;
    uint32_t sample_count;
    uint16_t *pcm_buf;

    /* 1. 打开文件 */
    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) {
        return -1;
    }

    /* 2. 获取文件大小, 计算采样数 */
    file_size = f_size(&fil);
    if (file_size < 2 || (file_size % 2) != 0) {
        f_close(&fil);
        return -1; /* 无效的 PCM 文件 */
    }
    sample_count = (uint32_t)(file_size / 2U);

    /* 3. 在 SDRAM 中分配缓冲区 */
    pcm_buf = (uint16_t *)sdram_malloc((size_t)file_size);
    if (pcm_buf == NULL) {
        f_close(&fil);
        return -1; /* 内存不足 */
    }

    /* 4. 读取整个文件 */
    res = f_read(&fil, pcm_buf, (UINT)file_size, &br);
    f_close(&fil);

    if (res != FR_OK || br != (UINT)file_size) {
        sdram_free(pcm_buf);
        return -1;
    }

    // 紧接着 speaker_play_pcm(pcm_buf, sample_count, loop); 之前
    printf("[AUDIO] file=%s samples=%lu first_10:[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", path, sample_count,
           (int16_t)pcm_buf[0], (int16_t)pcm_buf[1], (int16_t)pcm_buf[2], (int16_t)pcm_buf[3], (int16_t)pcm_buf[4],
           (int16_t)pcm_buf[5], (int16_t)pcm_buf[6], (int16_t)pcm_buf[7], (int16_t)pcm_buf[8], (int16_t)pcm_buf[9]);

    /* 5. 播放 */
    speaker_play_pcm(pcm_buf, sample_count, loop);

    return 0;
}

void icon_btns_click_audio_play(lv_event_t *e) {
    // speaker_set_volume(20);
    speaker_play_pcm_file("0:/audio/icon_btns_click.pcm", 0);
}
