/*
 * src/wasm_export_native.c
 *
 * Implements the native functions that the WASM module calls through
 * WAMR's NativeSymbol mechanism.
 *
 * Signature strings follow WAMR conventions:
 *   '(' param-types ')' return-type
 *   i = i32,  I = i64,  f = f32,  F = f64,  * = pointer,  $ = string
 *   Empty params/return → "()" or ""
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "wasm_export.h"
#include "wasm_native_led.h"

LOG_MODULE_REGISTER(native_led, LOG_LEVEL_DBG);

/* ------------------------------------------------------------------ */
/*  Native function implementations                                    */
/* ------------------------------------------------------------------ */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec my_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(pb8_out), gpios);


static void native_led_init(wasm_exec_env_t exec_env){

     /* GPIO init */
   gpio_pin_configure_dt(&my_pin, GPIO_OUTPUT_INACTIVE);
}




/* void led_toggle(void) */
static void native_led_toggle(wasm_exec_env_t exec_env)
{

    gpio_pin_toggle_dt(&my_pin);

}

/* void sleep_ms(int32_t ms) */
static void native_sleep_ms(wasm_exec_env_t exec_env, int32_t ms)
{
    (void)exec_env;
    if (ms > 0) {
        k_msleep(ms);
    }
}

/* ------------------------------------------------------------------ */
/*  NativeSymbol table                                                 */
/* ------------------------------------------------------------------ */
static NativeSymbol native_symbols[] = {
    { "led_init",   native_led_init,   "()",  NULL },
    { "led_toggle", native_led_toggle, "()",  NULL },
    { "sleep_ms",   native_sleep_ms,   "(i)",  NULL }
};

bool register_native_led_functions(void)
{
    int n = sizeof(native_symbols) / sizeof(native_symbols[0]);
    if (!wasm_runtime_register_natives("env", native_symbols, n)) {
        return false;
    }
    LOG_INF("Registered %d native LED symbols", n);
    return true;
}
