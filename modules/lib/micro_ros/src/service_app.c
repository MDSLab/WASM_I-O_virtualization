/*
 * service_app.c
 *

 * Build command:
 *
 *  /opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown -nostdlib -Wl,--no-entry -Wl,--export=app_main -Wl,--allow-undefined -O2 -o service_app.wasm service_app.c
 *   xxd -i service_app.wasm > ../src/service_app_wasm.h
 * 
 *
 */

/* Declare the host-provided functions as imports */
extern void micro_ros_service_init();
extern void sleep_ms(int ms);
/* ------------------------------------------------------------------ */
/*  Entry point called by the WAMR host                               */
/* ------------------------------------------------------------------ */
__attribute__((export_name("app_main")))
void app_main(void)
{
    micro_ros_service_init();
   
}
