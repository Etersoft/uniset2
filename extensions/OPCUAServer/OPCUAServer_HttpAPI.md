@page page_OPCUAServer_HTTP OPCUAServer HTTP API

# OPCUAServer HTTP API

- [help](#sec_opcua_http_api_help)
- [status](#sec_opcua_http_api_status)
- [params](#sec_opcua_http_api_params)

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
    "params": {
      "updateTime_msec": 100
    }
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
