// --------------------------------------------------------------------------
#include <string>
#include <sys/wait.h>
#include <error.h>
#include <errno.h>
#include <Debug.h>
#include <UniSetActivator.h>
#include <ThreadCreator.h>
#include "Extensions.h"
#include "RTUExchange.h"
#include "MBSlave.h"
#include "MBTCPMaster.h"
#include "SharedMemory.h"
#include "IOControl.h"
//#include "UniExchange.h"
#include "UNetExchange.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
const int MaxAddNum = 10;
// --------------------------------------------------------------------------
void on_sigchild( int sig )
{
//  while( waitpid(0,NULL,WNOHANG) > 0 ){}
    while(1)
    {
        int istatus;
        pid_t pid = waitpid( -1, &istatus, WNOHANG );
        if( pid == -1 && errno == EINTR )  continue;
        if( pid <= 0 )  break;
    }
}
// --------------------------------------------------------------------------
static void help_print( int argc, const char* argv[] );
// --------------------------------------------------------------------------
int main( int argc, const char **argv )
{
    if( argc>1 && ( strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 ) )
    {
        help_print( argc, argv );
        return 0;
    }

    try
    {
        string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
        conf = new Configuration(argc, argv, confile);

        string logfilename = conf->getArgParam("--logfile", "smemory-plus.log");
        string logname( conf->getLogDir() + logfilename );
        UniSetExtensions::dlog.logFile( logname );
        ulog.logFile( logname );
        conf->initDebug(UniSetExtensions::dlog,"dlog");

        UniSetActivator act;
        // ------------ SharedMemory ----------------
        SharedMemory* shm = SharedMemory::init_smemory(argc,argv);
        if( shm == NULL )
            return 1;

        act.addManager(static_cast<class UniSetManager*>(shm));
        
        // ------------ IOControl ----------------
        std::list< ThreadCreator<IOControl>* > lst_iothr;
        for( int i=0; i<MaxAddNum; i++ )
        {
            stringstream s;
            s << "--add-io";
            if( i>0 ) s << i;
                
            bool add_io = findArgParam(s.str(),argc,argv) != -1;
            
            if( add_io )
            {
                stringstream p;
                p << "io";
                if( i > 0 ) p << i;

                if( dlog.is_info() )
                    dlog.info() << "(smemory-plus): add IOControl(" << p.str() << ")" << endl;

                IOControl* ic = IOControl::init_iocontrol(argc,argv,shm->getId(),shm,p.str());
                if( ic == NULL )
                    return 1;

                ThreadCreator<IOControl>* io_thr = new ThreadCreator<IOControl>(ic, &IOControl::execute);
                if( io_thr == NULL )
                    return 1;

                act.addObject(static_cast<class UniSetObject*>(ic));
                lst_iothr.push_back( io_thr );
            }
        }
        // ------------- RTU Exchange --------------
        for( int i=0; i<MaxAddNum; i++ )
        {
            stringstream s;
            s << "--add-rtu";
            if( i>0 ) s << i;
                
            bool add_rtu = findArgParam(s.str(),argc,argv) != -1;
            if( add_rtu )
            {
                stringstream p;
                p << "rtu";
                if( i > 0 ) p << i;

                if( dlog.is_info() )
                    dlog.info() << "(smemory-plus): add RTUExchange(" << p.str() << ")" << endl;
            
                RTUExchange* rtu = RTUExchange::init_rtuexchange(argc,argv,shm->getId(),shm,p.str());
                if( rtu == NULL )
                    return 1;
                    
                act.addObject(static_cast<class UniSetObject*>(rtu));
            }
        }
        // ------------- MBSlave --------------
        for( int i=0; i<MaxAddNum; i++ )
        {
            stringstream s;
            s << "--add-mbslave";
            if( i>0 ) s << i;
                
            bool add_mbslave = findArgParam(s.str(),argc,argv) != -1;
            if( add_mbslave )
            {
                stringstream p;
                p << "mbs";
                if( i > 0 ) p << i;
   
                if( dlog.is_info() )
                    dlog.info() << "(smemory-plus): add MBSlave(" << p.str() << ")" << endl;

                MBSlave* mbs = MBSlave::init_mbslave(argc,argv,shm->getId(),shm,p.str());
                if( mbs == NULL )
                    return 1;

                act.addObject(static_cast<class UniSetObject*>(mbs));
            }
        }

        // ------------- MBTCPMaster --------------
        for( int i=0; i<MaxAddNum; i++ )
        {
            stringstream s;
            s << "--add-mbmaster";
            if( i>0 ) s << i;
    
            bool add_mbmaster = findArgParam(s.str(),argc,argv) != -1;

            if( add_mbmaster )
            {
                stringstream p;
                p << "mbtcp";
                if( i > 0 ) p << i;

                if( dlog.is_info() )
                    dlog.info() << "(smemory-plus): add MBTCPMaster(" << p.str() << ")" << endl;

                MBTCPMaster* mbm1 = MBTCPMaster::init_mbmaster(argc,argv,shm->getId(),shm,p.str());
                if( mbm1 == NULL )
                    return 1;

                act.addObject(static_cast<class UniSetObject*>(mbm1));
            }
        }
        // ------------- UNetUDP --------------
        bool add_unet = findArgParam("--add-unet",argc,argv) != -1;
        if( add_unet )
        {
            UNetExchange* unet = UNetExchange::init_unetexchange(argc,argv,shm->getId(),shm);
            if( unet == NULL )
                return 1;

            if( dlog.is_info() )
                dlog.info() << "(smemory-plus): add UNetExchnage.." << endl;

            act.addObject(static_cast<class UniSetObject*>(unet));
        }
        // ---------------------------------------
           // попытка решить вопрос с "зомби" процессами
        signal( SIGCHLD, on_sigchild );
        // ---------------------------------------
        SystemMessage sm(SystemMessage::StartUp);
        act.broadcast( sm.transport_msg() );

        for( std::list< ThreadCreator<IOControl>* >::iterator it=lst_iothr.begin(); it!=lst_iothr.end(); ++it )
            (*it)->start();

        act.run(false);
        on_sigchild(SIGTERM);
        
        return 0;
    }
    catch(Exception& ex)
    {
        ulog.crit() << "(smemory-plus): " << ex << endl;
    }
    catch( CORBA::SystemException& ex )
    {
        ulog.crit() << "(smemory-plus): " << ex.NP_minorString() << endl;
    }
    catch(...)
    {
        ulog.crit() << "(smemory-plus): catch(...)" << endl;
    }

    on_sigchild(SIGTERM);
    return 1;
}
// --------------------------------------------------------------------------
void help_print( int argc, const char* argv[] )
{
    const int mnum = MaxAddNum - 1;
    
    cout << "--add-io[1..." << mnum << "]         - Start IOControl" << endl;
    cout << "--add-rtu[1..." << mnum << "]        - Start RTUExchange (rtu master)" << endl;
    cout << "--add-mbslave[1..." << mnum << "]    - Start ModbusSlave (RTU or TCP)" << endl;
    cout << "--add-mbmaster[1..." << mnum << "]   - Start MBTCPMaster" << endl;
    cout << "--add-unet              - Start UNetExchange (UNetUDP)" << endl;

    cout << endl << "###### SM options ######" << endl;
    SharedMemory::help_print(argc,argv);

    cout << endl << "###### IO options ###### (prefix: --ioX)" << endl;
    IOControl::help_print(argc,argv);

    cout << endl << "###### RTU options ###### (prefix: --rtuX)" << endl;
    RTUExchange::help_print(argc,argv);

    cout << endl << "###### ModbusSlave options (prefix: --mbsX) ######" << endl;
    MBSlave::help_print(argc,argv);

    cout << endl << "###### ModbusTCP Master options (prefix: --mbtcpX) ######" << endl;
    MBTCPMaster::help_print(argc,argv);

    cout << endl << "###### UNetExchange options ######" << endl;
    UNetExchange::help_print(argc,argv);

    cout << endl << "###### Common options ######" << endl;
    cout << "--confile            - Use confile. Default: configure.xml" << endl;
    cout << "--logfile            - Use logfile. Default: smemory-plus.log" << endl;
}
