#include "JSEngine_os_nb.h"
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

using namespace Poco;
using namespace Poco::Net;

JSEngineOS_NB::JSEngineOS_NB(JSRuntime* rt, JSContext* ctx)
    : _rt(rt), _ctx(ctx)
{
    _reactor = std::make_unique<SocketReactor>();
    _obsAccept  = std::make_unique<NObserver<JSEngineOS_NB, ReadableNotification>>(*this, &JSEngineOS_NB::onAccept);
    _obsRead    = std::make_unique<NObserver<JSEngineOS_NB, ReadableNotification>>(*this, &JSEngineOS_NB::onReadable);
    _obsWrite   = std::make_unique<NObserver<JSEngineOS_NB, WritableNotification>>(*this, &JSEngineOS_NB::onWritable);
    _obsError   = std::make_unique<NObserver<JSEngineOS_NB, ErrorNotification>>(*this, &JSEngineOS_NB::onError);
    _obsShutdown = std::make_unique<NObserver<JSEngineOS_NB, ShutdownNotification>>(*this, &JSEngineOS_NB::onShutdown);
}

JSEngineOS_NB::~JSEngineOS_NB()
{
    try
    {
        stop();
    }
    catch (...) {}
}

void JSEngineOS_NB::startReactor()
{
    if (_started.load()) return;

    _stopping.store(false);
    _reactorThread = std::thread([this]
    {
        try
        {
            _started.store(true);
            _reactor->run();
        }
        catch (...)
        {
        }
        _started.store(false);
    });
}

void JSEngineOS_NB::preStop()
{
    if (!_reactor) return;

    _stopping.store(true);
    disarmAll();

    try
    {
        _reactor->stop();
    }
    catch (...) {}

    try
    {
        _reactor->wakeUp();
    }
    catch (...) {}
}

void JSEngineOS_NB::joinReactor()
{
    if (_reactorThread.joinable())
    {
        try
        {
            _reactorThread.join();
        }
        catch (...) {}
    }
}

bool JSEngineOS_NB::listen(const std::string& addr, uint16_t port, std::string* err)
{
    try
    {
        if (_listening)
        {
            stop();
            _reactor = std::make_unique<SocketReactor>();
            _obsAccept  = std::make_unique<NObserver<JSEngineOS_NB, ReadableNotification>>(*this, &JSEngineOS_NB::onAccept);
            _obsRead    = std::make_unique<NObserver<JSEngineOS_NB, ReadableNotification>>(*this, &JSEngineOS_NB::onReadable);
            _obsWrite   = std::make_unique<NObserver<JSEngineOS_NB, WritableNotification>>(*this, &JSEngineOS_NB::onWritable);
            _obsError   = std::make_unique<NObserver<JSEngineOS_NB, ErrorNotification>>(*this, &JSEngineOS_NB::onError);
            _obsShutdown = std::make_unique<NObserver<JSEngineOS_NB, ShutdownNotification>>(*this, &JSEngineOS_NB::onShutdown);
        }

        SocketAddress sa(addr, port);
        _server = ServerSocket(sa);
        _server.setBlocking(false);
        _server.setReuseAddress(true);
        _server.setReusePort(true);

        startReactor();
        _reactor->addEventHandler(_server, *_obsAccept);
        _listening = true;
        return true;
    }
    catch (const std::exception& ex)
    {
        if (err) *err = ex.what();
    }
    catch (...)
    {
        if (err) *err = "unknown error in listen()";
    }

    return false;
}

void JSEngineOS_NB::stop()
{
    preStop();
    joinReactor();
    closeAll();

    try
    {
        _server.close();
    }
    catch (...) {}

    _listening = false;
    {
        std::lock_guard lk(_qMu);
        _queue.clear();
    }
}

