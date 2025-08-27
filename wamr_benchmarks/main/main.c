// main.c
#include "bench_harness.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void bench_native_mm(void);  // in alg_native.c
void bench_wasm_mm(void);    // in wamr_host.c

#define NATIVE_STACK_BYTES  (6 * 1024)   // tune if needed
#define WAMR_STACK_BYTES    (8 * 1024) 

static void *native_bench_thread(void *arg) {
    (void)arg;
    bench_native_mm();

    UBaseType_t min_free_words = uxTaskGetStackHighWaterMark(NULL);
    size_t used_bytes = NATIVE_STACK_BYTES - (size_t)min_free_words * sizeof(StackType_t);
    printf("{\"name\":\"stack_peak\",\"mode\":\"native\",\"bytes\":%u}\n", (unsigned)used_bytes);
    return NULL;
}

static void *wasm_bench_thread(void *arg) {
    (void)arg;
    bench_wasm_mm(); 
    return NULL;
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(200));
    printf("=== ESP: native vs WAMR ===\n");

    // --- Native in its own pthread ---
    pthread_t tn;
    pthread_attr_t an;
    pthread_attr_init(&an);
    pthread_attr_setdetachstate(&an, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&an, NATIVE_STACK_BYTES);
    int rc = pthread_create(&tn, &an, native_bench_thread, NULL);
    assert(rc == 0);
    pthread_join(tn, NULL);
    pthread_attr_destroy(&an);

    // --- WAMR in its own pthread (required by WAMR’s espidf port) ---
    pthread_t tw;
    pthread_attr_t aw;
    pthread_attr_init(&aw);
    pthread_attr_setdetachstate(&aw, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&aw, WAMR_STACK_BYTES);
    rc = pthread_create(&tw, &aw, wasm_bench_thread, NULL);
    assert(rc == 0);
    pthread_join(tw, NULL);
    pthread_attr_destroy(&aw);

    printf("=== done ===\n");
    fflush(stdout);
}
