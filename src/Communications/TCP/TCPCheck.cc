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
	bool TCPCheck::check( const std::string& _iaddr, timeout_t tout )
	{
		auto v = uniset::explode_str(_iaddr, ':');

		if( v.size() < 2 )
			return false;

		return check( v[0], uniset::uni_atoi(v[1]), tout );
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
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			~TGuard()
			{
				if( t.isRunning() )
					t.stop();
			}

		protected:
			ThreadCreator<T> t;
	};
	// -----------------------------------------------------------------------------
	bool TCPCheck::check( const std::string& _ip, int _port, timeout_t tout )
	{
		ip = _ip;
		port = _port;
		tout_msec = tout;
		thr_finished = false;
		result = false;

		TGuard<TCPCheck> t(this, &TCPCheck::check_thread);

		std::unique_lock<std::mutex> lock(thr_mutex);
		thr_event.wait_for(lock, std::chrono::milliseconds(tout), [ = ]()
		{
			return ( thr_finished == true );
		} );

		return result;
	}
	// -----------------------------------------------------------------------------
	void TCPCheck::check_thread()
	{
		thr_finished = false;
		result = false;

		try
		{
			UTCPStream t;
			t.create(ip, port, tout_msec);
			t.setKeepAliveParams( (tout_msec > 1000 ? tout_msec / 1000 : 1) );
			result = true;
			t.disconnect();
		}
		catch( ... ) {}

		thr_finished = true;
	}
	// -----------------------------------------------------------------------------
	bool TCPCheck::ping( const std::string& _ip, timeout_t tout, const std::string& _ping_args )
	{
		ip = _ip;
		tout_msec = tout;
		ping_args = _ping_args;
		thr_finished = false;
		result = false;


		TGuard<TCPCheck> t(this, &TCPCheck::ping_thread);

		std::unique_lock<std::mutex> lock(thr_mutex);
		thr_event.wait_for(lock, std::chrono::milliseconds(tout), [ = ]()
		{
			return ( thr_finished == true );
		} );

		return result;
	}
	// -----------------------------------------------------------------------------
	void TCPCheck::ping_thread()
	{
		thr_finished = false;
		result = false;

		ostringstream cmd;
		cmd << "ping " << ping_args << " " << ip << " 2>/dev/null 1>/dev/null";

		int ret = system(cmd.str().c_str());
		int res = WEXITSTATUS(ret);

		result = (res == 0);
		thr_finished = true;
	}
	// -----------------------------------------------------------------------------
} // end of namespace uniset
