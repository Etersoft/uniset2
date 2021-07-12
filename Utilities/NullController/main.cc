#include "Configuration.h"
#include "NullController.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
    cout << "Usage: uniset-nullController"
         << "--confile configure.xml. По умолчанию: configure.xml." << endl
         << " --name ObjectId [--confile configure.xml] [--askfile filename] \n"
         << " --s-filter-field name - поле для фильтрования списка датчиков\n"
         << " --s-filter-value value - значение для поля фильтрования списка датчиков \n"
         << " --c-filter-field name - поле для фильтрования списка заказчиков по каждому датчику\n"
         << " --c-filter-value value - значение для поля фильтрования списка заказчиков по каждому датчику\n"
         << " --dbDumping [0,1] - создавать ли dump-файл \n";
}
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc <= 1 )
        {
            cerr << "\nНе указаны необходимые параметры\n\n";
            short_usage();
            return 0;
        }

        if( !strcmp(argv[1], "--help") )
        {
            short_usage();
            return 0;
        }

        auto conf = uniset_init(argc, argv, "configure.xml");


        // определяем ID объекта
        string name = conf->getArgParam("--name");

        if( name.empty())
        {
            cerr << "(nullController): не задан ObjectId!!! (--name)\n";
            return 0;
        }

        ObjectId ID = conf->oind->getIdByName(conf->getControllersSection() + "/" + name);

        if( ID == uniset::DefaultObjectId )
        {
            cerr << "(nullController): идентификатор '" << name
                 << "' не найден в конф. файле!"
                 << " в секции " << conf->getControllersSection() << endl;
            return 0;
        }

        // определяем ask-файл
        string askfile = conf->getArgParam("--askfile");

        if( askfile.empty())
            askfile = conf->getConfFileName();

        // определяем фильтр
        const string s_field = conf->getArgParam("--s-filter-field");
        const string s_fvalue = conf->getArgParam("--s-filter-value");
        const string c_field = conf->getArgParam("--c-filter-field");
        const string c_fvalue = conf->getArgParam("--c-filter-value");

        // надо ли писать изменения в БД
        bool dbDumping = conf->getArgInt("--dbDumping");

        auto nc = make_shared<NullController>(ID, askfile, s_field, s_fvalue, c_field, c_fvalue, dbDumping);
        auto act = UniSetActivator::Instance();
        act->add(nc);
        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(nullController::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(nullController::main): catch ..." << endl;
    }

    return 1;
}
