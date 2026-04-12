#include "LCD_tli.h"



#define HORIZONTAL_SYNCHRONOUS_PULSE  20
#define HORIZONTAL_BACK_PORCH         140
#define ACTIVE_WIDTH                  1024
#define HORIZONTAL_FRONT_PORCH        160

#define VERTICAL_SYNCHRONOUS_PULSE    3
#define VERTICAL_BACK_PORCH           20
#define ACTIVE_HEIGHT                 600
#define VERTICAL_FRONT_PORCH          12

//__ALIGNED(16) uint8_t blended_address_buffer[58292];






/*
	R7:	G6		G7:	K2		B7:	D2
	R6:	B1		G6:	K1		B6:	A15
	R5:	A9		G5:	K0		B5:	B5
	R4:	A5		G4:	J11		B4:	A10
	R3:	B0		G3:	J10		B3:	A8
	R2:			G2:	A6		B2:	
	HSYNC:	C6
	VSYNC:	A7
	DEN:	F10
	PCLK:	G7
*/
static void tli_gpio_config(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    //rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);
    //rcu_periph_clock_enable(RCU_GPIOH);
    //rcu_periph_clock_enable(RCU_GPIOI);
    rcu_periph_clock_enable(RCU_GPIOJ);
    rcu_periph_clock_enable(RCU_GPIOK);

    gpio_af_set(GPIOA, GPIO_AF_14, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9|GPIO_PIN_15);
    gpio_af_set(GPIOA, GPIO_AF_13, GPIO_PIN_8);
    gpio_af_set(GPIOA, GPIO_AF_12, GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15);

    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_0|GPIO_PIN_1);
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_5);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5);

	gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_6);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_6);

	gpio_af_set(GPIOD, GPIO_AF_9, GPIO_PIN_2);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_2);

	gpio_af_set(GPIOF, GPIO_AF_14, GPIO_PIN_10);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_10);
	
	gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_6|GPIO_PIN_7);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6|GPIO_PIN_7);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_6|GPIO_PIN_7);
	
	gpio_af_set(GPIOJ, GPIO_AF_14, GPIO_PIN_10|GPIO_PIN_11);
    gpio_mode_set(GPIOJ, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10|GPIO_PIN_11);
    gpio_output_options_set(GPIOJ, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_10|GPIO_PIN_11);
	
	gpio_af_set(GPIOK, GPIO_AF_14, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
    gpio_mode_set(GPIOK, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
    gpio_output_options_set(GPIOK, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
	
    /* LCD PWM BackLight(PG3) */
    gpio_mode_set(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_3);
    gpio_bit_set(GPIOG, GPIO_PIN_3);
}

static void tli_config(void)
{
    tli_parameter_struct               tli_init_struct;
    tli_layer_parameter_struct         tli_layer_init_struct;

    rcu_periph_clock_enable(RCU_TLI);
    tli_gpio_config();

    rcu_pll_input_output_clock_range_config(IDX_PLL2, RCU_PLL2RNG_1M_2M, RCU_PLL2VCO_150M_420M);
    /* configure the PLL2 clock: CK_PLL2R = HXTAL_VALUE / 8 * 150 / 3 */
    if(ERROR == rcu_pll2_config(8, 150, 3, 3, 2)) {
        while(1) {
        }
    }
    rcu_pll_clock_output_enable(RCU_PLL2R);
    rcu_tli_clock_div_config(RCU_PLL2R_DIV2);

    rcu_osci_on(RCU_PLL2_CK);	//EN PLL2

    if(ERROR == rcu_osci_stab_wait(RCU_PLL2_CK)) {
        while(1) {
        }
    }

    /* configure TLI parameter struct */
    tli_init_struct.signalpolarity_hs = TLI_HSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_vs = TLI_VSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_de = TLI_DE_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_pixelck = TLI_PIXEL_CLOCK_TLI;

    /* LCD display timing configuration */
    tli_init_struct.synpsz_hpsz = HORIZONTAL_SYNCHRONOUS_PULSE - 1;
    tli_init_struct.synpsz_vpsz = VERTICAL_SYNCHRONOUS_PULSE - 1;
    tli_init_struct.backpsz_hbpsz = HORIZONTAL_SYNCHRONOUS_PULSE + HORIZONTAL_BACK_PORCH - 1;
    tli_init_struct.backpsz_vbpsz = VERTICAL_SYNCHRONOUS_PULSE + VERTICAL_BACK_PORCH - 1;
    tli_init_struct.activesz_hasz = HORIZONTAL_SYNCHRONOUS_PULSE + HORIZONTAL_BACK_PORCH + ACTIVE_WIDTH
                                    - 1;
    tli_init_struct.activesz_vasz = VERTICAL_SYNCHRONOUS_PULSE + VERTICAL_BACK_PORCH + ACTIVE_HEIGHT -
                                    1;
    tli_init_struct.totalsz_htsz = HORIZONTAL_SYNCHRONOUS_PULSE + HORIZONTAL_BACK_PORCH + ACTIVE_WIDTH +
                                   HORIZONTAL_FRONT_PORCH - 1;
    tli_init_struct.totalsz_vtsz = VERTICAL_SYNCHRONOUS_PULSE + VERTICAL_BACK_PORCH + ACTIVE_HEIGHT +
                                   VERTICAL_FRONT_PORCH - 1;
    /* configure LCD background R,G,B values */
    tli_init_struct.backcolor_red = 0xFF;
    tli_init_struct.backcolor_green = 0xFF;
    tli_init_struct.backcolor_blue = 0xFF;
    tli_init(&tli_init_struct);

    /* TLI layer configuration */
    tli_layer_init_struct.layer_window_leftpos = 0 + HORIZONTAL_SYNCHRONOUS_PULSE +
                                                 HORIZONTAL_BACK_PORCH + 0;
    tli_layer_init_struct.layer_window_rightpos = (0 + 1024 + HORIZONTAL_SYNCHRONOUS_PULSE +
                                                   HORIZONTAL_BACK_PORCH - 1);
    tli_layer_init_struct.layer_window_toppos = 0 + VERTICAL_SYNCHRONOUS_PULSE + VERTICAL_BACK_PORCH;
    tli_layer_init_struct.layer_window_bottompos = (0 + 600 + VERTICAL_SYNCHRONOUS_PULSE +
                                                    VERTICAL_BACK_PORCH - 1);
    tli_layer_init_struct.layer_ppf = LAYER_PPF_RGB565;
    tli_layer_init_struct.layer_sa = 0xFF;
    tli_layer_init_struct.layer_default_blue = 0xFF;
    tli_layer_init_struct.layer_default_green = 0xFF;
    tli_layer_init_struct.layer_default_red = 0xFF;
    tli_layer_init_struct.layer_default_alpha = 0x0;
    tli_layer_init_struct.layer_acf1 = LAYER_ACF1_PASA;
    tli_layer_init_struct.layer_acf2 = LAYER_ACF2_PASA;
    tli_layer_init_struct.layer_frame_bufaddr = GMEM_BASE;
    tli_layer_init_struct.layer_frame_line_length = ((1024 * 2) + 3);
    tli_layer_init_struct.layer_frame_buf_stride_offset = (1024 * 2);
    tli_layer_init_struct.layer_frame_total_line_number = 600;
    tli_layer_init(LAYER0, &tli_layer_init_struct);
}

void lcd_config(void)
{
    /* configure TLI peripheral */
    tli_config();
	
	/* enable TLI layers */
    tli_layer_enable(LAYER0);
    //tli_layer_enable(LAYER1);
    tli_reload_config(TLI_FRAME_BLANK_RELOAD_EN);
    /* enable TLI */
    tli_enable();

    /* configure TLI and display blend image */
    //tli_blend_config();
    //tli_reload_config(TLI_REQUEST_RELOAD_EN);


}
