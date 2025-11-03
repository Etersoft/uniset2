#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/Socket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketNotification.h>

extern "C" {
#include "quickjs/quickjs.h"
    bool qjs_init_poco_net(JSContext* ctx);
    void qjs_net_pre_stop(void);
    void qjs_shutdown_poco_net(JSContext* ctx);
}

class JSEngineOS_NB
{
    public:
        enum class EvType : uint8_t { Accepted, Readable, Writable, Closed, Error };
        struct Ev
        {
            EvType type;
            int fd;
            int err = 0;
            std::string what;
        };

        JSEngineOS_NB(JSRuntime* rt, JSContext* ctx);
        ~JSEngineOS_NB();

        void startReactor();
        void preStop();
        void joinReactor();

        bool listen(const std::string& addr, uint16_t port, std::string* err = nullptr);
        void stop();

        int  srecv(int fd, void* buf, int len);
        int  ssend(int fd, const void* buf, int len);
        void sclose(int fd);
        void pullEvents(std::vector<Ev>& out);

        JSContext* jsctx() const noexcept
        {
            return _ctx;
        }
        JSRuntime* jsrt()   const noexcept
        {
            return _rt;
        }

    private:
        void onAccept(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pN);
        void onReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pN);
        void onWritable(const Poco::AutoPtr<Poco::Net::WritableNotification>& pN);
        void onError(const Poco::AutoPtr<Poco::Net::ErrorNotification>& pN);
        void onShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pN);

        void enqueue(Ev e);
        void armClient(Poco::Net::StreamSocket& ss);
        void disarmAll();
        void closeAll();
        void closeClient(int fd);
        Poco::Net::StreamSocket* findSocket(int fd);

    private:
        JSRuntime* _rt = nullptr;
        JSContext* _ctx = nullptr;

        std::unique_ptr<Poco::Net::SocketReactor> _reactor;
        std::thread _reactorThread;
        std::atomic<bool> _started{false};
        std::atomic<bool> _stopping{false};

        Poco::Net::ServerSocket _server;
        bool _listening = false;

        std::unique_ptr<Poco::NObserver<JSEngineOS_NB, Poco::Net::ReadableNotification>> _obsAccept;
        std::unique_ptr<Poco::NObserver<JSEngineOS_NB, Poco::Net::ReadableNotification>> _obsRead;
        std::unique_ptr<Poco::NObserver<JSEngineOS_NB, Poco::Net::WritableNotification>> _obsWrite;
        std::unique_ptr<Poco::NObserver<JSEngineOS_NB, Poco::Net::ErrorNotification>>    _obsError;
        std::unique_ptr<Poco::NObserver<JSEngineOS_NB, Poco::Net::ShutdownNotification>> _obsShutdown;

        std::unordered_map<int, std::unique_ptr<Poco::Net::StreamSocket>> _clients;

        std::mutex _qMu;
        std::deque<Ev> _queue;
};
