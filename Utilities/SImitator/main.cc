#include <iostream>
#include <cmath>
#include <random>
#include <functional>
#include "Exceptions.h"
#include "UInterface.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;

using ModifyFunc = std::function<long(long)>;

// -----------------------------------------------------------------------------
void help_print()
{
    cout << endl;
    cout << "--sid id1@Node1,id2,..,idXX@NodeXX  - датчики" << endl;
    cout << "или" << endl;
    cout << "--filter-field name     - Считывать список датчиков, только у которых есть поле field" << endl;
    cout << "--filter-value val      - Считывать список датчиков, только у которых field=value" << endl;
    cout << endl;
    cout << "--min val               - Нижняя граница датчика. По умолчанию 0" << endl;
    cout << "--max val               - Верхняя граница датчика. По умолчанию 100 " << endl;
    cout << "--step val              - Шаг датчика. По умолчанию 1" << endl;
    cout << "--pause msec            - Пауза. По умолчанию 200 мсек" << endl << endl;
    cout << "--func [cos|sin|random] - Функция модификации значения. По умолчания не используется." << endl << endl;
    cout << uniset::Configuration::help() << endl;
}
// -----------------------------------------------------------------------------
struct ExtInfo:
    public uniset::ParamSInfo
{
    UniversalIO::IOType iotype;
};
// -----------------------------------------------------------------------------
long mf_nop( long v )
{
    return v;
}
long mf_sin( long v )
{
    return v * sin(v);
}
long mf_cos( long v )
{
    return v * cos(v);
}

// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
    //  std::ios::sync_with_stdio(false);
    ModifyFunc mf = mf_nop;

    try
    {
        // help
        // -------------------------------------
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            help_print();
            return 0;
        }

        // -------------------------------------
        std::random_device rnd;
        std::mt19937 gen(rnd());

        auto conf = uniset_init(argc, argv, "configure.xml" );
        UInterface ui(conf);

        const string sid = conf->getArgParam("--sid");
        auto funcname = conf->getArg2Param("--func", "");
        const string f_field = conf->getArg2Param("--filter-field", "");
        const string f_value = conf->getArg2Param("--filter-value", "");

        if( !funcname.empty() )
        {
            if( funcname == "sin" )
                mf = mf_sin;
            else if( funcname == "cos" )
                mf = mf_cos;
            else if( funcname == "random" )
            {

            }
            else
            {
                cerr << "Unknown modify function '" << funcname << "'. Must be [sin,cos]" << endl;
                return 1;
            }
        }


        std::list<ExtInfo> sensors;

        if( !f_field.empty() )
        {
            cout << "init sensors with filter: " << f_field << "='" << f_value << "'" << endl;
            UniXML_iterator it = conf->getXMLSensorsSection();

            if( !it.goChildren() )
            {
                cerr << "not found sensors section in " << conf->getConfFileName() << endl;
                return 1;
            }

            for( ; it; it++ )
            {
                if( uniset::check_filter(it, f_field, f_value) )
                {
                    auto id = conf->getSensorID(it.getProp("name"));

                    if( id == DefaultObjectId )
                        continue;

                    ExtInfo i;
                    i.si.id = id;
                    i.si.node = conf->getLocalNode();
                    i.iotype = uniset::getIOType(it.getProp("iotype"));
                    sensors.push_back(i);
                }
            }

            cout << "found " << sensors.size() << " sensors.." << endl;
        }
        else if( !sid.empty() )
        {
            auto lst = uniset::getSInfoList(sid, conf);

            if( lst.empty() )
            {
                cerr << endl << "Use --sid id1,..,idXX or --filter-field name [--filter-value value]" << endl << endl;
                return 1;
            }

            for( auto&& it : lst )
            {
                UniversalIO::IOType t = conf->getIOType( it.si.id );

                if( t != UniversalIO::AI && t != UniversalIO::AO )
                {
                    cerr << endl << "WARNING! Неверный типа датчика '" << t << "' для id='" << it.fname << "'. Тип должен быть AI или AO." << endl << endl;
                    // return 1;
                }

                if( it.si.node == DefaultObjectId )
                    it.si.node = conf->getLocalNode();

                ExtInfo i;
                i.si = it.si;
                i.iotype = t;
                sensors.push_back(i);
            }
        }

        if( sensors.empty() )
        {
            cerr << endl << "Use --sid id1,..,idXX or --filter-field name [--filter-value value]" << endl << endl;
            return 1;
        }


        int amin = conf->getArgInt("--min", "0");
        int amax = conf->getArgInt("--max", "100");

        if( amin > amax )
        {
            int temp = amax;
            amax = amin;
            amin = temp;
        }

        // init random function
        std::uniform_int_distribution<> rndgen(amin, amax);
        ModifyFunc mf_rand = [&rndgen, &gen]( long )
        {
            return rndgen(gen);
        };

        if( funcname == "random" )
            mf = mf_rand;

        int astep = conf->getArgInt("--step", "1");

        if( astep <= 0 )
        {
            cerr << endl << "Ошибка, используйте --step val - любое положительное число" << endl << endl;
            return 1;
        }

        int amsec = conf->getArgInt("--pause", "200");

        if(amsec <= 10)
        {
            cerr << endl << "Ошибка, используйте --pause val - любое положительное число > 10" << endl << endl;
            return 1;
        }

        cout << endl << "------------------------------" << endl;
        cout << " Вы ввели следующие параметры:" << endl;
        cout << "------------------------------" << endl;
        cout << "  sid = " << sid << endl;
        cout << "  min = " << amin << endl;
        cout << "  max = " << amax << endl;
        cout << "  step = " << astep << endl;
        cout << "  pause = " << amsec << endl;

        if( !funcname.empty() )
            cout << "  function = " << funcname << endl;

        cout << "------------------------------" << endl << endl;

        int i = amin - astep, j = amax;


        while(1)
        {
            if(i >= amax)
            {
                j -= astep;

                if(j < amin)               // Принудительная установка нижней границы датчика
                    j = amin;

                long val = mf(j);

                cout << "\r" << " i = " << val << "     " << flush;

                for( const auto& it : sensors )
                {
                    try
                    {
                        ui.setValue(it.si, val, DefaultObjectId);
                    }
                    catch( const uniset::Exception& ex )
                    {
                        cerr << endl << "save id=" << it.fname << " " << ex << endl;
                    }
                }

                if(j <= amin)
                {
                    i = amin;
                    j = amax;
                }
            }
            else
            {
                i += astep;

                if(i > amax)               // Принудительная установка верхней границы датчика
                    i = amax;

                long val = mf(i);

                cout << "\r" << " i = " << val << "     " << flush;

                for( const auto& it: sensors )
                {
                    try
                    {
                        ui.setValue(it.si, val, DefaultObjectId);
                    }
                    catch( const uniset::Exception& ex )
                    {
                        cerr << endl << "save id=" << it.fname << " " << ex << endl;
                    }
                }
            }

            msleep(amsec);
        }
    }
    catch( const uniset::Exception& ex )
    {
        cerr << endl << "(simitator): " << ex << endl;
        return 1;
    }
    catch( ... )
    {
        cerr << endl << "(simitator): catch..." << endl;
        return 1;
    }

    return 0;
}
// ------------------------------------------------------------------------------------------