void JSEngineOS_NB::onAccept(const AutoPtr<ReadableNotification>& pN)
{
    if (_stopping.load()) return;

    try
    {
        if (pN->socket() != _server) return;

        StreamSocket ss = _server.acceptConnection();
        ss.setBlocking(false);
        int fd = ss.impl()->sockfd();
        auto up = std::make_unique<StreamSocket>(std::move(ss));
        StreamSocket& ref = *up;
        _clients.emplace(fd, std::move(up));
        armClient(ref);
        enqueue(Ev{EvType::Accepted, fd, 0, "accepted"});
    }
    catch (const std::exception& ex)
    {
        enqueue(Ev{EvType::Error, _server.impl()->sockfd(), 0, std::string("accept failed: ") + ex.what()});
    }
    catch (...)
    {
        enqueue(Ev{EvType::Error, _server.impl()->sockfd(), 0, "accept failed: unknown"});
    }
}

void JSEngineOS_NB::armClient(StreamSocket& ss)
{
    if (_stopping.load()) return;

    try
    {
        _reactor->addEventHandler(ss, *_obsRead);
        _reactor->addEventHandler(ss, *_obsWrite);
        _reactor->addEventHandler(ss, *_obsError);
    }
    catch (...) {}
}

void JSEngineOS_NB::onReadable(const AutoPtr<ReadableNotification>& pN)
{
    if (_stopping.load()) return;

    try
    {
        int fd = pN->socket().impl()->sockfd();
        enqueue(Ev{EvType::Readable, fd, 0, {}});
    }
    catch (...)
    {
        enqueue(Ev{EvType::Error, -1, 0, "onReadable failure"});
    }
}

void JSEngineOS_NB::onWritable(const AutoPtr<WritableNotification>& pN)
{
    if (_stopping.load()) return;

    try
    {
        int fd = pN->socket().impl()->sockfd();
        enqueue(Ev{EvType::Writable, fd, 0, {}});
    }
    catch (...)
    {
        enqueue(Ev{EvType::Error, -1, 0, "onWritable failure"});
    }
}

void JSEngineOS_NB::onError(const AutoPtr<ErrorNotification>& pN)
{
    if (_stopping.load()) return;

    try
    {
        int fd = pN->socket().impl()->sockfd();
        enqueue(Ev{EvType::Error, fd, 0, "socket error"});
    }
    catch (...)
    {
        enqueue(Ev{EvType::Error, -1, 0, "onError failure"});
    }
}

void JSEngineOS_NB::onShutdown(const AutoPtr<ShutdownNotification>& pN)
{
    (void)pN;
}

void JSEngineOS_NB::enqueue(Ev e)
{
    std::lock_guard lk(_qMu);
    _queue.emplace_back(std::move(e));
}

void JSEngineOS_NB::disarmAll()
{
    for (auto& kv : _clients)
    {
        auto& sp = kv.second;

        if (!sp) continue;

        StreamSocket& s = *sp;

        try
        {
            _reactor->removeEventHandler(s, *_obsRead);
        }
        catch (...) {}

        try
        {
            _reactor->removeEventHandler(s, *_obsWrite);
        }
        catch (...) {}

        try
        {
            _reactor->removeEventHandler(s, *_obsError);
        }
        catch (...) {}
    }

    try
    {
        if (_listening) _reactor->removeEventHandler(_server, *_obsAccept);
    }
    catch (...) {}
}

void JSEngineOS_NB::closeClient(int fd)
{
    auto it = _clients.find(fd);

    if (it != _clients.end())
    {
        try
        {
            StreamSocket& s = *it->second;

            try
            {
                _reactor->removeEventHandler(s, *_obsRead);
            }
            catch (...) {}

            try
            {
                _reactor->removeEventHandler(s, *_obsWrite);
            }
            catch (...) {}

            try
            {
                _reactor->removeEventHandler(s, *_obsError);
            }
            catch (...) {}

            try
            {
                s.shutdown();
            }
            catch (...) {}

            try
            {
                s.close();
            }
            catch (...) {}
        }
        catch (...) {}

        _clients.erase(it);
    }
}

