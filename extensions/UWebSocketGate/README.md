# UWebSocketGate

## Общее описание
UWebSocketGate это сервис, который позволяет подключаться через websocket и получать события
об изменнии датчиков, а так же изменять состояние (см. Команды).

Подключение к websocket-у доступно по адресу:
```
ws://host:port/wsgate/
```

Помимо этого UWebSocketGate работает в режиме мониторинга изменений датчиков.
Для этого достаточно зайти на страничку по адресу:
```
http://host:port/wsgate/?s1,s2,s3,s4
```

## Конфигурирование
Для конфигурования необходимо создать секцию вида:
```
<UWebSocketGate name="UWebSocketGate" .../>
```

Количество создаваемых websocket-ов можно ограничить при помощи параметра
maxWebsockets (--prefix-max-conn).

## Технические детали
Вся релизация построена на "однопоточном" eventloop. Если датчики долго не меняются,
то периодически посылается "ping" сообщение.

## HTTP API
UWebSocketGate поднимает собственный HTTP сервер и поддерживает стандартный набор
эндпоинтов uniset HTTP API:

- `GET /api/v2/list` - список доступных объектов (в ответе один элемент: ObjectName).
- `GET /api/v2/<ObjectName>/help` - описание API объекта.
- `GET /api/v2/<ObjectName>/status` - состояние объекта и список websocket-сессий.
- `GET /api/v2/<ObjectName>/list` - короткий список (аналог `/list` для объекта).

В ответах `/api/v2/<ObjectName>/...` рядом с `objectType` возвращается
`extensionType: "UWebSocketGate"`. В `status` также присутствует информация о LogServer
(`logserver`).

Пример ответа `/api/v2/<ObjectName>/status`:
```
{
  "object": {
    "id": 5001,
    "name": "UWebSocketGate",
    "objectType": "Object",
    "extensionType": "UWebSocketGate",
    "logserver": {
      "state": "OK",
      "info": "localhost:5005"
    }
  },
  "websockets": [
    {
      "remote": "127.0.0.1:53422",
      "state": "active",
      "subscribed": 3
    }
  ]
}
```

Пример ответа `/api/v2/<ObjectName>/list`:
```
{
  "data": [
    {
      "id": 5001,
      "name": "UWebSocketGate",
      "objectType": "Object",
      "extensionType": "UWebSocketGate"
    }
  ]
}
```

Пример ответа `/api/v2/list`:
```
{
  "data": [
    "UWebSocketGate"
  ]
}
```

Пример ответа `/api/v2/<ObjectName>/help`:
```
{
  "object": {
    "id": 5001,
    "name": "UWebSocketGate",
    "objectType": "Object",
    "extensionType": "UWebSocketGate"
  },
  "help": [
    "/api/v2/<ObjectName>/help",
    "/api/v2/<ObjectName>/status",
    "/api/v2/<ObjectName>/list"
  ]
}
```

### Параметры HTTP сервера
Основные параметры командной строки (префикс по умолчанию `--ws-`):

- `--ws-host ip` - IP, на котором слушает HTTP сервер. По умолчанию: `localhost`.
- `--ws-port num` - порт, на котором принимать запросы. По умолчанию: `8081`.
- `--ws-max-queued num` - размер очереди запросов к HTTP серверу. По умолчанию: `100`.
- `--ws-max-threads num` - разрешённое количество потоков для HTTP сервера. По умолчанию: `3`.
- `--ws-cors-allow addr` - CORS заголовок `Access-Control-Allow-Origin`. По умолчанию: `*`.

### Параметры WebSocket
Основные параметры командной строки (префикс по умолчанию `--ws-`):

- `--ws-max-conn num` - максимальное количество одновременных подключений. По умолчанию: `50`.
- `--ws-heartbeat-time msec` - период сердцебиения. По умолчанию: `3000` мсек.
- `--ws-send-time msec` - период посылки сообщений. По умолчанию: `200` мсек.
- `--ws-max-send num` - максимум сообщений за одну посылку. По умолчанию: `5000`.
- `--ws-max-cmd num` - максимум команд за один цикл обработки. По умолчанию: `200`.
- `--ws-pong-timeout msec` - таймаут ожидания pong. По умолчанию: `5000` мсек.
- `--ws-max-lifetime msec` - максимальное время жизни соединения. `0` = без ограничений.

### Команды WebSocket: примеры
Подключение:
```
ws://host:port/wsgate/
```

Примеры команд (строка, отправляется в сокет):
```
ask:10,20,AI_AS
get:10,20,AI_AS
set:10=1,20=0
freeze:10=1
unfreeze:10
del:10,20
```

