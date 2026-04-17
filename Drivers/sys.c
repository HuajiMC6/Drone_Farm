#include "drivers.h"

// volatile bool touch_status;
lcd_touch_point_t tp[5];

static void debug_com_init(void) {
    rcu_periph_clock_enable(RCU_UART7);
    rcu_periph_clock_enable(RCU_GPIOJ);

    /* connect port to USARTx_Tx */
    gpio_af_set(GPIOJ, GPIO_AF_8, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_mode_set(GPIOJ, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOJ, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_8 | GPIO_PIN_9);

    /* USART configure */
    usart_deinit(UART7);
    usart_baudrate_set(UART7, 115200U);
    // usart_receive_config(UART7, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART7, USART_TRANSMIT_ENABLE);
    usart_enable(UART7);
}

/* retarget the C library printf function to the UART */
int fputc(int ch, FILE *f) {
    usart_data_transmit(UART7, (uint8_t)ch);
    while (RESET == usart_flag_get(UART7, USART_FLAG_TBE))
        ;
    return ch;
}

static void mpu_config(void) {
    mpu_region_init_struct mpu_init_struct;
    mpu_region_struct_para_init(&mpu_init_struct);

    /* disable the MPU */
    ARM_MPU_Disable();
    ARM_MPU_SetRegion(0, 0);

    /* configure the MPU attributes for Reserved, no access */
    mpu_init_struct.region_base_address = 0x0;
    mpu_init_struct.region_size = MPU_REGION_SIZE_4GB;
    mpu_init_struct.access_permission = MPU_AP_NO_ACCESS;
    mpu_init_struct.access_bufferable = MPU_ACCESS_NON_BUFFERABLE;
    mpu_init_struct.access_cacheable = MPU_ACCESS_NON_CACHEABLE;
    mpu_init_struct.access_shareable = MPU_ACCESS_SHAREABLE;
    mpu_init_struct.region_number = MPU_REGION_NUMBER0;
    mpu_init_struct.subregion_disable = 0x87;
    mpu_init_struct.instruction_exec = MPU_INSTRUCTION_EXEC_NOT_PERMIT;
    mpu_init_struct.tex_type = MPU_TEX_TYPE0;
    mpu_region_config(&mpu_init_struct);
    mpu_region_enable();

    /* configure the MPU attributes for SDRAM */
    mpu_init_struct.region_base_address = 0xC0000000;
    mpu_init_struct.region_size = MPU_REGION_SIZE_32MB;
    mpu_init_struct.access_permission = MPU_AP_FULL_ACCESS;
    mpu_init_struct.access_bufferable = MPU_ACCESS_NON_BUFFERABLE;
    mpu_init_struct.access_cacheable = MPU_ACCESS_NON_CACHEABLE;
    mpu_init_struct.access_shareable = MPU_ACCESS_NON_SHAREABLE;
    mpu_init_struct.region_number = MPU_REGION_NUMBER1;
    mpu_init_struct.subregion_disable = 0x0;
    mpu_init_struct.instruction_exec = MPU_INSTRUCTION_EXEC_NOT_PERMIT;
    mpu_init_struct.tex_type = MPU_TEX_TYPE0;
    mpu_region_config(&mpu_init_struct);
    mpu_region_enable();

    /* enable the MPU */
    ARM_MPU_Enable(MPU_MODE_PRIV_DEFAULT);
}

void systick_config(void) {
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        /* capture error */
        while (1) {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

void delay_us(uint32_t nus) {
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;

    told = SysTick->VAL;
    ticks = nus * 600;

    while (1) {
        tnow = SysTick->VAL;
        if (tnow != told) {
            if (tnow < told) {
                tcnt += told - tnow;
            } else {
                tcnt += told + reload - tnow;
            }
            told = tnow;
            if (tcnt >= ticks) {
                break;
            }
        }
    }
}

void graphic_dma_init() {
    dma_single_data_parameter_struct dma_init_struct;
    /* enable DMA clock */
    rcu_periph_clock_enable(RCU_DMA0);
    /* enable DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMAMUX);
    /* initialize the com */
    nvic_irq_enable(DMA0_Channel0_IRQn, 2, 1);

    /* initialize DMA channel 0 */
    dma_deinit(DMA0, DMA_CH0);
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.request = DMA_REQUEST_M2M;
    dma_init_struct.direction = DMA_MEMORY_TO_MEMORY;
    dma_init_struct.memory0_addr = 0;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_init_struct.number = 0;
    dma_init_struct.periph_addr = 0;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH0, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH0);

    /* enable DMA channel 0 transfer complete interrupt */
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXCTL_FTFIE);
}

graphic_dma_struct gdma;
__IO uint16_t gdma_lines;

void graphic_dma_copy(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *src) {
    gdma.width = x2 - x1 + 1;
    gdma.height = y2 - y1 + 1;
    gdma.src = src;
    gdma.des = (uint16_t *)GMEM_BASE + x1 + y1 * 1024;
    gdma_lines = gdma.height;

    DMA_CHPADDR(DMA0, DMA_CH0) = (uint32_t)gdma.src;
    DMA_CHM0ADDR(DMA0, DMA_CH0) = (uint32_t)gdma.des;
    DMA_CHCNT(DMA0, DMA_CH0) = gdma.width;

    /* clean d-cache by address before DMA transmission */
    SCB_CleanDCache_by_Addr((uint32_t *)src, 2 * gdma.width * gdma.height);

    dma_channel_enable(DMA0, DMA_CH0);
}

void config_PLL1(void) {
    rcu_pll_input_output_clock_range_config(IDX_PLL1, RCU_PLL1RNG_1M_2M, RCU_PLL1VCO_150M_420M);
    /* configure the PLL1 clock: CK_PLL1R = HXTAL_VALUE / 8 * 150 / 3 */
    if (ERROR == rcu_pll1_config(8, 400, 3, 3, 1)) {
        while (1) {
        }
    }
    rcu_pll_clock_output_enable(RCU_PLL1R);
    rcu_osci_on(RCU_PLL1_CK); // EN PLL1

    if (ERROR == rcu_osci_stab_wait(RCU_PLL1_CK)) {
        while (1) {
        }
    }
}

FATFS fs;

void sys_init() {
    FRESULT res;

    SCB_EnableICache();
    SCB_EnableDCache();

    mpu_config();

    /* enable interrupt */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    // nvic_irq_enable(SPI1_IRQn, 0, 1);
    nvic_irq_enable(SysTick_IRQn, 0, 1);

    systick_config();

    config_PLL1();
    exmc_synchronous_dynamic_ram_init(EXMC_SDRAM_DEVICE0);

    debug_com_init();

    lcd_config();
    atk_rgblcd_touch_init();

    graphic_dma_init();

    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    if (!gpio_input_bit_get(GPIOE, GPIO_PIN_3)) {
        res = f_mount(&fs, "0:", 1);
        if (res != FR_OK) {
            while (1)
                ;
        }
    }
}