void JSEngineOS_NB::closeAll()
{
    for (auto it = _clients.begin(); it != _clients.end(); )
    {
        int fd = it->first;
        ++it;
        closeClient(fd);
    }
}

StreamSocket* JSEngineOS_NB::findSocket(int fd)
{
    auto it = _clients.find(fd);

    if (it == _clients.end()) return nullptr;

    return it->second.get();
}

int JSEngineOS_NB::srecv(int fd, void* buf, int len)
{
    StreamSocket* s = findSocket(fd);

    if (!s) return -1;

    try
    {
        int n = s->receiveBytes(buf, len);

        if (n == 0)
        {
            enqueue(Ev{EvType::Closed, fd, 0, "peer closed"});
            closeClient(fd);
        }

        return n;
    }
    catch (...)
    {
        return -1;
    }
}

int JSEngineOS_NB::ssend(int fd, const void* buf, int len)
{
    StreamSocket* s = findSocket(fd);

    if (!s) return -1;

    try
    {
        return s->sendBytes(buf, len);
    }
    catch (...)
    {
        return -1;
    }
}

void JSEngineOS_NB::sclose(int fd)
{
    closeClient(fd);
    enqueue(Ev{EvType::Closed, fd, 0, "closed"});
}

void JSEngineOS_NB::pullEvents(std::vector<Ev>& out)
{
    std::deque<Ev> local;
    {
        std::lock_guard lk(_qMu);
        local.swap(_queue);
    }
    out.assign(local.begin(), local.end());
}

// -------------- QuickJS shim --------------

