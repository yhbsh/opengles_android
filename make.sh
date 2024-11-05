#!/bin/bash

TARGETS=("3d" "activity" "audio" "camera" "cube" "game" "list" "triangle" "video")

mkdir -p build
for target in "${TARGETS[@]}"; do
    echo "Building $target..."
    make -C "apps/$target" > /dev/null
    cp apps/$target/$target.apk build/
done
