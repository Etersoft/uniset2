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
// -------------------------------------------------------------------------
#include <vector>
#include <sstream>
#include <Poco/URI.h>
#include "Exceptions.h"
#include "JHttpServer.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace Poco::Net;
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
JHttpServer::JHttpServer( size_t _httpMaxThreads, size_t _httpMaxRequestQueue ):
    httpMaxThreads(_httpMaxThreads),
    httpMaxRequestQueue(_httpMaxRequestQueue)
{
    mylog = make_shared<DebugStream>();
    mylog->setLogName("JHttpServer");
    started = false;
}
// -------------------------------------------------------------------------
JHttpServer::~JHttpServer()
{
    stop();
}
// ----------------------------------------------------------------------------
bool JHttpServer::isRunning()
{
    return started.load();
}
// ----------------------------------------------------------------------------
void JHttpServer::stop()
{
    if( !started.load() )
        return;

    started.store(false);

    // Clear pending requests with error response
    queue_.clear(503, "Service Unavailable", "Server is stopping");
    queue_.shutdown();
    workerRunning_.store(false);

    if( http )
    {
        try
        {
            http->stop();
        }
        catch( Poco::Exception& ex )
        {
            mylog->crit() << "(JHttpServer::stop): error: " << ex.displayText() << endl;
        }
    }
}
// ----------------------------------------------------------------------------
void JHttpServer::start( const std::string& _host, int _port )
{
    if( started.load() )
        return;

    softStopping_.store(false);
    queue_.reset();

    if( !http )
    {
        sa = Poco::Net::SocketAddress(_host, _port);

        try
        {
            workerRunning_.store(true);

            HTTPServerParams* httpParams = new HTTPServerParams;
            httpParams->setMaxQueued(httpMaxRequestQueue);
            httpParams->setMaxThreads(httpMaxThreads);

            http = std::make_shared<Poco::Net::HTTPServer>(new Factory(this), ServerSocket(sa), httpParams);
        }
        catch( std::exception& ex )
        {
            std::stringstream err;
            err << "(JHttpServer::init): " << _host << ":" << _port << " ERROR: " << ex.what();
            throw uniset::SystemError(err.str());
        }
    }

    try
    {
        http->start();
        started.store(true);
        mylog->info() << "(JHttpServer::init): http server " << _host << ":" << _port << std::endl;
    }
    catch (std::exception& ex)
    {
        std::stringstream err;
        err << "(JHttpServer::init): " << _host << ":" << _port << " ERROR: " << ex.what();
        throw uniset::SystemError(err.str());
    }
}
// ----------------------------------------------------------------------------
class JHttpServer::Handler : public Poco::Net::HTTPRequestHandler
{
    public:
        explicit Handler(JHttpServer& owner) : owner_(owner) {}

