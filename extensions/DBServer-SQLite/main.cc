#include "Configuration.h"
#include "DBServer_SQLite.h"
#include "UniSetActivator.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
    cout << "Usage: uniset-mysql-dbserver [--name ObjectId] [--confile configure.xml]\n";
}
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    try
    {
        if( argc > 1 && !strcmp(argv[1],"--help") )
        {
            short_usage();
            return 0;
        }

        auto conf = uniset_init(argc,argv,"configure.xml");

        ObjectId ID = conf->getDBServer();

        // определяем ID объекта
        string name = conf->getArgParam("--name");
        if( !name.empty())
        {
            if( ID != UniSetTypes::DefaultObjectId )
            {
                ulog.warn() << "(DBServer::main): переопределяем ID заданнй в "
                        << conf->getConfFileName() << endl;
            }

            ID = conf->oind->getIdByName(conf->getServicesSection()+"/"+name);
            if( ID == UniSetTypes::DefaultObjectId )
            {
                cerr << "(DBServer::main): идентификатор '" << name
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

        DBServer_SQLite dbs(ID);

        UniSetActivatorPtr act = UniSetActivator::Instance();
        act->addObject(static_cast<class UniSetObject*>(&dbs));
        act->run(false);
    }
    catch(Exception& ex)
    {
        cerr << "(DBServer::main): " << ex << endl;
    }
    catch(...)
    {
        cerr << "(DBServer::main): catch ..." << endl;
    }

    return 0;
}
