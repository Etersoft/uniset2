.PHONY: build test clean logs shell up down

# Сборка образов
build:
	docker compose build

# Запуск тестов (полный цикл)
test: build
	docker compose up --abort-on-container-exit --exit-code-from tester
	docker compose down

# Интерактивный режим (для отладки)
up: build
	docker compose up -d node1 node2 node3 node4 node5
	@echo "Nodes are running. Use 'make shell NODE=node1' to connect"
	@echo "Use 'make logs' to see output, 'make down' to stop"

# Остановка нод
down:
	docker compose down

# Подключение к ноде (make shell NODE=node1)
shell:
	docker compose exec $(NODE) /bin/bash

# Логи всех сервисов
logs:
	docker compose logs -f

# Логи конкретного сервиса
logs-%:
	docker compose logs -f $*

# Очистка образов и volumes
clean:
	docker compose down -v --rmi local

# Пересборка без кэша
rebuild:
	docker compose build --no-cache

# Запуск только базовых нод (node1, node2) для быстрой проверки UNet
test-unet: build
	docker compose up -d node1 node2
	@echo "Waiting for nodes to start..."
	sleep 10
	docker compose exec node1 uniset2-admin --confile /app/configure.xml --setValue TestValue1_AS=12345
	sleep 2
	docker compose exec node2 uniset2-admin --confile /app/configure.xml --getValue TestValue1_AS
	docker compose down

# Статус сервисов
status:
	docker compose ps

# Тест Modbus (Node3=Master, Node4=Slave)
test-modbus: build
	docker compose up -d node4
	@echo "Waiting for MBSlave to start..."
	sleep 5
	docker compose up -d node3
	@echo "Waiting for MBTCPMaster to connect..."
	sleep 10
	docker compose logs node3 node4
	docker compose down

# Тест OPC UA (Node5)
test-opcua: build
	docker compose up -d node5
	@echo "Waiting for OPC UA server to start..."
	sleep 5
	docker compose logs node5
	docker compose down
