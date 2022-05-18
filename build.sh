#!/usr/bin/env bash

mkdir build
cd build
cmake .. -GNinja
ninja
./Open4X
