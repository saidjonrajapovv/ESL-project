#include <stdbool.h>
#include <stdint.h>
#include "nrfx_pwm.h"
#include "nrfx_gpiote.h"
#include "nrf_drv_clock.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

/* LED definitions */
#define LED1 13
#define LED2 14
#define LED3 15
#define LED4 16
#define LED5 17

/* Button definition */
#define BUTTON1 11

/* Device ID: ABCD -> A=1, B=2, C=3, D=4 */
const uint8_t led_sequence[] = {LED1, LED2, LED3, LED4};
const uint8_t sequence_length = sizeof(led_sequence) / sizeof(led_sequence[0]);

/* PWM Configuration */
#define PWM_FREQ_HZ 1000
#define PWM_TOP (16000000 / PWM_FREQ_HZ)  /* 16 MHz / 1 kHz = 16000 */
#define PWM_RAMP_STEP 50  /* Duty cycle step per timer tick */
#define PWM_RAMP_PERIOD_MS 10  /* Timer period for smooth ramping */

/* PWM driver instance */
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

/* PWM data */
static nrf_pwm_values_individual_t m_pwm_values;
static nrf_pwm_sequence_t const m_pwm_sequence =
{
    .values.p_individual = &m_pwm_values,
    .length = NRF_PWM_VALUES_LENGTH(m_pwm_values),
    .repeats = 0,
    .end_delay = 0
};

/* App timer */
APP_TIMER_DEF(m_pwm_timer);

/* State variables */
static uint16_t m_current_duty_cycle = 0;
static bool m_ramp_up = true;
static bool m_blinking_enabled = false;
static uint8_t m_current_led_index = 0;

/* Button state for double-click detection */
static uint32_t m_last_button_press_time = 0;
#define DOUBLE_CLICK_THRESHOLD_MS 300

/**@brief Function for configuring clock.
 */
