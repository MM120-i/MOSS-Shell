#!/bin/bash

case "$1" in
    test)
        make clean && make test
        ;;
    shell)
        make clean && make && ./shell.exe
        ;;
    *)
        echo "Usage: $0 {test|shell}"
        exit 1
        ;;
esac