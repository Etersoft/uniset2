// -----------------------------------------------------------------------------
#include <sstream>
#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "modbus/TCPCheck.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
TCPCheck::TCPCheck():
    iaddr(""),tout_msec(0)
{
}
// -----------------------------------------------------------------------------
TCPCheck::~TCPCheck()
{

}
// -----------------------------------------------------------------------------
bool TCPCheck::check( const std::string& ip, int port, timeout_t tout, timeout_t sleep_msec )
{
    ostringstream s;
    s << ip << ":" << port;
    return check(s.str(), tout, sleep_msec);
}
// -----------------------------------------------------------------------------
bool TCPCheck::check( const std::string& _iaddr, timeout_t tout, timeout_t sleep_msec )
{
    iaddr = iaddr;
    tout_msec = tout;

    setResult(false);
    ThreadCreator<TCPCheck> t(this, &TCPCheck::check_thread);
    t.setCancel(ost::Thread::cancelDeferred);
    t.start();

    PassiveTimer pt(tout);
    while( !pt.checkTime() && t.isRunning() )
        msleep(sleep_msec);

    if( t.isRunning() ) // !getResult() )
        t.stop();

    return getResult();
}
// -----------------------------------------------------------------------------
void TCPCheck::check_thread()
{
    setResult(false);
    try
    {
        ost::Thread::setException(ost::Thread::throwException);
        ost::TCPStream t(iaddr.c_str(),ost::Socket::IPV4,536,true,tout_msec);
        setResult(true);
        t.disconnect();
    }
    catch(...){}
}
// -----------------------------------------------------------------------------
