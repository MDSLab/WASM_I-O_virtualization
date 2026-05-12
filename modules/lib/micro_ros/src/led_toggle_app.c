/*
 * led_toggle.c
 *
 * Build command:
 *
 *  /opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown -nostdlib -Wl,--no-entry -Wl,--export=app_main -Wl,--allow-undefined -O2 -o led_toggle_app.wasm led_toggle_app.c
 *   xxd -i led_toggle_app.wasm > ../src/led_toggle_app_wasm.h
 *
 *
 */

/* Declare the host-provided functions as imports */

extern void green_led_toggle();
extern void micro_ros_app_init();
extern void sleep_ms(int ms);
/* ------------------------------------------------------------------ */
/*  Entry point called by the WAMR host                               */
/* ------------------------------------------------------------------ */
__attribute__((export_name("app_main")))
void app_main(void)
{
    micro_ros_app_init();
    while (1) {
        green_led_toggle();
    }
}
