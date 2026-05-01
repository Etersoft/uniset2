#!/usr/bin/env quickjs
/*
 * Copyright (c) 2026 Pavel Vainerman.
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
var unisetIEC61131CodesysModuleVersion = "v1"
// --------------------------------------------------------------------------
if (typeof IEC61131_STRICT === 'undefined')
    var IEC61131_STRICT = false;

var IEC61131_MS_PER_SECOND = 1000.0;
var IEC61131_DERIVATIVE_SAMPLES = 4;
var IEC61131_DERIVATIVE_READY_SAMPLES = 3;
var IEC61131_FOUR_POINT_DENOMINATOR = 6.0;

function _codesys_checkBool(name, value)
{
    if (typeof value !== 'boolean')
        throw new TypeError("IEC61131 Codesys [" + name + "]: expected boolean, got " + typeof value + " (" + value + ")");
}

function _codesys_checkFiniteNumber(name, value)
{
    if (typeof value !== 'number' || !isFinite(value))
        throw new RangeError("IEC61131 Codesys [" + name + "]: expected finite number, got " + value);
}

function _codesys_checkFiniteNonNegative(name, value)
{
    _codesys_checkFiniteNumber(name, value);
    if (value < 0)
        throw new RangeError("IEC61131 Codesys [" + name + "]: expected non-negative finite number, got " + value);
}

function _codesys_makeSampleBuffer(value)
{
    var buf = [];
    for (var i = 0; i < IEC61131_DERIVATIVE_SAMPLES; i++)
        buf.push(value);
    return buf;
}
// --------------------------------------------------------------------------
/**
 * Codesys Util Library — popular function blocks
 * Reference: https://content.helpme-codesys.com/en/libs/Util/Current/
 *
 * Signals:      BLINK
 * Analog:       HYSTERESIS, LIMITALARM, LIN_TRAFO
 * Control:      PID, RAMP_REAL
 * Math:         DERIVATIVE, INTEGRAL
 *
 * All blocks follow Codesys Util library semantics exactly.
 * Must be called cyclically from uniset_on_step().
 *
 * Usage:
 *   load("uniset2-iec61131-codesys.js");
 *   const blink1 = new BLINK(500, 500);
 *   function uniset_on_step() {
 *       blink1.update(true);
 *       out_Led = blink1.OUT ? 1 : 0;
 *   }
 */
// --------------------------------------------------------------------------

/**
 * BLINK — генератор мигающего сигнала
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Signals/BLINK.html
 *
 * ENABLE=true: OUT чередуется TRUE (TIMEHIGH мс) / FALSE (TIMELOW мс).
 * ENABLE=false: OUT удерживает последнее значение (Codesys spec).
 *
 * @param {number} TIMEHIGH - длительность TRUE (мс)
 * @param {number} TIMELOW - длительность FALSE (мс)
 */
class BLINK
{
    constructor(TIMEHIGH, TIMELOW)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNonNegative("BLINK.TIMEHIGH", TIMEHIGH);
            _codesys_checkFiniteNonNegative("BLINK.TIMELOW", TIMELOW);
        }

        this.TIMEHIGH = TIMEHIGH;
        this.TIMELOW = TIMELOW;
        this.OUT = false;
        this._lastToggle = 0;
        this._running = false;
    }

    /**
     * @param {boolean} ENABLE - включить генератор
     * @returns {boolean} OUT
     */
    update(ENABLE)
    {
        if (IEC61131_STRICT)
            _codesys_checkBool("BLINK.ENABLE", ENABLE);

        if (!ENABLE)
        {
            // Codesys spec: output keeps its value when ENABLE=false
            this._running = false;
            return this.OUT;
        }

        var now = Date.now();

        if (!this._running)
        {
            // First enable or re-enable: start fresh cycle from HIGH
            this._running = true;
            this._lastToggle = now;
            this.OUT = true;
            return this.OUT;
        }

        var elapsed = now - this._lastToggle;
        var needed = this.OUT ? this.TIMEHIGH : this.TIMELOW;

        if (elapsed >= needed)
        {
            this.OUT = !this.OUT;
            this._lastToggle = now;
        }

        return this.OUT;
    }
}

