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
//--------------------------------------------------------------------------
#include <ostream>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "USingleProcess.h"
//---------------------------------------------------------------------------
using namespace uniset;
using namespace std;
//---------------------------------------------------------------------------
USingleProcess::USingleProcess( xmlNode* cnode, int argc, const char* const argv[], const std::string& prefix )
{

    std::string lockfile = "";

    std::string param = "--run-lock";
    if( !prefix.empty() )
        param = prefix + "-run-lock";

    int n = uniset::findArgParam(param, argc, argv);

    if( n != -1 )
    {
        if( n >= argc )
        {
            ostringstream err;
            err << "Undefined lock file name. Use " << param << " filename";
            throw SystemError(err.str());
        }

        lockfile = std::string(argv[n + 1]);
    }
    else if( cnode )
    {
        UniXML::iterator it(cnode);
        lockfile = it.getProp("lockfile");
    }

    if( !lockfile.empty() )
    {
        rlock = std::make_shared<RunLock>(lockfile);

        if( rlock->isLocked() )
        {
            ostringstream err;
            err << "ERROR: process is already running.. Lockfile: " << lockfile;
            throw SystemError(err.str());
        }

        rlock->lock();
    }
}
//---------------------------------------------------------------------------
USingleProcess::~USingleProcess()
{
    if( rlock )
        rlock->unlock();
}
//---------------------------------------------------------------------------
