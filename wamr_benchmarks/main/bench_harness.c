// bench_harness.c
#include "bench_harness.h"
#include <math.h>

void run_timed(int iters, bench_fn_t fn, void *ctx,
               bench_result_t *out, const char *name, const char *mode,
               size_t artifact_size_bytes)
{
    size_t heap_b = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t larg_b = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Warmup
    for (int i=0;i<3;i++) fn(ctx);

    // Timed runs
    double sum=0.0, sum2=0.0;
    for (int i=0;i<iters;i++) {
        u64 t0 = now_us();
        fn(ctx);
        u64 t1 = now_us();
        double dt = (double)(t1 - t0);
        sum += dt;
        sum2 += dt*dt;
    }
    double mean = sum/iters;
    double var  = (iters>1) ? (sum2/iters - mean*mean) : 0.0;
    if (var < 0) var = 0;
    double sd = sqrt(var);

    size_t heap_a = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t larg_a = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    out->name = name;
    out->mode = mode;
    out->micros_avg = (u64)(mean + 0.5);
    out->micros_stddev = (u64)(sd + 0.5);
    out->cycles_avg = (uint32_t)(mean * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ + 0.5);
    out->heap_before = heap_b;
    out->heap_after = heap_a;
    out->heap_delta = (ssize_t)heap_a - (ssize_t)heap_b;
    out->largest_before = larg_b;
    out->largest_after = larg_a;
    out->stack_hwmark_words = uxTaskGetStackHighWaterMark(NULL);
    out->artifact_size = artifact_size_bytes;
}
