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
#include <sstream>
#include <iostream>
#include <future>
#include <chrono>
#include "UniSetTypes.h"
#include "TCPCheck.h"
#include "UTCPStream.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    bool TCPCheck::check( const std::string& _iaddr, timeout_t tout ) noexcept
    {
        auto v = uniset::explode_str(_iaddr, ':');

        if( v.size() < 2 )
            return false;

        return check( v[0], uniset::uni_atoi(v[1]), tout );
    }
    // -----------------------------------------------------------------------------
    bool TCPCheck::check( const std::string& ip, int port, timeout_t tout_msec ) noexcept
    {
        try
        {
            std::future<bool> future = std::async(std::launch::async, [ = ]()
            {
                // Сама проверка...
                bool result = false;

                try
                {
                    UTCPStream t;
                    t.create(ip, port, tout_msec);
                    // если удалось создать соединение, значит OK
                    result = t.isConnected();
                    t.disconnect();
                }
                catch( ... ) {}

                return result;
            });

            std::future_status status;

            do
            {
                status = future.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(tout_msec));

                if( status == std::future_status::timeout )
                    return false;
            }
            while( status != std::future_status::ready );

            return future.get();
        }
        catch( std::exception& ex )
        {

        }

        return false;
    }
    // -----------------------------------------------------------------------------
    bool TCPCheck::ping( const std::string& ip, timeout_t tout_msec, const std::string& ping_args ) noexcept
    {
        try
        {
            std::future<bool> future = std::async(std::launch::async, [ = ]()
            {
                // Сама проверка...
                ostringstream cmd;
                cmd << "ping " << ping_args << " " << ip << " 2>/dev/null 1>/dev/null";

                int ret = system(cmd.str().c_str());
                int res = WEXITSTATUS(ret);
                return (res == 0);
            });

            std::future_status status;

            do
            {
                status = future.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(tout_msec));

                if( status == std::future_status::timeout )
                    return false;
            }
            while( status != std::future_status::ready );

            return future.get();
        }
        catch( std::exception& ex )
        {

        }

        return false;
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
