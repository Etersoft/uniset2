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
// -------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "RunLock.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    //  std::ios::sync_with_stdio(false);

    if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
    {
        cout << endl;
        cout << "Usage: uniset2-smemory args1 args2" << endl;
        cout << endl;
        SharedMemory::help_print(argc, argv);
        cout << uniset::Configuration::help() << endl;
        return 0;
    }

    std::shared_ptr<RunLock> rlock = nullptr;

    try
    {
        int n = uniset::findArgParam("--run-lock",argc, argv);
        if( n != -1 )
        {
            if( n >= argc )
            {
                cerr << "Unknown lock file. Use --run-lock filename" << endl;
                return 1;
            }

            rlock = make_shared<RunLock>(argv[n+1]);
            if( rlock->isLocked() )
            {
                cerr << "ERROR: process is already running.. Lockfile: " << argv[n+1] << endl;
                return 1;
            }

            cout << "Run with lockfile: " << string(argv[n+1]) << endl;
            rlock->lock();
        }

        auto conf = uniset_init(argc, argv);

        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( const SystemError& err )
    {
        cerr << "(smemory): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << "(smemory): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "(smemory): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(smemory): catch(...)" << endl;
    }

    if( rlock )
        rlock->unlock();

    return 1;
}
