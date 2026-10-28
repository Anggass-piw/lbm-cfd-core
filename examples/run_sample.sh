#!/usr/bin/env sh
set -eu

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/lbm_demo 32 20 20 100 lbm_output.vtk
