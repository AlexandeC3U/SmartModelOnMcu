 #include <stdio.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "wasm_export.h"
 #include "bh_platform.h"
 #include "esp_log.h"
 #include <pthread.h>
 #include <assert.h>
 #include <string.h>
 
 #define LOG_TAG "wamr"
 
 /* Native `abort` function */
static void native_abort(wasm_exec_env_t exec_env, int32_t code) {
    (void)exec_env;
    ESP_LOGE(LOG_TAG, "WASM abort(%d)", (int)code);
    abort();
}

static NativeSymbol env_natives[] = {
    { "abort", native_abort, "(i)", NULL }
};

/* Embedded WASM from: target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/test.wasm" BINARY) */
extern const uint8_t _binary_test_wasm_start[];
extern const uint8_t _binary_test_wasm_end[];
 
 /* Custom allocator wrappers */
static void *wamr_malloc(size_t size) {
    return heap_caps_aligned_alloc(8, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
static void *wamr_realloc(void *ptr, size_t size) {
    return heap_caps_realloc(ptr, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
static void wamr_free(void *ptr) {
    heap_caps_free(ptr);
}

static void * app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;
 
    wasm_application_execute_main(module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        printf("%s\n", exception);
    return NULL;
}
 
 void * iwasm_main(void *arg)
 {
     (void)arg; /* unused */
     /* setup variables for instantiating and running the wasm module */
     uint8_t *wasm_file_buf = NULL;
     unsigned wasm_file_buf_size = 0;
     wasm_module_t wasm_module = NULL;
     wasm_module_inst_t wasm_module_inst = NULL;
     char error_buf[128];
     void *ret;
     RuntimeInitArgs init_args;
 
     /* configure memory allocation */
     memset(&init_args, 0, sizeof(RuntimeInitArgs));
     init_args.mem_alloc_type = Alloc_With_Allocator;
     init_args.mem_alloc_option.allocator.malloc_func = wamr_malloc;
     init_args.mem_alloc_option.allocator.realloc_func = wamr_realloc;
     init_args.mem_alloc_option.allocator.free_func = wamr_free;
 
     ESP_LOGI(LOG_TAG, "Initialize WASM runtime");
     /* initialize runtime environment */
     if (!wasm_runtime_full_init(&init_args)) {
         ESP_LOGE(LOG_TAG, "Init runtime failed.");
         return NULL;
     }
 
     ESP_LOGI(LOG_TAG, "Run wamr with interpreter");
 
     const uint8_t *wasm_file_from_flash = _binary_test_wasm_start;
     wasm_file_buf_size = (uint32_t)(_binary_test_wasm_end - _binary_test_wasm_start);

     wasm_file_buf = wamr_malloc(wasm_file_buf_size);
     if (!wasm_file_buf) {
         ESP_LOGE(LOG_TAG, "Failed to allocate buffer for WASM file");
         goto fail1interp;
     }
     memcpy(wasm_file_buf, wasm_file_from_flash, wasm_file_buf_size);

     /* load WASM module */
     wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_buf_size,
                                     error_buf, sizeof(error_buf));

     /* The buffer can be freed now */
     wamr_free(wasm_file_buf);

     if (!wasm_module) {
         ESP_LOGE(LOG_TAG, "Error in wasm_runtime_load: %s", error_buf);
         goto fail1interp;
     }

     /* For WASI app, I need to register natives and set args */
     wasm_runtime_register_natives("env", env_natives, sizeof(env_natives)/sizeof(NativeSymbol));
     wasm_runtime_set_wasi_args(wasm_module, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

     ESP_LOGI(LOG_TAG, "Instantiate WASM runtime");
     if (!(wasm_module_inst =
               wasm_runtime_instantiate(wasm_module, 8 * 1024, // stack size
                                        8 * 1024,              // heap size
                                        error_buf, sizeof(error_buf)))) {
         ESP_LOGE(LOG_TAG, "Error while instantiating: %s", error_buf);
         goto fail2interp;
     }
 
     ESP_LOGI(LOG_TAG, "run main() of the application");
     ret = app_instance_main(wasm_module_inst);
     assert(!ret);
 
     /* destroy the module instance */
     ESP_LOGI(LOG_TAG, "Deinstantiate WASM runtime");
     wasm_runtime_deinstantiate(wasm_module_inst);
 
 fail2interp:
     /* unload the module */
     ESP_LOGI(LOG_TAG, "Unload WASM module");
     wasm_runtime_unload(wasm_module);
 
 fail1interp:
     /* destroy runtime environment */
     ESP_LOGI(LOG_TAG, "Destroy WASM runtime");
     wasm_runtime_destroy();
 
     return NULL;
 }
 
 void app_main(void)
 {
     pthread_t t;
     int res;
 
     pthread_attr_t tattr;
     pthread_attr_init(&tattr);
     pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
     pthread_attr_setstacksize(&tattr, 8192);
 
     res = pthread_create(&t, &tattr, iwasm_main, (void *)NULL);
     assert(res == 0);
 
     res = pthread_join(t, NULL);
     assert(res == 0);
 
     ESP_LOGI(LOG_TAG, "Exiting...");
 }