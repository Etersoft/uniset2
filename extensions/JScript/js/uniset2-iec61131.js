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
var unisetIEC61131ModuleVersion = "v1"
// --------------------------------------------------------------------------
/**
 * Strict mode flag. When true, constructors and update() validate all parameters.
 * Enable for development/debugging, disable for production (zero overhead).
 *
 * Usage:
 *   IEC61131_STRICT = true;  // before creating blocks
 */
var IEC61131_STRICT = false;

function _iec_checkBool(name, value)
{
    if (typeof value !== 'boolean')
        throw new TypeError("IEC61131 [" + name + "]: expected boolean, got " + typeof value + " (" + value + ")");
}

function _iec_checkFinitePositive(name, value)
{
    if (typeof value !== 'number' || !isFinite(value) || value < 0)
        throw new RangeError("IEC61131 [" + name + "]: expected non-negative finite number, got " + value);
}

function _iec_checkFiniteInt(name, value)
{
    if (typeof value !== 'number' || !isFinite(value) || value !== Math.floor(value))
        throw new RangeError("IEC61131 [" + name + "]: expected finite integer, got " + value);
}
// --------------------------------------------------------------------------
/**
 * IEC 61131-3 Standard Function Blocks
 * Reference: IEC 61131-3:2013, Edition 3, Tables 36-45
 *
 * Bistable (Tables 36-37):   RS, SR
 * Edge detection (Tables 38-39): R_TRIG, F_TRIG
 * Counters (Tables 40-42):   CTU, CTD, CTUD
 * Timers (Tables 43-45):     TON, TOF, TP
 *
 * All blocks follow IEC 61131-3 semantics.
 * Timer blocks use Date.now() for time tracking and must be called
 * cyclically (e.g. from uniset_on_step()).
 *
 * Usage:
 *   load("uniset2-iec61131.js");
 *   const ton = new TON(3000);  // 3s on-delay
 *   function uniset_on_step() {
 *       ton.update(in_Sensor1);
 *       out_Output1 = ton.Q ? 1 : 0;
 *   }
 */
// --------------------------------------------------------------------------

/**
 * RS - Reset-dominant bistable (IEC 61131-3, Table 36)
 *
 * Body: Q1 := NOT RESET1 AND (SET OR Q1)
 *
 * Truth table:
 *   SET  RESET1  Q1
 *    0     0     Q1 (no change)
 *    1     0     1
 *    0     1     0
 *    1     1     0  (reset dominant)
 */
class RS
{
    constructor()
    {
        this.Q1 = false;
    }

    /**
     * @param {boolean} SET - set input
     * @param {boolean} RESET1 - reset input (dominant)
     * @returns {boolean} Q1
     */
    update(SET, RESET1)
    {
        if (IEC61131_STRICT)
        {
            _iec_checkBool("RS.SET", SET);
            _iec_checkBool("RS.RESET1", RESET1);
        }

        this.Q1 = !RESET1 && (SET || this.Q1);
        return this.Q1;
    }
}

// --------------------------------------------------------------------------
/**
 * SR - Set-dominant bistable (IEC 61131-3, Table 37)
 *
 * Body: Q1 := SET1 OR (NOT RESET AND Q1)
 *
 * Truth table:
 *   SET1  RESET  Q1
 *    0      0    Q1 (no change)
 *    1      0    1
 *    0      1    0
 *    1      1    1  (set dominant)
 */
class SR
{
    constructor()
    {
        this.Q1 = false;
    }

    /**
     * @param {boolean} SET1 - set input (dominant)
     * @param {boolean} RESET - reset input
     * @returns {boolean} Q1
     */
    update(SET1, RESET)
    {
        if (IEC61131_STRICT)
        {
            _iec_checkBool("SR.SET1", SET1);
            _iec_checkBool("SR.RESET", RESET);
        }

        this.Q1 = SET1 || (!RESET && this.Q1);
        return this.Q1;
    }
}

// --------------------------------------------------------------------------
/**
 * R_TRIG - Rising edge detector (IEC 61131-3, Table 38)
 *
 * Body: Q := CLK AND NOT M; M := CLK;
 * Q is true for one cycle when CLK transitions from false to true.
 */
class R_TRIG
{
    constructor()
    {
        this.CLK_prev = false;
        this.Q = false;
    }

