#!/bin/bash

last_mtime=0

while true; do
    latest=$(find src include -name "*.c" -o -name "*.h" 2>/dev/null | xargs ls -rt 2>/dev/null | tail -1)
    
    if [ -n "$latest" ]; then
        current_mtime=$(stat -c %Y "$latest" 2>/dev/null || stat -f %m "$latest" 2>/dev/null)
        
        if [ "$current_mtime" -gt "$last_mtime" ]; then
            echo "--- Detected change: $latest ---"
            last_mtime="$current_mtime"
            make test
            echo ""
        fi
    fi
    
    sleep 2
done
