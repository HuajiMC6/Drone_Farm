#include "gd32h7xx_it.h"
#include "drivers.h"

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    /* if NMI exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
    /* if SVC exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
    /* if PendSV exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles FPU exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void FPU_IRQHandler(void)
{
    while(1) { 
    }
}

//uint8_t cnt_touch;
/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
	lv_tick_inc(1);
	//if(cnt_touch >= 9)cnt_touch = 0;
	//else cnt_touch++;
	//if(cnt_touch==0)touch_status = atk_rgblcd_touch_scan(tp, 5);
}

void DMA0_Channel0_IRQHandler(void)
{
    if(RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
        
		gdma_lines -= 1;
		if(gdma_lines)
		{
			gdma.src += gdma.width;
			gdma.des += 1024;

			DMA_CHPADDR(DMA0, DMA_CH0) = (uint32_t)gdma.src;
			DMA_CHM0ADDR(DMA0, DMA_CH0) = (uint32_t)gdma.des;
			
			dma_channel_enable(DMA0, DMA_CH0);
		}
    }
}

/*!
    \brief      this function handles SPI1 exception
    \param[in]  none
    \param[out] none
    \retval     none

uint32_t i = 0;
void SPI1_IRQHandler(void)
{
	uint16_t buf;
    if(SET == spi_i2s_interrupt_flag_get(SPI1, SPI_I2S_INT_FLAG_TP)) {
        if(i >= 0x2f00000) {
            i = 0;
        }
		buf = *((uint8_t*)(0xC0800000+(i/4)*2)) | ((*((uint8_t*)(0xC0800000+(i/4)*2+1)))<<8);
        spi_i2s_data_transmit(SPI1, buf);
        i+=2;
    }
}*/
