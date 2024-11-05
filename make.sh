#!/bin/bash

TARGETS=("3d" "activity" "audio" "camera" "cube" "game" "list" "triangle" "video")

for target in "${TARGETS[@]}"; do
    echo "Building $target..."
    make -C "apps/$target"
done
