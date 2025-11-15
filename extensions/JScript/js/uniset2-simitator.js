/*
 * JavaScript port of Utilities/SImitator.
 * Generates values for the selected sensors in a loop and writes them via ui.setValue().
 */
(function(global) {
    'use strict';

    const DEFAULTS = {
        sensors: [],
        sid: '',
        min: 0,
        max: 100,
        step: 1,
        pause: 200,
        func: 'none',
        logger: null
    };

    const FUNC_NONE = 'none';
    const FUNC_SIN = 'sin';
    const FUNC_COS = 'cos';
    const FUNC_RANDOM = 'random';
    const SUPPORTED_FUNCS = new Set([FUNC_NONE, FUNC_SIN, FUNC_COS, FUNC_RANDOM]);

    function getLogger(custom) {
        if (custom && typeof custom.info === 'function') {
            return custom;
        }
        return {
            info: function() {},
            warn: function() {
                if (global.console && typeof global.console.warn === 'function')
                    global.console.warn.apply(global.console, arguments);
            },
            error: function() {
                if (global.console && typeof global.console.error === 'function')
                    global.console.error.apply(global.console, arguments);
            },
            debug: function() {}
        };
    }

    function ensureUi() {
        if (!global.ui || typeof global.ui.setValue !== 'function')
            throw new Error("uniset2-simitator: global 'ui' object with setValue() is required");

        return global.ui;
    }

    function parseSidList(listStr) {
        if (!listStr || typeof listStr !== 'string')
            return [];

        return listStr.split(',').map(function(token) {
            const trimmed = token.trim();

            if (!trimmed)
                return null;

            const atIndex = trimmed.indexOf('@');
            const pure = atIndex >= 0 ? trimmed.slice(0, atIndex) : trimmed;

            if (/^-?\d+$/.test(pure))
            {
                return Number(pure);
            }

            return pure;
        }).filter(function(entry) {
            return entry !== null && entry !== undefined && entry !== '';
        });
    }

    function normalizeSensors(opts) {
        let sensors = [];

        if (Array.isArray(opts.sensors) && opts.sensors.length)
            sensors = opts.sensors.slice();
        else if (opts.sid)
            sensors = parseSidList(opts.sid);

        sensors = sensors.filter(function(item) {
            return typeof item === 'string' || typeof item === 'number';
        });

        if (!sensors.length)
            throw new Error("uniset2-simitator: sensor list is empty (specify sensors array, sid or filterField)");

        return sensors;
    }

    function clampRange(minVal, maxVal) {
        if (minVal > maxVal)
            return { min: maxVal, max: minVal };

        return { min: minVal, max: maxVal };
    }

    function ensureStepCallbackSupport() {
        if (!global.uniset || typeof global.uniset.step_cb !== 'function')
            throw new Error("uniset2-simitator: uniset.step_cb() is not available");
    }

    function Simitator(ui, sensors, opts) {
        this.ui = ui;
        this.sensors = sensors;
        this.config = opts;
        this.logger = getLogger(opts.logger);
        this.direction = 1;
        this.currentValue = opts.min;
        this.stepHandler = null;
        this.running = false;
        this.lastTickTime = Date.now();
        this.randomFn = null;
        this.customFn = typeof opts.func === 'function' ? opts.func : null;

        if (opts.func === FUNC_RANDOM)
        {
            this.randomFn = function() {
                const min = opts.min;
                const max = opts.max;
                const span = max - min + 1;
                return min + Math.floor(Math.random() * span);
            };
        }

        this.logger.info("[simitator] sensors=%s min=%d max=%d step=%d pause=%d func=%s",
                         sensors.join(','), opts.min, opts.max, opts.step, opts.pause, opts.func);
    }

    Simitator.prototype.isRunning = function() {
        return this.running;
    };

    Simitator.prototype.start = function() {
        if (this.isRunning())
            return this;

        ensureStepCallbackSupport();

        if (!this.stepHandler)
        {
            const self = this;
            this.stepHandler = function() {
                self.onStep();
            };
            global.uniset.step_cb(this.stepHandler);
        }

        this.lastTickTime = Date.now();
        this.running = true;
        return this;
    };

    Simitator.prototype.stop = function() {
        this.running = false;
        return this;
    };

    Simitator.prototype.onStep = function() {
        if (!this.running)
            return;

        const now = Date.now();

        if (now - this.lastTickTime < this.config.pause)
            return;

        this.lastTickTime = now;
        this.tick();
    };

    Simitator.prototype.tick = function() {
        const baseValue = this.randomFn ? this.randomFn() : this.nextValue();
        const finalValue = this.applyFunction(baseValue);
        this.broadcast(finalValue);
    };

    Simitator.prototype.nextValue = function() {
        if (this.direction > 0)
        {
            this.currentValue += this.config.step;

            if (this.currentValue >= this.config.max)
            {
                this.currentValue = this.config.max;
                this.direction = -1;
            }
        }
        else
        {
            this.currentValue -= this.config.step;

            if (this.currentValue <= this.config.min)
            {
                this.currentValue = this.config.min;
                this.direction = 1;
            }
        }

        return this.currentValue;
    };

    Simitator.prototype.applyFunction = function(value) {
        if (this.customFn)
        {
            try
            {
                return this.customFn(value);
            }
            catch (err)
            {
                this.logger.error("[simitator] custom apply() failed: %s", err && err.message ? err.message : err);
                return value;
            }
        }

        switch(this.config.func)
        {
            case FUNC_SIN: return Math.round(value * Math.sin(value));
            case FUNC_COS: return Math.round(value * Math.cos(value));
            case FUNC_RANDOM: return value;
            default: return value;
        }
    };

    Simitator.prototype.broadcast = function(value) {
        var self = this;
        this.sensors.forEach(function(sensor) {
            try
            {
                self.ui.setValue(sensor, value);
            }
            catch (err)
            {
                self.logger.warn("[simitator] failed to set value for %s: %s", sensor, err && err.message ? err.message : err);
            }
        });
    };

    function createSimitator(userOptions) {
        const opts = Object.assign({}, DEFAULTS, userOptions || {});

        if (opts.step <= 0)
            throw new Error("uniset2-simitator: step must be > 0");

        if (opts.pause <= 10)
            throw new Error("uniset2-simitator: pause must be greater than 10 ms");

        if (typeof opts.func !== 'function' && !SUPPORTED_FUNCS.has(opts.func))
            throw new Error("uniset2-simitator: unsupported func '" + opts.func + "'");

        const range = clampRange(opts.min, opts.max);
        opts.min = range.min;
        opts.max = range.max;

        const sensors = normalizeSensors(opts);
        const ui = ensureUi();
        const sim = new Simitator(ui, sensors, opts);

        return sim;
    }

    global.createSimitator = createSimitator;

    if (typeof module !== 'undefined' && module.exports)
        module.exports = { createSimitator: createSimitator };

})(typeof globalThis !== 'undefined' ? globalThis : this);
