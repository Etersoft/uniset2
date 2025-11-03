// uniset2-mini-http.js — minimal HTTP over non-blocking `net` shim (Poco SocketReactor)
// Assumes C++ layer emits events via net.poll(): {fd, type: 'accepted'|'readable'|'writable'|'closed'|'error', err?, what?}
(function (global) {
  'use strict';

  const CRLF = '\r\n';
  const CRLF2 = '\r\n\r\n';

  // --- UTF-8 helpers (no TextEncoder/Decoder required) ---
  function utf8Encode(str) {
    const out = [];
    for (let i = 0; i < str.length; i++) {
      let cp = str.charCodeAt(i);
      if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < str.length) {
        const lo = str.charCodeAt(i + 1);
        if (lo >= 0xDC00 && lo <= 0xDFFF) {
          cp = ((cp - 0xD800) << 10) + (lo - 0xDC00) + 0x10000;
          i++;
        }
      }
      if (cp <= 0x7F) out.push(cp);
      else if (cp <= 0x7FF) { out.push(0xC0 | (cp >> 6), 0x80 | (cp & 0x3F)); }
      else if (cp <= 0xFFFF) { out.push(0xE0 | (cp >> 12), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F)); }
      else { out.push(0xF0 | (cp >> 18), 0x80 | ((cp >> 12) & 0x3F), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F)); }
    }
    return new Uint8Array(out).buffer;
  }
  function utf8Decode(ab) {
    const bytes = new Uint8Array(ab);
    let out = '', i = 0;
    while (i < bytes.length) {
      const b0 = bytes[i++];
      if (b0 < 0x80) { out += String.fromCharCode(b0); continue; }
      const b1 = bytes[i++] & 0x3F;
      if ((b0 & 0xE0) === 0xC0) { out += String.fromCharCode(((b0 & 0x1F) << 6) | b1); continue; }
      const b2 = bytes[i++] & 0x3F;
      if ((b0 & 0xF0) === 0xE0) { out += String.fromCharCode(((b0 & 0x0F) << 12) | (b1 << 6) | b2); continue; }
      const b3 = bytes[i++] & 0x3F;
      const cp = ((b0 & 0x07) << 18) | (b1 << 12) | (b2 << 6) | b3;
      const off = cp - 0x10000;
      out += String.fromCharCode(0xD800 + (off >> 10), 0xDC00 + (off & 0x3FF));
    }
    return out;
  }

  function concatAB(a, b) {
    if (!a || a.byteLength === 0) return b || new ArrayBuffer(0);
    if (!b || b.byteLength === 0) return a;
    const out = new Uint8Array(a.byteLength + b.byteLength);
    out.set(new Uint8Array(a), 0);
    out.set(new Uint8Array(b), a.byteLength);
    return out.buffer;
  }

  function parseHeaders(s) {
    const lines = s.split('\r\n');
    const [method, path, proto] = lines[0].split(' ');
    const headers = Object.create(null);
    for (let i = 1; i < lines.length; i++) {
      const ln = lines[i];
      if (!ln) continue;
      const k = ln.substring(0, ln.indexOf(':')).trim().toLowerCase();
      const v = ln.substring(ln.indexOf(':') + 1).trim();
      headers[k] = v;
    }
    return { method, path, proto, headers };
  }

  function makeResponse(fd, con) {
    let statusCode = 200;
    let statusText = 'OK';
    const defaultHeaders = Object.create(null);
    let __sent = false;
    function writeHead(code, text, hdrs) {
      if (__sent) return;
      statusCode = code|0;
      statusText = text || statusText;
      if (hdrs) for (const k in hdrs) defaultHeaders[k.toLowerCase()] = ''+hdrs[k];
    }
    function rawSend(bodyAB) {
      if (__sent) return; // prevent double-send
      __sent = true;
      const head = `HTTP/1.1 ${statusCode} ${statusText}\r\n` +
                   Object.entries(defaultHeaders).map(([k,v]) => `${k}: ${v}`).join(CRLF) + CRLF2;
      con.queueSend(utf8Encode(head));
      con.queueSend(bodyAB || new ArrayBuffer(0));
    }
    return {
      writeHead,
      setHeader(k,v){ defaultHeaders[String(k).toLowerCase()] = String(v); },
      status(code){ statusCode = code|0; return this; },
      send(data){
        if (__sent) return;
        if (data == null) data = '';
        let bodyAB;
        if (typeof data === 'string') bodyAB = utf8Encode(data);
        else if (data instanceof ArrayBuffer) bodyAB = data;
        else if (ArrayBuffer.isView(data)) bodyAB = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
        else bodyAB = utf8Encode(String(data));
        if (!defaultHeaders['content-length']) defaultHeaders['content-length'] = String(bodyAB.byteLength);
        if (!defaultHeaders['content-type']) defaultHeaders['content-type'] = 'text/plain; charset=utf-8';
        rawSend(bodyAB);
      },
      json(obj){
        if (__sent) return;
        const s = JSON.stringify(obj ?? {});
        defaultHeaders['content-type'] = 'application/json; charset=utf-8';
        this.send(s);
      }
    };
  }

  class HTTPConn {
    constructor(fd, handler, onClose) {
      this.fd = fd|0;
      this.handler = handler;
      this.onClose = onClose;
      this.buf = new ArrayBuffer(0);
      this.alive = true;
      this.sendQueue = []; // ArrayBuffer[]
      this.sendOffset = 0;
      this.keepAlive = true;
      this.contentLeft = 0;
      this.headersParsed = false;
      this.headerText = '';
      this.req = null;
    }
    feedData(ab) {
      if (!this.alive) return;
      this.buf = concatAB(this.buf, ab);
      while (true) {
        if (!this.headersParsed) {
          const s = utf8Decode(this.buf);
          const idx = s.indexOf(CRLF2);
          if (idx < 0) return;
          this.headerText = s.substring(0, idx);
          const rest = s.substring(idx + 4);
          const restAB = utf8Encode(rest);
          this.buf = restAB;
          const { method, path, proto, headers } = parseHeaders(this.headerText);
          this.req = { method, path, proto, headers };
          const cl = parseInt(headers['content-length'] || '0', 10) || 0;
          this.contentLeft = cl;
          this.headersParsed = true;
          if (/close/i.test(headers['connection'] || '')) this.keepAlive = false;
        }
        if (this.contentLeft > 0) {
          const have = this.buf.byteLength;
          if (have < this.contentLeft) return;
          // consume body
          const bodyAB = this.buf.slice(0, this.contentLeft);
          this.req.body = bodyAB;
          this.buf = this.buf.slice(this.contentLeft);
          this.contentLeft = 0;
        }
        // готов полный запрос
        const res = makeResponse(this.fd, this);
        try {
          this.handler(this.req, res);
        } catch(e) {
          this.queueString("HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
          this.keepAlive = false;
        }
        // reset for next request if keepAlive
        this.headersParsed = false;
        this.headerText = '';
        this.req = null;
        if (!this.keepAlive) { this.close(); return; }
        // loop to check if next request already in buffer
      }
    }
    queueString(s){ this.queueSend(utf8Encode(String(s))); }
    queueSend(ab){ if (ab && ab.byteLength) this.sendQueue.push(ab); }
    flushSend() {
      if (!this.alive) return;
      while (this.sendQueue.length) {
        let ab = this.sendQueue[0];
        const arr = new Uint8Array(ab);
        const ptr = arr.byteOffset + this.sendOffset;
        const len = arr.byteLength - this.sendOffset;
        if (len <= 0) { this.sendQueue.shift(); this.sendOffset = 0; continue; }
        const chunk = ab.slice(this.sendOffset, this.sendOffset + len);
        const n = net.write(this.fd, chunk);
        if (!n || n <= 0) return; // would block
        if (n < len) { this.sendOffset += n; return; }
        // sent fully
        this.sendQueue.shift();
        this.sendOffset = 0;
      }
    }
    close(){
      if (!this.alive) return;
      try { net.close(this.fd); } catch(_) {}
      this.alive = false;
      try { this.onClose && this.onClose(); } catch(_) {}
    }
  }

  function startMiniHttp(opts) {
    opts = opts || {};
    const host = opts.host || '0.0.0.0';
    const port = opts.port | 0 || 9090;
    const autoLoop = !!opts.autoLoop;
    const intervalMs = opts.intervalMs | 0 || 10;
    const handler = typeof opts.onRequest === 'function' ? opts.onRequest : function(req, res) {
      if (req.method === 'GET' && req.path === '/ping') return res.send('pong');
      return res.json({ ok: true });
    };

    // listen; C++ shim may return true instead of numeric fd for server
    let serverFd = null;
    try { serverFd = net.listen(host, port); } catch(_) { serverFd = null; }

    const conns = new Map();
    function step() {
      const evs = (typeof net.poll === 'function' ? (net.poll() || []) : []);
      for (let i = 0; i < evs.length; i++) {
        const ev = evs[i] || {};
        const type = (ev.type || '').toLowerCase();
        const fd = ev.fd | 0;
        if (!fd) continue;
        if (type === 'accepted') {
          if (!conns.has(fd)) conns.set(fd, new HTTPConn(fd, handler, () => conns.delete(fd)));
        } else if (type === 'readable') {
          const c = conns.get(fd);
          if (!c || !c.alive) continue;
          while (true) {
            const ab = net.read(fd, 65536);
            if (!ab) break;
            c.feedData(ab);
          }
        } else if (type === 'writable') {
          const c = conns.get(fd);
          if (c && c.alive) c.flushSend();
        } else if (type === 'closed' || type === 'error') {
          const c = conns.get(fd);
          if (c) c.close();
        }
      }
      // post-flush
      for (const c of conns.values()) if (c && c.alive) c.flushSend();
    }

    let timerId = null;
    // Prefer engine-driven loop via uniset.step_cb if available
    if (global.uniset && typeof global.uniset.step_cb === 'function') {
      global.uniset.step_cb(step);
    } else if (autoLoop && typeof setInterval === 'function') {
      timerId = setInterval(step, intervalMs);
    }

    function close(){
      if (timerId != null && typeof clearInterval === 'function') clearInterval(timerId);
      for (const [,c] of conns) c.close();
      conns.clear();
      try { if (typeof serverFd === 'number') net.close(serverFd); } catch(_) {}
    }

    if( global.uniset && typeof global.uniset.stop_cb === 'function' ) {
      try {
        global.uniset.stop_cb(function () {
          try {
            close();
          } catch (_) {}
        });
      } catch (_) {}
    }

    return { host, port, serverFd, step, close };
  }

  if (typeof module !== 'undefined' && module.exports) module.exports = { startMiniHttp };
  else global.startMiniHttp = startMiniHttp;

})(typeof globalThis !== 'undefined' ? globalThis : this);
