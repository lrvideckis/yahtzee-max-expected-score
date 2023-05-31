#!/bin/bash

g++ \
    -std=c++17 \
    -O2 \
    -Wall \
    -Wextra \
    -Wunused \
    -Wconversion \
    -Wpedantic \
    -Wshadow \
    -Wlogical-op \
    -Wformat=2 \
    -Wfloat-equal \
    -Wcast-qual \
    -Wcast-align \
    -Wshift-overflow=2 \
    -Wduplicated-cond \
    -Werror \
    -fsanitize=address,undefined \
    -fno-sanitize-recover=all \
    -fstack-protector \
    -D_GLIBCXX_DEBUG \
    -D_GLIBCXX_SANITIZE_VECTOR \
    -D_GLIBCXX_DEBUG_PEDANTIC \
    -D_GLIBCXX_ASSERTIONS \
    -D_FORTIFY_SOURCE=2 \
    yahtzee.cpp -o yahtzee.out
