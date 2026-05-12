/*
 * src/wasm_native_led.h
 *
 * Dichiarazioni per le funzioni native (host-side) esposte al modulo WASM.
 */

#ifndef WASM_NATIVE_LED_H
#define WASM_NATIVE_LED_H

#include <zephyr/drivers/gpio.h>
#include "wasm_export.h"

/* Chiamata da main.c dopo init runtime, prima del load del modulo */
bool register_native_led_functions(void);

/* Accessor al gpio_dt_spec definito in main.c */
const struct gpio_dt_spec *get_led_spec(int32_t index);

#endif /* WASM_NATIVE_LED_H */