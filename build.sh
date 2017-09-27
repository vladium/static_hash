#!/bin/bash

cpp_flags='-std=c++11 -O3 -msse4.2 -g -Wall'

mkdir -p build

for obj in vr/crc32 main test
do
    g++ ${cpp_flags} -Isrc -c -o build/$(basename ${obj}).o src/${obj}.cpp
done

g++ -o "static_hash"  build/*.o

