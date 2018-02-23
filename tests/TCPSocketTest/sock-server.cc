#include "UTCPSocket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketStream.h"
#include <iostream>
// --------------------------------------------------------------------------
using namespace std;
using namespace  Poco;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	try
	{
		uniset::UTCPSocket sock("localhost", 2048);

		while(true)
		{
			Net::StreamSocket ss = sock.acceptConnection();
			Poco::FIFOBuffer buf(1000);

			if( ss.poll(uniset::UniSetTimer::millisecToPoco(5000), Poco::Net::Socket::SELECT_READ) )
			{
				int r = ss.receiveBytes(buf);

				cout << "recv(" << r << "): used=" << buf.used() << " [ ";

				for( size_t i = 0; i < r; i++)
					cout << (int)buf[i];

				//				std::unique_ptr<char[]> data(new char[buf.used()]);
				//				buf.read(data.get(), r);
				//				cout << std::string(data.get(), r) << endl;
				cout << " ]" << endl;
			}
		}

		return 0;
	}
	catch( const std::exception& e )
	{
		cerr << "(sock-server): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(sock-server): catch(...)" << endl;
	}

	return 1;
}
