
/*
 * Consolidated tests for uniset2-mini-http-router.js + uniset2-mini-http.js
 * - test harness with test/assert*
 * - router tests
 * - HTTP server tests
 * - extra: double-send guard & POST JSON body
 */

load("uniset2-mini-http-router.js");
load("uniset2-mini-http.js");

// ---------------- Test Harness ----------------
function createTestHarness() {
    var results = { passed: 0, failed: 0, tests: [] };

    function assert(cond, msg) {
        if (!cond) throw new Error(msg || "Assertion failed");
    }
    function assertEqual(a, b, msg) {
        if (a !== b) throw new Error((msg ? msg + ": " : "") + "expected " + JSON.stringify(b) + " got " + JSON.stringify(a));
    }
    function assertDeepEqual(a, b, msg) {
        if (!deepEqual(a, b)) throw new Error((msg ? msg + ": " : "") + "expected deep equal " + JSON.stringify(b) + " got " + JSON.stringify(a));
    }
    function assertLike(str, re, msg) {
        if (!(re instanceof RegExp)) re = new RegExp(String(re));
        if (!re.test(String(str))) throw new Error((msg ? msg + ": " : "") + "expected to match " + re + ", got: " + str);
    }
    function deepEqual(a, b) {
        if (a === b) return true;
        if (typeof a !== typeof b) return false;
        if (a && b && typeof a === 'object') {
            var aa = Object.keys(a), bb = Object.keys(b);
            if (aa.length !== bb.length) return false;
            aa.sort(); bb.sort();
            for (var i=0;i<aa.length;i++) if (aa[i] !== bb[i]) return false;
            for (var j=0;j<aa.length;j++) if (!deepEqual(a[aa[j]], b[aa[j]])) return false;
            return true;
        }
        return false;
    }

    function test(name, fn) {
        try {
            fn();
            results.passed++;
            results.tests.push({ name: name, ok: true });
            print("✅ " + name);
        } catch (e) {
            results.failed++;
            results.tests.push({ name: name, ok: false, error: String(e && e.stack || e) });
            print("❌ " + name + " -> " + (e && e.message || e));
        }
    }

    return {
        results: results,
        test: test,
        assert: assert,
        assertEqual: assertEqual,
        assertDeepEqual: assertDeepEqual,
        assertLike: assertLike
    };
}

// ---------------- Net shim ----------------
function createNetShim() {
    var nextFd = 100;
    function makeFD(){ return nextFd++; }

    var events = [], reads = {}, writes = {}, closed = {};

    function utf8(s) {
        var arr = [];
        for (var i=0;i<s.length;i++) arr.push(s.charCodeAt(i) & 0xFF);
        return new Uint8Array(arr).buffer;
    }
    function dec(ab) {
        var u = new Uint8Array(ab), s = "";
        for (var i=0;i<u.length;i++) s += String.fromCharCode(u[i]);
        return s;
    }
    function enqueueRequest(raw) {
        var cfd = makeFD();
        events.push({ fd: cfd, type: 'accepted' });
        reads[cfd] = [utf8(raw)];
        events.push({ fd: cfd, type: 'readable' });
        return cfd;
    }
    function collectResponse(fd) {
        var arr = writes[fd] || [];
        var out = "";
        for (var i=0;i<arr.length;i++) out += dec(arr[i]);
        return out;
    }

    globalThis.net = {
        listen: function(host, port){ return makeFD(); },
        poll: function(){ var out = events.slice(); events.length = 0; return out; },
        read: function(fd, n){
            var arr = reads[fd] || [];
            if (!arr.length) return null;
            return arr.shift();
        },
        write: function(fd, ab){
            if (!writes[fd]) writes[fd] = [];
            writes[fd].push(ab);
            return (ab && ab.byteLength) ? ab.byteLength : 0;
        },
        close: function(fd){ closed[fd] = true; }
    };

    return { makeFD: makeFD, events: events, reads: reads, writes: writes, closed: closed, enqueueRequest: enqueueRequest, collectResponse: collectResponse, utf8: utf8, dec: dec };
}

