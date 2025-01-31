#!/bin/bash

glslangValidator -V custom_model.vert -o custom_model.vert.spv
glslangValidator -V custom_model.frag -o custom_model.frag.spv
