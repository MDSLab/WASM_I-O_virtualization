/*
 * src/main.c - Host WAMR+Zephyr LED toggle (threaded)
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "wasm_export.h"
#include "wasm_native_led.h"
#include "led_toggle_wasm.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ── WAMR sizing ─────────────────────────────────────────────────────────── */
#define WASM_STACK_SIZE   8192
#define WASM_HEAP_SIZE    8192

/* ── Thread config ───────────────────────────────────────────────────────── */
#define WASM_THREAD_STACK_SIZE   4096
#define WASM_THREAD_PRIORITY     5

K_THREAD_STACK_DEFINE(wasm_thread_stack, WASM_THREAD_STACK_SIZE);
static struct k_thread wasm_thread_data;

/* ── Shared state passed to the thread ───────────────────────────────────── */
struct wasm_thread_ctx {
    wasm_module_inst_t instance;
    int                result;   /* populated by thread before exit */
};


/* ── Thread entry ─────────────────────────────────────────────────────────── */
static void wasm_thread_entry(void *p1, void *p2, void *p3)
{
    struct wasm_thread_ctx *ctx = (struct wasm_thread_ctx *)p1;
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    ctx->result = 0;

    wasm_function_inst_t func =
        wasm_runtime_lookup_function(ctx->instance, "app_main");
    if (!func) {
        LOG_ERR("app_main not found");
        ctx->result = -ENOENT;
        return;
    }

    wasm_exec_env_t exec_env =
        wasm_runtime_create_exec_env(ctx->instance, WASM_STACK_SIZE);
    if (!exec_env) {
        LOG_ERR("Failed to create exec_env");
        ctx->result = -ENOMEM;
        return;
    }

    if (!wasm_runtime_call_wasm(exec_env, func, 0, NULL)) {
        LOG_ERR("Exec error: %s",
                wasm_runtime_get_exception(ctx->instance));
        ctx->result = -EFAULT;
    }

    wasm_runtime_destroy_exec_env(exec_env);
}

/* ── main ─────────────────────────────────────────────────────────────────── */
int main(void)
{
    LOG_INF("WAMR LED Toggle - starting");

   

    /* WAMR init */
    RuntimeInitArgs init_args = {0};
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERR("WAMR init failed");
        return -ENOMEM;
    }

    /* Registra funzioni native GPIO */
    if (!register_native_led_functions()) {
        LOG_ERR("Native registration failed");
        wasm_runtime_destroy();
        return -EFAULT;
    }

    /* Carica modulo WASM */
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(
        (uint8_t *)led_toggle_wasm, led_toggle_wasm_len,
        error_buf, sizeof(error_buf));
    if (!module) {
        LOG_ERR("Load failed: %s", error_buf);
        wasm_runtime_destroy();
        return -EINVAL;
    }

    /* Istanzia */
    wasm_module_inst_t instance = wasm_runtime_instantiate(
        module, WASM_STACK_SIZE, WASM_HEAP_SIZE,
        error_buf, sizeof(error_buf));
    if (!instance) {
        LOG_ERR("Instantiate failed: %s", error_buf);
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        return -EINVAL;
    }

    /* Prepara contesto e lancia thread */
    struct wasm_thread_ctx ctx = {
        .instance = instance,
        .result   = 0,
    };

    k_tid_t tid = k_thread_create(
        &wasm_thread_data,
        wasm_thread_stack, WASM_THREAD_STACK_SIZE,
        wasm_thread_entry,
        &ctx, NULL, NULL,
        WASM_THREAD_PRIORITY, 0, K_NO_WAIT);

    LOG_INF("WASM thread spawned (tid %p)", (void *)tid);

    /* Attendi il completamento del thread */
    k_thread_join(&wasm_thread_data, K_FOREVER);

    if (ctx.result != 0) {
        LOG_ERR("WASM thread exited with error: %d", ctx.result);
    } else {
        LOG_INF("WASM thread completed successfully");
    }

    /* Cleanup */
    wasm_runtime_deinstantiate(instance);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();

    return ctx.result;
}