### Примеры ответов WebSocket
Ping:
```
{
  "data": [
    {"type": "Ping"}
  ]
}
```

Ошибка (например, слишком длинная команда):
```
{
  "data": [
    {"type": "Error", "message": "Payload to big"}
  ]
}
```

Короткий ответ (get):
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 63,
      "error": "",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

Уведомление об изменении (sensorInfo):
```
{
  "data": [
    {
      "type": "SensorInfo",
      "tv_nsec": 343079769,
      "tv_sec": 1614521238,
      "value": 63,
      "sm_tv_nsec": 976745544,
      "sm_tv_sec": 1614520060,
      "sm_type": "AI",
      "error": "",
      "id": 10,
      "name": "AI_AS",
      "node": 3000,
      "supplier_id": 5003,
      "supplier": "TestProc",
      "undefined": false,
      "calibration": {
        "cmax": 0,
        "cmin": 0,
        "precision": 3,
        "rmax": 0,
        "rmin": 0
      }
    }
  ]
}
```

Пример подтверждения установки (set/freeze):
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 1,
      "error": "",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

Пример ошибки по конкретному датчику:
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 0,
      "error": "Access denied",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

Пример ответа на `unfreeze`:
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 0,
      "error": "",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

Пример ответа на `del` (успешное удаление подписки):
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 0,
      "error": "",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

Пример ошибки для `ask`:
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "id": 10,
      "value": 0,
      "error": "Unknown sensor",
      "supplier_id": 5003,
      "supplier": "TestProc"
    }
  ]
}
```

### Подписки
После команды `ask:` сервер начинает присылать `SensorInfo` при изменениях датчика.
Команда `del:` прекращает отправку уведомлений для указанных датчиков.

## Сообщения
Общий формат сообщений:
```
{
  "data": [
    {
      "type": "SensorInfo",
      ...
    },
    {
      "type": "SensorInfo",
      ...
    },
    {
      "type": "OtherType",
      ...
    },
    {
      "type": "YetAnotherType",
      ...
    },
  ]
}
```

Example:
```
{
  "data": [
    {
      "type": "SensorInfo",
      "tv_nsec": 343079769,
      "tv_sec": 1614521238,
      "value": 63
      "sm_tv_nsec": 976745544,
      "sm_tv_sec": 1614520060,
      "sm_type": "AI",
      "error": "",
      "id": 10,
      "name": "AI_AS",
      "node": 3000,
      "supplier_id": 5003,
      "supplier": "TestProc",
      "undefined": false,
      "calibration": {
        "cmax": 0,
        "cmin": 0,
        "precision": 3,
        "rmax": 0,
        "rmin": 0
      },
    }
  ]
}
```

### Get
Функция get возвращает результат в укороченном формате:
```
{
  "data": [
    {
      "type": "ShortSensorInfo",
      "value": 63
      "error": "",
      "id": 10,
    }
  ]
}
```

### Ping
Для того, чтобы соединение не закрывалось при отсутствии данных, каждые ping_sec посылается
специальное сообщение:
```
{
  "data": [
    {"type": "Ping"}
  ]
}
```

По умолчанию каждый 3 секунды, но время можно задавать параметром "wsHeartbeatTime" (msec)
или аргументом командной строки:
```
--prefix-ws-heartbeat-time msec
```

## Команды
Через websocket можно посылать команды.
На текущий момент формат команды строковый.
Т.е. для подачи команды, необходимо послать просто строку.
Поддерживаются следующие команды:

- "set:id1=val1,id2=val2,name3=val4,..." - выставить значение датчиков
- "ask:id1,id2,name3,..." - подписаться на уведомления об изменении датчиков (sensorInfo)
- "del:id1,id2,name3,..." - отказаться от уведомления об изменении датчиков
- "get:id1,id2,name3,..." - получить текущее значение датчиков (разовое сообщение ShortSensorInfo)
- "freeze:id1=val1,id2=val2,name3=val4,..." - выставить значение и заморозить изменение датчиков
- "unfreeze:id1,id2,name3..." - разморозить изменения датчиков

Если длина команды превышает допустимое значение, то возвращается ошибка:
```
{
  "data": [
    {"type": "Error", "message": "Payload to big"}
  ]
}
```

Под хранение сообщений для отправки выделяется Kbuf*maxSend. Kbuf в текущей реализации равен 10.
Т.е. если настроено maxSend=5000 сообщений, то буфер сможет максимально хранить 50000 сообщений.
