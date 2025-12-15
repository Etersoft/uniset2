@page page_SharedMemory SharedMemory

# SharedMemory

Документ описывает расширение SharedMemory и его возможности.

## Задачи

SharedMemory расширяет возможности `IONotifyController`. Основные функции:

- Конфигурирование списка датчиков.
- Уведомление о рестарте SM.
- Механизм зависимостей между датчиками.
- Слежение за «сердцебиением» процессов.
- Аварийный дамп (история значений).
- Пульсар.
- Логирование в БД.
- Работа с резервной SM.
- Контроль доступа (ACL).
- HTTP API.

## Конфигурирование датчиков

SM позволяет ограничивать перечень регистрируемых датчиков, заказчиков и зависимостей через фильтры командной строки:

```
--s-filter-field / --s-filter-value   # фильтр датчиков
--c-filter-field / --c-filter-value   # фильтр заказчиков (consumers)
--d-filter-field / --d-filter-value   # фильтр зависимостей (depends)
--t-filter-field / --t-filter-value   # фильтр порогов (thresholds)
```

Если `--X-filter-field` задан без `--X-filter-value`, загружаются все объекты, у которых поле не пустое.
Без фильтров загружается весь список из `<sensors>`.

### Пример

В конфигурации:

```xml
<sensors>
  <item id="12" name="Sensor12" myfilter="m1">
    <consumers>
      <item name="Consumer1" type="object" mycfilter="c1"/>
    </consumers>
  </item>
  <item id="121" name="Sensor121" myfilter="m1">
    <consumers>
      <item name="Consumer1" type="object" mycfilter="c1"/>
    </consumers>
  </item>
</sensors>
```

Запуск с `--s-filter-field myfilter --s-filter-value m1` загрузит датчики 12 и 121.
Пара `--c-filter-field mycfilter --c-filter-value c1` отфильтрует список заказчиков.

## Уведомление о рестарте SM

Параметры:

```
--e-filter <name>        # по какому полю собирать получателей
--e-startup-pause <msec> # пауза перед отправкой (по умолчанию 1500 мс)
```

В конфигурации объектов указывайте `evnt_<name>="1"`. После старта SM разошлёт SystemMessage::WatchDog.

## Зависимости между датчиками

Датчик может быть заблокирован, пока «разрешающий» датчик не примет нужное значение. Пример:

```xml
<item id="20050" iotype="AI" name="Sensor1">
  <consumers>
    <item name="Consumer1" type="objects"/>
  </consumers>
  <depends>
    <depend block_invert="1" name="Node_Not_Respond_FS"/>
  </depends>
</item>
```

Параметры в `<depend>`:
- `block_invert` — инвертировать разрешающий датчик.

Работает для дискретных датчиков.

## Слежение за «сердцебиением»

Для каждого процесса задаются два датчика: аналоговый счётчик и дискретный флаг. Процесс инкрементирует счётчик, SM периодически декрементирует. Пока счётчик > 0, дискретный датчик держится в 1. Если счётчик «упал» — дискретный сбрасывается в 0. При наличии WDT и параметра `heartbeat_reboot_msec` возможен перезапуск контроллера.

Аналоговый датчик помечается `heartbeat="1"` и включает:

- `heartbeat_ds_name` — связанный дискретный датчик.
- `heartbeat_node` — фильтр (`--heartbeat-node`).
- `heartbeat_reboot_msec` — таймаут ожидания перезапуска.

Пример:

```xml
<item default="10" heartbeat="1" heartbeat_ds_name="_41_04_S" heartbeat_node="ses"
      heartbeat_reboot_msec="10000" id="103104" iotype="AI" name="_31_04_AS"
      textname="SES: IO heartbeat"/>
<item default="1" id="104104" iotype="DI" name="_41_04_S">
  <MessagesList>
    <msg mtype="1" text="КСЭС: отключился ввод/вывод" value="0"/>
  </MessagesList>
</item>
```

## Аварийный дамп (History)

Циклические буферы значений с периодом `savetime` мс. Каждая «история» настраивается в `<History>`:

```xml
<SharedMemory name="SharedMemory" shmID="SharedMemory">
  <History savetime="200">
    <item id="1" fuse_id="AlarmFuse1_S" fuse_invert="1" size="30" filter="a1"/>
    <item id="2" fuse_id="AlarmFuse2_AS" fuse_value="2" size="30" filter="a2"/>
  </History>
</SharedMemory>
```

Параметры элемента:
- `id` — идентификатор истории.
- `size` — размер буфера.
- `fuse_id` — датчик-детонатор.
- `fuse_value` — значение срабатывания (для аналоговых).
- `fuse_invert` — инверсия детонатора.

Событие сброса дампа доступно через сигнал `SharedMemory::signal_history()`.

## Пульсар

Периодическое переключение заданного датчика. Параметры запуска:

```
--pulsar-id <sensor>
--pulsar-msec <period>  # по умолчанию 5000
```

## Логирование в БД

Включается опцией `--db-logging` (DBServer должен быть запущен).

## Резервная SharedMemory

SM может инициализироваться из резервной SM (см. параметры `--sm-default-sensor-permission`, `--sm-ignore-acl-errors` и логику `initFromReserv()`).

## Контроль доступа

ACL задаётся в конфигурации датчиков, пример:

```xml
<sensors>
  <sensor id="141" name="SensorRO"   type="DI" acl="IO_RW"/>
  <sensor id="142" name="SensorRW"   type="DI" acl="IO_RW"/>
  <sensor id="150" name="WriteOnly"  type="DO" acl="IO_WRITE_ONLY"/>
  <sensor id="160" name="ClosedAll"  type="DI" acl="IO_DENY_ALL"/>
  <sensor id="170" name="DefaultOnly" type="DI"/> <!-- использует defaultAccessMask -->
</sensors>
```

Параметр `readonly` устарел — используйте ACL. Имя процесса должно совпадать с именем из `<objects>`.
Горячая перезагрузка ACL — командой `SystemMessage::ReloadConfig`, например:

```
uniset2-admin --reloadconfig ShmID
```

Спецпараметр: `--http-api-disable-access-control` — отключить проверку прав для HTTP API.

## HTTP API

Маршруты:

- `/help` — список доступных команд.
- `/` — базовая информация.
- `/get?filter=id1,name2&id3&shortInfo` — значения датчиков. Параметры: `filter` (список ID/имён), `shortInfo` (короткая форма). Старый формат: `/get?id1,name2&id3`.
- `/set?supplier=Name&id1=val1&name2=val2...` — установить значения. `supplier` обязателен при включённом ACL.
- `/freeze` и `/unfreeze` — зафиксировать/снять фиксацию значений, параметры как у `/set`.
- `/sensors?offset=N&limit=M&search=text&filter=id1,id2&iotype=AI|AO|DI|DO` — получить список датчиков с фильтрами и пагинацией.
- `/consumers?sens1,sens2` — списки заказчиков по датчикам.
- `/lost` — заказчики, с которыми терялась связь.
- `/conf/get?id,name,...&props=textname,iotype,...` — параметры объектов из configure.xml (если `props` не задан, возвращаются все поля).
