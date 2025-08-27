#include "model_data.h"
#include "include/ground_truth_mnist.h"

#ifdef ESP_PLATFORM
  #include "esp_log.h"
  #include "esp_timer.h"
#else
  #include <cstdio>
  #include <chrono>
  // Minimal logging/timer shims for non-ESP (WASI) build
  #ifndef ESP_LOGI
    #define ESP_LOGI(tag, fmt, ...) std::printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__)
  #endif
  #ifndef ESP_LOGE
    #define ESP_LOGE(tag, fmt, ...) std::printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__)
  #endif
  static inline int64_t esp_timer_get_time() {
    using namespace std::chrono;
    return duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  }
#endif

#include <cstring>
#include <cmath>

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"

static const char* TAG = "TFLM";

// Arena (adjust if AllocateTensors fails)
#ifndef TENSOR_ARENA_SIZE
  #define TENSOR_ARENA_SIZE (16 * 1024)
#endif
alignas(16) static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// One embedded test image
static const int8_t* sample_image = __ground_truth_mnist;

int main(int argc, char** argv)
{
  (void)argc; (void)argv;

  const tflite::Model* model = tflite::GetModel(mnist_qat_model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    ESP_LOGE(TAG, "Model schema mismatch!");
    return 1;
  }

  // Keep only the ops the model really uses
  static tflite::MicroMutableOpResolver<6> resolver;
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddMean();
  resolver.AddReshape();

  static tflite::MicroInterpreter interpreter(
      model, resolver, tensor_arena, TENSOR_ARENA_SIZE, /*error_reporter=*/nullptr);

  if (interpreter.AllocateTensors() != kTfLiteOk) {
    ESP_LOGE(TAG, "AllocateTensors failed. Arena size=%d", (int)TENSOR_ARENA_SIZE);
    return 2;
  }

  TfLiteTensor* input  = interpreter.input(0);
  TfLiteTensor* output = interpreter.output(0);

  // Single inference
  ESP_LOGI(TAG, "Testing single image...");
  std::memcpy(input->data.int8, sample_image, input->bytes);

  int64_t t0 = esp_timer_get_time();
  if (interpreter.Invoke() != kTfLiteOk) {
    ESP_LOGE(TAG, "Inference failed");
    return 3;
  }
  int64_t t1 = esp_timer_get_time();

  output = interpreter.output(0);
  int8_t max_val = -128;
  int    max_idx = -1;
  for (int j = 0; j < output->dims->data[1]; ++j) {
    if (output->data.int8[j] > max_val) {
      max_val = output->data.int8[j];
      max_idx = j;
    }
  }

  ESP_LOGI(TAG, "Predicted digit: %d", max_idx);
  ESP_LOGI(TAG, "Inference time: %lld us", (long long)(t1 - t0));
  return 0;
}
