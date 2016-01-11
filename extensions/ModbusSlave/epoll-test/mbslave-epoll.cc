#include <event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>

using namespace std;

// Read/write buffer max length
static const size_t MAX_BUF = 512;

typedef struct
{
	struct event ev;
	char         buf[MAX_BUF];
	size_t       offset;
	size_t       size;
} ConnectionData;

void on_connect(int fd, short event, void* arg);
void client_read(int fd, short event, void* arg);
void client_write(int fd, short event, void* arg);

int main(int argc, char** argv)
{
	// Check arguments
	if (argc < 3)
	{
		std::cout << "Run with options: <ip address> <port>" << std::endl;
		return 1;
	}

	// Create server socket
	int server_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (server_sock == -1)
	{
		std::cerr << "Failed to create socket" << std::endl;
		return 1;
	}

	sockaddr_in sa;
	int         on      = 1;
	char*       ip_addr = argv[1];
	short       port    = atoi(argv[2]);

	sa.sin_family       = AF_INET;
	sa.sin_port         = htons(port);
	sa.sin_addr.s_addr  = inet_addr(ip_addr);

	// Set option SO_REUSEADDR to reuse same host:port in a short time
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
	{
		std::cerr << "Failed to set option SO_REUSEADDR" << std::endl;
		return 1;
	}

	// Bind server socket to ip:port
	if (bind(server_sock, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) == -1)
	{
		std::cerr << "Failed to bind server socket" << std::endl;
		return 1;
	}

	// Make server to listen
	if (listen(server_sock, 10) == -1)
	{
		std::cerr << "Failed to make server listen" << std::endl;
		return 1;
	}

	// Init events
	struct event evserver_sock;
	// Initialize
	event_init();
	// Set connection callback (on_connect()) to read event on server socket
	event_set(&evserver_sock, server_sock, EV_READ | EV_PERSIST, on_connect, &evserver_sock);
	// Add server event without timeout
	event_add(&evserver_sock, NULL);

	// Dispatch events
	event_dispatch();

	return 0;
}

// Handle new connection {{{
void on_connect(int fd, short event, void* arg)
{
	sockaddr_in client_addr;
	socklen_t   len = 0;

	// Accept incoming connection
	int sock = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &len);

	if (sock < 1)
	{
		return;
	}

	cerr << "*** (ON CONNECT): ..." << endl;


	// Set read callback to client socket
	ConnectionData* data = new ConnectionData;
	event_set(&data->ev, sock, EV_READ, client_read, data);
	// Reschedule server event
	event_add(reinterpret_cast<struct event*>(arg), NULL);
	// Schedule client event
	event_add(&data->ev, NULL);
}
//}}}

// Handle client request {{{
void client_read(int fd, short event, void* arg)
{
	cerr << "*** (READ): RESCHEDULE..." << endl;

	ConnectionData* data = reinterpret_cast<ConnectionData*>(arg);

	if (!data)
	{
		close(fd);
		return;
	}

	int len = read(fd, data->buf, MAX_BUF - 1);

	if (len < 1)
	{
		close(fd);
		delete data;
		return;
	}

	data->buf[len] = 0;
	data->size     = len;
	data->offset   = 0;
	// Set write callback to client socket
	event_set(&data->ev, fd, EV_WRITE, client_write, data);
	// Schedule client event
	event_add(&data->ev, NULL);
}
//}}}

// Handle client responce {{{
void client_write(int fd, short event, void* arg)
{
	ConnectionData* data = reinterpret_cast<ConnectionData*>(arg);

	if (!data)
	{
		close(fd);
		return;
	}

	// Send data to client
	int len = write(fd, data->buf + data->offset, data->size - data->offset);

	if (len < data->size - data->offset)
	{
		// Failed to send rest data, need to reschedule
		data->offset += len;
		event_set(&data->ev, fd, EV_WRITE, client_write, data);
		// Schedule client event
		event_add(&data->ev, NULL);
		return;
	}

	//	close(fd);
	//	delete data;

	cerr << "*** (WRITE): RESCHEDULE..." << endl;

	event_set(&data->ev, fd, EV_READ, client_read, data);
	event_add(&data->ev, NULL);

}
//}}}
