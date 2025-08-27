#include "model_data.h"
#include "include/all_ground_truth_mnist.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"

#include <cstring>
#include <cmath>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "TFLM";

#ifndef TENSOR_ARENA_SIZE
#define TENSOR_ARENA_SIZE (30 * 1024)
#endif
alignas(16) static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// ground truth images have been compiled in a way to work with a specific input format, will need to be recompiled to match different models
// Array of pointers to ground truth images
static const int8_t* all_ground_truth_images[10] = {
    __ground_truth_mnist_0, __ground_truth_mnist_1, __ground_truth_mnist_2,
    __ground_truth_mnist_3, __ground_truth_mnist_4, __ground_truth_mnist_5,
    __ground_truth_mnist_6, __ground_truth_mnist_7, __ground_truth_mnist_8,
    __ground_truth_mnist_9
};

void print_memory_info()
{
  uint32_t free_heap = esp_get_free_heap_size();
  uint32_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  ESP_LOGI("MemoryInfo", "Free internal heap: %u", free_heap);
  ESP_LOGI("MemoryInfo", "Total PSRAM: %u", total_psram);
  ESP_LOGI("MemoryInfo", "Free PSRAM: %u", free_psram);
}

extern "C" void app_main(void) {
    const tflite::Model* model = tflite::GetModel(mnist_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE("TFLM", "Model schema mismatch!");
        return;
    }

    static tflite::MicroMutableOpResolver<8> resolver;
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddMean();
    resolver.AddReshape();

    static tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE, NULL);
    if (interpreter.AllocateTensors() != kTfLiteOk) {
        ESP_LOGE("TFLM", "AllocateTensors failed. Arena size=%d", (int)TENSOR_ARENA_SIZE);
        return;
    }

    ESP_LOGI("TFLM", "Arena used bytes: %d", (int)interpreter.arena_used_bytes());

    #ifdef ESP_NN_SUPPORT
    ESP_LOGI("TFLM","ESP_NN_SUPPORT=1");
    #endif
    #ifdef CONFIG_NN_OPTIMIZED
    ESP_LOGI("TFLM","CONFIG_NN_OPTIMIZED=1");
    #endif
    #ifdef ARCH_ESP32_S3
    ESP_LOGI("TFLM","ARCH_ESP32_S3=1");
    #endif
    
    TfLiteTensor* input = interpreter.input(0);
    TfLiteTensor* output = interpreter.output(0);
    ESP_LOGI("TFLM", "Input type=%d scale=%f zero_point=%d bytes=%d", (int)input->type, input->params.scale, (int)input->params.zero_point, (int)input->bytes);
    ESP_LOGI("TFLM", "Output type=%d scale=%f zero_point=%d dims[0]=%d dims[1]=%d", (int)output->type, output->params.scale, (int)output->params.zero_point, output->dims->data[0], output->dims->data[1]);

    int correct_predictions = 0;
    for (int i = 0; i < 10; ++i) {
        ESP_LOGI(TAG, "Testing digit %d...", i);
        memcpy(input->data.int8, all_ground_truth_images[i], input->bytes);

        // Debug: input tensor statistics for each digit
        int8_t mn = 127, mx = -128; int32_t sum = 0;
        for (int j = 0; j < input->bytes; ++j) {
            int8_t v = input->data.int8[j];
            if (v < mn) mn = v;
            if (v > mx) mx = v;
            sum += v;
        }
        ESP_LOGI(TAG, "Input stats: min=%d max=%d mean=%.2f", (int)mn, (int)mx, sum / (float)input->bytes);

        size_t free_heap_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        int64_t start_time = esp_timer_get_time();

        if (interpreter.Invoke() != kTfLiteOk) {
            ESP_LOGE(TAG, "Inference failed for digit %d", i);
            continue;
        }

        int64_t end_time = esp_timer_get_time();
        size_t free_heap_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        
        output = interpreter.output(0);
        int8_t max_val = -128;
        int max_idx = -1;
        for (int j = 0; j < output->dims->data[1]; j++) {
            if (output->data.int8[j] > max_val) {
                max_val = output->data.int8[j];
                max_idx = j;
            }
        }

        if (max_idx == i) {
            correct_predictions++;
        }

        ESP_LOGI(TAG, "Inference time: %lld us", (end_time - start_time));
        ESP_LOGI(TAG, "Heap used: %d bytes", (int)(free_heap_before - free_heap_after));
        ESP_LOGI(TAG, "Ground Truth: %d, Predicted: %d", i, max_idx);
        ESP_LOGI(TAG, "------------------------------------");
        print_memory_info();  
    }
    ESP_LOGI(TAG, "Final Accuracy: %d / 10", correct_predictions);
} 