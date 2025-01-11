#!/bin/bash

rm -rf build
mkdir build
cd build
cmake ..
make
cp ./bin/imgui ../
cd ..
