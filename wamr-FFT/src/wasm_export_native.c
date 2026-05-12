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
#include <math.h>
#include "wasm_export.h"
#include "wasm_native.h"
#include "twiddle1024.h"

LOG_MODULE_REGISTER(wamr_fft, LOG_LEVEL_INF); 
#define N_MEAS 50
#define T_CRIT_95 2.0096f  // t-student per df=49

#define N_FFT 1024
#define NUM_ITER 100



static float buf[2 * N_FFT];



float misure[50];

// Bit reversal
static void bit_reverse(float *b)
{
    int j = 0;
    for (int i = 0; i < N_FFT; ++i) {
        if (i < j) {
            float tr = b[2*i+0];
            float ti = b[2*i+1];
            b[2*i+0] = b[2*j+0];
            b[2*i+1] = b[2*j+1];
            b[2*j+0] = tr;
            b[2*j+1] = ti;
        }
    int bit = N_FFT >> 1;

    while (j & bit) { j ^= bit; bit >>= 1; }
    j |= bit;
    }
}
 
// Radix-2 FFT
static void fft_radix2(float *b)
{
    bit_reverse(b);
    for (int len = 2; len <= N_FFT; len <<= 1) {
        int half_len = len >> 1;
        int stride = N_FFT / len;
        for (int i = 0; i < N_FFT; i += len) {
            for (int j = 0; j < half_len; j++) {
            int idx1 = i + j;
            int idx2 = idx1 + half_len;
            int k = j * stride;
            float wr = twiddle_cos[k];
            float wi = twiddle_sin[k];
            float xr = b[2*idx2+0];
            float xi = b[2*idx2+1];
            float tr = wr*xr - wi*xi;
            float ti = wr*xi + wi*xr;
            float ur = b[2*idx1+0];
            float ui = b[2*idx1+1];
            b[2*idx1+0] = ur + tr;
            b[2*idx1+1] = ui + ti;
            b[2*idx2+0] = ur - tr;
            b[2*idx2+1] = ui - ti;
            }
        }
    }
}

static void fft_init(void)
{
    for (int i = 0; i < N_FFT; ++i) {
        float x = (float)i;
        buf[2 * i + 0] = x;
        buf[2 * i + 1] = 0.5f * x;
    }
}
// FFT core implementation omitted (same as WASM module version)
void fft_bench(int iterations)
{
    for (int k = 0; k < iterations; ++k) {
        fft_radix2(buf);
    }
}
// DWT cycle counter
static void DWT_Init(void)
{
    // Enable trace
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Reset cycle counter
    DWT->CYCCNT = 0;
    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


uint32_t run_single_benchmark(void)
{
    uint32_t total_cycles = 0;
    uint32_t avg_cycles = 0;
    DWT_Init();
    fft_init();
    uint32_t start = DWT->CYCCNT;
    fft_bench(NUM_ITER);
    uint32_t end = DWT->CYCCNT;
    total_cycles = end - start;
    avg_cycles = total_cycles / NUM_ITER;
    return avg_cycles;
}




void compute_confidence_interval(float *misure) {
    float sum = 0.0f;
    float mean, sem, me;
    float variance = 0.0f;

    // Media
    for (int i = 0; i < N_MEAS; i++) {
        sum += misure[i];
    }
    mean = sum / N_MEAS;

    // Varianza (sample variance)
    for (int i = 0; i < N_MEAS; i++) {
        float diff = misure[i] - mean;
        variance += diff * diff;
    }
    variance /= (N_MEAS - 1);

    // Standard error
    sem = sqrtf(variance / N_MEAS);

    // Margine di errore
    me = T_CRIT_95 * sem;

    float ci_low = mean - me;
    float ci_high = mean + me;

    printk("Mean = %f\n", mean);
    printk("SEM = %f\n", sem);
    printk("CI 95%% = [%f, %f]\n", ci_low, ci_high);
}


void run_benchmark_init(void)
{
   
    float sum = 0.0f, sum_sq = 0.0f;

    for (int i = 0; i < N_MEAS; i++) {
        misure[i] = run_single_benchmark();
        sum += misure[i];
        sum_sq += misure[i] * misure[i];
    }

    float mean = sum / N_MEAS;
    float variance = (sum_sq - N_MEAS * mean * mean) / (N_MEAS - 1);
    
    compute_confidence_interval(misure);
}


/* ------------------------------------------------------------------ */
/*  NativeSymbol table                                                 */
/* ------------------------------------------------------------------ */
static NativeSymbol native_symbols[] = {
    { "run_benchmark",   run_benchmark_init,   "()",  NULL },
};

bool register_native_functions(void)
{
    int n = sizeof(native_symbols) / sizeof(native_symbols[0]);
    if (!wasm_runtime_register_natives("env", native_symbols, n)) {
        return false;
    }
    LOG_INF("Registered %d native symbols", n);
    return true;
}
