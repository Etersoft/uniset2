#!/bin/bash
set -e

CONFFILE="${CONFFILE:-/app/configure.xml}"
NODE_NAME="${NODE_NAME:-Node1}"

echo "=== UniSet2 E2E Node: $NODE_NAME ==="
echo "Config: $CONFFILE"

# Подготовка директории для omniNames
mkdir -p /var/log/omninames
rm -f /var/log/omninames/*.dat /var/log/omninames/*.bak 2>/dev/null || true

# Экспорт переменных для child-процессов
export CONFFILE
export NODE_NAME

# Если передана команда - выполнить её напрямую (для tester)
if [ $# -gt 0 ]; then
    echo "Running command: $@"
    exec "$@"
fi

# Функция ожидания порта
wait_for_port() {
    local host=$1
    local port=$2
    local timeout=${3:-30}
    local start=$(date +%s)

    echo "Waiting for $host:$port..."
    while ! nc -z "$host" "$port" 2>/dev/null; do
        local now=$(date +%s)
        if [ $((now - start)) -ge $timeout ]; then
            echo "ERROR: Timeout waiting for $host:$port"
            return 1
        fi
        sleep 0.5
    done
    echo "$host:$port is available"
}

# 1. Запуск omniNames в фоне
echo "Starting omniNames..."
omniNames -start -logdir /var/log/omninames &
OMNI_PID=$!

# Ждём готовности omniNames
wait_for_port localhost 2809 30

# 2. Создание репозитория объектов
echo "Creating object repository..."
uniset2-admin --confile "$CONFFILE" --create

# 3. Запуск uniset2-uno
# Uno запускает все расширения в одном процессе:
#   - SharedMemory
#   - UNetExchange (Node1, Node2)
#   - OPCUAServer (Node5)
#   - и т.д.
echo "Starting uniset2-uno for $NODE_NAME..."
exec uniset2-uno \
    --confile "$CONFFILE" \
    --localNode "$NODE_NAME" \
    --uno-name "$NODE_NAME"
