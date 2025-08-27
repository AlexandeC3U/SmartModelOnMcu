idf_component_register(
  SRCS
    "tflite_infer_main.cpp"
    "model_c_array.cc"
  INCLUDE_DIRS
    "."
    "include"
  PRIV_REQUIRES
    espressif__esp-tflite-micro
  REQUIRES
    idf::espressif__esp-tflite-micro
)
set(TFLITE_MICRO_USE_DEFAULT_EXECUTION_CONTEXT ON)
set(TFLITE_MICRO_KERNELS "conv max_pool fully_connected softmax reshape")
list(APPEND EXTRA_COMPONENT_DIRS "lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels")