# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

WAMR_DIR=${PWD}/../../..

CXX="$WASI_SDK_PATH/bin/clang++"
# SYSROOT="$WASI_SDK_PATH/share/wasi-sysroot"

# Common C/C++ flags for size
CFLAGS="
    -Oz -flto -ffunction-sections -fdata-sections -fvisibility=hidden
    -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics
    -DTF_LITE_STATIC_MEMORY -DNDEBUG -DTF_LITE_STRIP_ERROR_STRINGS -g0
"
LDFLAGS="
    -z stack-size=2048
    -Wl,--initial-memory=65536
    -Wl,--max-memory=65536
    -Wl,--export=__main_argc_argv
    -Wl,--gc-sections
    -Wl,--strip-all,--no-entry
    -lm
"

# Includes
INCLUDES=(
  -I../src
  -I../src/include
  -I../lib/managed_components/espressif__esp-tflite-micro
  -I../lib/managed_components/espressif__esp-tflite-micro/tensorflow
  -I../lib/managed_components/espressif__esp-tflite-micro/third_party/flatbuffers/include
  -I../lib/managed_components/espressif__esp-tflite-micro/third_party/gemmlowp
  -I../lib/managed_components/espressif__esp-tflite-micro/third_party/ruy
)

# App sources
APP_SRCS=(
  ../src/main.cpp
  ../src/model_c_array.cc
)

# Minimal TFLM runtime + kernels needed for MNIST model
TFLM_SRCS=(
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_interpreter.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_interpreter_graph.cc       
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_interpreter_context.cc    
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_allocator.cc                                 
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/memory_helpers.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_utils.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/tflite_bridge/micro_error_reporter.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_time.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_context.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_op_resolver.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_profiler.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/recording_micro_allocator.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/system_setup.cc

  
  # Arena + planners 
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/memory_planner/greedy_memory_planner.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/memory_planner/linear_memory_planner.cc

  # Core TFLite bits
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/core/c/common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/core/api/flatbuffer_conversions.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/core/api/tensor_utils.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/compiler/mlir/lite/schema/schema_utils.cc            
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/internal/common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/internal/quantization_util.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/internal/portable_tensor_utils.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/internal/tensor_ctypes.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/kernel_util.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/kernel_util.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/kernels/internal/reference/portable_tensor_utils.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/tflite_bridge/flatbuffer_conversions_bridge.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_resource_variable.cc

  # Only the kernels the model needs
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/conv.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/conv_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/fully_connected.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/fully_connected_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/softmax.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/softmax_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/depthwise_conv.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/depthwise_conv_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/reduce.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/reduce_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/reshape.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/kernels/reshape_common.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/micro_allocation_info.cc
  ../lib/managed_components/espressif__esp-tflite-micro/tensorflow/lite/micro/flatbuffer_utils.cc
)

echo "Building TFLM WASM app..."
"$CXX" $CFLAGS "${INCLUDES[@]}" \
  -o test.wasm \
  "${APP_SRCS[@]}" \
  "${TFLM_SRCS[@]}" \
  $LDFLAGS

# echo "Generating test_wasm.h..."
# ./build/binarydump -o ../main/test_wasm.h -n wasm_test_file_interp test.wasm
echo "Done."
