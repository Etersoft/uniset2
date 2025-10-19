/*
 * Copyright (c) 2025 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
// Упрощенная система таймеров на основе объектов
// generated with deep seek AI
// --------------------------------------------------------------------------
var unisetTimersModuleVersion = "v1"
var timerQueue = {};
var logLevel = "none"; // "none", "info", "debug"
var TimerInfinity = -1;
// --------------------------------------------------------------------------
function uniset2_timers_log(level) {
    var validLevels = ["none", "info", "debug"];

    if (level === undefined) {
        // Если аргумент не передан, возвращаем текущий уровень
        return logLevel;
    }

    if (validLevels.includes(level)) {
        var oldLevel = logLevel;
        logLevel = level;
        console.log("uniset2_timers: log level changed from '" + oldLevel + "' to '" + level + "'");
        return level;
    } else {
        console.log("uniset2_timers: invalid log level '" + level + "'. Valid levels: " + validLevels.join(", "));
        return false;
    }
}
// --------------------------------------------------------------------------
function infoLog(message) {
    if (logLevel === "info" || logLevel === "debug") {
        console.log("⏰ [TIMERS_INFO] " + message);
    }
}
// --------------------------------------------------------------------------
function debugLog(message) {
    if (logLevel === "debug") {
        console.log("⏰ [TIMERS_DEBUG] " + message);
    }
}

// --------------------------------------------------------------------------
function errorLog(message) {
    console.log("⏰ [TIMERS_ERROR] " + message);
}

// --------------------------------------------------------------------------
function askTimer(id, msec, repeatCount, callback) {

    if (repeatCount === undefined) repeatCount = -1;

    // Если msec = 0, удаляем таймер
    if (msec === 0) {
        return removeTimer(id);
    }

    if (!id || msec <= 0) {
        errorLog("askTimer: invalid parameters - id:" + id + " msec:" + msec);
        return false;
    }

    // Удаляем существующий таймер с таким же ID
    removeTimer(id);

    // Создаем новый таймер
    timerQueue[id] = {
        id: id,
        msec: msec,
        repeatCount: repeatCount,
        callback: callback,
        startTime: Date.now(),
        lastTrigger: 0,
        triggerCount: 0,
        active: true
    };

    infoLog("Timer CREATED: " + id + " msec:" + msec + " repeatCount:" + repeatCount);
    debugLog("askTimer: added timer " + id + " msec:" + msec + " repeatCount:" + repeatCount);
    return true;
}
// --------------------------------------------------------------------------
function uniset_process_timers() {
    var currentTime = Date.now();
    var timersToRemove = [];

    debugLog("processTimers: checking " + Object.keys(timerQueue).length + " timers");

    for (var id in timerQueue) {
        var timer = timerQueue[id];

        // Пропускаем неактивные таймеры
        if (!timer.active) {
            debugLog("processTimers: timer " + id + " is paused, skipping");
            continue;
        }

        var timeSinceLastTrigger = currentTime - (timer.lastTrigger || timer.startTime);

        debugLog("processTimers: timer " + id + " timeSinceLastTrigger:" + timeSinceLastTrigger + " msec:" + timer.msec);

        // Проверяем, сработал ли таймер
        if (timeSinceLastTrigger >= timer.msec) {

            if (timer.repeatCount >= 0 && timer.triggerCount >= timer.repeatCount) {
                // Таймер уже выполнил все повторения, пропускаем
                timersToRemove.push(id);
                continue;
            }

            var callbackExecuted = false;

            if (timer.callback !== null && timer.callback !== undefined) {
                try {
                    var callbackType = typeof timer.callback;
                    if (callbackType === 'function') {
                        debugLog("Executing callback for timer: " + id + " type:" + callbackType);

                        // ВАЖНО: Сохраняем callback в локальную переменную
                        var cb = timer.callback;

                        // Пробуем разные способы вызова
                        try {
                            // Способ 1: Прямой вызов
                            debugLog("Trying direct call for timer: " + id);
                            cb(timer.id, timer.triggerCount);
                            callbackExecuted = true;
                            debugLog("Direct call SUCCESS for timer: " + id);

                            // INFO: Логируем срабатывание таймера
                            infoLog("Timer TRIGGERED: " + id + " count:" + (timer.triggerCount + 1));

                        } catch (callError) {
                            errorLog("Direct call failed for timer " + id + ": " + callError);

                            // Способ 2: Вызов через call с глобальным контекстом
                            try {
                                debugLog("Trying call with global context for timer: " + id);
                                var globalObj = typeof global !== 'undefined' ? global :
                                    typeof window !== 'undefined' ? window :
                                        typeof globalThis !== 'undefined' ? globalThis : this;
                                cb.call(globalObj, timer.id, timer.triggerCount);
                                callbackExecuted = true;
                                debugLog("Call with global context SUCCESS for timer: " + id);

                                // INFO: Логируем срабатывание таймера
                                infoLog("Timer TRIGGERED: " + id + " count:" + (timer.triggerCount + 1));

                            } catch (callError2) {
                                errorLog("Call with global context failed for timer " + id + ": " + callError2);

                                // Способ 3: Вызов через apply
                                try {
                                    debugLog("Trying apply call for timer: " + id);
                                    cb.apply(globalObj, [timer.id, timer.triggerCount]);
                                    callbackExecuted = true;
                                    debugLog("Apply call SUCCESS for timer: " + id);

                                    // INFO: Логируем срабатывание таймера
                                    infoLog("Timer TRIGGERED: " + id + " count:" + (timer.triggerCount + 1));

                                } catch (callError3) {
                                    errorLog("Apply call failed for timer " + id + ": " + callError3);
                                }
                            }
                        }
                    } else {
                        errorLog("Callback for timer " + id + " is not a function, type:" + callbackType + " value:" + timer.callback);
                    }
                } catch (e) {
                    errorLog("Timer callback error for " + id + ": " + e);
                }
            } else {
                errorLog("Callback is null or undefined for timer: " + id);
            }

            if (!callbackExecuted) {
                errorLog("FAILED to execute callback for timer: " + id);
                timersToRemove.push(id);
                continue;
            }

            // Обновляем счетчики
            timer.lastTrigger = currentTime;
            timer.triggerCount++;

            debugLog("Timer " + id + " triggered " + timer.triggerCount + " times");

            // Проверяем, нужно ли удалять таймер
            if (timer.repeatCount >= 0 && timer.triggerCount >= timer.repeatCount) {
                timersToRemove.push(id);
                infoLog("Timer COMPLETED: " + timer.id + " triggered " + timer.triggerCount + " times");
                debugLog("processTimers: timer completed " + timer.id + " triggered " + timer.triggerCount + " times");
            }
        }
    }

    // Удаляем завершенные таймеры
    if (timersToRemove.length > 0) {
        debugLog("Removing " + timersToRemove.length + " completed timers: " + timersToRemove.join(", "));
        for (var i = 0; i < timersToRemove.length; i++) {
            removeTimer(timersToRemove[i]);
        }
    }
}
// --------------------------------------------------------------------------
// Удаление таймера
function removeTimer(id) {
    if (timerQueue[id]) {
        delete timerQueue[id];
        infoLog("Timer REMOVED: " + id);
        debugLog("askTimer: removed timer " + id);
        return true;
    }
    return false;
}
// --------------------------------------------------------------------------
// Приостановка таймера
function pauseTimer(id) {
    if (timerQueue[id]) {
        timerQueue[id].active = false;
        infoLog("Timer PAUSED: " + id);
        debugLog("askTimer: paused timer " + id);
        return true;
    }
    return false;
}
// --------------------------------------------------------------------------
// Возобновление таймера
function resumeTimer(id) {
    if (timerQueue[id]) {
        timerQueue[id].active = true;
        timerQueue[id].startTime = Date.now();
        timerQueue[id].lastTrigger = 0;
        infoLog("Timer RESUMED: " + id);
        debugLog("askTimer: resumed timer " + id);
        return true;
    }
    return false;
}
// --------------------------------------------------------------------------
// Получение информации о всех таймерах
function getTimerInfo() {
    var info = "Active timers: " + Object.keys(timerQueue).length + "\n";

    for (var id in timerQueue) {
        var timer = timerQueue[id];
        var status = timer.active ? "ACTIVE" : "PAUSED";
        var repeatInfo = timer.repeatCount < 0 ? "∞" : timer.repeatCount;
        var progress = timer.repeatCount < 0 ?
            timer.triggerCount :
            timer.triggerCount + "/" + timer.repeatCount;

        info += `ID: ${timer.id}, msec: ${timer.msec}, repeats: ${repeatInfo}, ` +
            `triggered: ${progress}, status: ${status}\n`;
    }

    return info;
}
// --------------------------------------------------------------------------
// Очистка всех таймеров
function clearAllTimers() {
    var count = Object.keys(timerQueue).length;
    timerQueue = {};
    infoLog("All timers CLEARED (" + count + " removed)");
    debugLog("All timers cleared (" + count + " removed)");
}
// --------------------------------------------------------------------------
// Получение статистики по таймеру
function getTimerStats(id) {
    var timer = timerQueue[id];
    if (!timer) return null;

    return {
        id: timer.id,
        msec: timer.msec,
        repeatCount: timer.repeatCount,
        triggerCount: timer.triggerCount,
        active: timer.active,
        timeSinceLastTrigger: timer.lastTrigger ? Date.now() - timer.lastTrigger : null
    };
}
// --------------------------------------------------------------------------
// Получение статуса
function getTimersStatus() {
    return {
        logLevel: logLevel,
        totalTimers: Object.keys(timerQueue).length,
        activeTimers: Object.keys(timerQueue).filter(id => timerQueue[id].active).length,
        version: unisetTimersModuleVersion
    };
}
// --------------------------------------------------------------------------

/* === HOIST TO globalThis FOR ESM COMPAT === */
try {
  if (typeof askTimer === 'function') globalThis.askTimer = askTimer;
  if (typeof uniset_process_timers === 'function') globalThis.uniset_process_timers = uniset_process_timers;
  if (typeof getTimerInfo === 'function') globalThis.getTimerInfo = getTimerInfo;
  if (typeof getTimerStats === 'function') globalThis.getTimerStats = getTimerStats;
  if (typeof getTimersStatus === 'function') globalThis.getTimersStatus = getTimersStatus;
  if (typeof pauseTimer === 'function') globalThis.pauseTimer = pauseTimer;
  if (typeof resumeTimer === 'function') globalThis.resumeTimer = resumeTimer;
  if (typeof removeTimer === 'function') globalThis.removeTimer = removeTimer;
  if (typeof clearAllTimers === 'function') globalThis.clearAllTimers = clearAllTimers;

  if (typeof uniset2_timers_log === 'function') globalThis.uniset2_timers_log = uniset2_timers_log;
  if (typeof infoLog === 'function') globalThis.infoLog = infoLog;
  if (typeof debugLog === 'function') globalThis.debugLog = debugLog;
  if (typeof errorLog === 'function') globalThis.errorLog = errorLog;

  if (typeof unisetTimersModuleVersion !== 'undefined')
    globalThis.unisetTimersModuleVersion = unisetTimersModuleVersion;

  if (typeof TimerInfinity !== "undefined" )
      globalThis.TimerInfinity = TimerInfinity;
} catch (_) {}
/* === END HOIST === */

