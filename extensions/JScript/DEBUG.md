# JScript Debug Visualizer

Веб-отладчик для мониторинга и отладки JavaScript-программ,
работающих на UniSet2 JScript (QuickJS). Работает через браузер,
не требует установки на клиенте.

## Быстрый старт

### 1. Подключить модуль отладки

```javascript
load("uniset2-iec61131.js");
load("uniset2-debug.js");

uniset_debug_start(8088);

uniset_inputs = [{ name: "AI_Temp_S" }];
uniset_outputs = [{ name: "DO_Heater_C" }];

const myTimer = new TON(3000);

function uniset_on_step() {
    myTimer.update(!!in_AI_Temp_S);
    out_DO_Heater_C = myTimer.Q ? 1 : 0;
}
```

### 2. Открыть в браузере

```
http://<host>:8088/debug/ui
```

Готово — в браузере отобразятся переменные, состояние FB, трасса и тренды.

## Возможности

### Live monitoring (переменные в реальном времени)

Панель Variables автоматически обнаруживает:
- **Входы** (`in_*`) — значения датчиков
- **Выходы** (`out_*`) — управляющие воздействия
- **Локальные переменные** — состояние алгоритма
- **FB instances** (TON, CTU, RS, ...) — таймеры, счётчики, триггеры

Изменения подсвечиваются: зелёная вспышка при увеличении, красная при уменьшении.
Boolean-значения отображаются как цветные индикаторы (● зелёный / ○ серый).

### FB Status (карточки функциональных блоков)

Каждый FB-инстанс отображается как карточка с типо-специфичной визуализацией:

| Тип FB | Отображение |
|--------|-------------|
| TON, TOF, TP | Progress bar (ET/PT), индикатор Q |
| CTU, CTD, CTUD | Gauge (CV/PV), индикатор Q |
| RS, SR | Индикатор Q1 |
| R_TRIG, F_TRIG | Индикатор импульса Q |
| PID | Значение Y, индикатор насыщения |
| BLINK | Индикатор OUT |

### Execution Trace (трасса выполнения)

Показывает какие ветки IF/CASE сработали в текущем цикле:
```
IF @42: ✓     (условие TRUE, зелёный)
CASE @55: → 1 (выбрана ветка 1)
IF @60: ✗     (условие FALSE, красный)
```

Для работы трассы нужен код с инструментацией (`--debug` флаг st2js)
или ручные вызовы `_dbg_if()` / `_dbg_case()`.

### Trend Charts (графики трендов)

Клик по переменной в таблице добавляет её на график:
- До 8 переменных одновременно (разные цвета)
- Окно: 30с / 1м / 5м / всё
- Автомасштабирование оси Y
- Экспорт в CSV

### Force Override (принудительная установка значений)

Правый клик по переменной → диалог Force:
- Ввод числового значения или toggle для boolean
- Принудительное значение применяется перед каждым циклом `uniset_on_step()`
- Форсированные переменные помечены 🔒 и жёлтым фоном
- Снятие форсировки: правый клик → Unforce

> **Ограничение v1**: форсировка применяется как вход для программы (в step_cb
> перед uniset_on_step). Программа может перезаписать выходное значение.
> Для входов и локальных переменных форсировка работает надёжно.

## Три режима использования

### Режим 1: Любой JS-скрипт (без st2js)

```javascript
load("uniset2-debug.js");
uniset_debug_start(8088);
```

Доступно: live переменные, FB status, тренды, force override.
Не доступно: execution trace (нет `_dbg_if`/`_dbg_case` вызовов).

### Режим 2: st2js с флагом --debug

```bash
python -m st2js thermostat.st --debug -o thermostat.js
```

Сгенерированный JS содержит:
- `_dbg_if(LINE, condition)` — обёртки для IF
- `_dbg_case(LINE, selector)` — обёртки для CASE
- `_dbg_begin_cycle()` / `_dbg_end_cycle()` — границы цикла
- `globalThis._program_meta` — метаданные программы (типы, scale, enum)

Доступно: всё из режима 1 + execution trace + smart display (масштабирование, enum-метки).

Загрузка отладчика в скрипт:
```javascript
// В начале сгенерированного JS добавить:
load("uniset2-debug.js");
uniset_debug_start(8088);
```

### Режим 3: Ручная трасса

```javascript
load("uniset2-debug.js");
uniset_debug_start(8088);

function uniset_on_step() {
    _dbg_begin_cycle();
    if (_dbg_if(1, in_Temperature > 80)) {
        out_Alarm = 1;
    }
    _dbg_end_cycle();
}
```

Выборочная инструментация конкретных ветвей.

## API

### uniset_debug_start(port, options)

Запуск debug-сервера.

```javascript
uniset_debug_start(8088, {
    history_depth: 1000,   // размер ring buffer (циклов)
    trace_enabled: true,   // сбор трассы
});
```

| Параметр | По умолчанию | Описание |
|----------|-------------|----------|
| `port` | 8088 | HTTP порт |
| `history_depth` | 1000 | Глубина ring buffer для трендов |
| `trace_enabled` | true | Сбор execution trace |

