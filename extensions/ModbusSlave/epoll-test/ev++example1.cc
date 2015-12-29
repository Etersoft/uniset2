#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <list>
#include <unistd.h>
#include <iostream>

using namespace std;

//
//   Buffer class - allow for output buffering such that it can be written out
//                                 into async pieces
//
struct Buffer {
        char       *data;
        ssize_t len;
        ssize_t pos;

        Buffer(const char *bytes, ssize_t nbytes) {
            pos = 0;
            len = nbytes;
            data = new char[nbytes];
            memcpy(data, bytes, nbytes);
        }

        virtual ~Buffer() {
            delete [] data;
        }

        char *dpos() {
            return data + pos;
        }

        ssize_t nbytes() {
            return len - pos;
        }
};

//
//   A single instance of a non-blocking Echo handler
//
class EchoInstance {
    private:
        ev::io           io;
        static int total_clients;
        int              sfd;
        //ev::idle io_idle;

        // Buffers that are pending write
        std::list<Buffer*>     write_queue;

        // Generic callback
        void idle_callback(ev::idle &watcher, int revents) {

             cerr << "idle..." << endl;
/*
             if (!write_queue.empty())
                 write_data(sfd);
             else
             {
                 io_idle.stop();
              }
*/
            // io.set(ev::READ|ev::WRITE);
            // io_idle.stop();
        }

        // Generic callback
        void callback(ev::io &watcher, int revents) {

            cerr << "call..." << endl;

            if (EV_ERROR & revents) {
                perror("got invalid event");
                return;
            }

            if (revents & EV_READ)
                read_cb(watcher);

#if 0
//            if (revents & EV_WRITE)
//                write_cb(watcher);
//            if( !write_queue.empty() )
//                write_cb(watcher);

//            io.set(ev::READ);
#else
            if (revents & EV_WRITE)
                write_cb(watcher);
            if (write_queue.empty()) {
                io.set(ev::READ);
            } else {
                io.set(ev::READ|ev::WRITE);
            }
#endif
            cerr << "events: " << revents << " active: " <<  io.is_active()
                 << " is_pending: " << io.is_pending()
                 << " total: " << total_clients
                 <<  endl;

       }

        // Socket is writable
        void write_cb(ev::io &watcher) {
            write_data(watcher.fd);
        }

        void write_data(int wfd) {

            if (write_queue.empty()) {
                //io.set(ev::READ);
                return;
            }

            Buffer* buffer = write_queue.front();

            ssize_t written = write(wfd, buffer->dpos(), buffer->nbytes());
            if (written < 0) {
                perror("write error");
                return;
            }

            buffer->pos += written;
            if (buffer->nbytes() == 0) {
                write_queue.pop_front();
                delete buffer;
            }

            if (write_queue.empty()) {
                //io.set(ev::READ);
            }

            cout << "write: " << written << endl;
        }

        // Receive message from client socket
        void read_cb(ev::io &watcher) {
            char       buffer[1024];

            ssize_t nread = recv(watcher.fd, buffer, sizeof(buffer), 0);
            //ssize_t nread = read(watcher.fd, buffer, sizeof(buffer));

            cout << "read: " << nread << endl;

            if (nread < 0) {
                perror("read error");
                return;
            }

            if (nread == 0) {
                // Gack - we're deleting ourself inside of ourself!
                delete this; // (pv): ??!!!
            } else {
                // Send message bach to the client
                write_queue.push_back(new Buffer(buffer, nread));
                //io_idle.start();
            }
        }

        // effictivly a close and a destroy
        virtual ~EchoInstance() {
            // Stop and free watcher if client socket is closing
            io.stop();
            //io_idle.stop();

            close(sfd);

            printf("%d client(s) disconnected.\n", --total_clients);
        }

    public:
        EchoInstance(int s) : sfd(s) {
            fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);

            printf("Got connection\n");
            total_clients++;

            io.set<EchoInstance, &EchoInstance::callback>(this);

            io.start(s, ev::READ);

            //io_idle.set<EchoInstance, &EchoInstance::idle_callback>(this);
            //io_idle.start();
        }
};

class EchoServer {
    private:
        ev::io           io;
        ev::sig         sio;
        int                 s;

    public:

        void io_accept(ev::io &watcher, int revents) {
            if (EV_ERROR & revents) {
                perror("got invalid event");
                return;
            }

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_sd = accept(watcher.fd, (struct sockaddr *)&client_addr, &client_len);

            if (client_sd < 0) {
                perror("accept error");
                return;
            }

            EchoInstance *client = new EchoInstance(client_sd);
        }

        static void signal_cb(ev::sig &signal, int revents) {
            signal.loop.break_loop();
        }

        EchoServer(int port) {
            printf("Listening on port %d\n", port);

            struct sockaddr_in addr;

            s = socket(PF_INET, SOCK_STREAM, 0);

            addr.sin_family = AF_INET;
            addr.sin_port     = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
                perror("bind");
            }

            fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);

            int on = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
               perror("Could not set socket %d option for reusability: \n");

            listen(s, 5);

            io.set<EchoServer, &EchoServer::io_accept>(this);
            io.start(s, ev::READ);

            sio.set<&EchoServer::signal_cb>();
            sio.start(SIGINT);
        }

        virtual ~EchoServer() {
            shutdown(s, SHUT_RDWR);
            close(s);
        }
};

int EchoInstance::total_clients = 0;

int main(int argc, char **argv)
{
    int port = 8192;

    if (argc > 1)
        port = atoi(argv[1]);

    ev::default_loop loop;
    EchoServer echo(port);

    loop.run(0 /* ev::EPOLL */);

    return 0;
}
