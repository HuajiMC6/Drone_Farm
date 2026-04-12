#include "joystick.h"
#include "drivers.h"
#include <stddef.h>

#define JS_ADC_PERIPH ADC2
#define JS_ADC_CH_Y ADC_CHANNEL_0
#define JS_ADC_CH_X ADC_CHANNEL_1

#define JS_GPIO_ADC_PORT GPIOC
#define JS_GPIO_ADC_PINS (GPIO_PIN_2 | GPIO_PIN_3)

#define JS_GPIO_SW_PORT GPIOB
#define JS_GPIO_SW_PIN GPIO_PIN_2

#define JS_ADC_MAX_VALUE 4095U
#define JS_DIR_FULL_SCALE 100
#define JS_DEAD_ZONE_RAW 300
#define JS_DEFAULT_CENTER (JS_ADC_MAX_VALUE / 2U)
#define JS_SAMPLE_AVG_COUNT 4U
#define JS_ADC_WAIT_TIMEOUT 200000U
#define JS_ADC_CAL_TIMEOUT 200000U

static uint16_t s_center_x = JS_DEFAULT_CENTER;
static uint16_t s_center_y = JS_DEFAULT_CENTER;
static uint8_t s_adc_ready = 0U;
static uint8_t s_last_adc_ok = 0U;

static void joystick_adc2_select_clock(uint32_t source)
{
    RCU_CFG3 &= ~((uint32_t)RCU_CFG3_ADC2SEL);
    RCU_CFG3 |= ((uint32_t)source << 28U);
}

static uint16_t joystick_channel_default(uint8_t channel)
{
    return (channel == JS_ADC_CH_X) ? s_center_x : s_center_y;
}