// --------------------------------------------------------------------------
/**
 * HYSTERESIS — гистерезисный компаратор
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Analog-Monitors/HYSTERESIS.html
 *
 * OUT=true когда IN < LOW (строго).
 * OUT=false когда IN > HIGH (строго).
 * На границах (IN==LOW или IN==HIGH) и в мёртвой зоне — выход не меняется.
 */
class HYSTERESIS
{
    constructor()
    {
        this.OUT = false;
    }

    /**
     * @param {number} IN - входное значение
     * @param {number} HIGH - верхний порог (выключение)
     * @param {number} LOW - нижний порог (включение)
     * @returns {boolean} OUT
     */
    update(IN, HIGH, LOW)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("HYSTERESIS.IN", IN);
            _codesys_checkFiniteNumber("HYSTERESIS.HIGH", HIGH);
            _codesys_checkFiniteNumber("HYSTERESIS.LOW", LOW);
        }

        if (IN < LOW)
            this.OUT = true;
        else if (IN > HIGH)
            this.OUT = false;
        // at boundary or in dead band: no change

        return this.OUT;
    }
}

// --------------------------------------------------------------------------
/**
 * LIMITALARM — контроль выхода за границы
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Analog-Monitors/LIMITALARM.html
 *
 * O=true если IN > HIGH, U=true если IN < LOW, IL=true иначе.
 */
class LIMITALARM
{
    constructor()
    {
        this.O = false;   // over (выше HIGH)
        this.U = false;   // under (ниже LOW)
        this.IL = true;   // in limits
    }

    /**
     * @param {number} IN - входное значение
     * @param {number} HIGH - верхний порог
     * @param {number} LOW - нижний порог
     * @returns {boolean} IL - в пределах
     */
    update(IN, HIGH, LOW)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("LIMITALARM.IN", IN);
            _codesys_checkFiniteNumber("LIMITALARM.HIGH", HIGH);
            _codesys_checkFiniteNumber("LIMITALARM.LOW", LOW);
        }

        this.O = IN > HIGH;
        this.U = IN < LOW;
        this.IL = !this.O && !this.U;
        return this.IL;
    }
}

// --------------------------------------------------------------------------
/**
 * LIN_TRAFO — линейное масштабирование
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Mathematical-Functions/LIN_TRAFO.html
 *
 * OUT = (IN - IN_MIN) / (IN_MAX - IN_MIN) * (OUT_MAX - OUT_MIN) + OUT_MIN
 * ERROR=true если IN_MIN==IN_MAX или IN вне диапазона [IN_MIN..IN_MAX].
 *
 * @param {number} IN_MIN - нижняя граница входного диапазона
 * @param {number} IN_MAX - верхняя граница входного диапазона
 * @param {number} OUT_MIN - нижняя граница выходного диапазона
 * @param {number} OUT_MAX - верхняя граница выходного диапазона
 */
class LIN_TRAFO
{
    constructor(IN_MIN, IN_MAX, OUT_MIN, OUT_MAX)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("LIN_TRAFO.IN_MIN", IN_MIN);
            _codesys_checkFiniteNumber("LIN_TRAFO.IN_MAX", IN_MAX);
            _codesys_checkFiniteNumber("LIN_TRAFO.OUT_MIN", OUT_MIN);
            _codesys_checkFiniteNumber("LIN_TRAFO.OUT_MAX", OUT_MAX);
        }

        this.IN_MIN = IN_MIN;
        this.IN_MAX = IN_MAX;
        this.OUT_MIN = OUT_MIN;
        this.OUT_MAX = OUT_MAX;
        this.OUT = 0;
        this.ERROR = false;
    }

    /**
     * @param {number} IN - входное значение
     * @returns {number} OUT - масштабированное значение
     */
    update(IN)
    {
        if (IEC61131_STRICT)
            _codesys_checkFiniteNumber("LIN_TRAFO.IN", IN);

        var range_in = this.IN_MAX - this.IN_MIN;

        if (range_in === 0)
        {
            this.ERROR = true;
            this.OUT = this.OUT_MIN;
            return this.OUT;
        }

        // Check if IN outside input range
        var lo = Math.min(this.IN_MIN, this.IN_MAX);
        var hi = Math.max(this.IN_MIN, this.IN_MAX);
        this.ERROR = (IN < lo || IN > hi);

        this.OUT = (IN - this.IN_MIN) / range_in
                 * (this.OUT_MAX - this.OUT_MIN) + this.OUT_MIN;
        return this.OUT;
    }
}

