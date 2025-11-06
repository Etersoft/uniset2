
// uniset2-mini-http.js — minimal JS layer with deferred start and diagnostics.

(() => {
  const g = globalThis;
  if (!g.uniset) g.uniset = {};

  // startMiniHttp({ host, port, onRequest })
  function startMiniHttp(opts) {
    if (!opts) throw new Error("startMiniHttp: opts required");
    // if (started)
    //   throw new Error("startMiniHttp: is already started");
    if (typeof opts.onRequest !== "function") {
      throw new Error("startMiniHttp: opts.onRequest must be function(req,res)");
    }
    const userHandler = opts.onRequest;
    const onRequest = (req, res) => {
         if (!req.path) req.path = req.uri || '/';
         if (res.setHeader) {
            res.setHeader('Access-Control-Allow-Origin','*');
            res.setHeader('Access-Control-Allow-Methods','GET,POST,PUT,PATCH,DELETE,OPTIONS,HEAD');
            res.setHeader('Access-Control-Allow-Headers','Content-Type,Authorization');
         }

         if (req.method === 'OPTIONS') {
            if (res.sendStatus) return res.sendStatus(204);
            if (res.status) { res.status(204); return res.end(''); }
         }

         return userHandler(req, res);
    };

    const host = opts.host || "127.0.0.1";
    const port = opts.port || 9090;
    g.uniset.http_start(host,port, onRequest);
    return { host, port };
  }

  g.startMiniHttp = startMiniHttp;
})();
