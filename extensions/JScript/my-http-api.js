load("uniset2-mini-http.js");
load("uniset2-mini-http-router.js");
// ----------------------------------------------------------------------------
function startMyHttpServer() {
    const r = createRouter();

    r.route('GET', '/ping', http_pong);
    r.route('GET', '/info', http_info);

    // подпрефиксы (группы)
    const api = r.group('/api');
    api.route('GET', '/v1/info', http_info);

    // server
    startMiniHttp({
        host: "127.0.0.1",
        port: 9090,
        autoLoop: false,
        onRequest: r.handle
    });
}
// ----------------------------------------------------------------------------
function http_pong(req, res) {
    return res.end('pong');
}
// ----------------------------------------------------------------------------
function http_info(req, res) {
    mylog.info("HTTP INFO");

    const info = {
        script: 'main.js',
        date: new Date().toISOString()
    };
    return res.json(info);
}
// ----------------------------------------------------------------------------
