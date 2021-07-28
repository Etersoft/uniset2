/*
 * Copyright (c) 2021 Pavel Vainerman.
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
// --------------------------------------------------------------------------
//#include <sstream>
#include <fstream>
#include <Poco/File.h>
#include <Poco/Process.h>
#include "RunLock.h"
#include "Exceptions.h"
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------

RunLock::RunLock( const std::string& _lockfile ): 
    lockfile(_lockfile)
{
}

RunLock::~RunLock()
{
    if( isLocked() )
    {
        try
        {
            unlock();
        }
        catch(...){}
    }
}
// --------------------------------------------------------------------------
bool RunLock::isLockOwner() const
{
    if( !uniset::file_exist(lockfile) )
        return false;

    ifstream pidfile(lockfile.c_str());
    if( !pidfile )
        throw ORepFailed("(RunLock): can't read lockfile " + lockfile);

    int pid;
    pidfile >> pid;
    return pid == Poco::Process::id();
}
// --------------------------------------------------------------------------
bool RunLock::isLocked() const
{
    if( !uniset::file_exist(lockfile) )
        return false;

    ifstream pidfile(lockfile.c_str());
    if( !pidfile )
        throw ORepFailed("(RunLock): can't read lockfile " + lockfile);

    int pid;
    pidfile >> pid;

    return Poco::Process::isRunning(pid);
}
// --------------------------------------------------------------------------
bool RunLock::lock() const
{
    if( isLocked() )
        return true;

    ofstream pidfile(lockfile.c_str(), ios::out | ios::trunc);
    if( !pidfile )
        return false;
//      throw ORepFailed("(RunLock): can't create lockfile " + lockfile);

    pidfile << Poco::Process::id() << endl;
    pidfile.close();
    return true;
}
// --------------------------------------------------------------------------
bool RunLock::unlock() const
{
    if( !isLocked() )
        return true;

    if( !isLockOwner() )
        return false;

    Poco::File f(lockfile);

    try
    {
        f.remove(false);
        return true;
    }
    catch(...) {}

    return false;
}
// --------------------------------------------------------------------------
