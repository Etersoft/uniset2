#include <sstream>
#include <string>
#include <sys/wait.h>
#include "Configuration.h"
#include "Extensions.h"
// -------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -------------------------------------------------------------------------
namespace UniSetExtensions
{
    static std::shared_ptr<DebugStream> _dlog;

    std::shared_ptr<DebugStream> dlog()
    {
        if( _dlog )
            return _dlog;

        _dlog = make_shared<DebugStream>();

        auto conf = uniset_conf();
        if( conf )
            conf->initDebug(_dlog,"dlog");

        return _dlog;
    }
    // -------------------------------------------------------------------------
    static UniSetTypes::ObjectId shmID = DefaultObjectId;

    UniSetTypes::ObjectId getSharedMemoryID()
    {
        if( shmID != DefaultObjectId )
            return shmID;
        
        auto conf = uniset_conf();

        string sname = conf->getArgParam("--smemory-id","SharedMemory1");
        shmID = conf->getControllerID(sname);

        if( shmID == UniSetTypes::DefaultObjectId )
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
            s.replace(pos,2,"\n");
            pos = s.find("\\n");
        }
    }
    // -------------------------------------------------------------------------
    static xmlNode* xmlCalibrationsNode=0;

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

        for( ;it;it.goNext() )
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
} // end of namespace
// -------------------------------------------------------------------------
