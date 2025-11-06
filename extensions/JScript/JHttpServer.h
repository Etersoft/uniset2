/*
 * Copyright (c) 2025 Pavel Vainerman.
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
#ifndef JHttpServer_H_
#define JHttpServer_H_
// --------------------------------------------------------------------------
#include <memory>
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include "DebugStream.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    class JHttpServer final:
        public std::enable_shared_from_this<JHttpServer>,
        public Poco::Net::HTTPRequestHandlerFactory
    {
        public:
            JHttpServer( size_t httpMaxThreads, size_t httpMaxRequestQueue );
            ~JHttpServer() final;

            bool isRunning();
            void start( const std::string& host, int port );
            void stop();
            void softStop(std::chrono::milliseconds timeout);

            void setProcessTimeout(std::chrono::milliseconds d) noexcept
            {
                processTimeout_ = d;
            }
            std::chrono::milliseconds processTimeout() const noexcept
            {
                return processTimeout_;
            }

            inline std::shared_ptr<DebugStream> log() noexcept
            {
                return mylog;
            }

            class Factory : public HTTPRequestHandlerFactory
            {
                public:
                    Factory( JHttpServer* s ): srv(s) {}

                    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& req) override
                    {
                        return srv->createRequestHandler(req);
                    }
                private:
                    JHttpServer* srv;
            };

            struct RequestSnapshot
            {
                std::string method;
                std::string uri;
                std::string version;
                std::vector<std::pair<std::string, std::string>> headers;
                std::string body;
            };

            struct ResponseSnapshot
            {
                int status = 200;
                std::string reason = "OK";
                std::vector<std::pair<std::string, std::string>> headers;
                std::string body;
            };

            struct ResponseAdapter
            {
                ResponseSnapshot snap;
                bool finished = false;
            };

            using HandlerFn = std::function<ResponseSnapshot(const RequestSnapshot&)>;

            void httpLoop( HandlerFn& fn, size_t count, std::chrono::milliseconds& timeout );

            void setMaxQueueSize(size_t n) noexcept
            {
                queue_.setMaxSize(n);
            }

            class Handler;
            class RequestQueue
            {
                public:
                    struct Job
                    {
                        RequestSnapshot req;
                        std::promise<ResponseSnapshot> prom;
                    };

                    bool push(Job&& j)
                    {
                        {
                            std::lock_guard<std::mutex> lk(m_);

                            if (maxSize_ > 0 && q_.size() >= maxSize_) return false;

                            q_.emplace(std::move(j));
                        }
                        cv_.notify_one();
                        return true;
                    }

                    // returns false when shutting down and queue is empty
                    bool pop(Job& out, std::chrono::milliseconds timeout)
                    {
                        std::unique_lock<std::mutex> lk(m_);

                        bool ret = cv_.wait_for(lk, timeout, [&]
                        {
                            return stop_ || !q_.empty();
                        });

                        if( !ret )
                            return false;

                        if(stop_ && q_.empty())
                            return false;

                        out = std::move(q_.front());
                        q_.pop();
                        return true;
                    }


                    bool empty() const
                    {
                        std::lock_guard<std::mutex> lk(m_);
                        return q_.empty();
                    }

                    void setMaxSize(size_t n) noexcept
                    {
                        maxSize_ = n;
                    }

                    void shutdown()
                    {
                        {
                            std::lock_guard<std::mutex> lk(m_);
                            stop_ = true;
                        }
                        cv_.notify_all();
                    }

                private:
                    mutable std::mutex m_;
                    std::condition_variable cv_;
                    std::queue<Job> q_;
                    bool stop_ = false;
                    size_t maxSize_ = 0;
            };

        protected:
            friend class Factory;
            Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& ) override;

        private:
            Poco::Net::SocketAddress sa;
            std::shared_ptr<Poco::Net::HTTPServer> http;
            std::shared_ptr<DebugStream> mylog;
            std::atomic_bool started;

            // default timeout: 10s
            std::chrono::milliseconds processTimeout_{std::chrono::seconds(10)};

            // lifecycle
            std::atomic_bool started_{false};
            std::atomic_bool workerRunning_{false};
            std::atomic_bool softStopping_{false};
            std::atomic_bool workerBusy_{false};
            std::condition_variable cvStop_;
            std::mutex cvStopMutex_;

            size_t httpMaxThreads = { 3 };
            size_t httpMaxRequestQueue = { 50 };

            // request funnel
            RequestQueue queue_;

    };
    // ----------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif
