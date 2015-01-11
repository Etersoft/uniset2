// --------------------------------------------------------------------------
#include <string>
#include "SViewer.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
    cout << "Usage: uniset-sviewer-text [--fullname] [--polltime msec] [--confile uniset-confile]\n";
}
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
    std::ios::sync_with_stdio(false);
    try
    {
        if( argc > 1 && !strcmp(argv[1],"--help") )
        {
            short_usage();
            return 0;
        }

        auto conf = uniset_init(argc,argv,"configure.xml");

        bool fullname = false;
        if( findArgParam("--fullname",conf->getArgc(),conf->getArgv()) != -1 )
            fullname = true;

        SViewer sv(conf->getControllersSection(),!fullname);
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
    catch(Exception& ex )
    {
        cerr << "(main): Поймали исключение " << ex <<  endl;
    }
    catch(...)
    {
        cerr << "(main): Неизвестное исключение!!!!"<< endl;
    }

    return 1;
}
