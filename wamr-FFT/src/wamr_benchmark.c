/*
 * wasm_app/led_toggle.c
 *
 * WASM-side application.
 * Compiled with wasi-sdk (clang --target=wasm32-unknown-unknown) to produce
 * wamr_benchmark.wasm, which is then xxd-converted to led_toggle_wasm.h.
 *
 * Build command (from wasm_app/):
 *
 * /opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown -nostdlib -Wl,--no-entry -Wl,--export=app_main -Wl,--allow-undefined -O3 -o wamr_benchmark.wasm wamr_benchmark.c
 *
 *   xxd -i wamr_benchmark.wasm > wamr_benchmark.h
 *   # Then rename the array/length symbols to wamr_benchmark_wasm / wamr_benchmark_wasm_len
 *
 */

/* Declare the host-provided functions as imports */
extern void run_benchmark();

/* ------------------------------------------------------------------ */
/*  Entry point called by the WAMR host                               */
/* ------------------------------------------------------------------ */
__attribute__((export_name("app_main")))
void app_main(void)
{
    run_benchmark();
}
