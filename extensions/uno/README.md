# UniSet2 Uno

Комбинированное приложение, запускающее несколько расширений UniSet2 в одном процессе с прямым доступом к SharedMemory.

## Преимущества

- **Без накладных расходов IPC**: Расширения обращаются к SharedMemory напрямую через указатель (не через CORBA)
- **Один процесс**: Один процесс вместо 5-10 отдельных демонов
- **Простое развёртывание**: Один исполняемый файл
- **Меньше памяти**: Разделяемые библиотеки загружаются один раз
- **Удобная отладка**: Все расширения в одном процессе

## Использование

```bash
uniset2-uno --confile configure.xml [опции]
```

### Опции

| Опция | Описание |
|-------|----------|
| `--confile FILE` | Конфигурационный файл (обязательно) |
| `--app-name NAME` | Имя секции UniSetUno в конфиге |
| `--help` | Показать справку |

Плюс все стандартные опции UniSet (`--localNode` и т.д.)

## Конфигурация

Добавьте секцию `<UniSetUno>` в ваш `configure.xml`:

```xml
<UniSetUno name="MyApp">
  <extensions>
    <extension type="SharedMemory" name="SharedMemory"/>
    <extension type="UNetExchange" name="UNetExchange1" prefix="unet"/>
    <extension type="MBTCPMaster" name="MBTCPMaster1" prefix="mbtcp"/>
    <extension type="OPCUAServer" name="OPCUAServer1" prefix="opcua"/>
  </extensions>
</UniSetUno>
```

### Атрибуты расширений

| Атрибут | Описание | Обязательно |
|---------|----------|-------------|
| `type` | Тип расширения (см. ниже) | да |
| `name` | Имя объекта в конфигурации UniSet | да |
| `prefix` | Префикс командной строки для настроек расширения | нет |
| `args` | Дополнительные аргументы командной строки для расширения | нет |

### Атрибут args

Атрибут `args` позволяет передать дополнительные аргументы командной строки конкретному расширению.
Это полезно для задания специфичных параметров, которые не являются общими для всех расширений.

```xml
<extension type="UNetExchange" name="UNetExchange1" prefix="unet"
           args="--unet-filter-field unet --unet-filter-value node1"/>

<extension type="MBTCPMaster" name="MBTCPMaster1"
           args="--mbtcp-poll-time 500 --mbtcp-recv-timeout 100"/>
```

**Примечания:**
- Аргументы разделяются пробелами (как в командной строке)
- UniSetUno автоматически добавляет `--{prefix}-name {name}` для каждого расширения
- Если нужны кавычки в значениях, используйте XML-экранирование: `&quot;`

## Поддерживаемые расширения

| Тип | Описание | Префикс по умолчанию |
|-----|----------|---------------------|
| `SharedMemory` | Разделяемая память для данных датчиков | `smemory` |
| `UNetExchange` | Сетевой обмен по UDP | `unet` |
| `MBTCPMaster` | Modbus TCP Master | `mbtcp` |
| `MBTCPMultiMaster` | Modbus TCP Multi-Master | `mbtcp` |
| `MBSlave` | Modbus Slave | `mbs` |
| `RTUExchange` | Modbus RTU Exchange | `rs` |
| `OPCUAServer` | OPC UA Сервер | `opcua` |
| `OPCUAExchange` | OPC UA Клиент/Обмен | `opcua` |
| `IOControl` | Управление вводом-выводом COMEDI | `io` |
| `LogicProcessor` | Процессор логических элементов | `lproc` |
| `MQTTPublisher` | MQTT Publisher | `mqtt` |
| `UWebSocketGate` | WebSocket шлюз | `ws` |
| `BackendClickHouse` | Backend для ClickHouse | `clickhouse` |
| `DBServerPostgreSQL` | Сервер базы данных PostgreSQL | `pgsql` |

**Важно**: SharedMemory всегда должен быть указан первым.

## Пример

### Полная конфигурация

```xml
<?xml version="1.0" encoding="utf-8"?>
<UniSet>
  <!-- Стандартная конфигурация UniSet -->
  <ObjectsMap>
    <node name="localhost">
      <item name="SharedMemory" id="1000"/>
      <item name="UNetExchange1" id="1001"/>
      <item name="MBTCPMaster1" id="1002"/>
    </node>
  </ObjectsMap>

  <settings>
    <SharedMemory name="SharedMemory" .../>
    <UNetExchange1 name="UNetExchange1" .../>
    <MBTCPMaster1 name="MBTCPMaster1" .../>

    <!-- Конфигурация UniSetUno -->
    <UniSetUno name="MyApp">
      <extensions>
        <extension type="SharedMemory" name="SharedMemory"/>
        <extension type="UNetExchange" name="UNetExchange1" prefix="unet"/>
        <extension type="MBTCPMaster" name="MBTCPMaster1" prefix="mbtcp"/>
      </extensions>
    </UniSetUno>
  </settings>
</UniSet>
```

### Запуск

```bash
# Запуск с секцией UniSetUno по умолчанию
uniset2-uno --confile configure.xml --localNode localhost

# Запуск с указанием имени приложения
uniset2-uno --confile configure.xml --localNode localhost --app-name MyApp
```

## Архитектура

```
┌─────────────────────────────────────────────┐
│                 uniset2-uno                 │
│                                             │
│  ┌─────────────────────────────────────┐   │
│  │           SharedMemory              │   │
│  │         (IONotifyController)        │   │
│  └──────────────┬──────────────────────┘   │
│                 │ прямой указатель         │
│    ┌────────────┼────────────┐             │
│    │            │            │             │
│    ▼            ▼            ▼             │
│ ┌──────┐   ┌──────────┐   ┌───────┐       │
│ │ UNet │   │ MBTCPMaster│  │ OPCUA │       │
│ └──────┘   └──────────┘   └───────┘       │
│                                             │
│  UniSetActivator (единый ORB)              │
└─────────────────────────────────────────────┘
```

Все расширения разделяют один указатель на SharedMemory и работают в одном процессе с единым CORBA ORB.

## Сравнение

| Аспект | Отдельные процессы | uniset2-uno |
|--------|-------------------|-------------|
| IPC | CORBA/UDP | Прямой указатель |
| Процессы | 5-10 | 1 |
| Память | Больше (библиотеки дублируются) | Меньше |
| Отладка | Сложная | Простая |
| Изоляция | Да (падение = один процесс) | Нет (падение = все) |
| Масштабируемость | Много узлов | Один узел |

## Когда использовать

**Используйте uniset2-uno когда:**
- Работа на одном узле
- Нужны минимальные накладные расходы
- Требуется простое развёртывание
- Отладка/разработка

**Используйте отдельные процессы когда:**
- Нужна изоляция процессов
- Работа на нескольких узлах
- Одно расширение может упасть
- Нужна возможность независимого перезапуска
