/**
 * This file is shamlessly copied from the following repository:
 * https://github.com/espressif/esp-idf/tree/v4.3.4/examples/system/ulp/
 * Apache 2.0 License applies.
 */

#include "esp32/ulp.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
// #include "driver/adc.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_adc/adc_oneshot.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_periph.h"
#include "soc/sens_reg.h"
#include "ulp_main.h"
#include "ulp.h"
#include "ulp_adc.h"
#include <stdio.h>

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const uint32_t silent_count_resval = 200;
static const uint16_t hall_threshold = 20;
static const float voltage_mul = 1 / 370.0;

void init_ulp_program(void)
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
    ulp_hallthreshold = static_cast<uint32_t>(hall_threshold);
    // ulp_silentcnt = static_cast<uint32_t>(silent_count_resval);
    ulp_halloff = 30;  // Set it to the expected value, so we don't have a swing in...
    ulp_silentcnt = 1; // Set it to one, for the first measurement to be output
    ulp_detected = 0;
    ulp_detcnt = 0;

    // ulp_adc_cfg_t cfg = {
    //     .adc_n = ADC_UNIT_1,
    //     .channel = PWR_BAT_CHAN,
    //     .atten = ADC_ATTEN_DB_12,
    //     .width = ADC_BITWIDTH_12,
    //     .ulp_mode = ADC_ULP_MODE_FSM,
    // };

    // ESP_ERROR_CHECK(ulp_adc_init(&cfg));

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_FSM,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config_6db = {
        .atten = ADC_ATTEN_DB_6,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_chan_cfg_t config_12db = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, HALL_CHAN_N, &config_6db));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, HALL_CHAN_P, &config_6db));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PWR_3V3_CHAN, &config_12db));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PWR_BAT_CHAN, &config_12db));

    /* Set ULP wake up period to T = 20ms. */
    // ulp_set_wakeup_period(0, 20000);
    ulp_set_wakeup_period(0, 100000);

    /* enable adc1 */
    // adc1_ulp_enable();
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

void start_ulp_program(void)
{
    /* Reset sample counter */
    // ulp_sample_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}

uint16_t get_hallval(void)
{
    uint16_t hallval = static_cast<uint16_t>(ulp_hallval);

    // ESP_LOGW("ulp", "Hall value %lu, 0x%lx", ulp_hallval, ulp_hallval);

    return hallval;
}

void reset_silentcount(void)
{
    ulp_silentcnt = silent_count_resval;
}

void reset_detection(void)
{
    ulp_detected = false;
}

bool hall_detected(void)
{
    return ulp_detected;
}

uint16_t get_detection_cnt(void)
{
    return ulp_detcnt;
}

float get_5v(void)
{
    float retval;
    retval = static_cast<uint16_t>(ulp_pwr5val);
    retval *= voltage_mul;
    return retval;
}

float get_3v(void)
{
    float retval;
    retval = static_cast<uint16_t>(ulp_pwr3val);
    retval *= voltage_mul;
    return retval;
}

void print_hallraw(void)
{
    float val5v = get_5v();
    float val3v = get_3v();
    ESP_LOGW("ulp", "---------------------", );
    ESP_LOGW("ulp", "Hall 0P : %5lu, 0x%04lx", ulp_hallphase0p & 0x0FFFF, ulp_hallphase0p & 0x0FFFF);
    ESP_LOGW("ulp", "Hall 0N : %5lu, 0x%04lx", ulp_hallphase0n & 0x0FFFF, ulp_hallphase0n & 0x0FFFF);
    ESP_LOGW("ulp", "Hall 0D : %5lu, 0x%04lx", ulp_hallphase0d & 0x0FFFF, ulp_hallphase0d & 0x0FFFF);
    ESP_LOGW("ulp", "---------------------", );
    ESP_LOGW("ulp", "Hall 1P : %5lu, 0x%04lx", ulp_hallphase1p & 0x0FFFF, ulp_hallphase1p & 0x0FFFF);
    ESP_LOGW("ulp", "Hall 1N : %5lu, 0x%04lx", ulp_hallphase1n & 0x0FFFF, ulp_hallphase1n & 0x0FFFF);
    ESP_LOGW("ulp", "Hall 1D : %5lu, 0x%04lx", ulp_hallphase1d & 0x0FFFF, ulp_hallphase1d & 0x0FFFF);
    ESP_LOGW("ulp", "---------------------", );
    ESP_LOGW("ulp", "Hall Val: %5lu, 0x%04lx, 0x%08lx", ulp_hallval & 0x0FFFF, ulp_hallval & 0x0FFFF, ulp_hallval);
    ESP_LOGW("ulp", "Hall Thr: %5lu, 0x%04lx, 0x%08lx", ulp_hallthreshold & 0x0FFFF, ulp_hallthreshold & 0x0FFFF, ulp_hallthreshold);
    ESP_LOGW("ulp", "---------------------", );
    ESP_LOGW("ulp", "Offset  : %5lu, 0x%04lx", ulp_halloff & 0x0FFFF, ulp_halloff & 0x0FFFF);
    ESP_LOGW("ulp", "SilentCn: %5lu, 0x%04lx", ulp_silentcnt & 0x0FFFF, ulp_silentcnt & 0x0FFFF);
    ESP_LOGW("ulp", "Detected: %5lu, 0x%04lx", ulp_detected & 0x0FFFF, ulp_detected & 0x0FFFF);
    ESP_LOGW("ulp", "---------------------", );
    ESP_LOGW("ulp", "ADC 3V  : %.3f, %5lu, 0x%04lx, 0x%08lx", val3v, ulp_pwr3val & 0x0FFFF, ulp_pwr3val & 0x0FFFF, ulp_pwr3val);
    ESP_LOGW("ulp", "ADC 5V  : %.3f, %5lu, 0x%04lx, 0x%08lx", val5v, ulp_pwr5val & 0x0FFFF, ulp_pwr5val & 0x0FFFF, ulp_pwr5val);
    ESP_LOGW("ulp", "---------------------", );
}