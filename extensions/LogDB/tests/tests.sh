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
WS_URL="http://$http_host:$http_port/ws/connect/logserver1"

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
	[ -e "/tmp/websocket_pong_timeout.txt" ] && rm -f "/tmp/websocket_pong_timeout.txt"
	[ -e "/tmp/websocket_lifetime.txt" ] && rm -f "/tmp/websocket_lifetime.txt"
	[ -e "/tmp/websocket_reconnect.txt" ] && rm -f "/tmp/websocket_reconnect.txt"

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
		--logdb-httpserver-download-enable \
		--logdb-httpserver-logcontrol-enable \
		--logdb-ws-pong-timeout 5000 \
		--logdb-ws-max-lifetime 15000 \
		--logdb-ws-heartbeat-time 2000 &

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
	printf "✗ %20s: ERROR: %s\n" "$1" "$2" >&2
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
		echo "✓ HTTP API /count test passed"
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
		echo "✓ HTTP API /list test passed"
		return 0
	fi

	logdb_error "test_http_list" "get list must contain 'logserver1'. Response: $REQ"
	return 1
}

logdb_test_http_logcontrol_set() {
	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logcontrol/logserver1?set=crit"); then
		logdb_error "test_http_set" "curl request failed"
		return 1
	fi

	sleep 10

	REQ=$(echo 'SELECT text from logs order by id desc limit 30;' | sqlite3 "$dbfile" 2>/dev/null)
	CNT=$(echo "$REQ" | grep -io -w "crit" | wc -l 2>/dev/null || echo "0")

	if [ "$CNT" -ge 1 ]; then
		echo "✓ HTTP API /logcontol/set test passed ($CNT CRIT messages)"
		return 0
	fi

	logdb_error "test_http_set" "Not enough CRIT messages found: $CNT (expected at least 1)"
	echo "Raw output (first 5 lines):"
	echo "$REQ" | head -5
	return 1
}

logdb_test_http_logcontrol_reset() {
	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logcontrol/logserver1?reset"); then
		logdb_error "test_http_reset" "curl request failed"
		return 1
	fi

	sleep 5

	REQ=$(echo 'SELECT text from logs order by id desc limit 30;' | sqlite3 "$dbfile" 2>/dev/null)
	CNT=$(echo "$REQ" | grep -io -w "init" | wc -l 2>/dev/null || echo "0")

	if [ "$CNT" -ge 1 ]; then
		echo "✓ HTTP API /logcontol/reset test passed ($CNT init messages)"
		return 0
	fi

	logdb_error "test_http_reset" "Not enough init messages found: $CNT (expected at least 1)"
	echo "Raw output (first 5 lines):"
	echo "$REQ" | head -5
	return 1
}

logdb_test_http_logcontrol_get() {
	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logcontrol/logserver1?get"); then
		logdb_error "test_http_get" "curl request failed"
		return 1
	fi

	if echo "$REQ" | grep 'cmd' | grep -q 'init'; then
		echo "✓ HTTP API /logcontol/get test passed"
		return 0
	fi

	logdb_error "test_http_get" "Request failed. REQ: $REQ"
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
		echo "✓ HTTP API /download test passed: file downloaded successfully ($(wc -c <"$downloadfile") bytes)"
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
		"$WS_URL" 2>/dev/null)

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

	if ! REQ=$(curl -s --fail --max-time 10 --request GET "http://$http_host:$http_port/api/v01/logcontrol/logserver1?set=info"); then
		logdb_error "test_websocket_data" "curl request failed"
		return 1
	fi

	# Запускаем curl в фоне для получения данных
	curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time $timeout \
		"$WS_URL" >"$output_file" 2>&1 &

	local curl_pid=$!

	# Даем время на установление соединения и получение данных
	sleep 5

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
	if curl -s -I --max-time 10 "$WS_URL" >/dev/null 2>&1; then
		echo "✓ WebSocket endpoint is accessible"
		return 0
	else
		logdb_error "test_websocket_endpoint" "WebSocket endpoint is not accessible"
		return 1
	fi
}

# ------------------------------------------------------------------------------------------
# Тест переподключения к логсерверу после потери связи
logdb_test_reconnect() {
	local before after

	before=$(echo 'SELECT count(*) from logs;' | sqlite3 "$dbfile" 2>/dev/null)
	[ -z "$before" ] && before=0

	# Гасим логсервер
	if [ -n "$LOGSERVER_PID" ]; then
		kill "$LOGSERVER_PID" 2>/dev/null
		sleep 1
		kill -0 "$LOGSERVER_PID" 2>/dev/null && kill -9 "$LOGSERVER_PID" 2>/dev/null
	fi

	# Даем LogDB заметить обрыв
	sleep 2

	# Стартуем новый логсервер
	if ! logdb_run_logserver; then
		logdb_error "test_reconnect" "failed to restart logserver"
		return 1
	fi

	# Ждем переподключения и накопления логов
	sleep 6

	after=$(echo 'SELECT count(*) from logs;' | sqlite3 "$dbfile" 2>/dev/null)
	[ -z "$after" ] && after=0

	if [ "$after" -gt "$before" ]; then
		echo "✓ Reconnect test passed: count $before -> $after"
		return 0
	fi

	logdb_error "test_reconnect" "log count did not increase after restart (before=$before, after=$after)"
	return 1
}

