#!/bin/sh

rm -rf build

mkdir build

cd build

if ! cmake ..; then
    echo "Error: cmake failed."
    exit 1
fi

if ! make; then
    echo "Error: make failed."
    exit 1
fi

echo "Build success!"
