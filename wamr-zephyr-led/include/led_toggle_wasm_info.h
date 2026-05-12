/*
 * src/led_toggle_wasm.h
 *
 * Embedded WASM binary for the LED-toggle application.
 *
 * *** HOW TO REGENERATE ***
 *
 *  1. Install wasi-sdk (https://github.com/WebAssembly/wasi-sdk/releases)
 *
 *  2. From the wasm_app/ directory:
 *
 *       /opt/wasi-sdk/bin/clang                  \
 *           --target=wasm32-unknown-unknown        \
 *           -nostdlib                              \
 *           -Wl,--no-entry                         \
 *           -Wl,--export=app_main                  \
 *           -Wl,--allow-undefined                  \
 *           -O2                                    \
 *           -o led_toggle.wasm                     \
 *           led_toggle.c
 *
 *  3. Convert to C header:
 *
 *       xxd -i led_toggle.wasm led_toggle_wasm_raw.h
 *
 *  4. Copy the array and length from led_toggle_wasm_raw.h into this file,
 *     renaming them to `led_toggle_wasm` and `led_toggle_wasm_len`.
 *
 * The minimal WASM binary below is a hand-crafted placeholder that exports
 * an `app_main` function which blinks via the native imports.
 * Replace with the output of step 3 for production use.
 *
 * Disassembly (WAT) of the placeholder:
 *
 *   (module
 *     (import "env" "led_toggle" (func $led_toggle))
 *     (import "env" "sleep_ms"   (func $sleep_ms (param i32)))
 *     (func $app_main (export "app_main")
 *       (block $break
 *         (loop $loop
 *           call $led_toggle
 *           i32.const 500
 *           call $sleep_ms
 *           br $loop
 *         )
 *       )
 *     )
 *   )
 */

#ifndef LED_TOGGLE_WASM_H
#define LED_TOGGLE_WASM_H

#include <stdint.h>

/* Minimal hand-assembled WASM module (loop: toggle + sleep 500 ms) */
static const uint8_t led_toggle_wasm[] = {
    /* Magic + version */
    0x00, 0x61, 0x73, 0x6d,   /* \0asm                */
    0x01, 0x00, 0x00, 0x00,   /* version 1            */

    /* Type section (id=1): two types
       type[0] = () -> ()        (led_toggle, app_main)
       type[1] = (i32) -> ()     (sleep_ms)            */
    0x01,                     /* section id: type     */
    0x09,                     /* section size         */
    0x02,                     /* 2 types              */
    /* type[0]: () -> () */
    0x60, 0x00, 0x00,
    /* type[1]: (i32) -> () */
    0x60, 0x01, 0x7f, 0x00,

    /* Import section (id=2): two imports */
    0x02,                     /* section id: import   */
    0x1b,                     /* section size         */
    0x02,                     /* 2 imports            */
    /* import env.led_toggle : func type[0] */
    0x03, 0x65, 0x6e, 0x76,   /* "env"                */
    0x0a, 0x6c, 0x65, 0x64, 0x5f, 0x74, 0x6f, 0x67,
          0x67, 0x6c, 0x65,   /* "led_toggle"         */
    0x00, 0x00,               /* func, type[0]        */
    /* import env.sleep_ms : func type[1] */
    0x03, 0x65, 0x6e, 0x76,   /* "env"                */
    0x08, 0x73, 0x6c, 0x65, 0x65, 0x70, 0x5f, 0x6d,
          0x73,               /* "sleep_ms"           */
    0x00, 0x01,               /* func, type[1]        */

    /* Function section (id=3): one local function */
    0x03,                     /* section id: function */
    0x02,                     /* section size         */
    0x01,                     /* 1 function           */
    0x00,                     /* type[0]              */

    /* Export section (id=7) */
    0x07,                     /* section id: export   */
    0x0c,                     /* section size         */
    0x01,                     /* 1 export             */
    0x08, 0x61, 0x70, 0x70, 0x5f, 0x6d, 0x61, 0x69,
          0x6e,               /* "app_main"           */
    0x00, 0x02,               /* func, index 2        */

    /* Code section (id=10) */
    0x0a,                     /* section id: code     */
    0x0c,                     /* section size         */
    0x01,                     /* 1 function body      */
    0x0a,                     /* body size            */
    0x00,                     /* 0 locals             */
    /* body: loop { call 0; i32.const 500; call 1; br 0 } */
    0x03, 0x40,               /* loop (void)          */
    0x10, 0x00,               /* call $led_toggle     */
    0x41, 0xf4, 0x03,         /* i32.const 500        */
    0x10, 0x01,               /* call $sleep_ms       */
    0x0c, 0x00,               /* br 0 (loop)          */
    0x0b,                     /* end loop             */
    0x0b,                     /* end func             */
};

static const uint32_t led_toggle_wasm_len =
    (uint32_t)sizeof(led_toggle_wasm);

#endif /* LED_TOGGLE_WASM_H */
