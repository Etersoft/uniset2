#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/StreamCopier.h"
#include <iostream>
// --------------------------------------------------------------------------
using namespace std;
using namespace Poco;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	try
	{
		//		uniset_init(argc, argv);
		Net::SocketAddress sa("www.habrahabr.ru", 80);
		Net::StreamSocket socket(sa);
		Net::SocketStream str(socket);
		str << "GET / HTTP/1.1\r\n"
			"Host: habrahabr.ru\r\n"
			"\r\n";

		str.flush();
		StreamCopier::copyStream(str, cout);

		//Слушающий сокет
		Net::ServerSocket srv(8080); // Биндим и начинаем слушать

		while(true)
		{
			Net::StreamSocket ss = srv.acceptConnection();
			{
				Net::SocketStream str(ss);
				str << "HTTP/1.0 200 OK\r\n"
					"Content-Type: text/html\r\n"
					"\r\n"
					"<html><head><title>Доброго времени суток</title></head>"
					"<body><h1><a href=\"http://habrahabr.ru\">Добро пожаловать на Хабр.</a></h1></body></html>"
					<< flush;
			}
		}

		return 0;
	}
	catch( const std::exception& e )
	{
		cerr << "(poco-test): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(poco-test): catch(...)" << endl;
	}

	return 1;
}
