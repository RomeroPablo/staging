#!/bin/bash

#rm -rf build
#mkdir build
rm imgui.ini
cd build
cmake ..
bear -- make
cp bin/core ../bin/
cd ..
./bin/core &
