// --------------------------------------------------------------------------
#include <string>
#include "SViewer.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
    cout << "Usage: uniset-sviewer-text [--fullname] [-name TestProc] [--polltime msec] [--confile uniset-confile]\n";
    cout << endl;
    cout << uniset::Configuration::help() << endl;
}
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            short_usage();
            return 0;
        }

        auto conf = uniset_init(argc, argv, "configure.xml");

        ObjectId ID(DefaultObjectId);
        const string name = conf->getArgParam("--name", "TestProc");

        ID = conf->getObjectID(name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(main): идентификатор '" << name
                 << "' не найден в конф. файле!"
                 << " в секции " << conf->getObjectsSection() << endl;
            return 0;
        }

        bool fullname = false;

        if( findArgParam("--fullname", conf->getArgc(), conf->getArgv()) != -1 )
            fullname = true;

        SViewer sv(ID, conf->getControllersSection(), !fullname);
        timeout_t timeMS = conf->getArgInt("--polltime");

        if( timeMS )
        {
            cout << "(main): просматриваем с периодом " << timeMS << "[мсек]" <<  endl;
            sv.monitor(timeMS);
        }
        else
            sv.view();

        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(main): Поймали исключение " << ex.what() <<  endl;
    }
    catch(...)
    {
        cerr << "(main): Неизвестное исключение!!!!" << endl;
    }

    return 1;
}
