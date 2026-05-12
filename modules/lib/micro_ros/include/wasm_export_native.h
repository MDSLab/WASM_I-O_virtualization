/*
 * src/wasm_export_native.h
 *
 * Dichiarazioni per le funzioni native (host-side) esposte al modulo WASM.
 */

#ifndef WASM_EXPORT_NATIVE_H
#define WASM_EXPORT_NATIVE_H


#include "wasm_export.h"

/* Chiamata da main.c dopo init runtime, prima del load del modulo */
bool register_native_functions(void);



#endif /* WASM_EXPORT_NATIVE_H */