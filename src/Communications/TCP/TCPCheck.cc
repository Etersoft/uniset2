/*
 * Copyright (c) 2015 Pavel Vainerman.
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
// -----------------------------------------------------------------------------
#include <functional>
#include <sstream>
#include <cstdlib>
#include "UniSetTypes.h"
#include "PassiveTimer.h"
#include "ThreadCreator.h"
#include "TCPCheck.h"
#include "UTCPStream.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
namespace uniset
{
// -----------------------------------------------------------------------------
TCPCheck::TCPCheck() noexcept:
	tout_msec(0)
{
}
// -----------------------------------------------------------------------------
TCPCheck::~TCPCheck() noexcept
{

}
// -----------------------------------------------------------------------------
bool TCPCheck::check( const std::string& _iaddr, timeout_t tout, timeout_t sleep_msec )
{
	auto v = uniset::explode_str(_iaddr, ':');

	if( v.size() < 2 )
		return false;

	return check( v[0], uniset::uni_atoi(v[1]), tout, sleep_msec );
}
// -----------------------------------------------------------------------------
bool TCPCheck::check( const std::string& _ip, int _port, timeout_t tout, timeout_t sleep_msec )
{
	ip = _ip;
	port = _port;
	tout_msec = tout;

	setResult(false);

	ThreadCreator<TCPCheck> t(this, &TCPCheck::check_thread);
	//	t.setCancel(ost::Thread::cancelDeferred);
	t.start();

	PassiveTimer pt(tout);

	while( !pt.checkTime() && t.isRunning() )
		msleep(sleep_msec);

	if( t.isRunning() ) // !getResult() )
		t.stop();

	return result;
}
// -----------------------------------------------------------------------------
void TCPCheck::check_thread()
{
	setResult(false);

	try
	{
		UTCPStream t;
		t.create(ip, port, tout_msec);
		t.setKeepAliveParams( (tout_msec > 1000 ? tout_msec / 1000 : 1) );
		setResult(true);
		t.disconnect();
	}
	catch( ... ) {}
}
// -----------------------------------------------------------------------------
template<typename T>
class TGuard
{
	public:

		TGuard( T* m, typename ThreadCreator<T>::Action a ):
			t(m, a)
		{
			t.start();
		}

		~TGuard()
		{
			if( t.isRunning() )
				t.stop();
		}

		inline bool isRunning()
		{
			return t.isRunning();
		}

	protected:
		ThreadCreator<T> t;
};

// -----------------------------------------------------------------------------
bool TCPCheck::ping( const std::string& _ip, timeout_t tout, timeout_t sleep_msec, const std::string& _ping_args )
{
	ip = _ip;
	tout_msec = tout;
	ping_args = _ping_args;

	setResult(false);

	TGuard<TCPCheck> t(this, &TCPCheck::ping_thread);

	PassiveTimer pt(tout);

	while( !pt.checkTime() && t.isRunning() )
		msleep(sleep_msec);

	return result;
}
// -----------------------------------------------------------------------------
void TCPCheck::ping_thread()
{
	setResult(false);

	ostringstream cmd;
	cmd << "ping " << ping_args << " " << ip << " 2>/dev/null 1>/dev/null";

	int ret = system(cmd.str().c_str());
	int res = WEXITSTATUS(ret);

	setResult((res == 0));
}
// -----------------------------------------------------------------------------
} // end of namespace uniset
