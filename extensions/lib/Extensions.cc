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
#include <sstream>
#include <string>
#include <sys/wait.h>
#include "Configuration.h"
#include "Exceptions.h"
#include "Extensions.h"
// -------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    namespace extensions
    {
        static std::shared_ptr<DebugStream> _dlog;

        std::shared_ptr<DebugStream> dlog()
        {
            if( _dlog )
                return _dlog;

            _dlog = make_shared<DebugStream>();

            _dlog->setLogName("dlog");

            auto conf = uniset_conf();

            if( conf )
                conf->initLogStream(_dlog, "dlog");

            return _dlog;
        }
        // -------------------------------------------------------------------------
        static uniset::ObjectId shmID = DefaultObjectId;

        uniset::ObjectId getSharedMemoryID()
        {
            if( shmID != DefaultObjectId )
                return shmID;

            auto conf = uniset_conf();

            string sname = conf->getArgParam("--smemory-id", "SharedMemory1");
            shmID = conf->getControllerID(sname);

            if( shmID == uniset::DefaultObjectId )
            {
                ostringstream err;
                err << ": Unknown ID for '" << sname << "'" << endl;
                dcrit << err.str() << endl;
                throw SystemError(err.str());
            }

            // cout << "(uniset): shm=" << name << " id=" << shmID << endl;
            return shmID;
        }
        // -------------------------------------------------------------------------
        void escape_string( string& s )
        {
            if( s.empty() )
                return;

            string::size_type pos = s.find("\\n");

            while( pos != string::npos )
            {
                s.replace(pos, 2, "\n");
                pos = s.find("\\n");
            }
        }
        // -------------------------------------------------------------------------
        static xmlNode* xmlCalibrationsNode = 0;

        xmlNode* getCalibrationsSection()
        {
            if( xmlCalibrationsNode )
                return xmlCalibrationsNode;

            xmlCalibrationsNode = uniset_conf()->getNode("Calibrations");
            return xmlCalibrationsNode;

        }
        // -------------------------------------------------------------------------

        xmlNode* findNode( xmlNode* node, const string& snode, const string& field )
        {
            if( !node )
                return 0;

            UniXML::iterator it(node);

            if( !it.goChildren() )
                return 0;

            for( ; it; it.goNext() )
            {
                if( snode == it.getProp(field) )
                    return it;
            }

            return 0;
        }
        // -------------------------------------------------------------------------
        Calibration* buildCalibrationDiagram( const std::string& dname )
        {
            xmlNode* root = getCalibrationsSection();

            if( !root )
            {
                ostringstream err;
                err << "(buildCalibrationDiagram): НЕ НАЙДЕН корневой узел для калибровочных диаграмм";
                dcrit << err.str() << endl;
                throw SystemError( err.str());
            }

            xmlNode* dnode = findNode( root, dname, "name" );

            if( !dnode )
            {
                ostringstream err;
                err << "(buildCalibrationDiagram): НЕ НАЙДЕНА калибровочная диаграмма '" << dname << "'";
                dcrit << err.str() << endl;
                throw SystemError( err.str());
            }

            return new Calibration(dnode);
        }
        // -------------------------------------------------------------------------
        void on_sigchild( int sig )
        {
            while(1)
            {
                int istatus;
                pid_t pid = waitpid( -1, &istatus, WNOHANG );

                if( pid == -1 && errno == EINTR )  continue;

                if( pid <= 0 )  break;
            }
        }
        // --------------------------------------------------------------------------
    } // end of namespace extensions
} // end of namespace uniset
// -------------------------------------------------------------------------
