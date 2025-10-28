/*
 * Copyright (c) 2025 Pavel Vainerman.
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
#include "UniSetActivator.h"
#include "JSProxy.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            JSProxy::help_print();
            return 0;
        }

        uniset_init(argc, argv);

        auto qjs = JSProxy::init(argc, argv, "js");

        if( !qjs )
            return 1;

        auto act = UniSetActivator::Instance();
        act->add(qjs);

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);
        return 0;
    }
    catch( std::exception& ex )
    {
        cerr << "(JSProxy::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(JSProxy::main): catch ..." << endl;
    }

    return 1;
}
