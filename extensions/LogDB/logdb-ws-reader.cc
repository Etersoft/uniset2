#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/NetException.h>
#include <Poco/Exception.h>
#include <Poco/Buffer.h>
#include <Poco/URI.h>

using namespace Poco;
using namespace Poco::Net;
using namespace std::chrono_literals;

static std::atomic_bool stopFlag{false};

void onSignal(int)
{
    stopFlag = true;
}

struct Config
{
    std::string wsUrl = "ws://localhost:8080/ws/connect/logserver1";
    int recv_buf = 0; // 0 - не менять
    int pause_after_frames = 0; // после N кадров засыпаем
    int pause_ms = 0;           // на сколько засыпать
    int max_frames = 0;         // 0 - без лимита
    bool verbose = false;
};

static void print_help()
{
    std::cout << "uniset2-logdb-ws-reader options:\n"
              << "  --ws URL                 WebSocket URL (обязательно). Пример: --ws ws://localhost:8080/ws/connect/logserver1\n"
              << "  --recv-buf BYTES         set SO_RCVBUF (client side)\n"
              << "  --pause-after N          sleep after receiving N frames\n"
              << "  --pause-ms MS            sleep duration for pause-after\n"
              << "  --max-frames N           stop after N frames (0 = infinite)\n"
              << "  -v|--verbose             устарело (кадры выводятся всегда)\n"
              << "  -h|--help                this help\n";
}

static bool parse_args(int argc, char** argv, Config& cfg)
{
    bool ws_set = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);

        if (a == "-h" || a == "--help")
        {
            print_help();
            return false;
        }

        if (a == "--ws" && i + 1 < argc)
        {
            cfg.wsUrl = argv[++i];
            ws_set = true;
        }
        else if (a == "--recv-buf" && i + 1 < argc) cfg.recv_buf = std::stoi(argv[++i]);
        else if (a == "--pause-after" && i + 1 < argc) cfg.pause_after_frames = std::stoi(argv[++i]);
        else if (a == "--pause-ms" && i + 1 < argc) cfg.pause_ms = std::stoi(argv[++i]);
        else if (a == "--max-frames" && i + 1 < argc) cfg.max_frames = std::stoi(argv[++i]);
        else if (a == "--verbose" || a == "-v") cfg.verbose = true; // сохраняем для совместимости, но не нужен
    }

    if (!ws_set)
    {
        std::cerr << "ERROR: --ws URL is required (e.g. --ws ws://localhost:8080/ws/connect/logserver1)\n";
        print_help();
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    Config cfg;

    if (!parse_args(argc, argv, cfg))
        return 0;

    if (cfg.verbose)
    {
        try
        {
            Poco::URI u(cfg.wsUrl);
            std::cout << "connect to: " << u.toString()
                      << "  scheme=" << u.getScheme()
                      << "  host=" << u.getHost()
                      << "  port=" << u.getPort()
                      << "  path+query=" << (u.getPathAndQuery().empty() ? "/" : u.getPathAndQuery())
                      << std::endl;
        }
        catch(const std::exception& ex)
        {
            std::cerr << "parse ws url error: " << ex.what() << std::endl;
        }
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    try
    {
        Poco::URI uri(cfg.wsUrl);

        if (uri.getScheme() != "ws" && uri.getScheme() != "http")
            throw std::runtime_error("Only ws:// or http:// schemes are supported");

        std::string path = uri.getPathAndQuery();

        if (path.empty())
            path = "/";

        if (cfg.verbose)
        {
            std::cout << "HTTP request: host=" << uri.getHost()
                      << " port=" << uri.getPort()
                      << " path=" << path
                      << std::endl;
        }

        HTTPClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        request.setHost(uri.getHost(), uri.getPort());
        request.set("Connection", "Upgrade");
        request.set("Upgrade", "websocket");
        request.set("Sec-WebSocket-Version", "13");
        request.set("Sec-WebSocket-Key", "67+diN4x1mlKAw9fNm4vFQ=="); // постоянный ключ нам неважен

        HTTPResponse response;
        session.setTimeout(Timespan(5, 0));

        WebSocket ws(session, request, response);

#ifdef SO_RCVBUF

        if (cfg.recv_buf > 0)
        {
            ws.impl()->setOption(SOL_SOCKET, SO_RCVBUF, cfg.recv_buf);
        }

#endif

        Buffer<char> buf(4096);
        int frames = 0;

        while (!stopFlag)
        {
            int flags = 0;
            int n = ws.receiveFrame(buf.begin(), buf.size(), flags);

            if (n <= 0 || (flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_CLOSE)
            {
                std::cerr << "connection closed by server\n";
                break;
            }

            frames++;

            // пинговый кадр — пропускаем вывод, но не считаем ошибкой
            if (n == 1 && buf[0] == '.')
                continue;

            std::cout << std::string(buf.begin(), n) << std::endl;

            if (cfg.max_frames > 0 && frames >= cfg.max_frames)
            {
                std::cerr << "max frames reached\n";
                break;
            }

            if (cfg.pause_after_frames > 0 && cfg.pause_ms > 0 && (frames % cfg.pause_after_frames) == 0)
            {
                std::cerr << "pause " << cfg.pause_ms << " ms after " << frames << " frames\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.pause_ms));
            }
        }

        ws.close();
    }
    catch (const WebSocketException& ex)
    {
        std::cerr << "ws-reader websocket error: " << ex.displayText()
                  << " (code=" << ex.code() << ")" << std::endl;
        return 1;
    }
    catch (const NetException& ex)
    {
        std::cerr << "ws-reader net error: " << ex.displayText() << std::endl;
        return 1;
    }
    catch (const Poco::IOException& ex)
    {
        std::cerr << "ws-reader io error: " << ex.displayText() << std::endl;
        return 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "ws-reader error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