static void clock_init(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

/**@brief Function for initializing the PWM driver.
 */
static void pwm_init(void)
{
    ret_code_t err_code;
    
    nrfx_pwm_config_t pwm_config = NRFX_PWM_DEFAULT_CONFIG;
    pwm_config.output_pins[0] = LED1;
    pwm_config.output_pins[1] = LED2;
    pwm_config.output_pins[2] = LED3;
    pwm_config.output_pins[3] = LED4;
    pwm_config.top_value = PWM_TOP;
    pwm_config.base_clock = NRF_PWM_CLK_16MHz;
    pwm_config.count_mode = NRF_PWM_MODE_UP;
    pwm_config.load_mode = NRF_PWM_LOAD_INDIVIDUAL;
    
    err_code = nrfx_pwm_init(&m_pwm0, &pwm_config, NULL);
    APP_ERROR_CHECK(err_code);
    
    /* Initialize all PWM channels to 0 (LEDs off) */
    m_pwm_values.channel_0 = 0;
    m_pwm_values.channel_1 = 0;
    m_pwm_values.channel_2 = 0;
    m_pwm_values.channel_3 = 0;
    
    nrfx_pwm_simple_playback(&m_pwm0, &m_pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
}

/**@brief Function to set PWM duty cycle for a specific LED.
 * @param[in] led_index Index of the LED (0-3).
 * @param[in] duty_cycle Duty cycle value (0-PWM_TOP).
 */
static void set_led_duty_cycle(uint8_t led_index, uint16_t duty_cycle)
{
    if (led_index >= sequence_length)
        return;
    
    switch (led_index)
    {
        case 0:
            m_pwm_values.channel_0 = duty_cycle;
            break;
        case 1:
            m_pwm_values.channel_1 = duty_cycle;
            break;
        case 2:
            m_pwm_values.channel_2 = duty_cycle;
            break;
        case 3:
            m_pwm_values.channel_3 = duty_cycle;
            break;
        default:
            break;
    }
}

/**@brief Function to turn off all LEDs.
 */
static void turn_off_all_leds(void)
{
    m_pwm_values.channel_0 = 0;
    m_pwm_values.channel_1 = 0;
    m_pwm_values.channel_2 = 0;
    m_pwm_values.channel_3 = 0;
}

/**@brief Timer callback for PWM duty cycle ramping.
 */
static void pwm_timer_handler(void * p_context)
{
    if (!m_blinking_enabled)
        return;
    
    /* Update duty cycle */
    if (m_ramp_up)
    {
        if (m_current_duty_cycle < PWM_TOP)
        {
            m_current_duty_cycle += PWM_RAMP_STEP;
            if (m_current_duty_cycle > PWM_TOP)
                m_current_duty_cycle = PWM_TOP;
        }
        else
        {
            m_ramp_up = false;
        }
    }
    else
    {
        if (m_current_duty_cycle > 0)
        {
            m_current_duty_cycle -= PWM_RAMP_STEP;
        }
        else
        {
            m_ramp_up = true;
            /* Move to next LED in sequence */
            m_current_led_index = (m_current_led_index + 1) % sequence_length;
        }
    }
    
    /* Update the current LED */
    set_led_duty_cycle(m_current_led_index, m_current_duty_cycle);
}

/**@brief Function for initializing the app_timer module.
 */
static void app_timer_setup(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_pwm_timer,
                               APP_TIMER_MODE_REPEATED,
                               pwm_timer_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting the blinking sequence.
 */
static void start_blinking(void)
{
    if (m_blinking_enabled)
        return;
    
    m_blinking_enabled = true;
    m_current_duty_cycle = 0;
    m_ramp_up = true;
    m_current_led_index = 0;
    turn_off_all_leds();
    
    ret_code_t err_code = app_timer_start(m_pwm_timer,
                                          APP_TIMER_TICKS(PWM_RAMP_PERIOD_MS),
                                          NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for stopping the blinking sequence.
 */
static void stop_blinking(void)
{
    if (!m_blinking_enabled)
        return;
    
    m_blinking_enabled = false;
    
    ret_code_t err_code = app_timer_stop(m_pwm_timer);
    APP_ERROR_CHECK(err_code);
    
    /* Maintain current duty cycle (do not turn off LED) */
}

/**@brief GPIOTE event handler for button.
 */
static void gpiote_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (pin == BUTTON1 && action == NRF_GPIOTE_POLARITY_LOTOHI)
    {
        uint32_t current_time = app_timer_cnt_get();
        uint32_t time_since_last_press = app_timer_cnt_diff_compute(current_time, m_last_button_press_time);
        uint32_t threshold_ticks = APP_TIMER_TICKS(DOUBLE_CLICK_THRESHOLD_MS);
        
        if (time_since_last_press < threshold_ticks)
        {
            /* Double click detected */
            if (m_blinking_enabled)
            {
                stop_blinking();
            }
            else
            {
                start_blinking();
            }
            m_last_button_press_time = 0;  /* Reset to avoid triple-click */
        }
        else
        {
            /* Single click - just record the time */
            m_last_button_press_time = current_time;
        }
    }
}

/**@brief Function for initializing the GPIOTE module.
 */
static void gpiote_init(void)
{
    ret_code_t err_code;
    
    if (!nrfx_gpiote_is_init())
    {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
    
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    
    err_code = nrfx_gpiote_in_init(BUTTON1, &in_config, gpiote_event_handler);
    APP_ERROR_CHECK(err_code);
    
    nrfx_gpiote_in_event_enable(BUTTON1, true);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    /* Initialize clock */
    clock_init();
    nrf_delay_ms(100);
    
    /* Initialize PWM */
    pwm_init();
    
    /* Initialize app_timer */
    app_timer_setup();
    
    /* Initialize GPIOTE for button handling */
    gpiote_init();
    
    /* Infinite loop */
    while (true)
    {
        __WFE();
    }
    
    return 0;
}