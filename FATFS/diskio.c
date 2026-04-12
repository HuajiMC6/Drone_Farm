#include "diskio.h"
#include "sdcard.h"

sd_card_info_struct sd_cardinfo;                            /* information of SD card */

#define SD_CARD     0       /* SD卡,卷标为0 */


sd_error_enum sd_config(void)
{
    sd_error_enum status = SD_OK;
    uint32_t cardstate = 0;
	
    /* initialize the card */
    status = sd_init();
    if(SD_OK == status) {
        status = sd_card_information_get(&sd_cardinfo);
    }
    if(SD_OK == status) {
        status = sd_card_select_deselect(sd_cardinfo.card_rca);
    }
    status = sd_cardstatus_get(&cardstate);
    if(cardstate & 0x02000000) {
//        printf("\r\n The card is locked!");
#if 0
        /* unlock the card if necessary */
        status = sd_lock_unlock(SD_UNLOCK);
        if(SD_OK != status) {
            printf("\r\n Unlock failed!");
            while(1) {
            }
        } else {
            printf("\r\n The card is unlocked! Please reset MCU!");
        }
#endif
        while(1) {
        }
    }
    if((SD_OK == status) && (!(cardstate & 0x02000000))) {
        /* set bus mode */
        status = sd_bus_mode_config(SDIO_BUSMODE_4BIT, SD_SPEED_DEFAULT);
    }
    if(SD_OK == status) {
        /* set data transfer mode */
//        status = sd_transfer_mode_config( SD_DMA_MODE );
        status = sd_transfer_mode_config(SD_POLLING_MODE);
    }
    return status;
}

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    return RES_OK;
}

/**
 * @brief       初始化磁盘
 * @param       pdrv : 磁盘编号0~9
 * @retval      执行结果(参见FATFS, DSTATUS的定义)
 */
DSTATUS disk_initialize (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    uint8_t res = 0;

	res = sd_config();    /* SD卡初始化 */

    if (res != SD_OK)
    {
        return STA_NOINIT;
    }
    else
    {
        return 0; /* 初始化成功*/
    }
}

/**
 * @brief       读扇区
 * @param       pdrv   : 磁盘编号0~9
 * @param       buff   : 数据接收缓冲首地址
 * @param       sector : 扇区地址
 * @param       count  : 需要读取的扇区数
 * @retval      执行结果(参见FATFS, DRESULT的定义)
 */
DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address in LBA */
    UINT count      /* Number of sectors to read */
)
{
    uint8_t res = 0;

    if (!count) return RES_PARERR;   /* count不能等于0，否则返回参数错误 */

	if(count > 1)
		res = sd_multiblocks_read((uint32_t*)buff, sector, 512, count);
	else
		res = sd_block_read((uint32_t*)buff, sector, 512);
		
	//while (res)     /* 读出错 */
	//{
		//printf("sd rd error:%d\r\n", res);
	//    sd_config();  /* 重新初始化SD卡 */
	//    res = sd_multiblocks_read((uint32_t*)buff, sector, 512, count);
	//}

    /* 处理返回值，将返回值转成ff.c的返回值 */
    if (res == SD_OK)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR; 
    }
}

/**
 * @brief       写扇区
 * @param       pdrv   : 磁盘编号0~9
 * @param       buff   : 发送数据缓存区首地址
 * @param       sector : 扇区地址
 * @param       count  : 需要写入的扇区数
 * @retval      执行结果(参见FATFS, DRESULT的定义)
 */
DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number to identify the drive */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address in LBA */
    UINT count          /* Number of sectors to write */
)
{
    uint8_t res = 0;

    if (!count) return RES_PARERR;  /* count不能等于0，否则返回参数错误 */

	if(count > 1)
		res = sd_multiblocks_write((uint32_t *)buff, sector, 512, count);
	else
		res = sd_block_write((uint32_t*)buff, sector, 512);

	while (res!=SD_OK)     /* 写出错 */
	{
		//printf("sd wr error:%d\r\n", res);
		sd_config();  /* 重新初始化SD卡 */
		res = sd_multiblocks_write((uint32_t *)buff, sector, 512, count);
	}

    /* 处理返回值，将返回值转成ff.c的返回值 */
    if (res == SD_OK)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR; 
    }
}

/**
 * @brief       获取其他控制参数
 * @param       pdrv   : 磁盘编号0~9
 * @param       ctrl   : 控制代码
 * @param       buff   : 发送/接收缓冲区指针
 * @retval      执行结果(参见FATFS, DRESULT的定义)
 */
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res;

	switch (cmd)
	{
		case CTRL_SYNC:
			res = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(DWORD *)buff = sd_cardinfo.card_blocksize;
			res = RES_OK;
			break;

		case GET_BLOCK_SIZE:
			*(WORD *)buff = 1;
			res = RES_OK;
			break;

		case GET_SECTOR_COUNT:
			*(DWORD *)buff = sd_cardinfo.card_capacity / (sd_cardinfo.card_blocksize);
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
			break;
	}

    return res;
}
