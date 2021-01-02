#include "UTCPStream.h"
#include "UniSetTypes.h"
#include <iostream>
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    try
    {
        std::string host = "localhost";

        if( argc > 1 )
            host = std::string(argv[1]);

        uniset::UTCPStream tcp;

        while(true)
        {
            {
                tcp.create(host, 2048);
                std::string buf = "1234567890.10.11.12.13.14.15.16.17.18.29.20.21ksdjcb sdjcb sldfjvbds fljvhbds jfvhbdsbvjksdbfvjdssvksdbvjsdbvjsbfjskdfbvjsdfbvjdfbvFINISH\n";
                size_t nbytes = tcp.sendBytes(buf.data(), buf.size());
                cout << "real bytes: " << buf.size() << " send: " << nbytes << " bytes" << endl;
                tcp.disconnect();
            }
            msleep(5000);
        }

        return 0;
    }
    catch( const std::exception& e )
    {
        cerr << "(sock-client): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(sock-client): catch(...)" << endl;
    }

    return 1;
}
