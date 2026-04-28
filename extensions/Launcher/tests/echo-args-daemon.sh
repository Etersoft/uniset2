#!/bin/bash
echo "echo-args: argc=$#"
i=0
for a in "$@"; do
    echo "  argv[$i]=$a"
    i=$((i+1))
done
sleep 60
