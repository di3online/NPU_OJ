#!/bin/bash

if [ $# != 1 ]; then
    echo "USAGE: $0 source_file_name"
    exit 50
fi

mv -f $1 main.cc 2>/dev/null
exec g++ main.cc -lm -DONLINE_JUDGE -o main -Wall -pipe -O0 -std=c++0x -g -static
