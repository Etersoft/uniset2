/*
 * tests-uniset2-mini-http.js (revised)
 * - без сетевого шима/парсера HTTP
 * - проверяем: router, req.path/query/params, и контракт res: end/json/status/setHeader/sendStatus/writeHead/redirect
 */

load("uniset2-mini-http-router.js");
load("uniset2-mini-http.js");

// ---------------- Test Harness ----------------
function createTestHarness() {
    var results = { passed: 0, failed: 0, tests: [] };

    function printLn(s){ if (typeof print === 'function') print(s); else console.log(s); }
    function assert(cond, msg){ if (!cond) throw new Error(msg || "Assertion failed"); }
    function assertEqual(a, b, msg){
        if (a !== b) throw new Error((msg ? msg + ": " : "") + "expected " + JSON.stringify(b) + " got " + JSON.stringify(a));
    }
    function assertLike(str, re, msg){
        if (!(re instanceof RegExp)) re = new RegExp(String(re));
        if (!re.test(String(str))) throw new Error((msg ? msg + ": " : "") + "expected match " + re + ", got: " + str);
    }

    function test(name, fn){
        try { fn(); results.passed++; results.tests.push({ name, ok:true }); printLn("✅ " + name); }
        catch(e){ results.failed++; results.tests.push({ name, ok:false, error: String(e && e.message || e) }); printLn("❌ " + name + " -> " + e); }
    }

    function summary(){
        printLn("\n=== Summary: " + results.passed + " passed, " + results.failed + " failed ===");
        return results;
    }

    return { test, assert, assertEqual, assertLike, summary };
}

// ---------------- Minimal req/res shim (no network) ----------------
function mkReq(o){
    o = o || {};
    var uri = o.uri || o.url || o.path || "/";
    var path = uri, query = "";
    var qi = uri.indexOf("?");
    if (qi >= 0) { path = uri.slice(0, qi); query = uri.slice(qi + 1); }
    return {
        method: String(o.method || "GET").toUpperCase(),
        uri: uri,
        url: uri,
        path: path,
        query: query,
        headers: o.headers || Object.create(null),
        body: o.body || new ArrayBuffer(0),
        params: Object.create(null)
    };
}

function mkRes(onDone){
    var ended = false;
    var status = 200, reason = "OK";
    var headers = Object.create(null);
    function endWith(body){
        if (ended) throw new Error("double send not allowed");
        ended = true;
        onDone({ status, reason, headers, body: String(body || "") });
    }
    function reasonBy(code){
        switch(code|0){
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 302: return "Found";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 408: return "Request Timeout";
            case 500: return "Internal Server Error";
            default:  return "";
        }
    }
    var res = {
        setHeader: function(k,v){ headers[String(k).toLowerCase()] = String(v); },
        status: function(code, rsn){ status = code|0; if (typeof rsn === 'string') reason = rsn; return res; },
        setStatus: function(code){ status = code|0; },
        writeHead: function(code, hdrs){
            status = code|0;
            if (hdrs && typeof hdrs === 'object'){
                for (var k in hdrs) headers[String(k).toLowerCase()] = String(hdrs[k]);
            }
            return res;
        },
        redirect: function(location, code){
            var c = (code == null) ? 302 : (code|0);
            status = c;
            headers['location'] = String(location || '/');
            var rsn = reasonBy(c);
            if (rsn && !headers['content-length']) headers['content-length'] = "0";
            endWith("");
        },
        sendStatus: function(code){
            status = code|0;
            var rsn = reasonBy(status);
            if (status === 204){
                headers['content-length'] = "0"; endWith("");
            } else {
                headers['content-type'] = "text/plain; charset=utf-8";
                endWith(rsn || "Status");
            }
        },
        end: function(s){ endWith(String(s||"")); },
        send: function(s){ endWith(String(s||"")); },
        json: function(o){ headers['content-type'] = "application/json; charset=utf-8"; endWith(JSON.stringify(o||{})); }
    };
    return res;
}

