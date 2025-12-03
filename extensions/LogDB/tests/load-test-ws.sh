#!/bin/sh
#
# Нагрузочный тест для проверки проблемы с pool exhaustion
# Открывает много WebSocket соединений и проверяет:
# 1. Что 429 возвращается на правильном количестве соединений
# 2. Что после закрытия соединений новые могут подключиться
#

set -u

trap '' HUP INT QUIT PIPE TERM

RET=0
LOGSERVER_PID=
LOGDB_PID=
dbfile="load-test.db"
http_host="localhost"
http_port=8889
MAX_THREADS=10
WS_KEY="67+diN4x1mlKAw9fNm4vFQ=="

CURL_PIDS=""

# shellcheck disable=SC2317
atexit() {
    trap - EXIT

    echo "Cleaning up..."

    # Убиваем все curl процессы
    if [ -n "$CURL_PIDS" ]; then
        for pid in $CURL_PIDS; do
            kill "$pid" 2>/dev/null
        done
        sleep 1
        for pid in $CURL_PIDS; do
            kill -9 "$pid" 2>/dev/null
        done
    fi

    if [ -n "$LOGSERVER_PID" ]; then
        kill "$LOGSERVER_PID" 2>/dev/null
        sleep 1
        kill -9 "$LOGSERVER_PID" 2>/dev/null
    fi

    if [ -n "$LOGDB_PID" ]; then
        kill "$LOGDB_PID" 2>/dev/null
        sleep 1
        kill -9 "$LOGDB_PID" 2>/dev/null
    fi

    rm -f "$dbfile"* /tmp/ws_load_*.txt 2>/dev/null
    exit $RET
}

trap atexit EXIT

create_test_db() {
    if ! ./uniset2-logdb-adm create -f "$dbfile"; then
        echo "ERROR: Failed to create test database"
        return 1
    fi
    return 0
}

run_logserver() {
    # Порт должен совпадать с конфигом logdb-tests-conf.xml (port=3333)
    ./uniset2-test-logserver -i localhost -p 3333 -d 100 1>/dev/null 2>/dev/null &
    LOGSERVER_PID=$!
    sleep 1
    if ! kill -0 "$LOGSERVER_PID" 2>/dev/null; then
        echo "ERROR: Logserver failed to start"
        return 1
    fi
    echo "Logserver started (PID: $LOGSERVER_PID)"
    return 0
}

run_logdb() {
    # Важно: pong timeout + heartbeat = время обнаружения мёртвого соединения
    # heartbeat 1s + pong timeout 3s = ~4 секунды для обнаружения
    ./uniset2-logdb --logdb-single-confile logdb-tests-conf.xml \
        --logdb-dbfile "$dbfile" \
        --logdb-db-buffer-size 100 \
        --logdb-httpserver-host "$http_host" \
        --logdb-httpserver-port $http_port \
        --logdb-httpserver-max-threads $MAX_THREADS \
        --logdb-ls-check-connection-sec 1 \
        --logdb-ws-max 100 \
        --logdb-ws-heartbeat-time 1000 \
        --logdb-ws-pong-timeout 3000 &

    LOGDB_PID=$!
    sleep 2
    if ! kill -0 "$LOGDB_PID" 2>/dev/null; then
        echo "ERROR: LogDB failed to start"
        return 1
    fi
    echo "LogDB started (PID: $LOGDB_PID) with max-threads=$MAX_THREADS"
    return 0
}

wait_for_service() {
    local max_attempts=20
    local attempt=1

    echo "Waiting for service..."
    while [ $attempt -le $max_attempts ]; do
        if curl -s "http://$http_host:$http_port/api/v01/logdb/count" >/dev/null 2>&1; then
            echo "Service is ready"
            return 0
        fi
        sleep 1
        attempt=$((attempt + 1))
    done
    echo "ERROR: Service did not become available"
    return 1
}

# Открывает WebSocket соединение в фоне, возвращает PID
open_ws_connection() {
    local idx=$1
    local output="/tmp/ws_load_$idx.txt"

    curl -i -N -s \
        -H "Connection: Upgrade" \
        -H "Upgrade: websocket" \
        -H "Sec-WebSocket-Version: 13" \
        -H "Sec-WebSocket-Key: $WS_KEY" \
        --max-time 60 \
        "http://$http_host:$http_port/ws/connect/logserver1" >"$output" 2>&1 &

    echo $!
}

