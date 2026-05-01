# JScript Examples

## Thermostat — управление нагревателем

Демонстрирует полный цикл: ST-программа → конвертация → запуск с отладкой.

### Файлы

| Файл | Описание |
|------|----------|
| `thermostat.st` | Исходная ST-программа (IEC 61131-3) |
| `thermostat_mapping.yaml` | Маппинг ST-переменных ��а датчики UniSet |
| `main-plc.js` | Сгенерированный JS с отладочной инструментацией |

### Датчики (в test.xml)

| Датчик | Тип | Описание |
|--------|-----|----------|
| `AI_Temperature_S` | AI | Температура (×100, т.е. 2350 = 23.50°C) |
| `DI_Enable_S` | DI | Кнопка включения |
| `DI_Reset_S` | DI | Кнопка сброса |
| `DO_Heater_C` | DO | Выход на нагреватель |
| `DO_AlarmHigh_C` | DO | Авария перегрева |
| `AO_CycleCount_C` | AO | Счётчик циклов нагревателя |

### Генерация JS из ST

```bash
cd extensions/JScript/tools/st2js
python -m st2js ../../examples/thermostat.st \
    -m ../../examples/thermostat_mapping.yaml \
    --debug \
    -o ../../examples/main-plc.js
```

### Запуск

```bash
cd extensions/JScript
uniset2-jscript \
    --confile test.xml \
    --js-file examples/main-plc.js \
    --js-name JSProxy1 \
    --js-sleep-msec 150
```

### Отладка

Открыть в браузере: `http://localhost:8088/debug/ui`

Что видно:
- **Variables**: `AI_Temperature_S`, `DO_Heater_C`, `state` и др.
- **FB Status**: TON (onDelay), CTU (cycleCounter), R_TRIG (startEdge)
- **Trace**: какие IF/CASE сработали в текущем цикле
- **Trends**: клик по переменной → график

### Тестирование вручную

```bash
# Установить температуру = 15.00°C (1500 в int)
uniset2-admin --confile test.xml --setValue --name AI_Temperature_S --value 1500

# Включить
uniset2-admin --confile test.xml --setValue --name DI_Enable_S --value 1
uniset2-admin --confile test.xml --setValue --name DI_Enable_S --value 0

# Проверить выход
uniset2-admin --confile test.xml --getValue --name DO_Heater_C
```
