# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

WAMR_DIR=${PWD}/../../..

echo "Build wasm app .."
$WASI_SDK_PATH/bin/clang -O3 --target=wasm32 -nostdlib -fno-builtin \
  -z stack-size=4096 \
  -Wl,--no-entry \
  -Wl,--export=mm32x32_i16 \
  -Wl,--export=__data_end -Wl,--export=__heap_base \
  -Wl,--initial-memory=65536 -Wl,--max-memory=65536 -Wl,--stack-first \
  -Wl,--strip-all -Wl,--allow-undefined \
  -o alg_wasm.wasm alg_wasm.c

echo "Build binarydump tool .."
# rm -fr build && mkdir build && cd build
# cmake $WAMR_PATH/test-tools/binarydump-tool -G Ninja
# cmake --build .
# cd ..

echo "Generate test_wasm.h .."
./build/binarydump -o ../main/wasm_module_bytes.h -n alg_wasm_wasm alg_wasm.wasm

echo "Done"