// --------------------------------------------------------------------------
/**
 * RAMP_REAL — ограничитель скорости изменения
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Function-Manipulators/RAMP_REAL.html
 *
 * Выход следует за входом, но скорость изменения ограничена.
 *   ASCEND — макс. скорость нарастания (единиц за TIMEBASE)
 *   DESCEND — макс. скорость убывания (единиц за TIMEBASE)
 *   TIMEBASE — базовый интервал (мс). 0 = скорость за один вызов.
 *   RESET=true: расчёт останавливается, OUT удерживает последнее значение.
 *
 * @param {number} ASCEND - макс. скорость нарастания
 * @param {number} DESCEND - макс. скорость убывания
 * @param {number} TIMEBASE - базовый интервал в мс (0 = за вызов)
 */
class RAMP_REAL
{
    constructor(ASCEND, DESCEND, TIMEBASE)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNonNegative("RAMP_REAL.ASCEND", ASCEND);
            _codesys_checkFiniteNonNegative("RAMP_REAL.DESCEND", DESCEND);
            _codesys_checkFiniteNonNegative("RAMP_REAL.TIMEBASE", TIMEBASE || 0);
        }

        this.ASCEND = ASCEND;
        this.DESCEND = DESCEND;
        this.TIMEBASE = TIMEBASE || 0;
        this.OUT = 0;
        this._lastTime = 0;
        this._initialized = false;
    }

    /**
     * @param {number} IN - целевое значение
     * @param {boolean} RESET - заморозить OUT (Codesys: hold last value)
     * @returns {number} OUT
     */
    update(IN, RESET)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("RAMP_REAL.IN", IN);
            _codesys_checkBool("RAMP_REAL.RESET", RESET);
        }

        var now = Date.now();

        if (!this._initialized)
        {
            this.OUT = IN;
            this._lastTime = now;
            this._initialized = true;
            return this.OUT;
        }

        // Codesys spec: RESET=true freezes OUT at its current value
        if (RESET)
        {
            this._lastTime = now;
            return this.OUT;
        }

        var diff = IN - this.OUT;

        if (this.TIMEBASE === 0)
        {
            // TIMEBASE=0: ASCEND/DESCEND is max change per call
            if (diff > 0)
                this.OUT += Math.min(diff, this.ASCEND);
            else if (diff < 0)
                this.OUT -= Math.min(-diff, this.DESCEND);
        }
        else
        {
            // TIMEBASE>0: ASCEND/DESCEND is max change per TIMEBASE ms
            var dt = now - this._lastTime;
            this._lastTime = now;

            if (dt <= 0)
                return this.OUT;

            var factor = dt / this.TIMEBASE;

            if (diff > 0)
                this.OUT += Math.min(diff, this.ASCEND * factor);
            else if (diff < 0)
                this.OUT -= Math.min(-diff, this.DESCEND * factor);
        }

        return this.OUT;
    }
}

// --------------------------------------------------------------------------
/**
 * PID — ПИД-регулятор
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Controller/PID.html
 *
 * Y = KP * (e + (1/TN) * integral(e) + TV * de/dt) + Y_OFFSET
 * С ограничением выхода [Y_MIN..Y_MAX] и anti-windup (conditional integration).
 *
 * MANUAL=true: Y = Y_MANUAL (ручной режим, PID не считает).
 * RESET=true: сброс интегратора, Y = Y_OFFSET.
 *
 * @param {number} KP - пропорциональный коэффициент
 * @param {number} TN - время интегрирования (сек), 0 = без интеграла
 * @param {number} TV - время дифференцирования (сек), 0 = без дифференциала
 * @param {number} Y_MIN - нижний предел выхода
 * @param {number} Y_MAX - верхний предел выхода
 */
