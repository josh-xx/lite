#!/bin/bash

gcc src/*.c src/api/*.c src/lib/stb/*.c \
    -g \
    -std=gnu11 -fno-strict-aliasing \
    -Isrc -Ilua/src -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lm -lSDL2 -Llua/src -llua -ldl \
    -o lite
