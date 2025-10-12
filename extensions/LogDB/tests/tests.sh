#!/bin/sh

set -u # Запрет использования необъявленных переменных

trap '' HUP INT QUIT PIPE TERM

RET=0
LOGSERVER_PID=
LOGDB_PID=
dbfile="logdb-tests.db"
downloadfile="download.db.gz"
http_host="localhost"
http_port=8888
LOGFILE="/tmp/uniset-test.log"
ws_url="http://$http_host:$http_port/ws/connect/logserver1"
# WS_KEY=$(openssl rand -base64 16 | tr -d '\n')
WS_KEY="67+diN4x1mlKAw9fNm4vFQ=="

# shellcheck disable=SC2317
atexit() {
	trap - EXIT

	# Более безопасное завершение процессов
	if [ -n "$LOGSERVER_PID" ]; then
		kill "$LOGSERVER_PID" 2>/dev/null
		sleep 2
		kill -0 "$LOGSERVER_PID" 2>/dev/null && kill -9 "$LOGSERVER_PID" 2>/dev/null
	fi

	if [ -n "$LOGDB_PID" ]; then
		kill "$LOGDB_PID" 2>/dev/null
		sleep 2
		kill -0 "$LOGDB_PID" 2>/dev/null && kill -9 "$LOGDB_PID" 2>/dev/null
	fi

	# Очистка файлов
	[ -e "$dbfile" ] && rm -f "${dbfile}"*
	[ -e "$downloadfile" ] && rm -f "$downloadfile"
	[ -e "$LOGFILE" ] && rm -f "$LOGFILE"
	[ -e "/tmp/websocket_output.txt" ] && rm -f "/tmp/websocket_output.txt"

	exit $RET
}

trap atexit EXIT

create_test_db() {
	if ! ./uniset2-logdb-adm create -f "$dbfile"; then
		echo "ERROR: Failed to create test database"
		return 1
	fi
	echo "✓ Test database created successfully"
	return 0
}

logdb_run_logserver() {
	./uniset2-test-logserver -i localhost -p 3333 -d 500 1>/dev/null 2>/dev/null &
	LOGSERVER_PID=$!

	# Проверяем, что процесс запустился
	sleep 1
	if ! kill -0 "$LOGSERVER_PID" 2>/dev/null; then
		echo "ERROR: Logserver failed to start"
		return 1
	fi

	echo "✓ Logserver started successfully (PID: $LOGSERVER_PID)"
	return 0
}

logdb_run() {
	./uniset2-logdb --logdb-single-confile logdb-tests-conf.xml \
		--logdb-dbfile "$dbfile" \
		--logdb-db-buffer-size 5 \
		--logdb-httpserver-host "$http_host" \
		--logdb-httpserver-port $http_port \
		--logdb-ls-check-connection-sec 1 \
		--logdb-db-max-records 20000 \
		--logdb-httpserver-download-enable &

	LOGDB_PID=$!

	# Проверяем, что процесс запустился
	sleep 2
	if ! kill -0 "$LOGDB_PID" 2>/dev/null; then
		echo "ERROR: LogDB failed to start"
		return 1
	fi

	echo "✓ LogDB started successfully (PID: $LOGDB_PID)"
	return 0
}

logdb_error() {
	printf "%20s: ERROR: %s\n" "$1" "$2" >&2
}

# ------------------------------------------------------------------------------------------
# shellcheck disable=SC3043
wait_for_service() {
	local max_attempts=30
	local attempt=1

	echo "Waiting for service to become available..."
	while [ $attempt -le $max_attempts ]; do
		if curl -s "http://$http_host:$http_port/api/v01/logdb/count" >/dev/null 2>&1; then
			echo "✓ Service is now available after $attempt seconds"
			return 0
		fi
		sleep 1
		attempt=$((attempt + 1))
	done

	logdb_error "wait_for_service" "Service did not become available after $max_attempts seconds"
	return 1
}

logdb_test_count() {
	CNT=$(echo 'SELECT count(*) from logs;' | sqlite3 "$dbfile" 2>/dev/null)

	if [ "$CNT" != "0" ] && [ -n "$CNT" ]; then
		echo "✓ Database count test passed: $CNT records found"
		return 0
	fi

	logdb_error "test_count" "count of logs should be > 0 (got: '$CNT')"
	return 1
}

logdb_test_http_count() {
	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logdb/count?logserver1"); then
		logdb_error "test_http_count" "curl request failed"
		return 1
	fi

	if echo "$REQ" | grep -q '"count":'; then
		echo "✓ HTTP /count API test passed"
		return 0
	fi

	logdb_error "test_http_count" "get count of records fail. Response: $REQ"
	return 1
}

logdb_test_http_list() {
	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logdb/list"); then
		logdb_error "test_http_list" "curl request failed"
		return 1
	fi

	if echo "$REQ" | grep -q 'logserver1'; then
		echo "✓ HTTP /list API test passed"
		return 0
	fi

	logdb_error "test_http_list" "get list must contain 'logserver1'. Response: $REQ"
	return 1
}

