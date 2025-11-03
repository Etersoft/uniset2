// uniset2-mini-http-router.compat.js
// ES5-compatible build (no default params, no arrow functions).
// Usage identical to the main router.
(function (global) {
  'use strict';

  function makeCors(opts) {
    opts = opts || {};
    var allowOrigin = (opts.origin === undefined) ? '*' : String(opts.origin);
    var allowMethods = (opts.methods && opts.methods.length) ? opts.methods.join(',') : 'GET,POST,PUT,PATCH,DELETE,OPTIONS,HEAD';
    var allowHeaders = (opts.headers && opts.headers.length) ? opts.headers.join(',') : 'Content-Type,Authorization';
    var allowCredentials = !!opts.credentials;
    var maxAge = (opts.maxAge != null) ? String(opts.maxAge) : '600';
    var mw = function (req, res, next) {
      try {
        if (res.setHeader) {
          res.setHeader('Access-Control-Allow-Origin', allowOrigin);
          res.setHeader('Access-Control-Allow-Methods', allowMethods);
          res.setHeader('Access-Control-Allow-Headers', allowHeaders);
          if (allowCredentials) res.setHeader('Access-Control-Allow-Credentials', 'true');
          if (maxAge) res.setHeader('Access-Control-Max-Age', maxAge);
        }
        if (req && req.method === 'OPTIONS') {
          if (typeof res.setStatus === 'function') res.setStatus(204);
          if (typeof res.end === 'function') res.end('');
          return;
        }
      } catch(_) {}
      if (typeof next === 'function') next();
    };
    mw.__isRouterCors = true;
    return mw;
  }

  function safeDecode(s) { try { return decodeURIComponent(s); } catch(_) { return s; } }
  function parseQuery(qs) {
    var out = Object.create(null);
    if (!qs) return out;
    var parts = qs.split('&');
    for (var i=0;i<parts.length;i++) {
      var kv = parts[i]; if (!kv) continue;
      var j = kv.indexOf('=');
      var k = safeDecode(j>=0 ? kv.slice(0,j) : kv);
      var v = safeDecode(j>=0 ? kv.slice(j+1) : '');
      if (out[k] === undefined) out[k] = v;
      else if (Array.isArray(out[k])) out[k].push(v);
      else out[k] = [out[k], v];
    }
    return out;
  }
  function splitPathAndQuery(rawPath) {
    var q = rawPath.indexOf('?');
    if (q < 0) return { path: rawPath, query: '' };
    return { path: rawPath.slice(0,q), query: rawPath.slice(q+1) };
  }
  function normalizePath(p) {
    if (!p || p[0] !== '/') return '/' + (p||'');
    return p.replace(/\/{2,}/g,'/');
  }
  function compilePattern(pattern, kind) {
    if (pattern instanceof RegExp) return { kind: 'regex', re: pattern, params: [] };
    var p = String(pattern);
    var info = { params: [] };
    if (kind === 'prefix' || /\/\*$/.test(p)) {
      if (/\/\*$/.test(p)) p = p.slice(0,-2) || '/';
      var base = p === '/' ? '' : p;
      var re = new RegExp('^' + base.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '(?:/.*)?$');
      return { kind: 'prefix', re: re, base: base, params: [] };
    }
    var hasParam = /(^|\/):[A-Za-z_][A-Za-z0-9_]*/.test(p);
    var hasStar  = /\*/.test(p);
    if (!hasParam && !hasStar) return { kind: 'exact', str: p };
    var reSrc = '^';
    var i = 0;
    var segs = p.split('/');
    for (var si=0; si<segs.length; si++) {
      var seg = segs[si];
      if (i++===0) continue;
      reSrc += '/';
      if (seg === '*') { reSrc += '.*'; continue; }
      if (seg.charAt(0) === ':') {
        var name = seg.slice(1);
        info.params.push(name);
        reSrc += '([^/]+)';
      } else {
        reSrc += seg.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
      }
    }
    reSrc += '$';
    return { kind: 'param', re: new RegExp(reSrc), params: info.params };
  }
  function calcWeight(entry) {
    var prio = entry.prio|0;
    var spec = 0;
    if (entry.matcher.kind === 'exact') spec = 300;
    else if (entry.matcher.kind === 'param') spec = 200;
    else if (entry.matcher.kind === 'prefix') spec = 150;
    else if (entry.matcher.kind === 'regex') spec = 100;
    var len = (entry.patternStr ? entry.patternStr.length : 0);
    return (prio * 1e6) + (spec * 1e3) + len;
  }
  function matchEntry(entry, method, path) {
    if (entry.method !== 'ALL' && entry.method !== method) return null;
    var m = entry.matcher;
    if (m.kind === 'exact') return (m.str === path) ? {} : null;
    if (m.kind === 'prefix') return m.re.test(path) ? {} : null;
    if (m.kind === 'param') {
      var mm = m.re.exec(path);
      if (!mm) return null;
      var params = Object.create(null);
      for (var i=0;i<m.params.length;i++) params[m.params[i]] = safeDecode(mm[i+1]);
      return params;
    }
    if (m.kind === 'regex') {
      var mm2 = m.re.exec(path);
      return mm2 ? { 0: mm2[0] } : null;
    }
    return null;
  }

  function createRouter(basePrefix, options) {
    var __logger = null;
    basePrefix = basePrefix || '';
    var routes = [];
    var globalMW = [];
    var errorMW = [];

    // Default CORS
    (function initCors() {
      var corsOpt = options ? options.cors : undefined;
      if (corsOpt === false) return;
      var mw;
      if (typeof corsOpt === 'function') mw = corsOpt;
      else if (global.cors && typeof global.cors === 'function') {
        mw = global.cors((corsOpt === undefined || corsOpt === true) ? {} : corsOpt);
        if (mw) mw.__isRouterCors = true;
      } else {
        mw = makeCors((corsOpt === undefined || corsOpt === true) ? {} : corsOpt);
      }
      if (mw) globalMW.push(mw);
    })();

    function add(method, pattern, handler, opts) {
      method = String(method || 'ALL').toUpperCase();
      var METHODS = ['GET','POST','PUT','PATCH','DELETE','HEAD','OPTIONS','TRACE','CONNECT','ALL'];
      if (METHODS.indexOf(method) === -1) method = 'ALL';
      var o = opts || {};
      var kind = o.kind || undefined;
      var matcher = compilePattern(pattern, kind);
      var entry = {
        method: method,
        handler: handler,
        matcher: matcher,
        prio: (o.prio|0) || 0,
        before: Array.isArray(o.before) ? o.before.slice() : [],
        after: Array.isArray(o.after) ? o.after.slice() : [],
        patternStr: typeof pattern === 'string' ? pattern : ''
      };
      entry.weight = calcWeight(entry);
      routes.push(entry);
      routes.sort(function(a,b){ return b.weight - a.weight; });
      return api;
    }

    function use(fn) {
      if (Array.isArray(fn)) { for (var i=0;i<fn.length;i++) use(fn[i]); }
      else if (typeof fn === 'function') globalMW.push(fn);
      return api;
    }

    function useError(fn) {
      if (Array.isArray(fn)) { for (var i=0;i<fn.length;i++) useError(fn[i]); }
      else if (typeof fn === 'function') errorMW.push(fn);
      return api;
    }

    
    function setLogger(fn) {
      __logger = (typeof fn === 'function') ? fn : null;
      return api;
    }
function group(prefix, gopts) {
      var pfx = normalizePath(basePrefix + (prefix||''));
      var g = createRouter(pfx);
      g.use = function(fn){ use(fn); return g; };
      g.useError = function(fn){ useError(fn); return g; };
      g.route = function(method, pattern, handler, opts){
        var pat = (typeof pattern === 'string')
          ? normalizePath(pfx + (pattern || ''))
          : (pattern instanceof RegExp ? new RegExp(pattern.source) : pattern);
        return add(method, pat, handler, opts);
      };
      g.group = function(subprefix, subopts){ return group(normalizePath(pfx + (subprefix||'')), subopts); };
      g.handle = handle;
      return g;
    }

    function setCors(v) {
      for (var i = globalMW.length - 1; i >= 0; i--) {
        if (globalMW[i] && globalMW[i].__isRouterCors) globalMW.splice(i,1);
      }
      if (v === false) return api;
      var mw;
      if (typeof v === 'function') mw = v;
      else if (global.cors && typeof global.cors === 'function') mw = global.cors((v === undefined || v === true) ? {} : v);
      else mw = makeCors((v === undefined || v === true) ? {} : v);
      if (mw) { mw.__isRouterCors = true; globalMW.unshift(mw); }
      return api;
    }

    function finalizeChain(chain, req, res, onDone) {
      var idx = -1;
      var call = function(i, err){
        if (err) return callError(err);
        if (i <= idx) return callError(new Error('next() called multiple times'));
        idx = i;
        if (i === chain.length) { if (onDone) onDone(); return; }
        try {
          var fn = chain[i];
          if (typeof fn === 'function' && fn.length < 3) {
            fn(req, res);
            return call(i+1);
          } else {
            fn(req, res, function(e){ call(i+1, e); });
          }
        } catch(e) {
          callError(e);
        }
      };
      var callError = function(err){
        if (!errorMW.length) {
          try {
            if (!res.__ended) {
              if (res.setHeader) res.setHeader('Content-Type','text/plain; charset=utf-8');
              if (res.setStatus) res.setStatus(500); else if (res.status) res.status(500);
              if (res.end) { res.end('Internal Server Error'); } else if (res.send) { res.send('Internal Server Error'); }
            }
          } catch(_) {}
          return;
        }
        var j = -1;
        var callE = function(k, e){
          if (k <= j) return; j = k;
          if (k === errorMW.length) {
            if (!res.__ended) {
              try {
                if (res.setHeader) res.setHeader('Content-Type','text/plain; charset=utf-8');
                if (res.setStatus) res.setStatus(500); else if (res.status) res.status(500);
                if (res.end) { res.end('Internal Server Error'); } else if (res.send) { res.send('Internal Server Error'); }
              } catch(_) {}
            }
            return;
          }
          try { errorMW[k](e, req, res, function(e2){ callE(k+1, e2||e); }); }
          catch (e3) { callE(k+1, e3); }
        };
        callE(0, err);
      };
      call(0);
    }

    function handle(req, res) {
      var __t0 = Date.now();
      var __status = 200;
      if (typeof res.end !== 'function' && typeof res.send === 'function') {
        res.end = function(s){ return res.send(s); };
      }
      req.method = String(req.method || '').toUpperCase();
      var sp = splitPathAndQuery(req.path || '/');
      req.path = normalizePath(sp.path);
      req.query = parseQuery(sp.query);
      req.params = Object.create(null);

      if (typeof res.status === 'function' && !res.__wrappedStatus) {
        var __origStatus = res.status;
        res.__wrappedStatus = true;
        res.status = function(code){ __status = (code|0) || __status; return __origStatus.apply(this, arguments); };
      }


      
      if (typeof res.end === 'function' && !res.__loggedEndWrap) {
        var __end = res.end;
        res.__loggedEndWrap = true;
        res.end = function(){
          try {
            if (__logger && !this.__loggedOnce) {
              this.__loggedOnce = true;
              var dt = Math.max(0, Date.now() - __t0);
              var info = { method: req.method, path: req.path, status: __status, ms: dt };
              try { __logger(info); } catch (e) {}
            }
          } catch(_){ }
          return __end.apply(this, arguments);
        };
      }
if (typeof res.end === 'function' && !res.__wrappedEnd) {
        // keep original end wrapper

        var orig = res.end;
        res.__wrappedEnd = true;
        res.__ended = false;
        res.end = function(){
          res.__ended = true;
          return orig.apply(this, arguments);
        };
      }

      var found = null;
      var params = null;
      for (var i=0;i<routes.length;i++) {
        var r = routes[i];
        var p = matchEntry(r, req.method, req.path);
        if (p) { found = r; params = p; break; }
      }
      if (!found) {
        if (res.setStatus) res.setStatus(404); else if (res.status) res.status(404);
        if (res.setHeader) res.setHeader('Content-Type','text/plain; charset=utf-8');
        if (res.end) { res.end('Not Found'); } else if (res.send) { res.send('Not Found'); }
        return;
      }
      req.params = params;

      var chain = [];
      for (var i1=0;i1<globalMW.length;i1++) chain.push(globalMW[i1]);
      for (var i2=0;i2<found.before.length;i2++) chain.push(found.before[i2]);
      chain.push(found.handler);
      for (var i3=0;i3<found.after.length;i3++) chain.push(found.after[i3]);

      finalizeChain(chain, req, res, function () {
        if (!res.__ended) {
          try {
            if (res.setStatus) res.setStatus(500); else if (res.status) res.status(500);
            if (res.setHeader) res.setHeader('Content-Type','text/plain; charset=utf-8');
            if (res.end) { res.end('Handler did not complete response'); } else if (res.send) { res.send('Handler did not complete response'); }
          } catch(_) {}
        }
      });
    }

    var api = {
      route: function(method, pattern, handler, opts){ return add(method, pattern, handler, opts); },
      use: use,
      useError: useError,
      group: group,
      handle: handle,
      setCors: setCors,
      setLogger: setLogger
    };
    return api;
  }

  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { createRouter: createRouter };
  } else {
    global.createRouter = createRouter;
  }
})(typeof globalThis !== 'undefined' ? globalThis : this);
