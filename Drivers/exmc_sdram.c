#include "exmc_sdram.h"

/* define mode register content */
/* burst length */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000U)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001U)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002U)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0003U)

/* burst type */
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000U)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008U)

/* CAS latency */
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020U)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030U)

/* write mode */
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000U)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200U)

#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000U)

#define SDRAM_TIMEOUT                            ((uint32_t)0x0000FFFFU)


static void _delay(uint32_t count)
{
    __IO uint32_t index = 0;

    for(index = (100 * count); index != 0; index--) {
    }
}

void exmc_synchronous_dynamic_ram_init(uint32_t sdram_device)
{
	uint32_t command_content = 0, bank_select;
    uint32_t timeout = SDRAM_TIMEOUT;
	
    exmc_sdram_parameter_struct sdram_init_struct;
    exmc_sdram_timing_parameter_struct sdram_timing_init_struct;
    exmc_sdram_command_parameter_struct sdram_command_init_struct;
    exmc_sdram_struct_para_init(&sdram_init_struct);

    /* enable EXMC clock */
	RCU_CFG4 &= ~RCU_CFG4_EXMCSEL;
	RCU_CFG4 |= 2<<8;	//use PLL1R=400M
    rcu_periph_clock_enable(RCU_EXMC);
	/*	A0:F0	D0:D14	D8:E11	BA0:G4
		A1:F1	D1:D15	D9:E12	BA1:G5
		A2:F2	D2:D0	D10:E13	CLK:G8
		A3:F3	D3:D1	D11:E14	CKE:C5
		A4:F4	D4:E7	D12:E15	NCAS:G15
		A5:F5	D5:E8	D13:D8	NE0:C4
		A6:F12	D6:E9	D14:D9	NRAS:F11
		A7:F13	D7:E10	D15:D10	NWE:C0
		A8:F14					NBL0:E0
		A9:F15					NBL1:E1
		A10:G0	
		A11:G1
		A12:G2							*/
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5);

    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_af_set(GPIOF, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_af_set(GPIOG, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);


    /* specify which SDRAM to read and write */
    if(EXMC_SDRAM_DEVICE0 == sdram_device) {
        bank_select = EXMC_SDRAM_DEVICE0_SELECT;
    } else {
        bank_select = EXMC_SDRAM_DEVICE1_SELECT;
    }

    /* EXMC SDRAM device initialization sequence --------------------------------*/
    /* step 1 : configure SDRAM timing registers --------------------------------*/
    /* LMRD: 2 clock cycles */
    sdram_timing_init_struct.load_mode_register_delay = 2;
    /* XSRD: min = 70ns */
    sdram_timing_init_struct.exit_selfrefresh_delay = 14;
    /* RASD: min=40ns , max=120k (ns) */
    sdram_timing_init_struct.row_address_select_delay = 8;
    /* ARFD: min=55ns */
    sdram_timing_init_struct.auto_refresh_delay = 11;
    /* WRD:  min=1 Clock cycles +6ns */
    sdram_timing_init_struct.write_recovery_delay = 2;
    /* RPD:  min=15ns */
    sdram_timing_init_struct.row_precharge_delay = 3;
    /* RCD:  min=15ns */
    sdram_timing_init_struct.row_to_column_delay = 3;

    /* step 2 : configure SDRAM control registers ---------------------------------*/
    sdram_init_struct.sdram_device = sdram_device;
    sdram_init_struct.column_address_width = EXMC_SDRAM_COW_ADDRESS_9;
    sdram_init_struct.row_address_width = EXMC_SDRAM_ROW_ADDRESS_13;
    sdram_init_struct.data_width = EXMC_SDRAM_DATABUS_WIDTH_16B;
    sdram_init_struct.internal_bank_number = EXMC_SDRAM_4_INTER_BANK;
    sdram_init_struct.cas_latency = EXMC_CAS_LATENCY_3_SDCLK;
    sdram_init_struct.write_protection = DISABLE;
    sdram_init_struct.sdclock_config = EXMC_SDCLK_PERIODS_2_CK_EXMC;
    sdram_init_struct.burst_read_switch = ENABLE;
    sdram_init_struct.pipeline_read_delay = EXMC_PIPELINE_DELAY_2_CK_EXMC;
    sdram_init_struct.timing = &sdram_timing_init_struct;
    /* EXMC SDRAM bank initialization */
    exmc_sdram_init(&sdram_init_struct);

    /* step 3 : configure CKE high command---------------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_CLOCK_ENABLE;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 4 : insert 10ms delay----------------------------------------------*/
    _delay(10000);

    /* step 5 : configure precharge all command----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_PRECHARGE_ALL;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 6 : configure Auto-Refresh command-----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_AUTO_REFRESH;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_9_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 7 : configure load mode register command-----------------------------*/
    /* program mode register */
    command_content = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1        |
                      SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                      SDRAM_MODEREG_CAS_LATENCY_3           |
                      SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                      SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED;

    sdram_command_init_struct.command = EXMC_SDRAM_LOAD_MODE_REGISTER;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = command_content;

    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 8 : set the auto-refresh rate counter--------------------------------*/
    /* 64ms, 8192-cycle refresh, 64ms/8192=7.81us */
    /* SDCLK_Freq = SYS_Freq/2 */
    /* (7.81 us * SDCLK_Freq) - 20 */
    //exmc_sdram_refresh_count_set(1542);
exmc_sdram_refresh_count_set(155);
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
	
}

