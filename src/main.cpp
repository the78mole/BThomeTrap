#include "esp_log.h"
#include "esp_sleep.h"
#include <stdio.h>

#include "bluetooth.h"
#include "ulp.h"

// app_main must link to C code
#ifdef __cplusplus
extern "C"
{
#endif
    void app_main(void);

#ifdef __cplusplus
}
#endif

void app_main(void)
{
    // esp_netif_set_hostname();

    ESP_LOGI("main", "Starting up");
    ESP_LOGI("main", "Init BLE");
    ble_init();
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP)
    {
        ESP_LOGI("main", "Not ULP wakeup, initializing ULP");
        init_ulp_program();
    }
    else
    {
        ESP_LOGI("main", "ULP wakeup, do stuff");
        bool detected = hall_detected();
        uint16_t detcnt = get_detection_cnt();
        if (detected)
        {
            ESP_LOGW("main", "Hall movement detected.");
            ESP_LOGW("main", "Det Cnt : %d", detcnt);
        }
        else
        {
            ESP_LOGI("main", "Det Cnt : %d", detcnt);
        }

        uint16_t hallval = get_hallval();
        ESP_LOGI("main", "Hall val: %u, 0x%x", hallval, hallval);
        float val3v = get_3v();
        float val5v = get_5v();
        ESP_LOGW("main", "Supply  : %.3fV", val3v);
        ESP_LOGW("main", "Battery : %.3fV", val5v);

        ble_advert(detected, detcnt, val3v, val5v);

        reset_silentcount();
        reset_detection();
    }
    // uint32_t pulse_count = update_pulse_count();
    // ble_advert(pulse_count);

    ESP_LOGI("main", "Disable BLE.\n\n");
    ble_deinit();

    ESP_LOGI("main", "Entering deep sleep\n\n");
    start_ulp_program();

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}
