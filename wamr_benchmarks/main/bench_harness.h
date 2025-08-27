// bench_harness.h
#pragma once
#include <inttypes.h>            
#include <stddef.h>          
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

typedef uint64_t u64;

typedef struct {
    const char *name;
    u64 micros_avg;
    u64 micros_stddev;
    uint32_t cycles_avg;
    size_t heap_before;
    size_t heap_after;
    ssize_t heap_delta;           
    size_t largest_before;
    size_t largest_after;
    UBaseType_t stack_hwmark_words;
    size_t artifact_size;         
    const char *mode;            
} bench_result_t;

static inline u64 now_us(void) { return (u64)esp_timer_get_time(); }

// declare run_timed so alg_native.c sees it
typedef void (*bench_fn_t)(void *);
void run_timed(int iters, bench_fn_t fn, void *ctx,
               bench_result_t *out, const char *name, const char *mode,
               size_t artifact_size_bytes);

// pretty-print one JSON line with correct formats
static inline void print_result(const bench_result_t *r) {
    printf("{\"name\":\"%s\",\"mode\":\"%s\",\"us_avg\":%" PRIu64
           ",\"us_sd\":%" PRIu64 ",\"cycles_avg\":%" PRIu32
           ",\"heap_before\":%zu,\"heap_after\":%zu,\"heap_delta\":%zd"
           ",\"largest_before\":%zu,\"largest_after\":%zu"
           ",\"stack_hwmark_words\":%u,\"artifact_size\":%zu}\n",
           r->name, r->mode, r->micros_avg, r->micros_stddev, r->cycles_avg,
           r->heap_before, r->heap_after, r->heap_delta,
           r->largest_before, r->largest_after,
           (unsigned)r->stack_hwmark_words, r->artifact_size);
}
