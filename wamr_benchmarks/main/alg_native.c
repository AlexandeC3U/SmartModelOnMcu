// alg_native.c
#include <stdint.h>
#include <string.h>
#include "bench_harness.h"

#define N 32
typedef struct {
    int16_t A[N*N], B[N*N];
    int32_t C[N*N];
} mm_ctx_t;

// simple i16×i16->i32
static void mm32x32_i16(void *p) {
    mm_ctx_t *ctx = (mm_ctx_t*)p;
    for (int i=0;i<N;i++) {
        for (int j=0;j<N;j++) {
            int32_t acc = 0;
            for (int k=0;k<N;k++) {
                acc += (int32_t)ctx->A[i*N+k] * (int32_t)ctx->B[k*N+j];
            }
            ctx->C[i*N+j] = acc;
        }
    }
}

void bench_native_mm(void) {
    static mm_ctx_t ctx;
    for (int i=0;i<N*N;i++) { ctx.A[i] = (i*3+1)&0x7FFF; ctx.B[i] = (i*5+7)&0x7FFF; }
    memset(ctx.C, 0, sizeof(ctx.C));

    bench_result_t r;
    run_timed(10, mm32x32_i16, &ctx, &r, "mm32x32_i16", "native", sizeof(ctx)); // <- report BSS
    print_result(&r);
}