// ---------------- Tests ----------------
function runMiniHttpTests(){
    var T = createTestHarness();
    var test = T.test, assert = T.assert, assertEqual = T.assertEqual, assertLike = T.assertLike;

    print("\n=== Running mini-http (router + handler contract) ===\n");

    // Router: basic GET /ping
    test("Router: basic GET /ping", function(){
        var r = createRouter();
        r.route('GET', '/ping', function(req,res){ res.end('pong'); });
        r.handle(mkReq({ method:'GET', path:'/ping' }), mkRes(function(out){
            assertEqual(out.status, 200);
            assertEqual(out.body, 'pong');
        }));
    });

    // Router: params and query
    test("Router: params and query", function(){
        var r = createRouter();
        r.route('GET', '/api/user/:id', function(req,res){
            // router сам наполняет req.params и req.query
            res.json({
                id: req.params.id,
                verbose: req.query.verbose,
                verboseParam: req.params.verbose
            });
        });
        r.handle(mkReq({ method:'GET', path:'/api/user/42?verbose=1' }), mkRes(function(out){
            assertEqual(out.status, 200);
            assertLike(out.headers['content-type'], /application\/json/i);
            var obj = JSON.parse(out.body);
            assertEqual(obj.id, "42");
            assertEqual(obj.verbose, "1");
            assertEqual(obj.verboseParam, "1");
        }));
    });

    // Router: prefix groups and precedence
    test("Router: prefix groups and precedence", function(){
        var r = createRouter();
        r.route('GET', '/a/b', function(req,res){ res.end('E'); });
        r.route('GET', '/a/:x', function(req,res){ res.end('P'); });
        var api = r.group('/api');
        api.route('GET', '/v1/info', function(req,res){ res.json({ v:1 }); });

        r.handle(mkReq({ method:'GET', path:'/api/v1/info' }), mkRes(function(out){
            var obj = JSON.parse(out.body); assertEqual(obj.v, 1);
        }));
        r.handle(mkReq({ method:'GET', path:'/a/b' }), mkRes(function(out){
            assertEqual(out.body, 'E');
        }));
    });

    // Router: middleware + error handler
    test("Router: middleware + error handler", function(){
        var r = createRouter();
        var calls = [];
        r.use(function(req,res,next){ calls.push('m1'); next(); });
        r.route('GET', '/err', function(req,res){ throw new Error("boom"); });
        r.useError(function(err,req,res,next){ res.status(500).end("Internal Server Error"); });

        r.handle(mkReq({ method:'GET', path:'/err' }), mkRes(function(out){
            assertEqual(out.status, 500);
            assertEqual(out.body, "Internal Server Error");
            assertEqual(calls[0], 'm1');
        }));
    });

    // Router: CORS preflight
    test("Router: CORS preflight", function(){
        var r = createRouter();
        r.setCors(true);

        // ВАЖНО: маршрут OPTIONS, чтобы globalMW (CORS) выполнился до 404.
        // Хэндлер ничего не делает — CORS-middleware сам выставляет 204 и завершит ответ.
        r.route('OPTIONS', '/ping', function(req, res){ /* no-op */ });

        // Для полноты: основной GET маршрут
        r.route('GET', '/ping', function(req,res){ res.end('pong'); });

        var req = mkReq({ method:'OPTIONS', path:'/ping' });
        r.handle(req, mkRes(function(out){
            // допускаем 200/204, но в нашей CORS-реализации будет 204 + заголовки
            if (!(out.status === 200 || out.status === 204)) {
                throw new Error("expected 200/204, got " + out.status);
            }
            var keys = Object.keys(out.headers).join(',');
            if (!/access-control-allow-origin/i.test(keys)) {
                throw new Error("no CORS headers in response");
            }
        }));
    });


    // Router: 404
    test("Router: 404", function(){
        var r = createRouter();
        r.handle(mkReq({ method:'GET', path:'/nope' }), mkRes(function(out){
            assertEqual(out.status, 404);
            assertLike(out.body, /not found/i);
        }));
    });

    // res: sendStatus / writeHead / redirect
    test("Response helpers: sendStatus / writeHead / redirect", function(){
        // sendStatus
        (function(){
            var req = mkReq({ path:'/x' });
            var out = null;
            var res = mkRes(function(o){ out = o; });
            res.sendStatus(404);
            assertEqual(out.status, 404);
            assertLike(out.body.toLowerCase(), /not found/);
        })();

        // writeHead
        (function(){
            var req = mkReq({ path:'/x' });
            var out = null;
            var res = mkRes(function(o){ out = o; });
            res.writeHead(201, { "Content-Type": "text/plain" }).end("ok");
            assertEqual(out.status, 201);
            assertLike(out.headers['content-type'], /text\/plain/i);
            assertEqual(out.body, "ok");
        })();

        // redirect
        (function(){
            var req = mkReq({ path:'/x' });
            var out = null;
            var res = mkRes(function(o){ out = o; });
            res.redirect("/new-place", 302);
            assertEqual(out.status, 302);
            assertEqual(out.headers['location'], "/new-place");
        })();
    });

    // Double send guard
    test("Response: double send is guarded", function(){
        var out = null, err = null;
        var res = mkRes(function(o){ out = o; });
        res.end("once");
        try { res.end("twice"); } catch(e){ err = String(e && e.message || e); }
        assertLike(err, /double send/i);
        assertEqual(out.body, "once");
    });

    // Router + JSON in one-line simplest server handler contract
    test("Router integration returns JSON", function(){
        var r = createRouter();
        r.route('GET', '/api/v1/info', function(req,res){ res.json({hello:'world'}); });
        r.handle(mkReq({ method:'GET', path:'/api/v1/info' }), mkRes(function(out){
            assertEqual(out.status, 200);
            assertLike(out.headers['content-type'], /application\/json/i);
            assertEqual(out.body, '{"hello":"world"}');
        }));
    });

    return T.summary();
}

// Aliases for external runner
function runLogTests(){ return runMiniHttpTests(); }
if (typeof globalThis !== 'undefined') globalThis.runLogTests = runLogTests;

// Auto-run if standalone
if (typeof runMiniHttpTests === 'function') runMiniHttpTests();