static uint8_t joystick_adc_wait_eoc(void)
{
    uint32_t timeout = JS_ADC_WAIT_TIMEOUT;

    while (RESET == adc_flag_get(JS_ADC_PERIPH, ADC_FLAG_EOC))
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t joystick_adc_calibrate_safe(void)
{
    uint32_t timeout;

    ADC_CTL1(JS_ADC_PERIPH) |= (uint32_t)ADC_CTL1_RSTCLB;
    timeout = JS_ADC_CAL_TIMEOUT;
    while (RESET != (ADC_CTL1(JS_ADC_PERIPH) & ADC_CTL1_RSTCLB))
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    ADC_CTL1(JS_ADC_PERIPH) |= (uint32_t)ADC_CTL1_CLB;
    timeout = JS_ADC_CAL_TIMEOUT;
    while (RESET != (ADC_CTL1(JS_ADC_PERIPH) & ADC_CTL1_CLB))
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint16_t joystick_adc_read_channel(uint8_t channel)
{
    uint32_t sum = 0U;
    uint32_t i;

    if (s_adc_ready == 0U)
    {
        s_last_adc_ok = 0U;
        return joystick_channel_default(channel);
    }

    adc_regular_channel_config(JS_ADC_PERIPH, 0U, channel, 200U);

    for (i = 0U; i < JS_SAMPLE_AVG_COUNT; i++)
    {
        adc_flag_clear(JS_ADC_PERIPH, ADC_FLAG_EOC);
        adc_software_trigger_enable(JS_ADC_PERIPH, ADC_REGULAR_CHANNEL);
        if (joystick_adc_wait_eoc() == 0U)
        {
            s_last_adc_ok = 0U;
            return joystick_channel_default(channel);
        }
        sum += adc_regular_data_read(JS_ADC_PERIPH);
        adc_flag_clear(JS_ADC_PERIPH, ADC_FLAG_EOC);
    }

    s_last_adc_ok = 1U;
    return (uint16_t)(sum / JS_SAMPLE_AVG_COUNT);
}

static uint8_t joystick_adc_self_test(void)
{
    uint16_t sample;

    sample = joystick_adc_read_channel(JS_ADC_CH_X);
    if (s_last_adc_ok != 0U && sample <= JS_ADC_MAX_VALUE)
    {
        return 1U;
    }

    sample = joystick_adc_read_channel(JS_ADC_CH_Y);
    if (s_last_adc_ok != 0U && sample <= JS_ADC_MAX_VALUE)
    {
        return 1U;
    }

    return 0U;
}

static int8_t joystick_raw_to_dir(uint16_t value, uint16_t center)
{
    int32_t diff;
    int32_t scaled;
    int32_t max_span;

    if (value >= center)
    {
        diff = (int32_t)value - (int32_t)center;
        max_span = (int32_t)JS_ADC_MAX_VALUE - (int32_t)center;
    }
    else
    {
        diff = -((int32_t)center - (int32_t)value);
        max_span = (int32_t)center;
    }

    if ((diff < JS_DEAD_ZONE_RAW) && (diff > -JS_DEAD_ZONE_RAW))
    {
        return 0;
    }

    if (max_span <= 0)
    {
        return 0;
    }

    scaled = (diff * JS_DIR_FULL_SCALE) / max_span;
    if (scaled > JS_DIR_FULL_SCALE)
    {
        scaled = JS_DIR_FULL_SCALE;
    }
    else if (scaled < -JS_DIR_FULL_SCALE)
    {
        scaled = -JS_DIR_FULL_SCALE;
    }

    return (int8_t)scaled;
}

void joystick_init(void)
{
    uint32_t source;

    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC2);

    gpio_mode_set(JS_GPIO_ADC_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, JS_GPIO_ADC_PINS);

    gpio_mode_set(JS_GPIO_SW_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, JS_GPIO_SW_PIN);

    s_adc_ready = 0U;
    for (source = 0U; source < 4U; source++)
    {
        adc_deinit(JS_ADC_PERIPH);
        joystick_adc2_select_clock(source);
        adc_clock_config(JS_ADC_PERIPH, ADC_CLK_ASYNC_DIV4);
        adc_resolution_config(JS_ADC_PERIPH, ADC_RESOLUTION_12B);
        adc_data_alignment_config(JS_ADC_PERIPH, ADC_DATAALIGN_RIGHT);
        adc_special_function_config(JS_ADC_PERIPH, ADC_CONTINUOUS_MODE, DISABLE);
        adc_special_function_config(JS_ADC_PERIPH, ADC_SCAN_MODE, DISABLE);
        adc_external_trigger_config(JS_ADC_PERIPH, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
        adc_end_of_conversion_config(JS_ADC_PERIPH, ADC_EOC_SET_CONVERSION);
        adc_channel_length_config(JS_ADC_PERIPH, ADC_REGULAR_CHANNEL, 1U);

        adc_enable(JS_ADC_PERIPH);
        delay_us(20);

        if (joystick_adc_calibrate_safe() != 0U)
        {
            s_adc_ready = 1U;
            if (joystick_adc_self_test() != 0U)
            {
                break;
            }
        }
    }

    joystick_calibrate_center();
}

int8_t joystick_get_dir_x(void)
{
    uint16_t raw = joystick_adc_read_channel(JS_ADC_CH_X);
    return joystick_raw_to_dir(raw, s_center_x);
}

int8_t joystick_get_dir_y(void)
{
    uint16_t raw = joystick_adc_read_channel(JS_ADC_CH_Y);
    return joystick_raw_to_dir(raw, s_center_y);
}

uint16_t joystick_get_raw_x(void)
{
    return joystick_adc_read_channel(JS_ADC_CH_X);
}

uint16_t joystick_get_raw_y(void)
{
    return joystick_adc_read_channel(JS_ADC_CH_Y);
}

uint8_t joystick_get_switch(void)
{
    return (gpio_input_bit_get(JS_GPIO_SW_PORT, JS_GPIO_SW_PIN) == RESET) ? 1U : 0U;
}

uint8_t joystick_adc_ok(void)
{
    return s_last_adc_ok;
}

void joystick_calibrate_center(void)
{
    s_center_x = joystick_adc_read_channel(JS_ADC_CH_X);
    s_center_y = joystick_adc_read_channel(JS_ADC_CH_Y);
}