logdb_test_logfile() {
	if [ -f "$LOGFILE" ]; then
		echo "✓ Log file test passed: $LOGFILE exists"
		return 0
	fi

	logdb_error "test_logfile" "not found logfile: $LOGFILE"
	return 1
}

logdb_test_http_download() {
	if ! curl -s --fail --max-time 30 -o "$downloadfile" --request GET "http://$http_host:$http_port/api/v01/logdb/download"; then
		logdb_error "test_http_download" "curl download failed"
		return 1
	fi

	if [ -s "$downloadfile" ]; then # Проверяем что файл не пустой
		echo "✓ HTTP /download test passed: file downloaded successfully ($(wc -c <"$downloadfile") bytes)"
		return 0
	fi

	logdb_error "test_http_download" "download file is empty or not found: $downloadfile"
	return 1
}

# ------------------------------------------------------------------------------------------
# WebSocket тесты с использованием curl
# ------------------------------------------------------------------------------------------

# Тест WebSocket handshake
logdb_test_websocket_handshake() {
	# Пытаемся выполнить WebSocket handshake
	RESPONSE=$(curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time 10 \
		"$ws_url" 2>/dev/null)

	# Проверяем успешный ответ
	if echo "$RESPONSE" | grep -q "101 Switching Protocols"; then
		echo "✓ WebSocket handshake successful"
		return 0
	fi

	logdb_error "test_websocket_handshake" "WebSocket handshake failed - no 101 Switching Protocols"
	echo "Response: $RESPONSE"
	return 1
}

# Тест получения данных через WebSocket с поиском строк 'INFO'
# shellcheck disable=SC3043
logdb_test_websocket_data() {
	local timeout=15
	local info_count=0
	local output_file="/tmp/websocket_output.txt"

	# Запускаем curl в фоне для получения данных
	curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time $timeout \
		"$ws_url" >"$output_file" 2>&1 &

	local curl_pid=$!

	# Даем время на установление соединения и получение данных
	sleep 8

	# Проверяем, что процесс еще работает
	if ! kill -0 $curl_pid 2>/dev/null; then
		logdb_error "test_websocket_data" "WebSocket connection terminated prematurely"
		rm -f "$output_file"
		return 1
	fi

	# Ждем завершения или таймаута
	wait $curl_pid 2>/dev/null

	# Проверяем полученные данные
	if [ -s "$output_file" ]; then
		# Считаем количество строк с 'INFO'
		info_count=$(grep -o -w "INFO" "$output_file" | wc -l 2>/dev/null || echo "0")

		if [ "$info_count" -ge 3 ]; then
			echo "✓ WebSocket data test passed: found $info_count INFO messages"
			rm -f "$output_file"
			return 0
		else
			logdb_error "test_websocket_data" "Not enough INFO messages found: $info_count (expected at least 3)"
			echo "Raw output (first 10 lines):"
			head -10 "$output_file"
			rm -f "$output_file"
			return 1
		fi
	else
		logdb_error "test_websocket_data" "No data received from WebSocket"
		rm -f "$output_file"
		return 1
	fi
}

# Простой тест доступности WebSocket endpoint
logdb_test_websocket_endpoint() {
	# Просто проверяем, что endpoint отвечает
	if curl -s -I --max-time 10 "$ws_url" >/dev/null 2>&1; then
		echo "✓ WebSocket endpoint is accessible"
		return 0
	else
		logdb_error "test_websocket_endpoint" "WebSocket endpoint is not accessible"
		return 1
	fi
}

# ------------------------------------------------------------------------------------------
logdb_run_all_tests() {
	echo "Starting tests..."

	# Очистка перед началом
	rm -f "$LOGFILE" "${dbfile}"* "$downloadfile" "/tmp/websocket_output.txt" 2>/dev/null

	if ! create_test_db; then
		return 1
	fi

	if ! logdb_run_logserver; then
		return 1
	fi

	sleep 3

	if ! logdb_run; then
		return 1
	fi

	echo "Waiting for service to start..."
	if ! wait_for_service; then
		return 1
	fi

	sleep 2

	# =========== TESTS ============
	echo "Running tests..."

	logdb_test_count || RET=1
	logdb_test_http_count || RET=1
	logdb_test_http_list || RET=1
	logdb_test_logfile || RET=1
	logdb_test_http_download || RET=1

	# WebSocket тесты
	echo ""
	echo "Running WebSocket tests..."
	logdb_test_websocket_endpoint || RET=1
	logdb_test_websocket_handshake || RET=1
	logdb_test_websocket_data || RET=1

	# ==== finished ===
	if [ $RET -eq 0 ]; then
		echo "✓ All tests completed successfully!"
	else
		echo "✗ Some tests failed (exit code: $RET)"
	fi
	return $RET
}

# RUN
if ! logdb_run_all_tests; then
	echo "Test suite execution failed" >&2
	exit 1
fi

exit $RET
