#ifndef TTGO_POWER_H
#define TTGO_POWER_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void ttgo_power_init(void);
uint8_t ttgo_power_level(void);
uint16_t ttgo_power_voltage(void);
bool ttgo_power_enabled(void);
void ttgo_power_enable(bool value);
#ifdef __cplusplus
}
#endif
#endif // TTGO_POWER_H