/*!
    \brief      fill the buffer with specified value
    \param[in]  pbuffer: pointer to the buffer to fill data
    \param[in]  buffer_lengh: size of the buffer to fill
    \param[in]  start_value: first data to fill in the buffer
    \param[out] none
    \retval     none
*/
void fill_buffer(uint8_t *pbuffer, uint32_t buffer_lengh, uint16_t start_value)
{
    uint32_t index = 0;

    /* fill the buffer with specified values */
    for(index = 0; index < buffer_lengh; index++) {
//        pbuffer[index] = (uint8_t)(index + start_value);
		pbuffer[index] = (uint8_t)start_value;
    }
}

/*!
    \brief      write a byte buffer(data is 8 bits) to the EXMC SDRAM memory
    \param[in]  sdram_device: specify which a SDRAM memory block is written
    \param[in]  pbuffer: pointer to buffer
    \param[in]  write_addr: SDRAM memory internal address from which the data will be written
    \param[in]  byte_count_to_write: number of bytes to write
    \param[out] none
    \retval     none
*/
void sdram_writebuffer_8(uint32_t sdram_device, uint8_t *pbuffer, uint32_t write_addr, uint32_t byte_count_to_write)
{
    uint32_t temp_addr;

    /* select the base address according to EXMC_Bank */
    if(sdram_device == EXMC_SDRAM_DEVICE0) {
        temp_addr = SDRAM_DEVICE0_ADDR;
    } else {
        temp_addr = SDRAM_DEVICE1_ADDR;
    }

    /* while there is data to write */
    for(; byte_count_to_write != 0; byte_count_to_write--) {
        /* transfer data to the memory */
        *(uint8_t *)(temp_addr + write_addr) = *pbuffer++;

        /* increment the address */
        write_addr += 1;
    }
}

/*!
    \brief      read a block of 8-bit data from the EXMC SDRAM memory
    \param[in]  sdram_device: specify which a SDRAM memory block is written
    \param[in]  pbuffer: pointer to buffer
    \param[in]  read_addr: SDRAM memory internal address to read from
    \param[in]  byte_count_to_read: number of bytes to read
    \param[out] none
    \retval     none
*/
void sdram_readbuffer_8(uint32_t sdram_device, uint8_t *pbuffer, uint32_t read_addr, uint32_t byte_count_to_read)
{
    uint32_t temp_addr;

    /* select the base address according to EXMC_Bank */
    if(sdram_device == EXMC_SDRAM_DEVICE0) {
        temp_addr = SDRAM_DEVICE0_ADDR;
    } else {
        temp_addr = SDRAM_DEVICE1_ADDR;
    }

    /* while there is data to read */
    for(; byte_count_to_read != 0; byte_count_to_read--) {
        /* read a byte from the memory */
        *pbuffer++ = *(uint8_t *)(temp_addr + read_addr);

        /* increment the address */
        read_addr += 1;
    }
}
