#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "LostTestProc.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
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
		auto act = UniSetActivator::Instance();

		int num = conf->getArgPInt("--numproc", 1);

		int max = 10;

		if( num > max )
		{
			cerr << "'num' must be < " << max << endl;
			return 1;
		}

		for( int i = 1; i <= num; i++ )
		{
			ostringstream s;
			s << "TestProc" << i;

			cout << "..create " << s.str() << endl;
			auto tp = make_shared<LostTestProc>( conf->getObjectID(s.str()));
			act->add(tp);

			ostringstream sp;
			sp << "TestProc" << (i+max);

			cout << "..create passive " << sp.str() << endl;
			auto child = make_shared<LostPassiveTestProc>( conf->getObjectID(sp.str()));
			tp->setChildPassiveProc(child);
			act->add(child);
		}

		SystemMessage sm(SystemMessage::StartUp);
		act->broadcast( sm.transport_msg() );
		act->run(false);

		return 0;
	}
	catch( const SystemError& err )
	{
		cerr << "(lostmessage-test): " << err << endl;
	}
	catch( const UniSetTypes::Exception& ex )
	{
		cerr << "(lostmessage-test): " << ex << endl;
	}
	catch( const std::exception& e )
	{
		cerr << "(lostmessage-test): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(lostmessage-test): catch(...)" << endl;
	}

	return 1;
}
