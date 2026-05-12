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
bool register_native_functions(void);


#endif /* WASM_NATIVE_LED_H */