# Тест pool exhaustion protection: проверяем, что после закрытия соединений
# новые клиенты могут подключиться (соединения корректно удаляются из пула)
# shellcheck disable=SC3043
logdb_test_websocket_pool_cleanup() {
	local output_file="/tmp/websocket_pool_test.txt"
	local i

	# Запускаем несколько соединений и закрываем их
	for i in 1 2 3; do
		curl -i -N -s \
			-H "Connection: Upgrade" \
			-H "Upgrade: websocket" \
			-H "Sec-WebSocket-Version: 13" \
			-H "Sec-WebSocket-Key: $WS_KEY" \
			--max-time 3 \
			"$WS_URL" >/dev/null 2>&1 &
	done

	# Ждем завершения
	sleep 5

	# Теперь пробуем новое соединение - оно должно успешно установиться
	# (если cleanup работает, слоты освободились)
	RESPONSE=$(curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time 5 \
		"$WS_URL" 2>/dev/null)

	if echo "$RESPONSE" | grep -q "101 Switching Protocols"; then
		echo "✓ WebSocket pool cleanup test passed: new connection accepted after old ones closed"
		return 0
	fi

	logdb_error "test_websocket_pool_cleanup" "New connection failed after closing old ones"
	echo "Response: $RESPONSE"
	return 1
}

# Тест max lifetime: проверяем что соединение закрывается по истечении времени жизни
# shellcheck disable=SC3043
logdb_test_websocket_max_lifetime() {
	local output_file="/tmp/websocket_lifetime.txt"

	# Запускаем клиента
	{
		curl -i -N -s \
			-H "Connection: Upgrade" \
			-H "Upgrade: websocket" \
			-H "Sec-WebSocket-Version: 13" \
			-H "Sec-WebSocket-Key: $WS_KEY" \
			--max-time 25 \
			"$WS_URL" >"$output_file" 2>&1
	} &
	local curl_pid=$!

	# Ждем установления соединения
	sleep 2

	# Проверяем handshake
	if ! grep -q "101 Switching Protocols" "$output_file" 2>/dev/null; then
		logdb_error "test_websocket_max_lifetime" "WebSocket handshake failed"
		kill $curl_pid 2>/dev/null
		rm -f "$output_file"
		return 1
	fi

	# Ждем истечения max lifetime (сервер запущен с --logdb-ws-max-lifetime 15000)
	sleep 18

	# Проверяем, что curl процесс завершился
	if kill -0 $curl_pid 2>/dev/null; then
		kill $curl_pid 2>/dev/null
		logdb_error "test_websocket_max_lifetime" "Connection was not closed after max lifetime"
		rm -f "$output_file"
		return 1
	fi

	echo "✓ WebSocket max lifetime test passed: server closed connection after timeout"
	rm -f "$output_file"
	return 0
}

# Тест переподключения WebSocket: закрываем соединение на клиенте и убеждаемся, что новая попытка проходит
# shellcheck disable=SC3043
logdb_test_websocket_reconnect() {
	local output_file="/tmp/websocket_reconnect.txt"
	local timeout=12

	# Первый коннект
	curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time 4 \
		"$WS_URL" >"$output_file" 2>&1

	# Имитируем разрыв — просто даем сокету закрыться
	sleep 1

	# Вторая попытка (эмулирует автопереподключение фронтенда)
	curl -i -N -s \
		-H "Connection: Upgrade" \
		-H "Upgrade: websocket" \
		-H "Sec-WebSocket-Version: 13" \
		-H "Sec-WebSocket-Key: $WS_KEY" \
		--max-time $timeout \
		"$WS_URL" >>"$output_file" 2>&1

	if grep -q "101 Switching Protocols" "$output_file"; then
		echo "✓ WebSocket reconnect test passed"
		rm -f "$output_file"
		return 0
	fi

	logdb_error "test_websocket_reconnect" "Reconnect failed (no 101 Switching Protocols)"
	head -20 "$output_file"
	rm -f "$output_file"
	return 1
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

	sleep 4

	# =========== TESTS ============
	echo "Running tests..."

	logdb_test_count || RET=1
	logdb_test_logfile || RET=1
	logdb_test_http_count || RET=1
	logdb_test_http_list || RET=1
	logdb_test_http_download || RET=1
	logdb_test_http_logcontrol_set || RET=1
	logdb_test_http_logcontrol_reset || RET=1
	logdb_test_http_logcontrol_get || RET=1
	logdb_test_reconnect || RET=1

	# WebSocket тесты
	echo ""
	echo "Running WebSocket tests..."
	logdb_test_websocket_endpoint || RET=1
	logdb_test_websocket_handshake || RET=1
	logdb_test_websocket_data || RET=1
	logdb_test_websocket_reconnect || RET=1

	# WebSocket timeout тесты (занимают больше времени)
	echo ""
	echo "Running WebSocket timeout tests..."
	logdb_test_websocket_pool_cleanup || RET=1
	logdb_test_websocket_max_lifetime || RET=1

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
