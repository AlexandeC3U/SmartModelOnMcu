// main/wamr_host.c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "wasm_export.h"
#include "bench_harness.h"

extern const unsigned char alg_wasm_wasm[];
extern const unsigned int  alg_wasm_wasm_len;

static wasm_module_t      module  = NULL;
static wasm_module_inst_t inst    = NULL;
static wasm_exec_env_t    exec_env = NULL;
static uint8_t           *wasm_buf = NULL;

static void wasm_panic(const char *msg) {
    printf("WASM ERROR: %s\n", msg);
    abort();
}

static void wasm_init_or_die(void) {
    RuntimeInitArgs args;
    memset(&args, 0, sizeof(args));

    // Use ESP-IDF's malloc/realloc/free (simple & works well on ESP32)
    args.mem_alloc_type = Alloc_With_System_Allocator;

    if (!wasm_runtime_full_init(&args))
        wasm_panic("runtime init failed");

    // Copy the .wasm bytes from flash to RAM (safer on ESP32)
    wasm_buf = (uint8_t *)malloc(alg_wasm_wasm_len);
    if (!wasm_buf) wasm_panic("malloc wasm_buf failed");
    memcpy(wasm_buf, alg_wasm_wasm, alg_wasm_wasm_len);

    char err[128] = {0};

    module = wasm_runtime_load(wasm_buf, (uint32_t)alg_wasm_wasm_len, err, sizeof(err));
    if (!module) {
        printf("wasm_runtime_load error: %s\n", err);
        wasm_panic("module load failed");
    }

    // Instance-local WASM stack/heap
    const uint32_t wasm_stack = 4 * 1024; 
    const uint32_t wasm_heap  = 8 * 1024; 
    inst = wasm_runtime_instantiate(module, wasm_stack, wasm_heap, err, sizeof(err));
    if (!inst) {
        printf("wasm_runtime_instantiate error: %s\n", err);
        wasm_panic("instantiate failed");
    }

    exec_env = wasm_runtime_create_exec_env(inst, 4 * 1024);
    if (!exec_env)
        wasm_panic("create exec env failed");
}

static void wasm_deinit(void) {
    if (exec_env) { wasm_runtime_destroy_exec_env(exec_env); exec_env = NULL; }
    if (inst)     { wasm_runtime_deinstantiate(inst);        inst     = NULL; }
    if (module)   { wasm_runtime_unload(module);             module   = NULL; }
    if (wasm_buf) { free(wasm_buf);                          wasm_buf = NULL; }
    wasm_runtime_destroy();
}

//calls exported "mm32x32_i16"
static void call_mm32x32_i16(void *ctx) {
    (void)ctx;
    wasm_function_inst_t fn = wasm_runtime_lookup_function(inst, "mm32x32_i16");
    if (!fn) wasm_panic("export 'mm32x32_i16' not found");

    if (!wasm_runtime_call_wasm(exec_env, fn, 0, NULL)) {
        const char *e = wasm_runtime_get_exception(inst);
        printf("WASM exception: %s\n", e ? e : "(none)");
        wasm_panic("call failed");
    }
}

void bench_wasm_mm(void) {
    size_t heap_b = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t larg_b = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    wasm_init_or_die();

    bench_result_t r;
    run_timed(10, call_mm32x32_i16, NULL, &r, "mm32x32_i16", "wasm-fast",
              (size_t)alg_wasm_wasm_len);

    // fill in memory stats around the run
    r.heap_before    = heap_b;
    r.largest_before = larg_b;
    r.heap_after     = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    r.largest_after  = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    print_result(&r);

    wasm_deinit();
}
