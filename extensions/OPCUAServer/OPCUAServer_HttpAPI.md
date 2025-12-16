@page page_OPCUAServer_HTTP OPCUAServer HTTP API

# OPCUAServer HTTP API

- [help](#sec_opcua_http_api_help)
- [status](#sec_opcua_http_api_status)
- [params](#sec_opcua_http_api_params)
- [sensors](#sec_opcua_http_api_sensors)
- [get](#sec_opcua_http_api_get)

Базовый URL: `/api/v2/<object>/`

## /help {#sec_opcua_http_api_help}

Список доступных команд и краткое описание.

```
GET /<object>/help
```

## /status {#sec_opcua_http_api_status}

Текущее состояние OPC UA сервера.

```
GET /api/v2/OPCUAServer1/status
```

Пример ответа:

```json
{
  "result": "OK",
  "status": {
    "endpoint": {
      "name": "uniset2 OPC UA gate",
      "url": "urn:uniset2.server"
    },
    "httpEnabledSetParams": 0,
    "maxSessionTimeout": 5000,
    "name": "OPCUAServer1",
    "params": { "updateTime_msec": 100 },
    "variables": { "count": 10, "write": 5, "methods": 2 }
  }
}
```

## /getparam и /setparam {#sec_opcua_http_api_params}

Чтение/изменение runtime‑параметров.

Поддерживаемые параметры:
- `updateTime_msec` (ms)

Чтение:

```
GET /api/v2/OPCUAServer1/getparam?name=updateTime_msec
```

Ответ:

```json
{
  "result": "OK",
  "params": { "updateTime_msec": 300000 }
}
```

Изменение:

```
GET /api/v2/OPCUAServer1/setparam?updateTime_msec=6000
```

Ответ:

```json
{
  "result": "OK",
  "updated": { "updateTime_msec": 6000 }
}
```

Ограничения: `/setparam` может быть заблокирован (`httpEnabledSetParams`).

Коды ошибок:
- 400/5xx — некорректные значения (например, `updateTime_msec="abc"`).
- 400 — пустой `/setparam` (нет key=value) или `/getparam` (нет `name`).

## /sensors {#sec_opcua_http_api_sensors}

Список переменных OPC UA с пагинацией/фильтрами.

```
GET /api/v2/OPCUAServer1/sensors?limit=50&offset=0&search=Temp&iotype=AI
GET /api/v2/OPCUAServer1/sensors?filter=1001,TempSensor
```

Параметры: `limit` (default 50, 0 = все), `offset`, `search` (подстрока), `iotype` (AI|AO|DI|DO), `filter` (ID/имена).

Ответ: `sensors[]` c полями `id`, `name`, `iotype`, `value`, `vtype`, опционально `mask`/`precision`; также `total`, `limit`, `offset`.

## /get {#sec_opcua_http_api_get}

Чтение выбранных переменных.

```
GET /api/v2/OPCUAServer1/get?name=TempSensor
GET /api/v2/OPCUAServer1/get?id=1001&name=TempSensor2
```

Параметры: `name` или `id` (можно повторять, значение может быть списком `a,b,c`).

Ответ: `sensors[]` с полями `name`, `id`, `iotype`, `value`; для отсутствующих — `error`.
