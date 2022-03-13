#!/bin/bash

gcc src/*.c src/api/*.c src/lib/stb/*.c \
    -g \
    -std=gnu11 -fno-strict-aliasing \
    -Isrc -Ilua/src \
    -lm -lSDL2 -Llua/src -llua -ldl \
    -o lite
