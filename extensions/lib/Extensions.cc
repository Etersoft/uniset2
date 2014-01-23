#include <sstream>
#include <string>
#include "Configuration.h"
#include "Extensions.h"
// -------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -------------------------------------------------------------------------
namespace UniSetExtensions
{
    DebugStream dlog;
    // -------------------------------------------------------------------------
    static UniSetTypes::ObjectId shmID = DefaultObjectId;

    UniSetTypes::ObjectId getSharedMemoryID()
    {
        if( shmID != DefaultObjectId )
            return shmID;

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
    static int heartBeatTime = -1; // начальная инициализация
    int getHeartBeatTime()
    {
        if( heartBeatTime != -1 )
            return heartBeatTime;

        xmlNode* cnode = conf->getNode("HeartBeatTime");
        if( cnode == NULL )
        {
            ostringstream err;
            err << "Not found conf-node for HeartBeatTime";
            cerr << err.str() << endl;
            throw SystemError(err.str());
        }

        UniXML_iterator it(cnode);

        heartBeatTime = it.getIntProp("time_msec");
        if( heartBeatTime <= 0 )
        {
            heartBeatTime = 0;
            dwarn << "(getHeartBeatTime): механизм 'HEARTBEAT' ОТКЛЮЧЁН!" << endl;
        }

        dinfo << "(getHeartBeatTime): heartbeat time = " << heartBeatTime << endl;

        return heartBeatTime;
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
        
        xmlCalibrationsNode = conf->getNode("Calibrations");
        return xmlCalibrationsNode;
        
    }
    // -------------------------------------------------------------------------

    xmlNode* findNode( xmlNode* node, const string& snode, const string& field )
    {
        if( !node )
            return 0;
    
        UniXML_iterator it(node);
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

} // end of namespace
// -------------------------------------------------------------------------
