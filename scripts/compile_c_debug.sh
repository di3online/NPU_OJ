#!/bin/bash

if [ $# != 1 ]; then
    echo "USAGE: $0 source_file_name"
    exit 50
fi

mv -f $1 main.c 2>/dev/null

exec gcc main.c -lm -DONLINE_JUDGE -o main -Wall -g -pipe