class PID
{
    constructor(KP, TN, TV, Y_MIN, Y_MAX)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("PID.KP", KP);
            _codesys_checkFiniteNonNegative("PID.TN", TN);
            _codesys_checkFiniteNonNegative("PID.TV", TV);
            _codesys_checkFiniteNumber("PID.Y_MIN", Y_MIN);
            _codesys_checkFiniteNumber("PID.Y_MAX", Y_MAX);
        }

        this.KP = KP;
        this.TN = TN;
        this.TV = TV;
        this.Y_MIN = Y_MIN;
        this.Y_MAX = Y_MAX;
        this.Y_OFFSET = 0;
        this.Y_MANUAL = 0;
        this.MANUAL = false;
        this.Y = 0;
        this.LIMITS_ACTIVE = false;
        this.OVERFLOW = false;
        this._integral = 0;
        this._prevError = 0;
        this._lastTime = 0;
        this._initialized = false;
    }

    /**
     * @param {number} ACTUAL - текущее значение (PV)
     * @param {number} SET_POINT - уставка (SP)
     * @param {boolean} RESET - сброс интегратора
     * @returns {number} Y - управляющее воздействие
     */
    update(ACTUAL, SET_POINT, RESET)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("PID.ACTUAL", ACTUAL);
            _codesys_checkFiniteNumber("PID.SET_POINT", SET_POINT);
            _codesys_checkBool("PID.RESET", RESET);
        }

        var now = Date.now();

        if (RESET || !this._initialized)
        {
            this._integral = 0;
            this._prevError = 0;
            this._lastTime = now;
            this._initialized = true;
            this.Y = this.Y_OFFSET;
            this.LIMITS_ACTIVE = false;
            this.OVERFLOW = false;
            return this.Y;
        }

        // Manual mode: bypass PID, output Y_MANUAL directly
        if (this.MANUAL)
        {
            this.Y = this.Y_MANUAL;
            this._lastTime = now;
            this.LIMITS_ACTIVE = false;
            return this.Y;
        }

        var dt = (now - this._lastTime) / IEC61131_MS_PER_SECOND;
        this._lastTime = now;

        if (dt <= 0)
            return this.Y;

        var error = SET_POINT - ACTUAL;

        // Пропорциональная составляющая
        var p_term = this.KP * error;

        // Дифференциальная составляющая
        var d_term = 0;
        if (this.TV > 0)
        {
            d_term = this.KP * this.TV * (error - this._prevError) / dt;
        }
        this._prevError = error;

        // Интегральная составляющая с conditional anti-windup:
        // НЕ накапливаем если выход уже на пределе И ошибка толкает дальше за предел
        var i_term = 0;
        if (this.TN > 0)
        {
            var y_without_i = p_term + this.KP * (1.0 / this.TN) * this._integral + d_term + this.Y_OFFSET;
            var should_integrate = true;

            // Anti-windup: не интегрируем если на пределе в том же направлении
            if (y_without_i >= this.Y_MAX && error > 0)
                should_integrate = false;
            if (y_without_i <= this.Y_MIN && error < 0)
                should_integrate = false;

            if (should_integrate)
                this._integral += error * dt;

            i_term = this.KP * (1.0 / this.TN) * this._integral;

            // Check overflow
            this.OVERFLOW = !isFinite(this._integral);
            if (this.OVERFLOW)
                this._integral = 0;
        }

        // Суммирование
        var y_raw = p_term + i_term + d_term + this.Y_OFFSET;

        // Ограничение выхода
        if (y_raw > this.Y_MAX)
        {
            this.Y = this.Y_MAX;
            this.LIMITS_ACTIVE = true;
        }
        else if (y_raw < this.Y_MIN)
        {
            this.Y = this.Y_MIN;
            this.LIMITS_ACTIVE = true;
        }
        else
        {
            this.Y = y_raw;
            this.LIMITS_ACTIVE = false;
        }

        return this.Y;
    }
}

