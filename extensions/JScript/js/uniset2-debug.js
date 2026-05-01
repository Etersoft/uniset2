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
// uniset2-debug.js -- Debug visualizer runtime module for JScript
//
// Provides:
//   - uniset_debug_start(port, options) -- start debug HTTP server
//   - uniset_debug_watch(name, getter) -- manually register a watched variable
//   - _dbg_if(line, cond) / _dbg_case(line, value) -- trace instrumentation
//   - _dbg_begin_cycle() / _dbg_end_cycle() -- cycle trace boundaries
//   - HTTP endpoints under /debug/ prefix
// --------------------------------------------------------------------------
var unisetDebugModuleVersion = "v1";
// --------------------------------------------------------------------------
(function (g) {
    'use strict';

    // ---- Configuration defaults ----
    var DEFAULT_PORT = 8088;           // non-standard port to avoid conflicts with common services
    var DEFAULT_HISTORY_DEPTH = 1000;  // balance between memory usage and useful trend depth
    var DEFAULT_TRACE_ENABLED = true;

    // Common FB output properties for fallback enumeration (no _debug_meta)
    var COMMON_FB_PROPS = ["Q", "Q1", "QU", "QD", "CV", "ET", "PT", "PV", "OUT", "Y"];

    // ---- UI HTML content ----
    // Auto-load via loadFile() (searches configured module paths),
    // or set externally via uniset_debug_set_ui(htmlString).
    var _uiHtml = g._debug_ui_html || null;

    var _uiFallbackHtml =
        '<!DOCTYPE html><html><head><title>UniSet2 Debug UI</title></head>' +
        '<body style="background:#1a1a2e;color:#e0e0e0;font-family:sans-serif;padding:40px">' +
        '<h1>UniSet2 Debug Visualizer</h1>' +
        '<p>UI HTML not loaded. Serve <code>uniset2-debug-ui.html</code> ' +
        'from the same directory, or set <code>_debug_ui_html</code> before calling ' +
        '<code>uniset_debug_start()</code>.</p>' +
        '<p>API endpoints available: ' +
        '<a href="/debug/snapshot" style="color:#00d4ff">/debug/snapshot</a>, ' +
        '<a href="/debug/info" style="color:#00d4ff">/debug/info</a></p>' +
        '</body></html>';

    if (!_uiHtml && typeof loadFile === 'function')
    {
        try
        {
            _uiHtml = loadFile("uniset2-debug-ui.html");
        }
        catch (e) { /* ignore — HTML will be loaded on demand or not available */ }
    }

    // ---- Internal state ----
    var _config = {
        history_depth: DEFAULT_HISTORY_DEPTH,
        trace_enabled: DEFAULT_TRACE_ENABLED
    };

    var _cycle = 0;
    var _startTime = Date.now();
    var _lastStepTs = 0;
    var _dtMs = 0;
    var _started = false;

    // Force override map: varName -> forcedValue
    var _forced = {};

    // Manually watched variables: name -> getter function
    var _watched = {};

    // Trace buffer for current cycle
    var _trace = [];

    // Ring buffer for history
    var _ringBuffer = [];
    var _ringIndex = 0;
    var _ringFilled = false;

    // Last snapshot cycle for cycles_missed calculation
    var _lastSnapshotCycle = 0;

    // ---- Query string parser ----
    function parseQuery(qs) {
        var out = {};
        if (!qs) return out;
        var parts = qs.split('&');
        for (var i = 0; i < parts.length; i++) {
            var pair = parts[i];
            if (!pair) continue;
            var eqIdx = pair.indexOf('=');
            var key = eqIdx >= 0 ? decodeURIComponent(pair.slice(0, eqIdx)) : decodeURIComponent(pair);
            var val = eqIdx >= 0 ? decodeURIComponent(pair.slice(eqIdx + 1)) : '';
            out[key] = val;
        }
        return out;
    }

    // ---- Ring buffer operations ----
    function ringPush(entry) {
        if (_ringBuffer.length < _config.history_depth) {
            _ringBuffer.push(entry);
        } else {
            _ringBuffer[_ringIndex] = entry;
        }
        _ringIndex = (_ringIndex + 1) % _config.history_depth;
        if (_ringBuffer.length >= _config.history_depth) {
            _ringFilled = true;
        }
    }

    // Return ordered ring buffer entries (oldest first)
    function ringEntries(maxDepth) {
        var len = _ringBuffer.length;
        if (len === 0) return [];
        var depth = (maxDepth && maxDepth < len) ? maxDepth : len;
        var result = [];
        var start;
        if (_ringFilled) {
            // ring has wrapped; _ringIndex points to oldest slot
            start = _ringIndex;
            var skip = len - depth;
            for (var i = 0; i < depth; i++) {
                result.push(_ringBuffer[(start + skip + i) % len]);
            }
        } else {
            // ring has not wrapped; entries 0..len-1 in order
            start = len - depth;
            for (var j = start; j < len; j++) {
                result.push(_ringBuffer[j]);
            }
        }
        return result;
    }

    // Resize ring buffer when history_depth changes
    function ringResize(newDepth) {
        if (newDepth === _config.history_depth) return;
        var ordered = ringEntries(newDepth);
        _ringBuffer = ordered;
        _ringIndex = ordered.length % newDepth;
        _ringFilled = (ordered.length >= newDepth);
        _config.history_depth = newDepth;
    }

    // ---- Variable auto-detection ----

    // Scan globalThis for in_* and out_* variables into target object
    function scanInOutVars(target) {
        var keys = Object.keys(g);
        for (var i = 0; i < keys.length; i++) {
            var key = keys[i];
            if (key.indexOf('in_') === 0 || key.indexOf('out_') === 0) {
                var val = g[key];
                if (typeof val !== 'function' && typeof val !== 'object') {
                    target[key] = val;
                }
            }
        }
        return target;
    }

    // Scan FB instance fields into target object; if fullScan is true,
    // also include _debug_meta fields (for full snapshot), otherwise
    // only enumerable properties (for ring buffer entry).
    function scanFBFields(target, fullScan) {
        var keys = Object.keys(g);
        for (var i = 0; i < keys.length; i++) {
            var key = keys[i];
            if (key.charAt(0) === '_') continue;
            var obj;
            try { obj = g[key]; } catch (e) { continue; }
            if (obj === null || obj === undefined || typeof obj !== 'object' || Array.isArray(obj)) continue;
            var ctor = obj.constructor;
            if (ctor && ctor._debug_meta) {
                if (fullScan) {
                    var meta = ctor._debug_meta;
                    var fields = meta.fields || {};
                    var fieldNames = Object.keys(fields);
                    for (var fi = 0; fi < fieldNames.length; fi++) {
                        var fieldName = fieldNames[fi];
                        var dotName = key + '.' + fieldName;
                        try { target[dotName] = obj[fieldName]; } catch (e) { target[dotName] = null; }
                    }
                }
                var propNames = Object.keys(obj);
                for (var pi = 0; pi < propNames.length; pi++) {
                    var propName = propNames[pi];
                    var dotProp = key + '.' + propName;
                    if (!(dotProp in target)) {
                        try { target[dotProp] = obj[propName]; } catch (e) { target[dotProp] = null; }
                    }
                }
            }
        }
        return target;
    }

    function scanGlobals() {
        var vars = {};
        scanInOutVars(vars);
        return vars;
    }

    function scanFBInstances() {
        var instances = {};
        scanFBFields(instances, true);
        return instances;
    }

    // Collect watched variables
    function collectWatched() {
        var result = {};
        var names = Object.keys(_watched);
        for (var i = 0; i < names.length; i++) {
            var name = names[i];
            try {
                result[name] = _watched[name]();
            } catch (e) {
                result[name] = null;
            }
        }
        return result;
    }

    // ---- Snapshot collection (called lazily from HTTP handler) ----
    function collectSnapshot() {
        var vars = scanGlobals();
        var fbVars = scanFBInstances();
        var watchedVars = collectWatched();

        // Merge all into vars
        var fbKeys = Object.keys(fbVars);
        for (var i = 0; i < fbKeys.length; i++) {
            vars[fbKeys[i]] = fbVars[fbKeys[i]];
        }
        var wKeys = Object.keys(watchedVars);
        for (var j = 0; j < wKeys.length; j++) {
            vars[wKeys[j]] = watchedVars[wKeys[j]];
        }

        var forcedList = Object.keys(_forced);

        var missed = _cycle - _lastSnapshotCycle;
        _lastSnapshotCycle = _cycle;

        return {
            cycle: _cycle,
            ts: Date.now(),
            dt_ms: _dtMs,
            vars: vars,
            trace: _config.trace_enabled ? _trace.slice() : [],
            forced: forcedList,
            cycles_missed: missed > 0 ? missed : 0
        };
    }

    // Collect a minimal snapshot for the ring buffer (just values, cheap)
    function collectRingEntry() {
        var vals = {};
        scanInOutVars(vals);
        scanFBFields(vals, false);

        // Watched variables
        var watchNames = Object.keys(_watched);
        for (var i = 0; i < watchNames.length; i++) {
            var name = watchNames[i];
            try { vals[name] = _watched[name](); } catch (e) { vals[name] = null; }
        }

        return {
            ts: Date.now(),
            cycle: _cycle,
            vars: vals
        };
    }

    // ---- Trace instrumentation functions ----

    function _dbg_begin_cycle() {
        _trace = [];
    }

    function _dbg_end_cycle() {
        // Intentional no-op: keeps symmetry with _dbg_begin_cycle() for
        // instrumented code (st2js codegen emits both calls). Trace data
        // is read lazily on HTTP request, so no end-of-cycle work needed.
    }

    function _dbg_if(line, cond) {
        if (_config.trace_enabled) {
            _trace.push({ type: "if", line: line, result: !!cond });
        }
        return cond;
    }

    function _dbg_case(line, value) {
        if (_config.trace_enabled) {
            _trace.push({ type: "case", line: line, value: value });
        }
        return value;
    }

    // ---- step_cb: runs BEFORE uniset_on_step each cycle ----
    function debugStepCb() {
        var now = Date.now();
        if (_lastStepTs > 0) {
            _dtMs = now - _lastStepTs;
        }
        _lastStepTs = now;
        _cycle++;

        // Apply forced values (before uniset_on_step)
        var names = Object.keys(_forced);
        for (var i = 0; i < names.length; i++) {
            g[names[i]] = _forced[names[i]];
        }

        // Capture ring buffer entry (state after previous cycle's on_step)
        if (_cycle > 1) {
            ringPush(collectRingEntry());
        }
    }

    // ---- JSON helper ----
    function jsonResponse(status, obj) {
        return {
            status: status,
            headers: { 'Content-Type': 'application/json; charset=utf-8' },
            body: JSON.stringify(obj)
        };
    }

    function textResponse(status, contentType, body) {
        return {
            status: status,
            headers: { 'Content-Type': contentType },
            body: body
        };
    }

    // ---- HTTP route handler ----
    function handleDebugRequest(req) {
        var method = (req.method || 'GET').toUpperCase();
        var rawPath = req.path || req.uri || '/';
        var qIdx = rawPath.indexOf('?');
        var path = qIdx >= 0 ? rawPath.slice(0, qIdx) : rawPath;
        var queryStr = '';
        if (qIdx >= 0) {
            queryStr = rawPath.slice(qIdx + 1);
        } else if (typeof req.query === 'string' && req.query) {
            queryStr = req.query;
        }
        var query = parseQuery(queryStr);

        // Also merge req.query if it's an object (from router)
        if (typeof req.query === 'object' && req.query !== null && !Array.isArray(req.query)) {
            var rqKeys = Object.keys(req.query);
            for (var qi = 0; qi < rqKeys.length; qi++) {
                query[rqKeys[qi]] = req.query[rqKeys[qi]];
            }
        }

        // Normalize path
        path = path.replace(/\/+$/, '') || '/';

        // ---- Route: GET /debug/snapshot ----
        if (path === '/debug/snapshot' && method === 'GET') {
            return jsonResponse(200, collectSnapshot());
        }

        // ---- Route: GET /debug/history ----
        if (path === '/debug/history' && method === 'GET') {
            var varName = query['var'] || query.var;
            if (!varName) {
                return jsonResponse(400, { error: "missing 'var' parameter" });
            }
            var depth = parseInt(query.depth || query['depth'], 10);
            if (isNaN(depth) || depth <= 0) {
                depth = _config.history_depth;
            }
            var entries = ringEntries(depth);
            var data = [];
            for (var hi = 0; hi < entries.length; hi++) {
                var entry = entries[hi];
                if (varName in entry.vars) {
                    data.push([entry.ts, entry.vars[varName]]);
                }
            }
            return jsonResponse(200, {
                'var': varName,
                data: data
            });
        }

        // ---- Route: GET /debug/info ----
        if (path === '/debug/info' && method === 'GET') {
            var programName = '';
            if (g._program_meta && g._program_meta.name) {
                programName = g._program_meta.name;
            }
            // Count variables (approximate from last scan)
            var varsCount = 0;
            var gKeys = Object.keys(g);
            for (var vi = 0; vi < gKeys.length; vi++) {
                var gk = gKeys[vi];
                if (gk.indexOf('in_') === 0 || gk.indexOf('out_') === 0) {
                    varsCount++;
                }
            }
            varsCount += Object.keys(_watched).length;
            return jsonResponse(200, {
                version: unisetDebugModuleVersion,
                program: programName,
                cycle_ms: _dtMs,
                vars_count: varsCount,
                history_depth: _config.history_depth,
                uptime_ms: Date.now() - _startTime,
                cycle: _cycle,
                trace_enabled: _config.trace_enabled
            });
        }

        // ---- Route: POST /debug/force ----
        if (path === '/debug/force' && method === 'POST') {
            var forceBody;
            try {
                forceBody = JSON.parse(req.body || '{}');
            } catch (e) {
                return jsonResponse(400, { error: "invalid JSON body" });
            }
            var forceVar = forceBody['var'];
            if (!forceVar || typeof forceVar !== 'string') {
                return jsonResponse(400, { error: "missing 'var' in request body" });
            }
            if (!('value' in forceBody)) {
                return jsonResponse(400, { error: "missing 'value' in request body" });
            }
            _forced[forceVar] = forceBody.value;
            return jsonResponse(200, { ok: true });
        }

        // ---- Route: POST /debug/unforce ----
        if (path === '/debug/unforce' && method === 'POST') {
            var unforceBody;
            try {
                unforceBody = JSON.parse(req.body || '{}');
            } catch (e) {
                return jsonResponse(400, { error: "invalid JSON body" });
            }
            var unforceVar = unforceBody['var'];
            if (!unforceVar || typeof unforceVar !== 'string') {
                return jsonResponse(400, { error: "missing 'var' in request body" });
            }
            delete _forced[unforceVar];
            return jsonResponse(200, { ok: true });
        }

        // ---- Route: GET /debug/schema ----
        if (path === '/debug/schema' && method === 'GET') {
            var schema = { nodes: [], edges: [] };
            var meta = g._program_meta;
            if (meta) {
                // Input and output nodes
                function pushIONodes(arr, nodeType) {
                    if (!arr) return;
                    for (var i = 0; i < arr.length; i++) {
                        var item = arr[i];
                        var node = {
                            id: item.name, type: nodeType,
                            varType: item.type || "INT",
                            sensor: item.sensor || item.name
                        };
                        if (nodeType === 'input') node.scale = item.scale || null;
                        schema.nodes.push(node);
                    }
                }
                pushIONodes(meta.inputs, 'input');
                pushIONodes(meta.outputs, 'output');
                // GVL global variable nodes
                if (meta.globals) {
                    for (var sg = 0; sg < meta.globals.length; sg++) {
                        var gvar = meta.globals[sg];
                        schema.nodes.push({
                            id: gvar.name, type: "local",
                            varType: gvar.type || "STRUCT"
                        });
                    }
                }
                // FB instance nodes
                if (meta.fb_instances) {
                    for (var sf = 0; sf < meta.fb_instances.length; sf++) {
                        var fb = meta.fb_instances[sf];
                        var fbObj = g[fb.name];
                        var dmeta = (fbObj && fbObj.constructor) ? fbObj.constructor._debug_meta : null;
                        schema.nodes.push({
                            id: fb.name, type: "fb",
                            fbType: fb.type,
                            display: dmeta ? dmeta.display : null,
                            args: fb.args || {},
                            inputs: fb.inputs || null,
                            outputs: fb.outputs || null
                        });
                    }
                }
                // Operator nodes (virtual FB-like blocks for complex expressions)
                if (meta.operators) {
                    for (var so = 0; so < meta.operators.length; so++) {
                        var op = meta.operators[so];
                        schema.nodes.push({
                            id: op.id, type: "fb",
                            fbType: op.op,
                            isOperator: true,
                            inputs: op.inputs || []
                        });
                    }
                }
                // Edges from connections
                if (meta.connections) {
                    schema.edges = meta.connections;
                }
                // Program names for filter
                if (meta.programs) {
                    schema.programs = meta.programs;
                }
            }
            return jsonResponse(200, schema);
        }

        // ---- Route: GET /debug/objects ----
        if (path === '/debug/objects' && method === 'GET') {
            var objects = [];
            var meta = g._program_meta;
            if (meta && meta.fb_instances) {
                for (var oi = 0; oi < meta.fb_instances.length; oi++) {
                    var fbInfo = meta.fb_instances[oi];
                    var fbObj = g[fbInfo.name];
                    if (!fbObj) continue;
                    var dmeta = fbObj.constructor ? fbObj.constructor._debug_meta : null;
                    var fields = {};
                    if (dmeta && dmeta.fields) {
                        for (var fname in dmeta.fields) {
                            var fmeta = dmeta.fields[fname];
                            fields[fname] = {
                                value: fbObj[fname],
                                type: fmeta.type || "unknown",
                                label: fmeta.label || fname,
                                unit: fmeta.unit || null,
                                readonly: fmeta.readonly || false
                            };
                        }
                    } else {
                        // Fallback: enumerate common properties
                        for (var ci = 0; ci < COMMON_FB_PROPS.length; ci++) {
                            if (COMMON_FB_PROPS[ci] in fbObj) {
                                fields[COMMON_FB_PROPS[ci]] = {
                                    value: fbObj[COMMON_FB_PROPS[ci]],
                                    type: typeof fbObj[COMMON_FB_PROPS[ci]]
                                };
                            }
                        }
                    }
                    objects.push({
                        name: fbInfo.name,
                        type: fbInfo.type,
                        display: dmeta ? dmeta.display : null,
                        fields: fields
                    });
                }
            }
            return jsonResponse(200, { objects: objects });
        }

        // ---- Route: POST /debug/config ----
        if (path === '/debug/config' && method === 'POST') {
            var configBody;
            try {
                configBody = JSON.parse(req.body || '{}');
            } catch (e) {
                return jsonResponse(400, { error: "invalid JSON body" });
            }
            if ('history_depth' in configBody) {
                var newDepth = parseInt(configBody.history_depth, 10);
                if (isNaN(newDepth) || newDepth < 1) {
                    return jsonResponse(400, { error: "history_depth must be a positive integer" });
                }
                ringResize(newDepth);
            }
            if ('trace_enabled' in configBody) {
                _config.trace_enabled = !!configBody.trace_enabled;
            }
            return jsonResponse(200, { ok: true });
        }

        // ---- Route: GET /debug/ui ----
        // Full UI is in uniset2-debug-ui.html (same directory).
        // If _debug_ui_html was populated at startup we serve it;
        // otherwise return a minimal redirect/instruction page.
        if (path === '/debug/ui' && method === 'GET') {
            if (_uiHtml) {
                return textResponse(200, 'text/html; charset=utf-8', _uiHtml);
            }
            return textResponse(200, 'text/html; charset=utf-8', _uiFallbackHtml);
        }

        // ---- 404 for anything else under /debug/ ----
        if (path.indexOf('/debug/') === 0 || path === '/debug') {
            return jsonResponse(404, { error: "not found: " + path });
        }

        // Not a debug route, return null to allow pass-through
        return null;
    }

    // ---- Public API: uniset_debug_start ----
    function uniset_debug_start(port, options) {
        if (_started) {
            throw new Error("uniset_debug_start: already started");
        }

        port = port || DEFAULT_PORT;
        options = options || {};

        if ('history_depth' in options) {
            var hd = parseInt(options.history_depth, 10);
            if (!isNaN(hd) && hd > 0) {
                _config.history_depth = hd;
            }
        }
        if ('trace_enabled' in options) {
            _config.trace_enabled = !!options.trace_enabled;
        }

        _startTime = Date.now();
        _started = true;

        // Auto-register locals and FB instances from _program_meta
        if (g._program_meta)
        {
            var meta = g._program_meta;
            if (meta.locals)
            {
                for (var li = 0; li < meta.locals.length; li++)
                {
                    var lname = meta.locals[li].name;
                    (function(n) {
                        uniset_debug_watch(n, function() { return g[n]; });
                    })(lname);
                }
            }
            if (meta.fb_instances)
            {
                for (var fi = 0; fi < meta.fb_instances.length; fi++)
                {
                    var fb = meta.fb_instances[fi];
                    var fbObj = g[fb.name];
                    if (fbObj && fbObj.constructor && fbObj.constructor._debug_meta)
                    {
                        var fields = fbObj.constructor._debug_meta.fields || {};
                        for (var fname in fields)
                        {
                            (function(inst, field) {
                                uniset_debug_watch(inst + "." + field, function() {
                                    var o = g[inst];
                                    return o ? o[field] : undefined;
                                });
                            })(fb.name, fname);
                        }
                    }
                    else if (fbObj)
                    {
                        // No _debug_meta: register common FB properties
                        for (var ci = 0; ci < COMMON_FB_PROPS.length; ci++)
                        {
                            if (COMMON_FB_PROPS[ci] in fbObj)
                            {
                                (function(inst, field) {
                                    uniset_debug_watch(inst + "." + field, function() {
                                        var o = g[inst];
                                        return o ? o[field] : undefined;
                                    });
                                })(fb.name, COMMON_FB_PROPS[ci]);
                            }
                        }
                    }
                }
            }
        }

        // Register step_cb for force override and ring buffer capture
        if (g.uniset && typeof g.uniset.step_cb === 'function') {
            g.uniset.step_cb(debugStepCb);
        }

        // Start HTTP server with debug handler
        if (g.uniset && typeof g.uniset.http_start === 'function') {
            g.uniset.http_start("0.0.0.0", port, function (req, res) {
                // Add CORS headers
                var resp = handleDebugRequest(req);
                if (resp !== null) {
                    // Add CORS headers to response
                    if (!resp.headers) resp.headers = {};
                    resp.headers['Access-Control-Allow-Origin'] = '*';
                    resp.headers['Access-Control-Allow-Methods'] = 'GET,POST,OPTIONS';
                    resp.headers['Access-Control-Allow-Headers'] = 'Content-Type';

                    // Handle OPTIONS preflight
                    var method = (req.method || '').toUpperCase();
                    if (method === 'OPTIONS') {
                        return {
                            status: 204,
                            headers: resp.headers,
                            body: ''
                        };
                    }
                    return resp;
                }
                // Not a debug route — return 404
                return jsonResponse(404, { error: "not found" });
            });
        } else {
            // No uniset.http_start available; in test/standalone mode
            // the module still works for step_cb and data collection
        }
    }

    // ---- Public API: uniset_debug_watch ----
    function uniset_debug_watch(name, getter) {
        if (typeof name !== 'string' || !name) {
            throw new Error("uniset_debug_watch: name must be a non-empty string");
        }
        if (typeof getter !== 'function') {
            throw new Error("uniset_debug_watch: getter must be a function");
        }
        _watched[name] = getter;
    }

    // ---- Public API: uniset_debug_set_ui ----
    // Set the HTML content for the /debug/ui page.
    // Call before uniset_debug_start() if you want the embedded UI.
    // Example: uniset_debug_set_ui(std.loadFile("uniset2-debug-ui.html"));
    function uniset_debug_set_ui(htmlString) {
        if (typeof htmlString !== 'string') {
            throw new Error("uniset_debug_set_ui: argument must be a string");
        }
        _uiHtml = htmlString;
    }

    // ---- Install trace functions on globalThis ----
    // Always replace: if st2js defined stubs, we replace them with
    // collecting versions. If not yet defined, we define them.
    g._dbg_if = _dbg_if;
    g._dbg_case = _dbg_case;
    g._dbg_begin_cycle = _dbg_begin_cycle;
    g._dbg_end_cycle = _dbg_end_cycle;

    // ---- Export to globalThis ----
    g.uniset_debug_start = uniset_debug_start;
    g.uniset_debug_watch = uniset_debug_watch;
    g.uniset_debug_set_ui = uniset_debug_set_ui;

    // ---- Expose internals for testing (prefixed with _debug_) ----
    g._debug_internals = {
        getConfig: function () { return _config; },
        getCycle: function () { return _cycle; },
        getForced: function () { return _forced; },
        getTrace: function () { return _trace; },
        getRingBuffer: function () { return _ringBuffer; },
        getWatched: function () { return _watched; },
        collectSnapshot: collectSnapshot,
        collectRingEntry: collectRingEntry,
        handleDebugRequest: handleDebugRequest,
        debugStepCb: debugStepCb,
        ringPush: ringPush,
        ringEntries: ringEntries,
        ringResize: ringResize,
        reset: function () {
            _cycle = 0;
            _startTime = Date.now();
            _lastStepTs = 0;
            _dtMs = 0;
            _started = false;
            _forced = {};
            _forcedCount = 0;
            _watched = {};
            _trace = [];
            _ringBuffer = [];
            _ringIndex = 0;
            _ringFilled = false;
            _lastSnapshotCycle = 0;
            _config.history_depth = DEFAULT_HISTORY_DEPTH;
            _config.trace_enabled = DEFAULT_TRACE_ENABLED;
            _uiHtml = g._debug_ui_html || null;
        },
        getUiHtml: function () { return _uiHtml; }
    };

})(typeof globalThis !== 'undefined' ? globalThis : this);
// --------------------------------------------------------------------------
if (typeof module !== 'undefined' && module.exports)
{
    module.exports = {
        uniset_debug_start: globalThis.uniset_debug_start,
        uniset_debug_watch: globalThis.uniset_debug_watch,
        uniset_debug_set_ui: globalThis.uniset_debug_set_ui,
        _dbg_if: globalThis._dbg_if,
        _dbg_case: globalThis._dbg_case,
        _dbg_begin_cycle: globalThis._dbg_begin_cycle,
        _dbg_end_cycle: globalThis._dbg_end_cycle
    };
}
// --------------------------------------------------------------------------
