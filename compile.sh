#!/bin/bash

rm -rf build
mkdir build
rm imgui.ini
cd build
cmake ..
#cmake -DCMAKE_BUILD_TYPE=Release ..
make
cp bin/core ../bin/
cd ..
./bin/core &