// --------------------------------------------------------------------------
/**
 * DERIVATIVE — производная входного сигнала
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Mathematical-Functions/DERIVATIVE.html
 *
 * OUT = dIN/dt. Использует 4-точечный алгоритм для повышения точности:
 *   OUT = (IN[0] + 3*IN[1] - 3*IN[2] - IN[3]) / (6 * TM)
 * где IN[0] — текущее, IN[3] — 3 цикла назад.
 *
 * TM — время с предыдущего вызова (мс), задаётся вызывающим кодом.
 * Если TM=0 — используется Date.now() для автоматического расчёта.
 */
class DERIVATIVE
{
    constructor()
    {
        this.OUT = 0;
        this._buf = _codesys_makeSampleBuffer(0); // newest..oldest
        this._count = 0;
        this._lastTime = 0;
        this._initialized = false;
    }

    /**
     * @param {number} IN - входное значение
     * @param {number} TM - время с предыдущего вызова (мс), 0 = автоматически
     * @param {boolean} RESET - сброс
     * @returns {number} OUT - производная (единиц/сек)
     */
    update(IN, TM, RESET)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("DERIVATIVE.IN", IN);
            _codesys_checkFiniteNonNegative("DERIVATIVE.TM", TM || 0);
            _codesys_checkBool("DERIVATIVE.RESET", RESET);
        }

        var now = Date.now();

        if (RESET || !this._initialized)
        {
            this._buf = _codesys_makeSampleBuffer(IN);
            this._count = 0;
            this._lastTime = now;
            this._initialized = true;
            this.OUT = 0;
            return this.OUT;
        }

        // Shift buffer: oldest out, newest in
        this._buf[3] = this._buf[2];
        this._buf[2] = this._buf[1];
        this._buf[1] = this._buf[0];
        this._buf[0] = IN;
        this._count++;

        // Determine dt in seconds
        var dt_ms;
        if (TM && TM > 0)
        {
            dt_ms = TM;
        }
        else
        {
            dt_ms = now - this._lastTime;
        }
        this._lastTime = now;

        if (dt_ms <= 0)
            return this.OUT;

        var dt_sec = dt_ms / IEC61131_MS_PER_SECOND;

        if (this._count >= IEC61131_DERIVATIVE_READY_SAMPLES)
        {
            // 4-point derivative: (IN[0] + 3*IN[1] - 3*IN[2] - IN[3]) / (6*dt)
            this.OUT = (this._buf[0] + 3 * this._buf[1] - 3 * this._buf[2] - this._buf[3])
                     / (IEC61131_FOUR_POINT_DENOMINATOR * dt_sec);
        }
        else
        {
            // Not enough samples yet: simple first-order
            this.OUT = (this._buf[0] - this._buf[1]) / dt_sec;
        }

        return this.OUT;
    }
}

// --------------------------------------------------------------------------
/**
 * INTEGRAL — интеграл входного сигнала
 * Ref: https://content.helpme-codesys.com/en/libs/Util/Current/Mathematical-Functions/INTEGRAL.html
 *
 * OUT += IN * TM (метод прямоугольников).
 * TM — время с предыдущего вызова (мс), задаётся вызывающим кодом.
 * Если TM=0 — используется Date.now() для автоматического расчёта.
 * OVERFLOW=true если OUT выходит за пределы REAL.
 */
class INTEGRAL
{
    constructor()
    {
        this.OUT = 0;
        this.OVERFLOW = false;
        this._lastTime = 0;
        this._initialized = false;
    }

