@page page_OPCUAExchange_HTTP OPCUAExchange HTTP API

# OPCUAExchange HTTP API

- [help](#sec_opcuaex_http_api_help)
- [status](#sec_opcuaex_http_api_status)
- [params](#sec_opcuaex_http_api_params)
- [sensors](#sec_opcuaex_http_api_sensors)
- [get](#sec_opcuaex_http_api_get)
- [diagnostics](#sec_opcuaex_http_api_diagnostics)
- [control](#sec_opcuaex_http_api_control)

Базовый URL: `/api/v2/<object>/`

## /help {#sec_opcuaex_http_api_help}

Список доступных команд и краткое описание.

```
GET /<object>/help
```

## /status {#sec_opcuaex_http_api_status}

Текущее состояние OPC UA обмена (аналог `getInfo()` + поля расширения).

```
GET /api/v2/OPCUAExchange1/status
```

Пример:

```json
{
  "result": "OK",
  "status": {
    "name": "OPCUAExchange1",
    "LogServer": { "host": "127.0.0.1", "port": 5510, "state": "RUNNING", "info": { /*...*/ } },
    "channels": [
      { "index": 0, "disabled": 0, "ok": 1, "addr": "opc.tcp://127.0.0.1:4840" },
      { "index": 1, "disabled": 1 }
    ],
    "iolist_size": 32,
    "subscription": { "enabled": 1, "items": 120 },
    "monitor": "vmon: OK",
    "httpEnabledSetParams": 1,
    "httpControlAllow": 1,
    "httpControlActive": 0,
    "errorHistoryMax": 100,
    "errorHistorySize": 3,
    "lastErrors": [
      {
        "time": "2025-12-15T12:00:00",
        "lastSeen": "2025-12-15T12:05:10",
        "channel": 0,
        "operation": "read",
        "statusCode": "BadDisconnect",
        "nodeid": "ns=2;s=Temp",
        "count": 5
      }
    ]
  }
}
```

При отключённой подписке (`enableSubscription=0`) вместо `subscription` возвращаются `read_attributes`/`write_attributes` по «тикам».

## /getparam и /setparam {#sec_opcuaex_http_api_params}

Чтение/изменение runtime‑параметров обмена.

Поддерживаемые параметры: `polltime`, `updatetime`, `reconnectPause`, `timeoutIterate`, `exchangeMode`, `writeToAllChannels`, `currentChannel`, `connectCount`, `activated`, `iolistSize`, `httpControlAllow`, `httpControlActive`, `errorHistoryMax`.

История ошибок агрегирует повторы по ключу (канал, операция, статус, nodeid); для каждой записи возвращаются время первого появления (`time`), последнего (`lastSeen`) и счётчик `count`. Размер истории ограничен `errorHistoryMax`.

Чтение:

```
GET /api/v2/OPCUAExchange1/getparam?name=polltime&name=updatetime&name=reconnectPause&name=timeoutIterate&name=exchangeMode
```

Ответ:

```json
{
  "result": "OK",
  "params": {
    "polltime": 100,
    "updatetime": 500,
    "reconnectPause": 2000,
    "timeoutIterate": 50,
    "exchangeMode": 0
  }
}
```

Изменение:

```
GET /api/v2/OPCUAExchange1/setparam?polltime=200&updatetime=700&reconnectPause=3000&timeoutIterate=60&writeToAllChannels=1
```

Ответ:

```json
{
  "result": "OK",
  "updated": {
    "polltime": 200,
    "updatetime": 700,
    "reconnectPause": 3000,
    "timeoutIterate": 60,
    "writeToAllChannels": 1
  }
}
```

Ограничения:
- `/setparam` может быть заблокирован (`httpEnabledSetParams=0`).
- `exchangeMode` доступен на запись только при `httpControlActive=1` (см. контроль).

Ошибки:
- 400 — пустой запрос (`setparam` без key=value или `getparam` без `name`), некорректные значения.
- 403 — `/setparam` запрещён или попытка задать `exchangeMode` без активного HTTP‑контроля.
- 404/500 — внутренние ошибки.

## /sensors и /sensor {#sec_opcuaex_http_api_sensors}

Список сенсоров:

```
GET /api/v2/OPCUAExchange1/sensors?limit=100&offset=0&search=Temp&iotype=AI
GET /api/v2/OPCUAExchange1/sensors?filter=1000,Temp_AS,1002
```

Параметры: `limit` (default 50, 0 = все), `offset`, `search` (подстрока, case‑insensitive), `filter` (ID/имена, как в IONC `/get`), `iotype` (AI|AO|DI|DO).

Ответ содержит `sensors[]`, `total`, `limit`, `offset`.

Детали сенсора:

```
GET /api/v2/OPCUAExchange1/sensor?id=1000
```

Ответ включает `id`, `name`, `nodeid`, `iotype`, `value`, `tick`, `mask`, `offset`, `vtype`, `precision`, `status`, `channels[]` (статус по каналам).

Если сенсор не найден — HTTP 404 + `{"result":"ERROR","error":"sensor not found","query":{...}}`.

## /get {#sec_opcuaex_http_api_get}

Возвращает указанные датчики по ID/имени, совместимо с IONC `/get`.

```
GET /api/v2/OPCUAExchange1/get?filter=1000,1001,1002
GET /api/v2/OPCUAExchange1/get?filter=Temp_AS,Pressure_AS
GET /api/v2/OPCUAExchange1/get?filter=1000,Temp_AS,1002
```

Параметр: `filter` — список ID/имён через запятую.

Ответ: `sensors[]` с полями `id`, `name`, `iotype`, `value`, `vtype`, `device`, `mbreg`, `amode`, `count` (совместимо с IONC).

## /diagnostics {#sec_opcuaex_http_api_diagnostics}

Диагностика обмена/ошибок. Формат зависит от реализации; обычно включает историю ошибок и агрегированную статистику.

## /control {#sec_opcuaex_http_api_control}

Перехват управления режимом обмена (HTTP‑контроль):
- флаги `httpControlAllow` и `httpControlActive`;
- установка `exchangeMode` разрешена только при активном контроле.
