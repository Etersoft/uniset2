#!/bin/bash
# Демон, который живёт долго, но НИКОГДА не создаёт ready-marker.
# Используется для воспроизведения сценария: launcher запускает процесс,
# ждёт его готовности через readyCheck, ловит timeout, kill'ит,
# перезапускает — и так бесконечно.
NAME=${1:-unready}
echo "$NAME: started (PID $$), will never become ready"
trap 'echo "$NAME: caught signal, exiting"; exit 0' SIGTERM SIGINT

while true; do
    sleep 10
done
