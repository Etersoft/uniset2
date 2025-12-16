@page page_ModbusTCP_HTTP ModbusMaster HTTP API

# ModbusMaster HTTP API

- [help](#sec_mbttp_http_api_help)
- [root](#sec_mbttp_http_api_root)
- [reload](#sec_mbttp_http_api_reload)
- [mode](#sec_mbttp_http_api_mode)
- [params](#sec_mbttp_http_api_params)
- [status](#sec_mbttp_http_api_status)
- [registers](#sec_mbttp_http_api_registers)
- [devices](#sec_mbttp_http_api_devices)
- [get](#sec_mbttp_http_api_get)
- [takeControl / releaseControl](#sec_mbttp_http_api_control)

Базовый URL: `/api/v2/<object>/`

## /help {#sec_mbttp_http_api_help}

Возвращает список доступных команд и параметров.

```
GET /api/v2/<object>/help
```

## / {#sec_mbttp_http_api_root}

Стандартная информация об объекте (включая `extensionType`), плюс секция `LogServer` (если сервер включён).

```json
{
  "object": {
    "id": 1001,
    "name": "MBTCPMaster1",
    "extensionType": "ModbusMaster"
  }
}
```

## /reload {#sec_mbttp_http_api_reload}

Перезагрузка конфигурации. Параметр `confile` — абсолютный путь к альтернативному конфигу (необязателен).

```
GET /api/v2/<object>/reload?confile=/path/to/confile
```

Ответ:

```json
{ "result": "OK", "confile": "/path/to/confile" }
```

> Указанный `confile` должен соответствовать configure.xml (минимум секции процесса и датчиков).

## /mode {#sec_mbttp_http_api_mode}

Управление режимом обмена:
- `?get` — текущий режим
- `?supported=1` — поддерживаемые режимы
- `?set=<mode>` — установка (если не привязан к датчику; иначе вернёт ошибку «control via sensor is enabled…»)

Примеры:

```
GET /api/v2/MBTCPMaster1/mode?get
GET /api/v2/MBTCPMaster1/mode?supported=1
GET /api/v2/MBTCPMaster1/mode?set=writeOnly
```

## /getparam и /setparam {#sec_mbttp_http_api_params}

Чтение/изменение runtime‑параметров. Базовый путь: `/api/v2/<object>/...`.

Параметры:
- `force` (0|1)
- `force_out` (0|1)
- `maxHeartBeat` (ms)
- `recv_timeout` (ms)
- `sleepPause_msec` (ms)
- `polltime` (ms)
- `default_timeout` (ms)

Чтение:

```
GET /api/v2/MBTCPMaster1/getparam?name=force&name=recv_timeout&name=polltime
```

Ответ:

```json
{
  "result": "OK",
  "params": { "force": 0, "recv_timeout": 2000, "polltime": 1000 },
  "unknown": ["..."]
}
```

Изменение:

```
GET /api/v2/MBTCPMaster1/setparam?recv_timeout=3000&polltime=2000
```

Ответ:

```json
{
  "result": "OK",
  "updated": { "recv_timeout": 3000, "polltime": 2000 },
  "unknown": ["..."]
}
```

Правила и ошибки:
- Ключи передаются как `name=value`; пустые запросы дают 400.
- Для bool параметров допустимы строки `0|1|true`.
- Неизвестные имена попадают в `unknown`.
- Некорректные значения → 400/5xx.
- Изменение доступно только если в конфиге/CLI указано `--<prefix>-http-enabled-setparams=1`, иначе вернётся ошибка.

## /status {#sec_mbttp_http_api_status}

Текущее состояние (аналог `getInfo()`):

```
GET /api/v2/MBTCPMaster1/status
```

Ключевые поля: `result`, `status.name`, `monitor`, `activated`, `logserver`, `parameters.config`, статистика (если включена), список устройств, режим (`mode.name/id/control`), `force`, `force_out`, `maxHeartBeat`, `httpControlAllow`, `httpControlActive`, `httpEnabledSetParams`, `config_params` (`recv_timeout`, `sleepPause_msec`, `polltime`, `default_timeout`).

## /registers {#sec_mbttp_http_api_registers}

Список регистров с фильтрацией/пагинацией.

```
GET /api/v2/MBTCPMaster1/registers
GET /api/v2/MBTCPMaster1/registers?offset=0&limit=50
GET /api/v2/MBTCPMaster1/registers?search=Sensor&iotype=AI
GET /api/v2/MBTCPMaster1/registers?filter=1003,Sensor1_AI,1005
```

Параметры: `offset`, `limit`, `search`, `filter`, `iotype` (AI/AO/DI/DO). Фильтры по `addr/regs` не поддерживаются.

Пример ответа:

```json
{
  "result": "OK",
  "devices": {
    "1": { "respond": 1, "dtype": "rtu", "mode": 0, "safeMode": 0 }
  },
  "registers": [
    {
      "id": 1003,
      "name": "Sensor1_AI",
      "iotype": "AI",
      "value": 42,
      "vtype": "signed",
      "device": 1,
      "register": { "mbreg": 10, "mbfunc": 3, "mbval": 1 },
      "nbit": 0,
      "mask": 0,
      "precision": 0
    }
  ],
  "total": 150,
  "count": 50,
  "offset": 0,
  "limit": 50
}
```

## /devices {#sec_mbttp_http_api_devices}

Список устройств (по Modbus-адресам) с их состоянием. Реализация повторяет ModbusMaster API.

```json
{
  "result": "OK",
  "devices": [
    { "addr": 1, "respond": 1, "dtype": "rtu", "regCount": 10, "mode": 0, "safeMode": 0 }
  ],
  "count": 1
}
```

## /get {#sec_mbttp_http_api_get}

Возвращает указанные датчики по ID/имени, совместимо с IONC `/get`.

```
GET /api/v2/MBTCPMaster1/get?filter=1003,Sensor1_AI,1005
```

Ответ: массив `sensors[]` с полями `id`, `name`, `iotype`, `value`; для несуществующих — `{ "name": "<requested>", "error": "not found" }`.

## /takeControl и /releaseControl {#sec_mbttp_http_api_control}

Переключение в режим управления через HTTP (если в конфиге включён `httpControlAllow=1`):

```
GET /api/v2/MBTCPMaster1/takeControl
GET /api/v2/MBTCPMaster1/releaseControl
```

При запрете в конфиге возвращает `"result": "ERROR"`.