    /**
     * @param {number} IN - входное значение
     * @param {number} TM - время с предыдущего вызова (мс), 0 = автоматически
     * @param {boolean} RESET - сброс интегратора в 0
     * @returns {number} OUT - интеграл
     */
    update(IN, TM, RESET)
    {
        if (IEC61131_STRICT)
        {
            _codesys_checkFiniteNumber("INTEGRAL.IN", IN);
            _codesys_checkFiniteNonNegative("INTEGRAL.TM", TM || 0);
            _codesys_checkBool("INTEGRAL.RESET", RESET);
        }

        var now = Date.now();

        if (RESET || !this._initialized)
        {
            this.OUT = 0;
            this.OVERFLOW = false;
            this._lastTime = now;
            this._initialized = true;
            return this.OUT;
        }

        // Determine dt in seconds
        var dt_ms;
        if (TM && TM > 0)
        {
            dt_ms = TM;
        }
        else
        {
            dt_ms = now - this._lastTime;
        }
        this._lastTime = now;

        if (dt_ms <= 0)
            return this.OUT;

        var dt_sec = dt_ms / IEC61131_MS_PER_SECOND;
        this.OUT += IN * dt_sec;

        // Overflow check
        this.OVERFLOW = !isFinite(this.OUT);
        if (this.OVERFLOW)
            this.OUT = 0;

        return this.OUT;
    }
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// Debug metadata for uniset2-debug.js visualizer
// --------------------------------------------------------------------------
BLINK._debug_meta = {
    type: "signal", display: "indicator",
    fields: { OUT: { type: "bool", label: "Output" } }
};
HYSTERESIS._debug_meta = {
    type: "analog", display: "indicator",
    fields: { OUT: { type: "bool", label: "Output" } }
};
LIMITALARM._debug_meta = {
    type: "analog", display: "status",
    fields: {
        O:  { type: "bool", label: "Over" },
        U:  { type: "bool", label: "Under" },
        IL: { type: "bool", label: "In Limits" },
    }
};
LIN_TRAFO._debug_meta = {
    type: "math", display: "value",
    fields: {
        OUT:   { type: "real", label: "Output" },
        ERROR: { type: "bool", label: "Error" },
    }
};
RAMP_REAL._debug_meta = {
    type: "math", display: "value",
    fields: { OUT: { type: "real", label: "Output" } }
};
PID._debug_meta = {
    type: "controller", display: "chart",
    fields: {
        Y:             { type: "real", label: "Output" },
        LIMITS_ACTIVE: { type: "bool", label: "Saturated" },
        OVERFLOW:      { type: "bool", label: "Overflow" },
    }
};
DERIVATIVE._debug_meta = {
    type: "math", display: "value",
    fields: { OUT: { type: "real", label: "dIN/dt" } }
};
INTEGRAL._debug_meta = {
    type: "math", display: "value",
    fields: {
        OUT:      { type: "real", label: "Integral" },
        OVERFLOW: { type: "bool", label: "Overflow" },
    }
};

// --------------------------------------------------------------------------
// Export for use in Node.js or other environments
if (typeof module !== 'undefined' && module.exports)
{
    module.exports = { BLINK, HYSTERESIS, LIMITALARM, LIN_TRAFO, RAMP_REAL, PID, DERIVATIVE, INTEGRAL, get IEC61131_STRICT() { return IEC61131_STRICT; }, set IEC61131_STRICT(v) { IEC61131_STRICT = v; } };
}
// --------------------------------------------------------------------------
try
{
    if (typeof BLINK === 'function') globalThis.BLINK = BLINK;
    if (typeof HYSTERESIS === 'function') globalThis.HYSTERESIS = HYSTERESIS;
    if (typeof LIMITALARM === 'function') globalThis.LIMITALARM = LIMITALARM;
    if (typeof LIN_TRAFO === 'function') globalThis.LIN_TRAFO = LIN_TRAFO;
    if (typeof RAMP_REAL === 'function') globalThis.RAMP_REAL = RAMP_REAL;
    if (typeof PID === 'function') globalThis.PID = PID;
    if (typeof DERIVATIVE === 'function') globalThis.DERIVATIVE = DERIVATIVE;
    if (typeof INTEGRAL === 'function') globalThis.INTEGRAL = INTEGRAL;
}
catch (e) {}
// --------------------------------------------------------------------------
