#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "TestProc.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
    {
        cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
        SharedMemory::help_print(argc, argv);
        return 0;
    }

    try
    {
        auto conf = uniset_init(argc, argv);

        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);

        int num = conf->getArgPInt("--numproc", 1);

        for( int i = 1; i <= num; i++ )
        {
            ostringstream s;
            s << "TestProc" << i;

            cout << "..create " << s.str() << endl;
            auto tp = make_shared<TestProc>( conf->getObjectID(s.str()));
            //          tp->init_dlog(dlog());
            act->add(tp);
        }

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(false);

        return 0;
    }
    catch( const SystemError& err )
    {
        ucrit << "(smemory): " << err << endl;
    }
    catch( const uniset::Exception& ex )
    {
        ucrit << "(smemory): " << ex << endl;
    }
    catch( const std::exception& e )
    {
        ucrit << "(smemory): " << e.what() << endl;
    }
    catch(...)
    {
        ucrit << "(smemory): catch(...)" << endl;
    }

    return 1;
}