    /**
     * @param {boolean} CLK - input signal
     * @returns {boolean} Q - true on rising edge
     */
    update(CLK)
    {
        if (IEC61131_STRICT)
            _iec_checkBool("R_TRIG.CLK", CLK);

        this.Q = CLK && !this.CLK_prev;
        this.CLK_prev = CLK;
        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * F_TRIG - Falling edge detector (IEC 61131-3, Table 39)
 *
 * Body: Q := NOT CLK AND M; M := CLK;
 * Q is true for one cycle when CLK transitions from true to false.
 */
class F_TRIG
{
    constructor()
    {
        this.CLK_prev = false;
        this.Q = false;
    }

    /**
     * @param {boolean} CLK - input signal
     * @returns {boolean} Q - true on falling edge
     */
    update(CLK)
    {
        if (IEC61131_STRICT)
            _iec_checkBool("F_TRIG.CLK", CLK);

        this.Q = !CLK && this.CLK_prev;
        this.CLK_prev = CLK;
        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * CTU - Up counter (IEC 61131-3, Table 40)
 *
 * Counts rising edges on CU. Resets to 0 on RESET.
 * Q is true when CV >= PV.
 *
 * Body:
 *   IF RESET THEN CV := 0;
 *   ELSIF CU THEN CV := CV + 1;
 *   END_IF;
 *   Q := (CV >= PV);
 *
 * @param {number} PV - preset value (count target)
 */
class CTU
{
    constructor(PV)
    {
        if (IEC61131_STRICT)
            _iec_checkFiniteInt("CTU.PV", PV);

        this.PV = PV;
        this.CV = 0;
        this.Q = false;
        this._prevCU = false;
    }

    /**
     * @param {boolean} CU - count up input (counts on rising edge)
     * @param {boolean} RESET - reset counter to 0
     * @returns {boolean} Q - true when CV >= PV
     */
    update(CU, RESET)
    {
        if (IEC61131_STRICT)
        {
            _iec_checkBool("CTU.CU", CU);
            _iec_checkBool("CTU.RESET", RESET);
        }

        if (RESET)
        {
            this.CV = 0;
        }
        else if (CU && !this._prevCU)
        {
            this.CV++;
        }

        this._prevCU = CU;
        this.Q = this.CV >= this.PV;
        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * CTD - Down counter (IEC 61131-3, Table 41)
 *
 * Counts down on rising edges of CD. Loads PV on LOAD.
 * Q is true when CV <= 0.
 *
 * Body:
 *   IF LOAD THEN CV := PV;
 *   ELSIF CD AND (CV > 0) THEN CV := CV - 1;
 *   END_IF;
 *   Q := (CV <= 0);
 *
 * @param {number} PV - preset value (initial count)
 */
class CTD
{
    constructor(PV)
    {
        if (IEC61131_STRICT)
            _iec_checkFiniteInt("CTD.PV", PV);

        this.PV = PV;
        this.CV = 0;
        this.Q = true;
        this._prevCD = false;
    }

    /**
     * @param {boolean} CD - count down input (counts on rising edge)
     * @param {boolean} LOAD - load PV into CV
     * @returns {boolean} Q - true when CV <= 0
     */
    update(CD, LOAD)
    {
        if (IEC61131_STRICT)
        {
            _iec_checkBool("CTD.CD", CD);
            _iec_checkBool("CTD.LOAD", LOAD);
        }

        if (LOAD)
        {
            this.CV = this.PV;
        }
        else if (CD && !this._prevCD)
        {
            if (this.CV > 0)
                this.CV--;
        }

        this._prevCD = CD;
        this.Q = this.CV <= 0;
        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * CTUD - Up/Down counter (IEC 61131-3, Table 42)
 *
 * Counts up on CU rising edge, down on CD rising edge.
 * QU is true when CV >= PV, QD is true when CV <= 0.
 * RESET sets CV=0, LOAD sets CV=PV. RESET has priority over LOAD.
 *
 * Body:
 *   IF RESET THEN CV := 0;
 *   ELSIF LOAD THEN CV := PV;
 *   ELSE
 *     IF CU AND (CV < PVmax) THEN CV := CV + 1; END_IF;
 *     IF CD AND (CV > 0) THEN CV := CV - 1; END_IF;
 *   END_IF;
 *   QU := (CV >= PV);
 *   QD := (CV <= 0);
 *
 * @param {number} PV - preset value
 */
class CTUD
{
    constructor(PV)
    {
        if (IEC61131_STRICT)
            _iec_checkFiniteInt("CTUD.PV", PV);

        this.PV = PV;
        this.CV = 0;
        this.QU = false;
        this.QD = true;
        this._prevCU = false;
        this._prevCD = false;
    }

    /**
     * @param {boolean} CU - count up (rising edge)
     * @param {boolean} CD - count down (rising edge)
     * @param {boolean} RESET - reset CV to 0
     * @param {boolean} LOAD - load PV into CV
     * @returns {boolean} QU (also read .QU, .QD, .CV as properties)
     */
    update(CU, CD, RESET, LOAD)
    {
        if (IEC61131_STRICT)
        {
            _iec_checkBool("CTUD.CU", CU);
            _iec_checkBool("CTUD.CD", CD);
            _iec_checkBool("CTUD.RESET", RESET);
            _iec_checkBool("CTUD.LOAD", LOAD);
        }

        if (RESET)
        {
            this.CV = 0;
        }
        else if (LOAD)
        {
            this.CV = this.PV;
        }
        else
        {
            if (CU && !this._prevCU)
                this.CV++;

            if (CD && !this._prevCD)
            {
                if (this.CV > 0)
                    this.CV--;
            }
        }

        this._prevCU = CU;
        this._prevCD = CD;
        this.QU = this.CV >= this.PV;
        this.QD = this.CV <= 0;
        return this.QU;
    }
}

// --------------------------------------------------------------------------
/**
 * TON - On-delay timer (IEC 61131-3, Table 43)
 *
 * When IN becomes true, Q becomes true after PT milliseconds.
 * When IN becomes false, Q immediately becomes false and ET resets to 0.
 * ET counts elapsed time while IN is true (capped at PT).
 *
 * Timing diagram:
 *   IN:  ___/‾‾‾‾‾‾‾‾‾‾‾‾\___
 *   Q:   _________/‾‾‾‾‾‾\___
 *   ET:  ___/‾‾‾‾‾|PT|___\___
 *            <-PT->
 *
 * Must be called cyclically (e.g. from uniset_on_step()).
 *
 * @param {number} PT - preset time in milliseconds
 */
class TON
{
    constructor(PT)
    {
        if (IEC61131_STRICT)
            _iec_checkFinitePositive("TON.PT", PT);

        this.PT = PT;
        this.Q = false;
        this.ET = 0;
        this._running = false;
        this._startTime = 0;
    }

    /**
     * @param {boolean} IN - enable input
     * @returns {boolean} Q - output (true after delay)
     */
    update(IN)
    {
        if (IEC61131_STRICT)
            _iec_checkBool("TON.IN", IN);

        if (IN)
        {
            if (!this._running)
            {
                this._running = true;
                this._startTime = Date.now();
            }

            this.ET = Date.now() - this._startTime;

            if (this.ET >= this.PT)
            {
                this.ET = this.PT;
                this.Q = true;
            }
        }
        else
        {
            this.Q = false;
            this.ET = 0;
            this._running = false;
        }

        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * TOF - Off-delay timer (IEC 61131-3, Table 44)
 *
 * When IN becomes true, Q immediately becomes true.
 * When IN becomes false, Q remains true for PT milliseconds, then becomes false.
 * ET counts elapsed time after IN goes false (capped at PT).
 *
 * Timing diagram:
 *   IN:  ___/‾‾‾‾‾‾\____________
 *   Q:   ___/‾‾‾‾‾‾‾‾‾‾‾‾\______
 *   ET:  ___________/‾‾‾‾‾|PT|___
 *                    <-PT->
 *
 * Must be called cyclically (e.g. from uniset_on_step()).
 *
 * @param {number} PT - preset time in milliseconds
 */
class TOF
{
    constructor(PT)
    {
        if (IEC61131_STRICT)
            _iec_checkFinitePositive("TOF.PT", PT);

        this.PT = PT;
        this.Q = false;
        this.ET = 0;
        this._running = false;
        this._startTime = 0;
    }

    /**
     * @param {boolean} IN - enable input
     * @returns {boolean} Q - output (delayed off)
     */
    update(IN)
    {
        if (IEC61131_STRICT)
            _iec_checkBool("TOF.IN", IN);

        if (IN)
        {
            this.Q = true;
            this.ET = 0;
            this._running = false;
        }
        else
        {
            if (this.Q)
            {
                if (!this._running)
                {
                    this._running = true;
                    this._startTime = Date.now();
                }

                this.ET = Date.now() - this._startTime;

                if (this.ET >= this.PT)
                {
                    this.ET = this.PT;
                    this.Q = false;
                    this._running = false;
                }
            }
        }

        return this.Q;
    }
}

// --------------------------------------------------------------------------
/**
 * TP - Pulse timer (IEC 61131-3, Table 45)
 *
 * On rising edge of IN, Q becomes true for exactly PT milliseconds.
 * Once started, the pulse runs to completion regardless of IN changes.
 * ET counts elapsed time during the pulse (capped at PT).
 * ET resets to 0 when IN returns to false after pulse completion.
 * A new pulse cannot start until the current one has completed AND IN has returned to false.
 *
 * Timing diagram:
 *   IN:  ___/‾‾‾‾‾‾‾‾‾‾‾‾\___/‾‾‾\___
 *   Q:   ___/‾‾‾‾‾‾\__________/‾‾‾‾‾‾\
 *   ET:  ___/‾‾‾‾‾‾|PT|___\___/‾‾‾‾‾‾|
 *            <-PT->             <-PT->
 *
 * Must be called cyclically (e.g. from uniset_on_step()).
 *
 * @param {number} PT - pulse duration in milliseconds
 */
class TP
{
    constructor(PT)
    {
        if (IEC61131_STRICT)
            _iec_checkFinitePositive("TP.PT", PT);

        this.PT = PT;
        this.Q = false;
        this.ET = 0;
        this._running = false;
        this._startTime = 0;
        this._prevIN = false;
    }

    /**
     * @param {boolean} IN - trigger input
     * @returns {boolean} Q - pulse output
     */
    update(IN)
    {
        if (IEC61131_STRICT)
            _iec_checkBool("TP.IN", IN);

        if (this._running)
        {
            this.ET = Date.now() - this._startTime;

            if (this.ET >= this.PT)
            {
                this.ET = this.PT;
                this.Q = false;
                this._running = false;
            }
        }
        else if (IN && !this._prevIN)
        {
            this.Q = true;
            this._running = true;
            this._startTime = Date.now();
            this.ET = 0;
        }
        else if (!IN)
        {
            this.ET = 0;
        }

        this._prevIN = IN;
        return this.Q;
    }
}

// --------------------------------------------------------------------------
// Debug metadata for uniset2-debug.js visualizer
// --------------------------------------------------------------------------
RS._debug_meta = {
    type: "bistable", display: "indicator",
    fields: { Q1: { type: "bool", label: "Output" } }
};
SR._debug_meta = {
    type: "bistable", display: "indicator",
    fields: { Q1: { type: "bool", label: "Output" } }
};
R_TRIG._debug_meta = {
    type: "edge", display: "indicator",
    fields: { Q: { type: "bool", label: "Pulse" } }
};
F_TRIG._debug_meta = {
    type: "edge", display: "indicator",
    fields: { Q: { type: "bool", label: "Pulse" } }
};
CTU._debug_meta = {
    type: "counter", display: "gauge",
    fields: {
        Q:  { type: "bool", label: "Reached" },
        CV: { type: "int",  label: "Count" },
        PV: { type: "int",  label: "Preset", readonly: true },
    }
};
CTD._debug_meta = {
    type: "counter", display: "gauge",
    fields: {
        Q:  { type: "bool", label: "Zero" },
        CV: { type: "int",  label: "Count" },
        PV: { type: "int",  label: "Preset", readonly: true },
    }
};
CTUD._debug_meta = {
    type: "counter", display: "gauge",
    fields: {
        QU: { type: "bool", label: "Up reached" },
        QD: { type: "bool", label: "Down zero" },
        CV: { type: "int",  label: "Count" },
        PV: { type: "int",  label: "Preset", readonly: true },
    }
};
TON._debug_meta = {
    type: "timer", display: "progress",
    fields: {
        Q:  { type: "bool", label: "Output" },
        ET: { type: "time", label: "Elapsed", unit: "ms" },
        PT: { type: "time", label: "Preset", unit: "ms", readonly: true },
    }
};
TOF._debug_meta = {
    type: "timer", display: "progress",
    fields: {
        Q:  { type: "bool", label: "Output" },
        ET: { type: "time", label: "Elapsed", unit: "ms" },
        PT: { type: "time", label: "Preset", unit: "ms", readonly: true },
    }
};
TP._debug_meta = {
    type: "timer", display: "progress",
    fields: {
        Q:  { type: "bool", label: "Pulse" },
        ET: { type: "time", label: "Elapsed", unit: "ms" },
        PT: { type: "time", label: "Preset", unit: "ms", readonly: true },
    }
};

// --------------------------------------------------------------------------
// Export for use in Node.js or other environments
if (typeof module !== 'undefined' && module.exports)
{
    module.exports = { RS, SR, R_TRIG, F_TRIG, CTU, CTD, CTUD, TON, TOF, TP, get IEC61131_STRICT() { return IEC61131_STRICT; }, set IEC61131_STRICT(v) { IEC61131_STRICT = v; } };
}
// --------------------------------------------------------------------------
try
{
    if (typeof RS === 'function') globalThis.RS = RS;
    if (typeof SR === 'function') globalThis.SR = SR;
    if (typeof R_TRIG === 'function') globalThis.R_TRIG = R_TRIG;
    if (typeof F_TRIG === 'function') globalThis.F_TRIG = F_TRIG;
    if (typeof CTU === 'function') globalThis.CTU = CTU;
    if (typeof CTD === 'function') globalThis.CTD = CTD;
    if (typeof CTUD === 'function') globalThis.CTUD = CTUD;
    if (typeof TON === 'function') globalThis.TON = TON;
    if (typeof TOF === 'function') globalThis.TOF = TOF;
    if (typeof TP === 'function') globalThis.TP = TP;
}
catch (e) {}
// --------------------------------------------------------------------------