# Проверяет результат WebSocket соединения
check_ws_result() {
    local idx=$1
    local output="/tmp/ws_load_$idx.txt"

    if [ -f "$output" ]; then
        if grep -q "101 Switching Protocols" "$output" 2>/dev/null; then
            echo "success"
        elif grep -q "429\|Too Many\|SERVICE_UNAVAILABLE" "$output" 2>/dev/null; then
            echo "rejected"
        else
            echo "unknown"
        fi
    else
        echo "no_output"
    fi
}

get_status() {
    curl -s "http://$http_host:$http_port/api/v01/logdb/status" 2>/dev/null
}

# ============================================================================
# ТЕСТЫ
# ============================================================================

test_pool_exhaustion() {
    echo ""
    echo "=== Test: Pool Exhaustion ==="
    echo "Opening $((MAX_THREADS + 5)) WebSocket connections (max threads = $MAX_THREADS)..."

    local total=$((MAX_THREADS + 5))
    local pids=""
    local i=1

    # Открываем соединения
    while [ $i -le $total ]; do
        pid=$(open_ws_connection $i)
        pids="$pids $pid"
        CURL_PIDS="$CURL_PIDS $pid"
        i=$((i + 1))
        # Небольшая пауза чтобы соединения устанавливались последовательно
        sleep 0.2
    done

    # Даём время на установление соединений
    sleep 3

    # Проверяем статус
    echo ""
    echo "Status after opening connections:"
    STATUS=$(get_status)
    echo "$STATUS" | grep -E "http_active_requests|websockets_active" || echo "Full status: $STATUS"

    # Считаем успешные и отклонённые
    local success=0
    local rejected=0
    local unknown=0
    i=1

    while [ $i -le $total ]; do
        result=$(check_ws_result $i)
        case "$result" in
            success) success=$((success + 1)) ;;
            rejected) rejected=$((rejected + 1)) ;;
            *) unknown=$((unknown + 1)) ;;
        esac
        i=$((i + 1))
    done

    echo ""
    echo "Results: success=$success, rejected=$rejected, unknown=$unknown"

    # Проверяем что успешных примерно MAX_THREADS (с небольшой погрешностью)
    local min_expected=$((MAX_THREADS - 2))
    local max_expected=$((MAX_THREADS + 1))

    if [ $success -ge $min_expected ] && [ $success -le $max_expected ]; then
        echo "PASS: Pool exhaustion works correctly (success=$success, expected ~$MAX_THREADS)"
    else
        echo "FAIL: Expected ~$MAX_THREADS successful connections, got $success"
        RET=1
    fi

    # Убиваем curl процессы
    for pid in $pids; do
        kill "$pid" 2>/dev/null
    done
    sleep 1
    for pid in $pids; do
        kill -9 "$pid" 2>/dev/null
    done

    # Проверяем что curl процессы действительно завершены
    echo "Checking for remaining curl processes..."
    CURL_COUNT=$(pgrep -c curl 2>/dev/null || echo "0")
    echo "Remaining curl processes: $CURL_COUNT"

    # Проверяем TCP соединения на порту
    echo "Active TCP connections to port $http_port:"
    ss -tn state established "( dport = :$http_port or sport = :$http_port )" 2>/dev/null | head -5 || true

    # Ждём пока сервер обнаружит закрытые соединения через pong timeout
    # heartbeat 1s + pong timeout 3s + buffer = 10 секунд
    echo "Waiting for cleanup (pong timeout)..."
    sleep 10

    echo "Status after cleanup:"
    STATUS=$(get_status)
    echo "$STATUS" | grep -E "http_active_requests|websockets_active" || echo "Full status: $STATUS"

    # Очищаем файлы
    rm -f /tmp/ws_load_*.txt
    CURL_PIDS=""

    return $RET
}

