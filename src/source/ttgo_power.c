#include "ttgo_power.h"
#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_check.h>
#define ADC_EN_GPIO     GPIO_NUM_14          // enable pin (output)
#define BAT_ADC_UNIT    ADC_UNIT_1
#define BAT_ADC_CHANNEL ADC_CHANNEL_6        // GPIO34
#define BAT_ADC_ATTEN   ADC_ATTEN_DB_12      // see note below re: DB_11 vs DB_12
// Resting-voltage -> percentage points for a single LiPo cell.
// Approximate; based on typical discharge curves. Descending by mV.
typedef struct { int mv; int pct; } batt_point_t;

static const batt_point_t k_curve[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80},
    {3980, 75},  {3950, 70}, {3910, 65}, {3870, 60}, {3850, 55},
    {3840, 50},  {3820, 45}, {3800, 40}, {3790, 35}, {3770, 30},
    {3750, 25},  {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},
    {3270, 0},
};
#define CURVE_LEN (sizeof(k_curve) / sizeof(k_curve[0]))

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t         s_cali_handle;

void ttgo_power_init(void) {
// enable battery power
        gpio_config_t en_conf = {
        .pin_bit_mask = (1ULL << ADC_EN_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&en_conf);
    ttgo_power_enable(true);

    // --- ADC1 oneshot unit ---
    
    adc_oneshot_unit_init_cfg_t unit_cfg;
    memset(&unit_cfg,0,sizeof(unit_cfg));
    unit_cfg.unit_id = BAT_ADC_UNIT;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg;
    memset(&chan_cfg,0,sizeof(chan_cfg));
    chan_cfg.atten    = BAT_ADC_ATTEN;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;    // 12-bit on ESP32
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHANNEL, &chan_cfg));

    // --- Calibration (line fitting for classic ESP32) ---
    adc_cali_line_fitting_config_t cali_cfg;
    memset(&cali_cfg,0,sizeof(cali_cfg));
    cali_cfg.unit_id  = BAT_ADC_UNIT;
    cali_cfg.atten    = BAT_ADC_ATTEN;
    cali_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali_handle));

}
uint8_t ttgo_power_level(void) {
    uint16_t mv = ttgo_power_voltage();
    if (mv >= k_curve[0].mv)            return 100;
    if (mv <= k_curve[CURVE_LEN-1].mv) return 0;

    for (size_t i = 0; i < CURVE_LEN - 1; i++) {
        int hi_mv = k_curve[i].mv,   hi_p = k_curve[i].pct;
        int lo_mv = k_curve[i+1].mv, lo_p = k_curve[i+1].pct;
        if (mv <= hi_mv && mv >= lo_mv) {
            // linear interpolation within this segment
            return lo_p + (mv - lo_mv) * (hi_p - lo_p) / (hi_mv - lo_mv);
        }
    }
    return 0; // unreachable
}
uint16_t ttgo_power_voltage(void) {
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, BAT_ADC_CHANNEL, &raw));

    int mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali_handle, raw, &mv));

    return mv * 2;   // undo the on-board 1:2 voltage divider
}
static bool s_power_enabled=false;
bool ttgo_power_enabled(void) {
    return s_power_enabled;
}
void ttgo_power_enable(bool value) {
    gpio_set_level(ADC_EN_GPIO,value);
    s_power_enabled = value;

}