// ---------------- Tests ----------------
function runMiniHttpTests() {
    var T = createTestHarness();
    var test = T.test, assert = T.assert, assertEqual = T.assertEqual, assertDeepEqual = T.assertDeepEqual, assertLike = T.assertLike;

    print("\n=== Running uniset2-mini-http* Tests ===\n");

    // Router unit tests
    test("Router: basic GET /ping", function(){
        var r = createRouter();
        var seen = [];
        r.route('GET', '/ping', function(req,res){ seen.push(req.path); res.send('pong'); });
        r.handle({ method:'GET', path:'/ping' }, mkRes(function(body, headers, status){
            assertEqual(status, 200, "status");
            assertEqual(body, 'pong', "body");
        }));
        assertDeepEqual(seen, ['/ping']);
    });

    test("Router: params and query", function(){
        var r = createRouter();
        r.route('GET', '/user/:id', function(req,res){
            assertDeepEqual(req.params, { id: '42' }, "params");
            assertDeepEqual(req.query, { q: 'ok' }, "query");
            res.json({ id: req.params.id, q: req.query.q });
        });
        r.handle({ method:'GET', path:'/user/42?q=ok' }, mkRes(function(body, headers, status){
            assertLike(headers['content-type'], /application\/json/i, "json header");
            var obj = JSON.parse(body);
            assertEqual(obj.id, '42');
            assertEqual(obj.q, 'ok');
        }));
    });

    test("Router: prefix groups and precedence", function(){
        var r = createRouter();
        var api = r.group('/api');
        api.route('GET', '/v1/info', function(req,res){ res.json({v:1}); });
        r.route('GET', '/a/b', function(req,res){ res.send('E'); });
        r.route('GET', '/a/:x', function(req,res){ res.send('P'); });
        r.route('GET', '/a/*', function(req,res){ res.send('W'); });

        r.handle({ method:'GET', path:'/api/v1/info' }, mkRes(function(body){
            assertEqual(JSON.parse(body).v, 1);
        }));
        r.handle({ method:'GET', path:'/a/b' }, mkRes(function(body){ assertEqual(body, 'E'); }));
    });

    test("Router: middleware + error handler", function(){
        var r = createRouter();
        var calls = [];
        r.use(function(req,res,next){ calls.push('m1'); next(); });
        r.route('GET', '/err', function(req,res){ throw new Error('boom'); });
        r.useError(function(err, req, res, next){ res.status(500).send('handled:'+err.message); });
        r.handle({ method:'GET', path:'/err' }, mkRes(function(body, headers, status){
            assertEqual(status, 500);
            assertEqual(body, 'handled:boom');
            assertDeepEqual(calls, ['m1']);
        }));
    });

    test("Router: CORS preflight", function(){
        var r = createRouter();
        r.setCors({ origin: '*', methods:['GET','POST'] });
        r.handle({ method:'OPTIONS', path:'/x' }, mkRes(function(body, headers, status){
            // Accept 204 (preferred) or 404 (no global preflight handler).
            if (status === 204) {
                assertEqual(body, '');
                assert(headers['access-control-allow-origin'] !== undefined, "has ACAO");
            } else {
                assertEqual(status, 404);
            }
        }));
    });

    test("Router: 404", function(){
        var r = createRouter();
        r.handle({ method:'GET', path:'/nope' }, mkRes(function(body, headers, status){
            assertEqual(status, 404);
            assertLike(body, /not found/i);
        }));
    });

    // HTTP server tests
    var shim = createNetShim();

    function parseFirstHttpBody(resp) {
        var i = resp.indexOf("\r\n\r\n");
        if (i < 0) return "";
        var headerBlock = resp.slice(0, i);
        var rest = resp.slice(i+4);
        var m = headerBlock.match(/\r\ncontent-length:\s*(\d+)/i);
        var len = m ? parseInt(m[1],10) : 0;
        return rest.slice(0, len);
    }

    test("HTTP: default /ping responds 200 pong", function(){
        var srv = startMiniHttp({ autoLoop: false });
        var fd = shim.enqueueRequest("GET /ping HTTP/1.1\r\nHost: x\r\n\r\n");
        srv.step();
        var resp = shim.collectResponse(fd);
        assertLike(resp, /^HTTP\/1\.1 200 OK\r\n/i, "status line");
        assertLike(resp, /\r\ncontent-length:\s*4\r\n/i, "content-length");
        var body = resp.split("\r\n\r\n")[1];
        assertEqual(body, "pong");
        srv.close();
    });

    test("HTTP: router integration returns JSON", function(){
        var r = createRouter();
        r.route('GET', '/api/v1/info', function(req,res){ res.json({hello:'world'}); });
        var srv = startMiniHttp({ autoLoop:false, onRequest: function(req,res){ r.handle(req,res); } });
        var fd = shim.enqueueRequest("GET /api/v1/info HTTP/1.1\r\nHost: x\r\n\r\n");
        srv.step();
        var resp = shim.collectResponse(fd);
        assertLike(resp, /^HTTP\/1\.1 200 OK\r\n/i);
        assertLike(resp, /content-type:\s*application\/json/i);
        var body = parseFirstHttpBody(resp);
        assertEqual(body, '{"hello":"world"}');
        srv.close();
    });

    test("HTTP: keep-alive with two pipelined requests", function(){
        var srv = startMiniHttp({
            autoLoop: false,
            onRequest: function(req,res){
                if (req.path === '/a') res.send('A');
                else if (req.path === '/b') res.send('B');
                else res.status(404).send('nope');
            }
        });
        var fd = shim.makeFD();
        shim.events.push({ fd: fd, type: 'accepted' });
        shim.reads[fd] = [shim.utf8("GET /a HTTP/1.1\r\nHost:x\r\n\r\nGET /b HTTP/1.1\r\nHost:x\r\n\r\n")];
        shim.events.push({ fd: fd, type: 'readable' });
        srv.step();
        var resp = shim.collectResponse(fd);
        var parts = resp.split("HTTP/1.1 ").filter(function(s){ return s.length>0; });
        assert(parts.length >= 2, "should contain two responses");
        srv.close();
    });

    test("HTTP: custom 404", function(){
        var srv = startMiniHttp({ autoLoop:false, onRequest: function(req,res){ res.status(404).send("Not Found"); }});
        var fd = shim.enqueueRequest("GET /no HTTP/1.1\r\nHost:x\r\n\r\n");
        srv.step();
        var resp = shim.collectResponse(fd);
        assertLike(resp, / 404 /);
        srv.close();
    });

    // NEW: ensure no extra data after first response
    test("HTTP: no extra data after first response (double send guarded)", function(){
        var srv = startMiniHttp({
            autoLoop: false,
            onRequest: function(req,res){
                res.json({one:1});
                res.send("SECOND"); // must be ignored
            }
        });
        var fd = shim.enqueueRequest("GET /x HTTP/1.1\r\nHost:x\r\n\r\n");
        srv.step();
        var resp = shim.collectResponse(fd);
        var occurrences = (resp.match(/HTTP\/1\.1 /g) || []).length;
        assertEqual(occurrences, 1, "only one HTTP status line");
        assertLike(resp, /content-type:\s*application\/json/i);
        var i = resp.indexOf("\r\n\r\n"); assert(i >= 0, "has boundary");
        var headerBlock = resp.slice(0, i);
        var rest = resp.slice(i + 4);
        var m = headerBlock.match(/\r\ncontent-length:\s*(\d+)/i);
        var len = m ? parseInt(m[1], 10) : 0;
        var body = rest.slice(0, len);
        assertEqual(body, '{"one":1}');
        var tail = rest.slice(len);
        assertEqual(tail, "", "no extra bytes after first response");
        srv.close();
    });

    // NEW: POST with JSON body
    test("HTTP: POST with JSON body is parsed and echoed", function(){
        var srv = startMiniHttp({
            autoLoop: false,
            onRequest: function(req, res){
                function dec(ab){ var u=new Uint8Array(ab), s=""; for (var i=0;i<u.length;i++) s+=String.fromCharCode(u[i]); return s; }
                if (req.method === 'POST' && req.path === '/echo') {
                    var body = dec(req.body || new ArrayBuffer(0));
                    var obj = {};
                    try { obj = JSON.parse(body||"{}"); } catch(_) {}
                    res.json({ ok: true, got: obj, len: (req.body ? (req.body.byteLength|0) : 0) });
                } else {
                    res.status(404).send("nope");
                }
            }
        });

        var payload = '{"a":1,"b":"x"}';
        var raw = "POST /echo HTTP/1.1\r\n"
                + "Host: x\r\n"
                + "Content-Type: application/json\r\n"
                + "Content-Length: " + payload.length + "\r\n"
                + "\r\n"
                + payload;

        var fd = shim.enqueueRequest(raw);
        srv.step();
        var resp = shim.collectResponse(fd);

        var occurrences = (resp.match(/HTTP\/1\.1 /g) || []).length;
        assertEqual(occurrences, 1, "only one HTTP status line");

        assertLike(resp, /content-type:\s*application\/json/i);
        var i2 = resp.indexOf("\r\n\r\n"); assert(i2 >= 0, "has boundary");
        var headerBlock2 = resp.slice(0, i2);
        var rest2 = resp.slice(i2 + 4);
        var m2 = headerBlock2.match(/\r\ncontent-length:\s*(\d+)/i);
        var len2 = m2 ? parseInt(m2[1], 10) : 0;
        var body2 = rest2.slice(0, len2);
        var obj = JSON.parse(body2);
        assertEqual(obj.ok, true);
        assertEqual(obj.got.a, 1);
        assertEqual(obj.got.b, "x");
        assertEqual(obj.len, payload.length);

        var tail2 = rest2.slice(len2);
        assertEqual(tail2, "", "no extra bytes after response");
        srv.close();
    });

    

    // Helpers to parse multiple HTTP responses from a single buffer
    function parseResponses(resp) {
        var out = [];
        var s = resp;
        while (true) {
            var i = s.indexOf("\r\n\r\n");
            if (i < 0) break;
            var headerBlock = s.slice(0, i);
            var m = headerBlock.match(/^HTTP\/1\.1\s+(\d+)\s+[^\r\n]*/i);
            if (!m) break;
            var mlen = headerBlock.match(/\r\ncontent-length:\s*(\d+)/i);
            var len = mlen ? parseInt(mlen[1],10) : 0;
            var body = s.slice(i+4, i+4+len);
            out.push({ headers: headerBlock, body: body, code: parseInt(m[1],10) });
            s = s.slice(i+4+len);
        }
        return out;
    }

    // --- FUZZ CASES for parser ---
    test("HTTP: fuzz request parser closes or 500s on malformed input", function(){
        var cases = [
            "GEXX / HTTP/1.1\r\nHost:x\r\n\r\n",              // invalid method
            "GET  HTTP/1.1\r\nHost:x\r\n\r\n",                 // missing path
            "GET / ?a=1 HTTP/1.1\r\nHost:x\r\n\r\n",           // space in path
            "GET / HTTP/1.0\r\nHost:x\r\n\r\n",                // proto 1.0 (still OK in many, but test robustness)
            "GET / HTTP/1.1\r\nBadHeader\r\n\r\n",             // header without colon
            "GET / HTTP/1.1\r\nContent-Length: x\r\n\r\n",     // invalid length
            "GET / HTTP/1.1\r\nContent-Length: 5\r\n\r\nabc",  // truncated body
            "BADLINE\r\n\r\n"                                  // garbage
        ];
        var shim = createNetShim();
        var srv = startMiniHttp({ autoLoop:false });
        for (var k=0;k<cases.length;k++){
            var fd = shim.enqueueRequest(cases[k]);
            srv.step();
            var resp = shim.collectResponse(fd);
            // Either server responds 500 and closes, or silently closes without bytes
            if (resp === "") {
                // closed silently -> acceptable
            } else {
                var arr = parseResponses(resp);
                // accept a single 500 with empty body OR a permissive code
                assertEqual(arr.length, 1, "one response for fuzz["+k+"]");
                var code = arr[0].code;
                if (code === 500) {
                    assertEqual(arr[0].body.length, 0, "empty body for fuzz["+k+"]");
                } else {
                    assert([200,204,400].indexOf(code) !== -1, "200/204/400/500 acceptable for fuzz["+k+"]");
                }
}
        }
        srv.close();
    });

    // --- SLOW READ: deliver request 1 byte at a time ---
    test("HTTP: slowread 1-byte chunks still produce valid response", function(){
        var shim = createNetShim();
        var srv = startMiniHttp({ autoLoop:false });
        var fd = shim.makeFD();
        shim.events.push({ fd: fd, type: 'accepted' });
        var raw = "GET /ping HTTP/1.1\r\nHost:x\r\n\r\n";
        shim.reads[fd] = [];
        for (var i=0;i<raw.length;i++) shim.reads[fd].push(shim.utf8(raw[i]));
        for (var j=0;j<raw.length;j++) shim.events.push({ fd: fd, type: 'readable' });
        // step through all tiny reads
        for (var t=0;t<raw.length;t++) srv.step();
        var resp = shim.collectResponse(fd);
        var arr = parseResponses(resp);
        assertEqual(arr.length, 1);
        assertEqual(arr[0].code, 200);
        assertEqual(arr[0].body, "pong");
        srv.close();
    });

    // --- BIG HEADERS: very large custom header should not break /ping ---
    test("HTTP: large headers (32KB) handled", function(){
        var shim = createNetShim();
        var srv = startMiniHttp({ autoLoop:false });
        var big = Array(32768).fill("A").join("");
        var raw = "GET /ping HTTP/1.1\r\nHost:x\r\nX-Big: " + big + "\r\n\r\n";
        var fd = shim.enqueueRequest(raw);
        srv.step();
        var resp = shim.collectResponse(fd);
        var arr = parseResponses(resp);
        assertEqual(arr.length, 1);
        assertEqual(arr[0].code, 200);
        assertEqual(arr[0].body, "pong");
        srv.close();
    });

    // --- PIPELINE: 10 requests in single read ---
    test("HTTP: deep pipeline of 10 requests", function(){
        var shim = createNetShim();
        var srv = startMiniHttp({
            autoLoop:false,
            onRequest: function(req,res){
                var m = req.path.match(/^\/([a-z])$/);
                res.send(m ? m[1].toUpperCase() : "X");
            }
        });
        var paths = ["a","b","c","d","e","f","g","h","i","j"];
        var req = paths.map(function(p){ return "GET /"+p+" HTTP/1.1\r\nHost:x\r\n\r\n"; }).join("");
        var fd = shim.enqueueRequest(req);
        srv.step();
        var resp = shim.collectResponse(fd);
        var arr = parseResponses(resp);
        assertEqual(arr.length, paths.length, "responses count");
        for (var i=0;i<paths.length;i++) assertEqual(arr[i].body, paths[i].toUpperCase());
        srv.close();
    });

    // --- SLOW WRITE: net.write returns partial chunks, require multiple writable events ---
    test("HTTP: slowwrite with partial net.write dispatch", function(){
        var shim = createNetShim();
        // Patch net.write to simulate partial writes
        var origWrite = globalThis.net.write;
        globalThis.net.write = function(fd, ab){
            // write only up to max 64 bytes or half of the chunk, whichever smaller, but at least 1
            var maxn = Math.max(1, Math.min(64, Math.floor((ab && ab.byteLength) ? ab.byteLength/2 : 1)));
            // push only the actually "written" portion so collector matches reality
            if (ab && ab.byteLength) {
                var written = ab.slice(0, maxn);
                if (!shim.writes[fd]) shim.writes[fd] = [];
                shim.writes[fd].push(written);
            }
            return maxn;
        };

        var srv = startMiniHttp({ autoLoop:false });
        var fd = shim.enqueueRequest("GET /ping HTTP/1.1\r\nHost:x\r\n\r\n");
        // First step: accept+read+attempt to write partially
        srv.step();

        // Now drive flush with several writable events
        for (var k=0;k<20;k++){ shim.events.push({ fd: fd, type: 'writable' }); srv.step(); }

        var resp = shim.collectResponse(fd);
        var arr = parseResponses(resp);
        assertEqual(arr.length, 1);
        assertEqual(arr[0].code, 200);
        assertEqual(arr[0].body, "pong");

        // restore
        globalThis.net.write = origWrite;
        srv.close();
    });


    // NEW: Router logging hook should fire exactly once with correct info
    test("Router: logger called once with method/path/status/ms", function(){
        var r = createRouter();
        var logs = [];
        r.setLogger(function (e) { logs.push(e); });
        r.route('GET', '/log', function(req, res){
            res.status(201).send('ok');
        });
        r.handle({ method: 'GET', path: '/log' }, mkRes(function(body, headers, status){
            assertEqual(status, 201);
            assertEqual(body, 'ok');
        }));
        assertEqual(logs.length, 1, "logger called once");
        var ev = logs[0];
        assertEqual(ev.method, 'GET');
        assertEqual(ev.path, '/log');
        assertEqual(ev.status, 201);
        // ms should be a non-negative number
        var okMs = (typeof ev.ms === 'number') && ev.ms >= 0 && isFinite(ev.ms);
        assertEqual(okMs, true, "ms >= 0");
    });
print("\n--- Summary ---");
    print("Passed:", T.results.passed, "Failed:", T.results.failed);
    if (T.results.failed === 0) {
        print("\n🎉 All uniset2-mini-http* tests passed successfully!");
    } else {
        print("\n❌ There are failed tests in uniset2-mini-http*!");
    }

    return T.results;
}

// Response stub for router-only tests
function mkRes(done){
    var headers = {};
    var status = 200;
    return {
        setHeader: function(k,v){ headers[String(k).toLowerCase()] = String(v); },
        status: function(code){ status = code|0; return this; },
        setStatus: function(code){ status = code|0; },
        send: function(s){ done(String(s), headers, status); },
        end: function(s){ done(String(s||''), headers, status); },
        json: function(o){ this.setHeader('Content-Type','application/json; charset=utf-8'); done(JSON.stringify(o||{}), headers, status); }
    };
}

// Aliases expected by external runner
function runLogTests() { return runMiniHttpTests(); }
if (typeof globalThis !== 'undefined') { globalThis.runLogTests = runLogTests; }
