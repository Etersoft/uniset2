#!/bin/bash
# Процесс, который падает сразу после старта.
# Используется для воспроизведения сценария «бесконечно рестартующий процесс».
NAME=${1:-crash-daemon}
echo "$NAME: started (PID $$), about to crash"
sleep 0.05
echo "$NAME: crashing"
exit 1
