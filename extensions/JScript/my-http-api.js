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
    api.route('GET', '/v1/status/:id', http_status);

    // server
    startMiniHttp({
        host: "127.0.0.1",
        port: 9090,
        onRequest: r.handle
    });

    // startMiniHttp({
    //     host: "127.0.0.1",
    //     port: 9090,
    //     onRequest: (req, res) => {
    //         if (req.uri === "/ping") return res.end("pong");
    //         if (req.uri === "/info") return res.json({ status: "ok", time: Date.now() });
    //         res.status(404, "Not Found"); res.end("no such endpoint");
    //     }
    // });

}
// ----------------------------------------------------------------------------
function http_pong(req, res) {
    return res.end('pong');
}
// ----------------------------------------------------------------------------
function http_info(req, res) {
    const info = {
        script: 'main.js',
        date: new Date().toISOString()
    };
    return res.json(info);
}
// ----------------------------------------------------------------------------
function http_status(req, res) {
    const id = req.params.id;
    const qq = req.query.qq;
    res.json({ status: id, query: req.query, params: req.params });
}
// ----------------------------------------------------------------------------
