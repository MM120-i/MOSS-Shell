#!/bin/bash

last_mtime=0

while true; do
    latest=$(find src include -name "*.c" -o -name "*.h" 2>/dev/null | xargs ls -rt 2>/dev/null | tail -1)
    
    if [ -n "$latest" ]; then
        current_mtime=$(stat -c %Y "$latest" 2>/dev/null || stat -f %m "$latest" 2>/dev/null)
        
        if [ "$current_mtime" -gt "$last_mtime" ]; then
            echo "--- Detected change: $latest ---"
            last_mtime="$current_mtime"
            
            output=$(make test 2>&1)
            
            if echo "$output" | grep -q "FAILED"; then
                failed=$(echo "$output" | grep "FAILED" | head -1)
                echo "X FAILED: $failed"
            else
                total=0
                while IFS= read -r line; do
                    count=$(echo "$line" | sed 's/.*\[\s*PASSED\s*\]\s*\([0-9]*\).*/\1/')
                    total=$((total + count))
                done < <(echo "$output" | grep "\[  PASSED  \]")
                echo "All tests passed ($total tests)"
            fi
            echo ""
        fi
    fi
    
    sleep 1
done