extern "C" {

    static JSEngineOS_NB* g_nb = nullptr;

    static const char* evTypeToStr(JSEngineOS_NB::EvType t)
    {
        switch (t)
        {
            case JSEngineOS_NB::EvType::Accepted:
                return "accepted";

            case JSEngineOS_NB::EvType::Readable:
                return "readable";

            case JSEngineOS_NB::EvType::Writable:
                return "writable";

            case JSEngineOS_NB::EvType::Closed:
                return "closed";

            case JSEngineOS_NB::EvType::Error:
                return "error";
        }

        return "unknown";
    }

    static JSValue js_throw_type(JSContext* ctx, const char* msg)
    {
        return JS_ThrowTypeError(ctx, "%s", msg);
    }

    static JSValue js_net_listen(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        if (!g_nb) return JS_ThrowInternalError(ctx, "net not initialized");

        if (argc < 2) return js_throw_type(ctx, "listen(addr, port)");

        const char* addr = JS_ToCString(ctx, argv[0]);
        int32_t port = 0;

        if (!addr) return js_throw_type(ctx, "addr must be string");

        if (JS_ToInt32(ctx, &port, argv[1]))
        {
            JS_FreeCString(ctx, addr);
            return js_throw_type(ctx, "port must be int");
        }

        std::string err;
        bool ok = g_nb->listen(addr, (uint16_t)port, &err);
        JS_FreeCString(ctx, addr);

        if (!ok) return JS_ThrowInternalError(ctx, "%s", err.c_str());

        return JS_TRUE;
    }

    // Pull all queued events (to be consumed by JS mini-http)
    static JSValue js_net_poll(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        (void)this_val;
        (void)argc;
        (void)argv;

        if (!g_nb) return JS_ThrowInternalError(ctx, "net not initialized");

        JSValue arr = JS_NewArray(ctx);
        std::vector<JSEngineOS_NB::Ev> local;
        g_nb->pullEvents(local);

        for (size_t i = 0; i < local.size(); ++i)
        {
            const auto& e = local[i];
            JSValue obj = JS_NewObject(ctx);
            JS_DefinePropertyValueStr(ctx, obj, "type", JS_NewString(ctx, evTypeToStr(e.type)), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "fd", JS_NewInt32(ctx, e.fd), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "err", JS_NewInt32(ctx, e.err), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "what", JS_NewString(ctx, e.what.c_str()), JS_PROP_C_W_E);
            JS_SetPropertyUint32(ctx, arr, (uint32_t)i, obj);
        }

        return arr;
    }

    // Compatibility aliases
    static JSValue js_net_step(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        return js_net_poll(ctx, this_val, argc, argv);
    }
    static JSValue js_net_dispatch(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        return js_net_poll(ctx, this_val, argc, argv);
    }

    static JSValue js_net_read(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        if (!g_nb) return JS_ThrowInternalError(ctx, "net not initialized");

        if (argc < 2) return js_throw_type(ctx, "read(fd, maxLen)");

        int32_t fd, maxLen;

        if (JS_ToInt32(ctx, &fd, argv[0]) || JS_ToInt32(ctx, &maxLen, argv[1]) || maxLen <= 0)
            return js_throw_type(ctx, "bad fd or maxLen");

        std::vector<uint8_t> buf((size_t)maxLen);
        int n = g_nb->srecv(fd, buf.data(), (int)buf.size());

        if (n <= 0) return JS_NULL;

        return JS_NewArrayBufferCopy(ctx, buf.data(), (size_t)n);
    }

    static JSValue js_net_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        if (!g_nb) return JS_ThrowInternalError(ctx, "net not initialized");

        if (argc < 2) return js_throw_type(ctx, "write(fd, data)");

        int32_t fd;

        if (JS_ToInt32(ctx, &fd, argv[0])) return js_throw_type(ctx, "fd must be int");

        size_t sz = 0;
        uint8_t* p = (uint8_t*)JS_GetArrayBuffer(ctx, &sz, argv[1]);

        if (!p && !JS_IsNull(argv[1])) return js_throw_type(ctx, "data must be ArrayBuffer/Uint8Array");

        int n = 0;

        if (p && sz) n = g_nb->ssend(fd, p, (int)sz);

        return JS_NewInt32(ctx, n < 0 ? 0 : n);
    }

    static JSValue js_net_close(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        if (!g_nb) return JS_ThrowInternalError(ctx, "net not initialized");

        if (argc < 1) return js_throw_type(ctx, "close(fd)");

        int32_t fd;

        if (JS_ToInt32(ctx, &fd, argv[0])) return js_throw_type(ctx, "fd must be int");

        g_nb->sclose(fd);
        return JS_UNDEFINED;
    }

    bool qjs_init_poco_net(JSContext* ctx)
    {
        if (g_nb) return true;

        JSRuntime* rt = JS_GetRuntime(ctx);
        g_nb = new JSEngineOS_NB(rt, ctx);

        JSValue net = JS_NewObject(ctx);
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, net, "listen",   JS_NewCFunction(ctx, js_net_listen,   "listen",   2));
        JS_SetPropertyStr(ctx, net, "poll",     JS_NewCFunction(ctx, js_net_poll,     "poll",     0));
        JS_SetPropertyStr(ctx, net, "step",     JS_NewCFunction(ctx, js_net_step,     "step",     0));
        JS_SetPropertyStr(ctx, net, "dispatch", JS_NewCFunction(ctx, js_net_dispatch, "dispatch", 0));
        JS_SetPropertyStr(ctx, net, "read",     JS_NewCFunction(ctx, js_net_read,     "read",     2));
        JS_SetPropertyStr(ctx, net, "write",    JS_NewCFunction(ctx, js_net_write,    "write",    2));
        JS_SetPropertyStr(ctx, net, "close",    JS_NewCFunction(ctx, js_net_close,    "close",    1));
        JS_SetPropertyStr(ctx, global, "net", net);
        JS_FreeValue(ctx, global);
        return true;
    }

    void qjs_net_pre_stop()
    {
        if (!g_nb) return;

        g_nb->preStop();
    }

    void qjs_shutdown_poco_net(JSContext* ctx)
    {
        (void)ctx;

        if (!g_nb) return;

        g_nb->joinReactor();
        g_nb->stop();
        delete g_nb;
        g_nb = nullptr;
    }

} // extern "C"
