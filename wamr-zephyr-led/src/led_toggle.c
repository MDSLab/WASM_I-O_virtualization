/*
 * wasm_app/led_toggle.c
 *
 * WASM-side application.
 * Compiled with wasi-sdk (clang --target=wasm32-unknown-unknown) to produce
 * led_toggle.wasm, which is then xxd-converted to led_toggle_wasm.h.
 *
 * Build command (from wasm_app/):
 *
 * /opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown -nostdlib -Wl,--no-entry -Wl,--export=app_main -Wl,--allow-undefined -O2 -o led_toggle.wasm led_toggle.c
 *
 *   xxd -i led_toggle.wasm > led_toggle_wasm.h
 *   # Then rename the array/length symbols to led_toggle_wasm / led_toggle_wasm_len
 *
 * The host registers these imports under the "env" module:
 *   led_on()       - turn LED on
 *   led_off()      - turn LED off
 *   led_toggle()   - toggle LED state
 *   sleep_ms(ms)   - block for <ms> milliseconds
 */

/* Declare the host-provided functions as imports */
extern void led_init();
extern void led_toggle();
extern void sleep_ms(int ms);

/* ------------------------------------------------------------------ */
/*  Entry point called by the WAMR host                               */
/* ------------------------------------------------------------------ */
__attribute__((export_name("app_main")))
void app_main(void)
{
    led_init();
    while (1) {
        led_toggle();
    }
}
