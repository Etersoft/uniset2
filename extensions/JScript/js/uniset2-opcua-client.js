/*
 * Lightweight helper around native uniset.opcua bindings.
 * Provides convenient object-based API for connect/read/write operations.
 * Compatible with load() usage.
 */
(function(global) {
    'use strict';

    function buildFactory(target) {
        const REQUIRED_METHODS = ["connect", "disconnect", "read", "write"];

        function ensureNative(unisetObj) {
            const u = unisetObj || target.uniset;

            if (!u || !u.opcua)
                throw new Error("uniset2-opcua-client: uniset.opcua is not available");

            const native = u.opcua;

            REQUIRED_METHODS.forEach(function(name) {
                if (typeof native[name] !== "function")
                    throw new Error("uniset2-opcua-client: native method '" + name + "' is missing");
            });

            return native;
        }

        function ensureObject(opts, where) {
            if (!opts || typeof opts !== "object" || Array.isArray(opts))
                throw new TypeError(where + ": options object is required");

            return opts;
        }

        function isPlainObject(value) {
            return value && typeof value === "object" && !Array.isArray(value);
        }

        function normalizeEndpointOptions(opts) {
            const o = ensureObject(opts, "connect");
            const endpoint = String(o.endpoint || o.url || o.address || "");

            if (!endpoint)
                throw new TypeError("connect.endpoint must be non-empty string");

            const user = o.user !== undefined ? o.user : o.username;
            const pass = o.pass !== undefined ? o.pass : o.password;

            return { endpoint, user, pass };
        }

        function normalizeNodeList(arg, where) {
            if (typeof arg === "string") {
                const id = arg.trim();

                if (!id)
                    throw new TypeError(where + ": nodeId must be non-empty string");

                return [id];
            }

            if (Array.isArray(arg)) {
                if (arg.length === 0)
                    throw new TypeError(where + ": nodeId list must not be empty");

                return arg.map(function(item, idx) {
                    if (typeof item !== "string" || !item.trim())
                        throw new TypeError(where + ": nodeId[" + idx + "] must be non-empty string");

                    return item;
                });
            }

            throw new TypeError(where + ": nodeId must be string or array of strings");
        }

        function normalizeWriteItem(item, idx) {
            const where = idx !== undefined ? "write[" + idx + "]" : "write";

            if (!item || typeof item !== "object")
                throw new TypeError(where + ": item must be object");

            const nodeId = String(item.nodeId || item.id || "");

            if (!nodeId)
                throw new TypeError(where + ": nodeId is required");

            if (item.value === undefined)
                throw new TypeError(where + ": value is required");

            const normalized = { nodeId: nodeId, value: item.value };

            if (item.type !== undefined)
                normalized.type = String(item.type);

            return normalized;
        }

        function normalizeWriteItems(arg, maybeValue, maybeType) {
            if (Array.isArray(arg))
                return arg.map(normalizeWriteItem);

            if (isPlainObject(arg)) {
                if ("nodeId" in arg || "id" in arg)
                    return [normalizeWriteItem(arg)];

                return Object.keys(arg).map(function(key, idx) {
                    return normalizeWriteItem({ nodeId: key, value: arg[key] }, idx);
                });
            }

            if (typeof arg === "string")
                return [normalizeWriteItem({ nodeId: arg, value: maybeValue, type: maybeType })];

            throw new TypeError("write: expected (nodeId, value[, type]) or array/object");
        }

        return function createOpcuaClient(unisetObj) {
            const native = ensureNative(unisetObj);

            function callNativeRead(nodes) {
                const payload = nodes.length === 1 ? nodes[0] : nodes;
                const raw = native.read(payload);
                return Array.isArray(raw) ? raw : [raw];
            }

            return {
                connect: function(opts) {
                    const params = normalizeEndpointOptions(opts || {});
                    if (params.user !== undefined || params.pass !== undefined)
                        return native.connect(params.endpoint, params.user || "", params.pass || "");

                    return native.connect(params.endpoint);
                },

                disconnect: function() {
                    return native.disconnect();
                },

                read: function(nodeIds, options) {
                    const list = normalizeNodeList(nodeIds, "read");
                    const raw = callNativeRead(list);

                    if (options && options.asMap) {
                        const map = {};
                        raw.forEach(function(item, idx) {
                            map[list[idx]] = item;
                        });
                        return map;
                    }

                    if (list.length === 1 && !(options && options.keepArray))
                        return raw[0];

                    return raw;
                },

                readValues: function(nodeIds, options) {
                    const list = normalizeNodeList(nodeIds, "readValues");
                    const raw = callNativeRead(list);
                    const values = raw.map(function(item) {
                        return item && item.ok ? item.value : undefined;
                    });

                    if (options && options.asMap) {
                        const map = {};
                        values.forEach(function(val, idx) {
                            map[list[idx]] = val;
                        });
                        return map;
                    }

                    if (list.length === 1 && !(options && options.keepArray))
                        return values[0];

                    return values;
                },

                write: function(items, value, type) {
                    const payload = normalizeWriteItems(items, value, type);
                    return native.write(payload);
                }
            };
        };
    }

    const factory = buildFactory(global);
    global.__uniset_opcua_client_factory = factory;

    if (typeof module !== "undefined" && module.exports)
        module.exports = { createOpcuaClient: factory };

    if (!global.uniset_createOpcuaClient)
        global.uniset_createOpcuaClient = factory;

})(typeof globalThis !== "undefined" ? globalThis : this);