### uniset_debug_watch(name, getter)

Ручная регистрация переменной для отслеживания.

```javascript
// Отслеживать переменную из замыкания
uniset_debug_watch("myClosureVar", () => myClosureVar);

// Отслеживать вычисляемое значение
uniset_debug_watch("tempCelsius", () => in_AI_Temp_S / 100);
```

### _program_meta (опционально)

Метаданные программы для улучшенного отображения. Генерируется st2js `--debug`
или задаётся вручную:

```javascript
globalThis._program_meta = {
    name: "Thermostat",
    inputs: [
        { name: "Temperature", sensor: "AI_Temp_S", type: "REAL", scale: 100, unit: "°C" },
        { name: "Enable", sensor: "DI_Enable_S", type: "BOOL" },
    ],
    outputs: [
        { name: "HeaterOn", sensor: "DO_Heater_C", type: "BOOL" },
    ],
    locals: [
        { name: "state", type: "INT", enum: { 0: "Idle", 1: "Running", 2: "Alarm" } },
    ],
    fb_instances: [
        { name: "onDelay", type: "TON", args: { PT: 5000 } },
        { name: "cycleCounter", type: "CTU", args: { PV: 100 } },
    ],
};
```

Эффект:
- `in_AI_Temp_S = 2350` → отображается как `23.50 °C` (scale + unit)
- `state = 1` → отображается как `Running` (enum)
- TON progress bar: `ET / PT = 2500 / 5000`

### _debug_meta на FB классах

Библиотеки `uniset2-iec61131.js` и `uniset2-iec61131-codesys.js` уже содержат
метаданные для всех 18 FB. Пользовательские FB тоже могут их добавить:

```javascript
class MyController {
    constructor() { this.output = 0; }
    execute() { /* ... */ }
}
MyController._debug_meta = {
    type: "controller",
    display: "chart",
    fields: {
        output: { type: "real", label: "Control Output", unit: "%" },
    }
};
```

## HTTP API

Все endpoints под префиксом `/debug/`:

| Endpoint | Метод | Описание |
|----------|-------|----------|
| `/debug/ui` | GET | HTML-страница отладчика |
| `/debug/snapshot` | GET | Текущее состояние: переменные, трасса, форсировка |
| `/debug/history?var=X&depth=N` | GET | Ring buffer для трендов |
| `/debug/info` | GET | Метаданные сервера |
| `/debug/force` | POST | Форсировать переменную: `{"var":"name","value":123}` |
| `/debug/unforce` | POST | Снять форсировку: `{"var":"name"}` |
| `/debug/config` | POST | Изменить настройки: `{"history_depth":2000}` |

### Пример ответа /debug/snapshot

```json
{
    "cycle": 12345,
    "ts": 1712234567890,
    "dt_ms": 152,
    "vars": {
        "in_AI_Temp_S": 2350,
        "out_DO_Heater_C": 1,
        "state": 1,
        "myTimer.Q": false,
        "myTimer.ET": 2500
    },
    "trace": [
        {"type": "if", "line": 42, "result": true},
        {"type": "case", "line": 55, "value": 1}
    ],
    "forced": ["out_DO_Heater_C"],
    "cycles_missed": 2
}
```

## Конфигурация

### Программная (в JS-скрипте)

```javascript
load("uniset2-debug.js");
uniset_debug_start(8088, {
    history_depth: 2000,
    trace_enabled: true,
});
```

### Через st2js mapping YAML

```yaml
options:
  debug_port: 8088
```

## Производительность

| Компонент | Overhead | Когда |
|-----------|----------|-------|
| Ring buffer (snapshot) | ~0.5ms/цикл | Всегда когда debug.js загружен |
| Trace collection | ~0.05ms/ветка | Только с `--debug` кодом |
| HTTP snapshot response | ~1ms/запрос | Только при подключённом браузере |
| Ring buffer memory | ~400KB | 50 vars × 1000 циклов |
| Без debug.js | 0 | `_dbg_*` stubs — no-op |

Для типичных PLC-программ (10-50 переменных, <100 ветвей) суммарный overhead
дебагера <1ms на 150ms цикл (~0.7%).

## Устранение неполадок

### Браузер не подключается

1. Проверить что `uniset_debug_start(port)` вызван в скрипте
2. Проверить доступность порта: `curl http://host:8088/debug/info`
3. Проверить firewall: порт должен быть открыт

### Переменные не отображаются

1. Переменные должны быть глобальными (`in_*`, `out_*`, `let` на верхнем уровне)
2. Для переменных в замыканиях: `uniset_debug_watch("name", () => value)`
3. FB instances должны быть глобальными `const`

### Трасса пустая

1. Код должен быть сгенерирован с `--debug`: `python -m st2js program.st --debug -o output.js`
2. Или вручную добавить `_dbg_if()` / `_dbg_case()` вызовы
3. Проверить `trace_enabled: true` в настройках

### Force не работает

1. Force применяется перед `uniset_on_step()` — программа может перезаписать значение
2. Для выходов: программа перезаписывает force'd значение в on_step
3. Надёжно работает для входов и локальных переменных
