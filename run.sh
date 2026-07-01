#!/bin/bash

case "$1" in
    test)
        make clean && make test
        ;;
    shell)
        make clean && make && ./shell.exe
        ;;
    web)
        make clean && make web && ./moss-web.exe
        ;;
    *)
        echo "Usage: $0 {test|shell|web}"
        exit 1
        ;;
esac