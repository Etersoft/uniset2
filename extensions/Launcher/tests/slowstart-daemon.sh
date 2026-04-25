#!/bin/bash
# Демон с очень долгим стартом: живёт минуты, никогда не создаёт
# ready-marker. Эмулирует процесс, который "стартует медленно".
NAME=${1:-slowstart}
echo "$NAME: started (PID $$), simulating very long startup"
trap 'echo "$NAME: caught signal, exiting"; exit 0' SIGTERM SIGINT

while true; do
    sleep 60
done
