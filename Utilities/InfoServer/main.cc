// $Id: main.cc,v 1.13 2008/12/14 21:57:49 vpashka Exp $
// --------------------------------------------------------------------------
#include "Configuration.h"
#include "InfoServer.h"
#include "ISRestorer.h"
#include "ObjectsActivator.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
	cout << "Usage: uniset-infoserver --name ObjectId [--askfile filename]"
		 << " [--confile configure.xml]\n";
}
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
	try
	{
		if( argc>1 && !strcmp(argv[1],"--help") )
		{
			short_usage();
			return 0;
		}

		uniset_init(argc,argv,"configure.xml");
	
		ObjectId ID = conf->getInfoServer();

		// определяем ID объекта
		string name = conf->getArgParam("--name");
		if( !name.empty() )
		{
			if( ID != UniSetTypes::DefaultObjectId )
			{
				unideb[Debug::WARN] << "(InfoServer::main): переопределяем ID заданнй в " 
						<< conf->getConfFileName() << endl;
			}

			ID = conf->oind->getIdByName(conf->getServicesSection()+"/"+name);	
			if( ID == UniSetTypes::DefaultObjectId )
			{
				cerr << "(InfoServer): идентификатор '" << name 
					<< "' не найден в конф. файле!"
					<< " в секции " << conf->getServicesSection() << endl;
				return 1;
			}
		}
		else if( ID == UniSetTypes::DefaultObjectId )
		{
			cerr << "(DBServer::main): Не удалось определить ИДЕНТИФИКАТОР сервера" << endl; 
			short_usage();
			return 1;
		}

		// определяем ask-файл
		string askfile = conf->getArgParam("--askfile",conf->getConfFileName());

		if( askfile.empty() )
		{
			InfoServer is(ID,new ISRestorer_XML(askfile));
			ObjectsActivator act;
			act.addObject(static_cast<class UniSetObject*>(&is));
			act.run(false);
		}
		else
		{
			InfoServer is(ID);
			ObjectsActivator act;
			act.addObject(static_cast<class UniSetObject*>(&is));
			act.run(false);
		}
	}
	catch(Exception& ex)
	{
		cerr << "(InfoServer::main): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(InfoServer::main): catch ..." << endl;
	}

	return 0;
}
