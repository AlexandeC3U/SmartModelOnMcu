# Copyright (C) 2021 Intel Corporation and others.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES wamr)

target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/test.wasm" BINARY)