# LogicProcessor - Логический процессор

Логический процессор работает по принципу PLC-контроллеров, выполняя бесконечный цикл:
опрос входов, шаг алгоритма, выставление выходов. Логика формируется из простых элементов:
AND, OR, NOT, Delay, A2D, SEL_R, RS.

Схема соединений задаётся XML-файлом. Существуют два варианта процесса:
- **PassiveLProcessor** (`uniset2-plogicproc`) - основной, работает по заказу датчиков через SharedMemory
- **LProcessor** (`uniset2-logicproc`) - активный, опрашивает датчики по таймеру

## Конфигурирование

Пример схемы (`schema.xml`):
```xml
<Schema>
  <elements>
    <item id="1" type="OR" inCount="2"/>
    <item id="2" type="AND" inCount="2"/>
    <item id="3" type="Delay" inCount="1" delayMS="3000"/>
    <item id="4" type="A2D" filterValue="6"/>
  </elements>
  <connections>
    <item type="ext" from="Input1_S" to="1" toInput="1"/>
    <item type="ext" from="Input2_S" to="1" toInput="2"/>
    <item type="int" from="1" to="2" toInput="1"/>
    <item type="int" from="2" to="3" toInput="1"/>
    <item type="out" from="3" to="Output_S"/>
  </connections>
</Schema>
```

Типы соединений:
- `type="ext"` - внешний вход (от датчика к элементу)
- `type="int"` - внутреннее соединение между элементами
- `type="out"` - выход на датчик

## HTTP API

PassiveLProcessor предоставляет HTTP API для получения текущего состояния схемы.
API доступен по базовому пути `/api/v2/{ObjectName}/` (требует сборку без `DISABLE_REST_API`).

### Endpoints

| Путь | Описание |
|------|----------|
| `schema` | Сводка: имя файла схемы, количество элементов/входов/выходов/связей |
| `schema/elements` | Все элементы с текущими значениями выходов |
| `schema/inputs` | Внешние входы с привязкой к датчикам |
| `schema/outputs` | Внешние выходы с привязкой к датчикам |
| `schema/connections` | Внутренние связи между элементами |

### GET schema

Сводная информация о загруженной схеме.

```json
{
  "schemaFile": "schema.xml",
  "elementCount": 7,
  "inputCount": 7,
  "outputCount": 2,
  "connectionCount": 5,
  "sleepTime": 100
}
```

### GET schema/elements

Список всех элементов схемы с текущим состоянием.

```json
{
  "elements": [
    { "id": "1", "type": "OR",    "out": 0, "inCount": 2, "outCount": 1 },
    { "id": "2", "type": "AND",   "out": 1, "inCount": 2, "outCount": 1 },
    { "id": "3", "type": "Delay", "out": 0, "inCount": 1, "outCount": 1 },
    { "id": "4", "type": "A2D",   "out": 0, "inCount": 1, "outCount": 1 }
  ]
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| `id` | string | Идентификатор элемента |
| `type` | string | Тип: OR, AND, NOT, Delay, A2D, SEL_R, RS |
| `out` | long | Текущее значение выхода |
| `inCount` | int | Количество входов |
| `outCount` | int | Количество выходов |

### GET schema/inputs

Внешние входы - привязка датчиков к входам элементов.

```json
{
  "inputs": [
    { "sid": 100, "value": 1, "elementId": "1", "numInput": 1 },
    { "sid": 101, "value": 0, "elementId": "1", "numInput": 2 }
  ]
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| `sid` | long | ObjectId датчика |
| `value` | long | Текущее значение датчика |
| `elementId` | string | ID целевого элемента |
| `numInput` | int | Номер входа элемента |

### GET schema/outputs

Внешние выходы - привязка элементов к выходным датчикам.

```json
{
  "outputs": [
    { "sid": 200, "elementId": "3", "outputValue": 0 },
    { "sid": 201, "elementId": "4", "outputValue": 42 }
  ]
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| `sid` | long | ObjectId выходного датчика |
| `elementId` | string | ID элемента-источника |
| `outputValue` | long | Текущее значение выхода элемента |

### GET schema/connections

Внутренние связи между элементами.

```json
{
  "connections": [
    { "from": "1", "to": "2", "toInput": 1 },
    { "from": "2", "to": "3", "toInput": 1 }
  ]
}
```

| Поле | Тип | Описание |
|------|-----|----------|
| `from` | string | ID элемента-источника |
| `to` | string | ID элемента-приёмника |
| `toInput` | int | Номер входа приёмника |

### Потокобезопасность

HTTP API возвращает "best-effort snapshot" текущего состояния. Значения элементов (`out`, `value`, `outputValue`)
читаются без блокировок и могут быть неконсистентны между собой в рамках одного ответа.