        void handleRequest(Poco::Net::HTTPServerRequest& request,
                           Poco::Net::HTTPServerResponse& response) override
        {
            // 1) Enqueue job and wait for response from the single worker
            if (owner_.softStopping_.load(std::memory_order_relaxed))
            {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
                response.setReason("Service Unavailable");
                response.set("Connection", "close");
                response.set("Content-Type", "text/plain");
                static const char* msg = "Server is shutting down";
                response.setContentLength(std::strlen(msg));
                std::ostream& os = response.send();
                os.write(msg, static_cast<std::streamsize>(std::strlen(msg)));
                os.flush();
                return;
            }

            // 2) Snapshot request (no Poco objects cross threads)
            RequestSnapshot snap;
            snap.method  = request.getMethod();
            snap.uri     = request.getURI();
            snap.version = request.getVersion();

            for (const auto& h : request)
                snap.headers.emplace_back(h.first, h.second);

            {
                std::ostringstream body;
                body << request.stream().rdbuf();
                snap.body = std::move(body).str();
            }

            RequestQueue::Job job;
            job.req = std::move(snap);
            auto fut = job.prom.get_future();
            auto cancelled = job.cancelled;

            if (!owner_.queue_.push(std::move(job)))
            {
                response.setStatus(Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
                response.setReason("Service Unavailable");
                response.set("Content-Type", "text/plain");
                response.set("Connection", "close");
                static const char* msg = "Server is busy";
                response.setContentLength(std::strlen(msg));
                std::ostream& os = response.send();
                os.write(msg, static_cast<std::streamsize>(std::strlen(msg)));
                os.flush();
                return;
            }

            const auto rc = fut.wait_for(owner_.processTimeout());

            if (rc == std::future_status::ready)
            {
                ResponseSnapshot out = fut.get();

                response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(out.status));
                response.setReason(out.reason);

                for (auto& kv : out.headers) response.set(kv.first, kv.second);

                response.setContentLength(out.body.size());

                std::ostream& os = response.send();
                os.write(out.body.data(), static_cast<std::streamsize>(out.body.size()));
                os.flush();
                return;
            }

            // Timeout: mark as cancelled and reply 504
            cancelled->store(true, std::memory_order_relaxed);

            response.setStatus(Poco::Net::HTTPResponse::HTTP_GATEWAY_TIMEOUT);
            response.setReason("Gateway Timeout");
            response.set("Content-Type", "text/plain");
            response.set("Connection", "close");
            static const char* msg = "Processing timeout";
            response.setContentLength(std::strlen(msg));
            std::ostream& os = response.send();
            os.write(msg, static_cast<std::streamsize>(std::strlen(msg)));
            os.flush();
        }
    private:
        JHttpServer& owner_;
};
// ----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* JHttpServer::createRequestHandler( const Poco::Net::HTTPServerRequest& )
{
    return new Handler(*this);
}
// ----------------------------------------------------------------------------
void JHttpServer::softStop(std::chrono::milliseconds timeout)
{
    if (!started.load())
        return;

    // mark soft-stopping: refuse new jobs in handler
    softStopping_.store(true, std::memory_order_relaxed);

    // wait until queue is empty and worker is idle (or timeout)
    {
        std::unique_lock<std::mutex> lk(cvStopMutex_);
        cvStop_.wait_for(lk, timeout, [this]
        {
            return queue_.empty() && !workerBusy_.load(std::memory_order_relaxed);
        });
    }

    // Clear any remaining requests
    queue_.clear(503, "Service Unavailable", "Server is shutting down");
    queue_.shutdown();
    workerRunning_.store(false);

    if (http)
    {
        try
        {
            http->stop(); // stop accepting new connections
        }
        catch (Poco::Exception& ex)
        {
            mylog->crit() << "(JHttpServer::softStop): error: " << ex.displayText() << endl;
        }
        catch (std::exception& ex)
        {
            mylog->crit() << "(JHttpServer::softStop): error: " << ex.what() << endl;
        }
        catch (...)
        {
            mylog->crit() << "(JHttpServer::softStop): error: unknown" << endl;
        }
    }

    started.store(false);
}
// ----------------------------------------------------------------------------
void JHttpServer::httpLoop( HandlerFn& fn, size_t count, std::chrono::milliseconds& timeout )
{
    if( !workerRunning_ || softStopping_ || !started )
        return;

    for( size_t i = 0; i < count; i++ )
    {
        RequestQueue::Job job;

        if (!queue_.pop(job, timeout)) break;

        // Skip cancelled requests (timed out on client side)
        if (job.cancelled->load(std::memory_order_relaxed))
            continue;

        workerBusy_.store(true, std::memory_order_relaxed);

        ResponseSnapshot resp;

        try
        {
            resp = fn(job.req);
        }
        catch (const std::exception& ex)
        {
            resp.status = 500;
            resp.reason = "Internal Server Error";
            resp.headers = { {"Content-Type", "text/plain"} };
            resp.body = std::string("Exception: ") + ex.what();
        }
        catch (...)
        {
            resp.status = 500;
            resp.reason = "Internal Server Error";
            resp.headers = { {"Content-Type", "text/plain"} };
            resp.body = "Unknown error";
        }

        // Check again before set_value (Handler might have timed out while we were processing)
        if (!job.cancelled->load(std::memory_order_relaxed))
            job.prom.set_value(std::move(resp));

        workerBusy_.store(false, std::memory_order_relaxed);
        cvStop_.notify_one();
    }
}