test_pool_recovery() {
    echo ""
    echo "=== Test: Pool Recovery ==="
    echo "Opening $MAX_THREADS WebSocket connections, closing them, then opening new ones..."

    local pids=""
    local i=1

    # Открываем MAX_THREADS соединений
    while [ $i -le $MAX_THREADS ]; do
        pid=$(open_ws_connection $i)
        pids="$pids $pid"
        CURL_PIDS="$CURL_PIDS $pid"
        i=$((i + 1))
        sleep 0.2
    done

    sleep 2
    echo "Status with $MAX_THREADS connections:"
    STATUS=$(get_status)
    echo "$STATUS" | grep -E "http_active_requests|websockets_active" || echo "Full status: $STATUS"

    # Закрываем все соединения
    echo "Closing all connections..."
    for pid in $pids; do
        kill "$pid" 2>/dev/null
    done
    sleep 1
    for pid in $pids; do
        kill -9 "$pid" 2>/dev/null
    done

    # Ждём пока сервер обнаружит закрытые соединения через pong timeout
    # heartbeat 1s + pong timeout 3s + buffer = 10 секунд
    echo "Waiting for server to detect closed connections (pong timeout)..."
    sleep 10

    echo "Status after closing:"
    STATUS=$(get_status)
    echo "$STATUS" | grep -E "http_active_requests|websockets_active" || echo "Full status: $STATUS"

    rm -f /tmp/ws_load_*.txt
    CURL_PIDS=""

    # Пробуем открыть новые соединения
    echo "Opening new connections..."
    pids=""
    i=1

    while [ $i -le 5 ]; do
        pid=$(open_ws_connection $i)
        pids="$pids $pid"
        CURL_PIDS="$CURL_PIDS $pid"
        i=$((i + 1))
        sleep 0.2
    done

    sleep 2

    # Проверяем что новые соединения успешны
    local success=0
    i=1
    while [ $i -le 5 ]; do
        result=$(check_ws_result $i)
        if [ "$result" = "success" ]; then
            success=$((success + 1))
        fi
        i=$((i + 1))
    done

    echo "New connections successful: $success/5"

    if [ $success -ge 4 ]; then
        echo "PASS: Pool recovery works correctly"
    else
        echo "FAIL: Pool did not recover properly"
        RET=1
    fi

    # Cleanup
    for pid in $pids; do
        kill "$pid" 2>/dev/null
    done
    sleep 1
    for pid in $pids; do
        kill -9 "$pid" 2>/dev/null
    done

    # Ждём пока сервер обнаружит закрытые соединения
    echo "Waiting for cleanup..."
    sleep 10

    rm -f /tmp/ws_load_*.txt
    CURL_PIDS=""

    return $RET
}

test_http_during_ws_load() {
    echo ""
    echo "=== Test: HTTP requests during WebSocket load ==="
    echo "Opening $((MAX_THREADS - 2)) WebSocket connections, then making HTTP requests..."

    local ws_count=$((MAX_THREADS - 2))
    local pids=""
    local i=1

    # Открываем ws_count соединений (оставляем 2 потока свободными)
    while [ $i -le $ws_count ]; do
        pid=$(open_ws_connection $i)
        pids="$pids $pid"
        CURL_PIDS="$CURL_PIDS $pid"
        i=$((i + 1))
        sleep 0.2
    done

    sleep 2
    echo "Status with $ws_count WebSocket connections:"
    STATUS=$(get_status)
    echo "$STATUS" | grep -E "http_active_requests|websockets_active" || echo "Full status: $STATUS"

    # Делаем HTTP запросы к /status (мониторинговый эндпоинт с резервом)
    echo "Making HTTP requests to /status (monitoring endpoint)..."
    local http_success=0
    for _ in 1 2 3 4 5; do
        HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "http://$http_host:$http_port/api/v01/logdb/status")
        echo "HTTP response code: $HTTP_CODE"
        if [ "$HTTP_CODE" = "200" ]; then
            http_success=$((http_success + 1))
        fi
        sleep 0.5
    done

    echo "HTTP requests successful: $http_success/5"

    if [ $http_success -ge 4 ]; then
        echo "PASS: HTTP requests work during WebSocket load"
    else
        echo "FAIL: HTTP requests blocked during WebSocket load"
        RET=1
    fi

    # Cleanup
    for pid in $pids; do
        kill "$pid" 2>/dev/null
    done
    sleep 1
    for pid in $pids; do
        kill -9 "$pid" 2>/dev/null
    done
    rm -f /tmp/ws_load_*.txt
    CURL_PIDS=""

    return $RET
}

# ============================================================================
# MAIN
# ============================================================================

echo "WebSocket Pool Load Test"
echo "========================"

if ! create_test_db; then
    exit 1
fi

if ! run_logserver; then
    exit 1
fi

if ! run_logdb; then
    exit 1
fi

if ! wait_for_service; then
    exit 1
fi

sleep 2

# Запускаем тесты
test_pool_exhaustion
test_pool_recovery
test_http_during_ws_load

echo ""
echo "========================"
if [ $RET -eq 0 ]; then
    echo "All tests PASSED"
else
    echo "Some tests FAILED"
fi

exit $RET
