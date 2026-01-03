# syntax=docker/dockerfile:1
# ============================================
# Стадия 1: Базовый образ с зависимостями
# ============================================
FROM alt:sisyphus AS base

# Смена репозитория на yandex (быстрее)
RUN apt-get update && apt-get -y install epm && epm repo change yandex

# Зависимости для сборки UniSet2 (из .github/workflows/testsuite.yml)
RUN apt-get update && apt-get -y install \
    git-core gcc-c++ catch2-devel \
    autoconf automake libtool make ccache \
    libsqlite3-devel libxml2-devel libsigc++2-devel \
    libpoco-devel libev-devel libomniORB-devel \
    xsltproc libomniORB-names libomniORB-idl libomniORB-utils \
    quickjs-devel quickjs-devel-static \
    libopen62541-devel libopen62541pp-devel \
    # Утилиты для тестов
    curl procps nmap netcat \
    && apt-get clean && rm -rf /var/cache/apt/*

# ============================================
# Стадия 2: Сборка проекта
# ============================================
FROM base AS builder

# Настройка ccache
ENV CC="ccache gcc" CXX="ccache g++"
ENV CCACHE_DIR=/ccache

WORKDIR /build

# Копируем все исходники (используется .dockerignore для исключений)
COPY . .

# Генерация configure и сборка
# Сборка для e2e тестов (SM, UNet, Modbus, OPC UA, JScript)
# --mount=type=cache монтирует кэш ccache между сборками
RUN --mount=type=cache,target=/ccache \
    ./autogen.sh --prefix=/usr \
       --disable-mysql --disable-pgsql --disable-rrd \
       --disable-io --disable-python --disable-docs \
       --disable-clickhouse --disable-opentsdb \
       --disable-mqtt --disable-netdata --disable-tests \
    && make -j$(nproc) \
    && make DESTDIR=/install install

# ============================================
# Стадия 3: Runtime образ
# ============================================
FROM base AS runtime

# Копируем собранные файлы
COPY --from=builder /install/usr /usr

# Копируем скрипты e2e
COPY testsuite/e2e/scripts/ /scripts/
RUN chmod +x /scripts/*.sh

# Рабочая директория для конфигов
WORKDIR /app

# Точка входа
ENTRYPOINT ["/scripts/entrypoint.sh"]
