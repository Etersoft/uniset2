/*
 * ESM wrapper for the modbus client helper.
 * Provides the same API as uniset2-modbus-client.js but via ES modules.
 */
const __global = typeof globalThis !== "undefined" ? globalThis : (typeof self !== "undefined" ? self : this);

function buildFactory(target)
{
    const REQUIRED_METHODS =
        ["connectTCP","disconnect","read01","read02","read03","read04",
         "write05","write06","write0F","write10","diag08","read4314"];

    function ensureNative(unisetObj) {
        const u = unisetObj || target.uniset;

        if (!u || !u.modbus)
            throw new Error("uniset2-modbus-client: uniset.modbus is not available");

        const native = u.modbus;

        REQUIRED_METHODS.forEach(function(name) {
            if (typeof native[name] !== "function")
                throw new Error("uniset2-modbus-client: native method '" + name + "' is missing");
        });

        return native;
    }

    function ensureObject(opts, where) {
        if (!opts || typeof opts !== "object")
            throw new TypeError(where + ": options object is required");

        return opts;
    }

    function toNumber(value, where, def) {
        if (value === undefined || value === null) {
            if (def !== undefined) return def;
            throw new TypeError(where + ": value is required");
        }

        const num = Number(value);

        if (!Number.isFinite(num))
            throw new TypeError(where + ": invalid number");

        return num;
    }

    function toUInt(value, where, def) {
        const num = toNumber(value, where, def);

        if (num < 0)
            throw new RangeError(where + ": must be >= 0");

        return Math.floor(num);
    }

    function parseReadOptions(opts, fname) {
        const o = ensureObject(opts, fname);
        return {
            slave: toUInt(o.slave, fname + ".slave"),
            reg: toUInt(o.reg, fname + ".reg"),
            count: toUInt(o.count, fname + ".count", 1)
        };
    }

    function ensureArray(arr, where) {
        if (!Array.isArray(arr))
            throw new TypeError(where + ": expected array");

        if (arr.length === 0)
            throw new RangeError(where + ": array must not be empty");

        return arr;
    }

    return function createModbusClient(unisetObj) {
        const native = ensureNative(unisetObj);

        return {
            connectTCP: function(opts) {
                const o = ensureObject(opts, "connectTCP");
                const host = String(o.host || "");

                if (!host)
                    throw new TypeError("connectTCP.host must be non-empty string");

                const port = toUInt(o.port, "connectTCP.port");
                const timeout = o.timeout === undefined ? undefined : toUInt(o.timeout, "connectTCP.timeout");
                const force = !!o.forceDisconnect;
                return native.connectTCP(host, port, timeout, force);
            },

            disconnect: function() {
                return native.disconnect();
            },

            read01: function(opts) {
                const p = parseReadOptions(opts, "read01");
                return native.read01(p.slave, p.reg, p.count);
            },

            read02: function(opts) {
                const p = parseReadOptions(opts, "read02");
                return native.read02(p.slave, p.reg, p.count);
            },

            read03: function(opts) {
                const p = parseReadOptions(opts, "read03");
                return native.read03(p.slave, p.reg, p.count);
            },

            read04: function(opts) {
                const p = parseReadOptions(opts, "read04");
                return native.read04(p.slave, p.reg, p.count);
            },

            write05: function(opts) {
                const o = ensureObject(opts, "write05");
                const slave = toUInt(o.slave, "write05.slave");
                const reg = toUInt(o.reg, "write05.reg");
                const value = !!o.value;
                return native.write05(slave, reg, value);
            },

            write06: function(opts) {
                const o = ensureObject(opts, "write06");
                const slave = toUInt(o.slave, "write06.slave");
                const reg = toUInt(o.reg, "write06.reg");
                const value = toUInt(o.value, "write06.value");
                return native.write06(slave, reg, value);
            },

            write0F: function(opts) {
                const o = ensureObject(opts, "write0F");
                const slave = toUInt(o.slave, "write0F.slave");
                const start = toUInt(o.reg || o.start, "write0F.reg");
                const values = ensureArray(o.values, "write0F.values")
                    .map(function(v) { return v ? 1 : 0; });
                return native.write0F(slave, start, values);
            },

            write10: function(opts) {
                const o = ensureObject(opts, "write10");
                const slave = toUInt(o.slave, "write10.slave");
                const start = toUInt(o.reg || o.start, "write10.reg");
                const values = ensureArray(o.values, "write10.values")
                    .map(function(v, idx) {
                        return toUInt(v, "write10.values[" + idx + "]");
                    });
                return native.write10(slave, start, values);
            },

            diag08: function(opts) {
                const o = ensureObject(opts, "diag08");
                const slave = toUInt(o.slave, "diag08.slave");
                const subfunc = toUInt(o.subfunc, "diag08.subfunc");
                const data = o.data === undefined ? 0 : toUInt(o.data, "diag08.data");
                return native.diag08(slave, subfunc, data);
            },

            read4314: function(opts) {
                const o = ensureObject(opts, "read4314");
                const slave = toUInt(o.slave, "read4314.slave");
                const devID = toUInt(o.devID || o.deviceId, "read4314.devID");
                const objID = toUInt(o.objID || o.objectId, "read4314.objID");
                return native.read4314(slave, devID, objID);
            }
        };
    };
}

const factory = __global.__uniset_modbus_client_factory || buildFactory(__global);
__global.__uniset_modbus_client_factory = factory;

export const createModbusClient = factory;
export const modbusClient = createModbusClient();
