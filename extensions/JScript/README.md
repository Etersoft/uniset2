# JScript - Поддержка скриптов на JS {#pageJScript}

JScript предоставляет полноценную среду выполнения JavaScript (QuickJS) с интеграцией
в систему UniSet. Работает циклически по принципу PLC: опрос входов → шаг алгоритма → выставление выходов.

## Архитектура

### Базовые возможности
- Загрузка и выполнение JS-скриптов
- Интеграция с UniSet (датчики, сообщения)
- Поддержка модульной системы с поиском по указанным п��тям
- Готовые модули для типовых задач

### Работа с датчиками UniSet

JScript автоматически создаёт **глобальные переменные** для работы с датчиками:

```javascript
// Конфигурация в JS:
uniset_inputs = [
    { name: "Sensor1", sensor: "real_sensor_name" },
    { name: "Sensor2" }
];

uniset_outputs = [
    { name: "Output1" },
    { name: "Output2" }
];

// Использование в коде:
function uniset_on_step() {
    // Чтение входных датчиков (автоматически создаются как глобальные переменные)
    let value = in_Sensor1;

    // Запись выходных датчиков
    out_Output1 = value + 10;
}
```

Если хочется, чтобы пе��еменная в коде отличалась от реального названия датчика, можно указать
реальное название через поле `sensor: "real_sensor_name"` — тогда `name` бу��ет использован
в коде, а привязка будет осуществлена по названию, указанному в `sensor`.

Также создаются переменные с именем датчика, содержащие ID датчика:
```javascript
console.log("Sensor1 id=", Sensor1)
```

### Система событий

JScript предоставляет набор callback-функций для реакции на системные события:

```javascript
// Вызывается при старте системы
function uniset_on_start() {
    mylog.info("System started");
}

// Вызывается при остановке системы
function uniset_on_stop() {
    mylog.info("System stopped");
}

// Вызывается на каждом шаге выполнения
function uniset_on_step() {
    out_Output1 = in_Sensor1 * 2;
}

// Вызывается при изменении значения датчика
function uniset_on_sensor(id, value, name) {
    mylog.info("Sensor " + id + " changed to " + value);
    if (id == Sensor1) {
        // ...
    }
}
```

### Система таймеров

Модуль `uniset2-timers.js` предоставляет расширенные возможности работы с таймерами:

```javascript
function uniset_on_start() {
    // Одноразовый таймер
    askTimer("single", 5000, 1, function(id, count) {
        mylog.info("Single timer executed");
    });

    // Периодический таймер (3 раза)
    askTimer("repeat", 1000, 3, function(id, count) {
        mylog.info("Repeating timer: " + count);
    });

    // Бесконечный таймер
    askTimer("infinite", 2000, -1, function(id, count) {
        mylog.info("Heartbeat: " + count);
    });
}

function uniset_on_stop() {
    clearAllTimers();
}
```

### Система логирования

Модуль `uniset2-log.js` предоставляет логирование в стиле UniSet:

```javascript
mylog = uniset_log_create("MyModule", true, true, true, true);
mylog.level("info", "warn", "crit", "level5");

mylog.info("Information message");
mylog.warn("Warning message");
mylog.crit("Critical message");
mylog.log5("Debug message level 5");
```

### Глобальный объект ui

JScript предоставляет глобальный объект `ui` для прямого взаимодействия с системой UniSet:

```javascript
ui.askSensor(sensor, command)    // Подписка/отписка на получение значений датчика
ui.getValue(sensor)              // Чтение текущего значения датчика
ui.setValue(sensor, value)       // Запись значения в датчик
```

> В качестве `sensor` можно ��спользовать **идентификатор** или **имя** датчика.

Доступные константы для команды подписки:
- `UIONotify` — получать все значения
- `UIODontNotify` — не получать значения (отписка)
- `UIONotifyChange` — получать значения только при изменении
- `UIONotifyFirstNotNull` — получать только первое ненулевое значение

```javascript
function uniset_on_step() {
    let currentValue = ui.getValue(Sensor1);
    if (currentValue > 100) {
        out_Output1 = 1;
    }

    let temp = ui.getValue(12345);
    mylog.info("Temperature: " + temp);
}
```

### Модульная система

Поддерживается загрузка модулей через функцию `load()` с поиском по указанным путям:

```javascript
load("uniset2-timers.js");
load("uniset2-log.js");
load("my_custom_module.js");
```

Доступные UniSet-модули:
- `uniset2-timers.js` — таймеры
- `uniset2-log.js` — логирование
- `uniset2-delaytimer.js` — аналог uniset::DelayTimer
- `uniset2-passivetimer.js` — аналог uniset::PassiveTimer
- `uniset2-iec61131.js` — стандартные функциональные блоки IEC 61131-3
- `uniset2-iec61131-codesys.js` — ��ункциональные блоки Codesys Util (BLINK, PID, HYSTERESIS, ...)
- `uniset2-debug.js` — отладчик-визуализатор (см. [DEBUG.md](DEBUG.md))

## Отладка и визуализация

Модуль `uniset2-debug.js` предоставляет веб-отладчик для мониторинга
работающего JScript-процесса через браузер:

```javascript
load("uniset2-debug.js");
uniset_debug_start(8088);
// Открыть http://host:8088/debug/ui
```

Возможности: live-мониторинг переменных и FB, трасса выполнения (IF/CASE),
графики трендов, принудительная установка значений (force override).

Работает с любым JS-скриптом без генерации кода. С флагом `st2js --debug`
дополнительно доступна трасса выполнения и метаданные программы.

Полная документация: **[DEBUG.md](DEBUG.md)**

## IEC 61131-3 — Стандартные функциональные блоки

Модуль `uniset2-iec61131.js` реализует стандартные функциональные блоки по
**IEC 61131-3:2013, Edition 3** (Tables 36–45).

| Блок | Тип | Таблица IEC | Описание |
|------|-----|-------------|----------|
| RS | Бистабильный | Table 36 | Триггер с доминантным сбросом |
| SR | Бистабильный | Table 37 | Триггер с доминантной установкой |
| R_TRIG | Детектор фронта | Table 38 | Нарастающий фронт |
| F_TRIG | Детектор фронта | Table 39 | Спадающий фронт |
| CTU | Счётчик | Table 40 | Счётчик вверх |
| CTD | Счётчик | Table 41 | Счётчик вниз |
| CTUD | Счётчик | Table 42 | Реверсивный счётчик |
| TON | Таймер | Table 43 | Задержка включения |
| TOF | Таймер | Table 44 | Задержка выключения |
| TP | Таймер | Table 45 | Формирователь импульса |

Подробное описание каждого блока с примерами, таблицами истинности и временными
диаграммами: **[IEC61131.md](IEC61131.md)**

## Конфигурация

### XML-конфигурация

```xml
<JSProxy name="JSProxy1">
    <modules>
        <module path="/usr/share/uniset2/js"/>
        <module path="/custom/js/modules"/>
    </modules>
</JSProxy>
```

### Параметры командной строки

| Параметр | Описание |
|----------|----------|
| `--js-name <name>` | Имя объекта в конфигурации |
| `--js-logfile <file>` | Файл для сохранения логов |
| `--js-sleep-msec <msec>` | Пауза между вызовами uniset_on_step (по умолчанию 150 мс) |
