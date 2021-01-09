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
#include "Configuration.h"
#include "IOControl.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    //  std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset2-iocontrol args1 args2" << endl;
        cout << endl;
        cout << "--io-confile       - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        cout << "--io-logfile fname - выводить логи в файл fname. По умолчанию iocontrol.log" << endl;
        IOControl::help_print(argc, argv);
        cout << "Global options:" << endl;
        cout << uniset::Configuration::help() << endl;
        return 0;
    }

    try
    {
        auto conf = uniset_init(argc, argv);

        string logfilename = conf->getArgParam("--io-logfile", "iocontrol.log");
        string logname( conf->getLogDir() + logfilename );
        dlog()->logFile( logname );
        ulog()->logFile( logname );

        ObjectId shmID = DefaultObjectId;
        string sID = conf->getArgParam("--smemory-id");

        if( !sID.empty() )
            shmID = conf->getControllerID(sID);
        else
            shmID = getSharedMemoryID();

        if( shmID == DefaultObjectId )
        {
            cerr << sID << "? SharedMemoryID not found in "
                 << conf->getControllersSection() << " section" << endl;
            return 1;
        }

        auto ic = IOControl::init_iocontrol(argc, argv, shmID);

        if( !ic )
        {
            dcrit << "(iocontrol): init не прошёл..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(ic);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );

        ulogany << "\n\n\n";
        ulogany << "(main): -------------- IOControl START -------------------------\n\n";
        dlogany << "\n\n\n";
        dlogany << "(main): -------------- IOControl START -------------------------\n\n";
        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(iocontrol): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(iocontrol): catch(...)" << endl;
    }

    return 1;
}
