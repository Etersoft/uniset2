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
#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "LProcessor.h"
#include "RunLock.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    //  std::ios::sync_with_stdio(false);
    std::shared_ptr<RunLock> rlock = nullptr;
    try
    {
        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << endl;
            cout << "Usage: uniset2-logicproc args1 args2" << endl;
            cout << endl;
            cout << "--sleepTime msec        - Время между шагам рассчёта. По умолчанию: 200 милисек" << endl;
            cout << "--sm-ready-timeout msec - Максимальное время ожидания готовности SharedMemory к работе, перед началом работы. По умолчанию: 2 минуты" << endl;
            cout << "--run-lock file         - Запустить с защитой от повторного запуска" << endl;
            cout << endl;
            cout << uniset::Configuration::help() << endl;
            return 0;
        }

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

        auto conf = uniset_init( argc, argv );

        string logfilename(conf->getArgParam("--logicproc-logfile"));

        if( logfilename.empty() )
            logfilename = "logicproc.log";

        std::ostringstream logname;
        string dir(conf->getLogDir());
        logname << dir << logfilename;
        ulog()->logFile( logname.str() );
        dlog()->logFile( logname.str() );

        string schema = conf->getArgParam("--schema");

        if( schema.empty() )
        {
            dcrit << "schema-file not defined. Use --schema" << endl;
            return 1;
        }

        LProcessor plc;
        plc.execute(schema);
        return 0;
    }
    catch( const LogicException& ex )
    {
        cerr << ex << endl;
    }
    catch( const uniset::Exception& ex )
    {
        cerr << ex << endl;
    }
    catch( ... )
    {
        cerr << " catch ... " << endl;
    }

    if( rlock )
        rlock->unlock();

    return 1;
}
// -----------------------------------------------------------------------------
