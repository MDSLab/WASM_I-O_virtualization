
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "wasm_export.h"
#include "wasm_native.h"
#include "wamr_benchmark.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ── WAMR sizing ─────────────────────────────────────────────────────────── */
#define WASM_STACK_SIZE   8192
#define WASM_HEAP_SIZE    8192




/* ── main ─────────────────────────────────────────────────────────────────── */
int main(void)
{
    printk("WAMR FFT Benchmark - starting");

    SysTick->CTRL = 0; // Disable SysTick
    __disable_irq();    // Optional: disable all IRQs to reduce

    SCB_EnableDCache();
    SCB_EnableICache();

    /* WAMR init */
    RuntimeInitArgs init_args = {0};
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    if (!wasm_runtime_full_init(&init_args)) {
        printk("WAMR init failed");
        return -ENOMEM;
    }

    /* Registra funzioni native GPIO */
    if (!register_native_functions()) {
        printk("Native registration failed");
        wasm_runtime_destroy();
        return -EFAULT;
    }

    /* Carica modulo WASM */
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(
        (uint8_t *)wamr_benchmark_aot, wamr_benchmark_aot_len,
        error_buf, sizeof(error_buf));
    if (!module) {
        printk("Load failed: %s", error_buf);
        wasm_runtime_destroy();
        return -EINVAL;
    }

    /* Istanzia */
    wasm_module_inst_t instance = wasm_runtime_instantiate(
        module, WASM_STACK_SIZE, WASM_HEAP_SIZE,
        error_buf, sizeof(error_buf));
    if (!instance) {
        printk("Instantiate failed: %s", error_buf);
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        return -EINVAL;
    }

    wasm_exec_env_t exec_env =
        wasm_runtime_create_exec_env(instance, WASM_STACK_SIZE);
    if (!exec_env) {
        printk("Failed to create exec_env\\n");
        wasm_runtime_deinstantiate(instance);
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        return -ENOMEM;
    }

     wasm_function_inst_t func =
        wasm_runtime_lookup_function(instance, "app_main");
    if (!func) {
        printk("app_main not found\\n");
        wasm_runtime_destroy_exec_env(exec_env);
        wasm_runtime_deinstantiate(instance);
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        return -ENOENT;
    }

    int ret = 0;
    if (!wasm_runtime_call_wasm(exec_env, func, 0, NULL)) {
        const char *exc = wasm_runtime_get_exception(instance);
        printk("Exec error: %s\\n", exc ? exc : "unknown");
        ret = -EFAULT;
    } else {
        printk("WASM benchmark completed\\n");
    }
}