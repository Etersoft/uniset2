@page page_ModbusSlave_HTTP ModbusSlave HTTP API

# ModbusSlave HTTP API

- [help](#sec_mbsalve_http_api_help)
- [root](#sec_mbsalve_http_api_root)
- [registers](#sec_mbsalve_http_api_registers)
- [get](#sec_mbsalve_http_api_get)
- [status](#sec_mbsalve_http_api_status)
- [params](#sec_mbsalve_http_api_params)

Базовый URL: `/api/v2/<object>/`

## /help {#sec_mbsalve_http_api_help}

Возвращает список доступных команд и их краткое описание.

```
GET /<object>/help
```

## / {#sec_mbsalve_http_api_root}

Возвращает стандартную информацию об объекте.

```
GET /<object>/
```

## /registers {#sec_mbsalve_http_api_registers}

Возвращает список регистров (датчиков) с пагинацией и фильтрами. Формат совместим с ModbusMaster.

```
GET /api/v2/MBSlave1/registers
GET /api/v2/MBSlave1/registers?offset=0&limit=50
GET /api/v2/MBSlave1/registers?search=Sensor&iotype=AI
GET /api/v2/MBSlave1/registers?addr=1,2&regs=10,20
GET /api/v2/MBSlave1/registers?filter=1003,Sensor1_AI,1005
```

Параметры:
- `offset` — пропустить первые N записей (default: 0)
- `limit` — максимум записей (0 = без ограничений)
- `search` — поиск по имени (подстрока, без учёта регистра)
- `filter` — список ID или имён (смешанный формат, как у IONC `/get`)
- `iotype` — AI | AO | DI | DO
- `addr` — Modbus-адреса через запятую
- `regs` — номера регистров через запятую

Пример ответа:

```json
{
  "result": "OK",
  "devices": { "1": {}, "49": {} },
  "registers": [
    {
      "id": 1003,
      "name": "Sensor1_AI",
      "iotype": "AI",
      "value": 42,
      "vtype": "signed",
      "device": 1,
      "mbreg": 10,
      "amode": "rw"
    }
  ],
  "total": 150,
  "count": 50,
  "offset": 0,
  "limit": 50
}
```

Описание полей: `id`, `name`, `iotype`, `value`, `vtype`, `device`, `mbreg`, `amode`, `total`, `count`, `offset`, `limit`.

## /get {#sec_mbsalve_http_api_get}

Возвращает указанные датчики по ID или именам. Совместим с IONC `/get`.

```
GET /api/v2/MBSlave1/get?filter=1003,1004,1005
GET /api/v2/MBSlave1/get?filter=Sensor1_AI,Sensor2_AO
GET /api/v2/MBSlave1/get?filter=1003,Sensor1_AI,1005
```

Параметры:
- `filter` — список ID или имён через запятую (числа трактуются как ID, остальное — имена).

Пример ответа:

```json
{
  "result": "OK",
  "registers": [
    {
      "id": 1003,
      "name": "Sensor1_AI",
      "iotype": "AI",
      "value": 42,
      "vtype": "signed",
      "device": 1,
      "mbreg": 10,
      "amode": "rw"
    }
  ],
  "count": 1
}
```

## /getparam и /setparam {#sec_mbsalve_http_api_params}

Чтение/изменение runtime‑параметров процесса. Базовый путь: `/api/v2/<object>/...`.

Поддерживаемые параметры:
- `force` (0|1)
- `sockTimeout` (ms)
- `sessTimeout` (ms)
- `updateStatTime` (ms)

Чтение:

```
GET /api/v2/MBSlave1/getparam?name=force&name=sockTimeout&name=sessTimeout&name=updateStatTime
```

Ответ:

```json
{
  "result": "OK",
  "params": {
    "force": 0,
    "sockTimeout": 30000,
    "sessTimeout": 2000,
    "updateStatTime": 4000
  },
  "unknown": ["..."]
}
```

Изменение:

```
GET /api/v2/MBSlave1/setparam?force=1&sockTimeout=45000&sessTimeout=2500&updateStatTime=5000
```

Ответ:

```json
{
  "result": "OK",
  "updated": {
    "force": 1,
    "sockTimeout": 45000,
    "sessTimeout": 2500,
    "updateStatTime": 5000
  },
  "unknown": ["..."]
}
```

Ограничения:
- `/setparam` может быть заблокирован флагом `httpEnabledSetParams`.
- При блокировке `/setparam` возвращает ошибку.

Коды ошибок:
- 400/5xx — некорректные значения (например, `sockTimeout="abc"`).
- 400 — пустой `/setparam` (нет key=value) или `/getparam` (нет `name`).

## /status {#sec_mbsalve_http_api_status}

Возвращает текущее состояние объекта (аналогично `getInfo()`).

```
GET /api/v2/MBSlave1/status
```

Ключевые поля ответа:
- `name`, `tcp.{ip,port}`, `logserver.{host,port}`
- `iomap.{size,map[]}`, `myaddr`
- `stat.{connectionCount,smPingOK,restartTCPServerCount}`
- `tcp_clients[]`, `tcp_sessions.{count,max_sessions,updateStatTime,items[]}`
- `sockTimeout`, `sessTimeout`, `updateStatTime`, `force`
