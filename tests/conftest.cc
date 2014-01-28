// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "Configuration.h"
#include "ORepHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
    if( argc>1 && strcmp(argv[1],"--help")==0 )
    {
        cout << "--confile    - Configuration file. Default: test.xml" << endl;
        return 0;
    }
    
    cout << "**** uni_atoi('')=" << uni_atoi("") << endl;

    try
    {
        string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "test.xml" );
        conf = new Configuration(argc, argv, confile);

        cout << "getLocalNode()=" << conf->getLocalNode() << endl;
        
        string t(conf->oind->getTextName(1));
        cout << "**** check getTextName: " << ( t.empty() ?  "FAILED" : "OK" ) << endl;

        string ln("/Projects/Sensors/VeryVeryLongNameSensor_ForTest_AS");
        string ln_t("VeryVeryLongNameSensor_ForTest_AS");
        cout << "**** check getShortName: " << ( ln_t == ORepHelpers::getShortName(ln) ? "OK" : "FAILED" ) << endl;

        string mn(conf->oind->getMapName(1));
        cout << "**** check getMapName: " << ( mn.empty() ?  "FAILED" : "OK" ) << endl;


        cout << "getSensorID(Input1_S): " << conf->getSensorID("Input1_S") << endl; 
        
        std::string iname = conf->oind->getNameById(1);
        cout << "getNameById(1): " << iname << endl;

        ObjectId i_id = conf->oind->getIdByName(mn);
        cout << "getIdByName(" << iname << "): " << (i_id == DefaultObjectId ? "FAIL" : "OK" ) << endl;

        UniversalIO::IOType t1=conf->getIOType(1);
        cout << "**** getIOType for " << mn << endl;
        cout << "**** check getIOType(id): (" << t1 << ") " << ( t1 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;        
        UniversalIO::IOType t2=conf->getIOType(mn);
        cout << "**** check getIOType(name): (" << t2 << ") " << ( t2 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;        
        UniversalIO::IOType t3=conf->getIOType("Input1_S");
        cout << "**** check getIOType(name): for short name 'Input1_S': (" << t3 << ") " << ( t3 == UniversalIO::UnknownIOType ?  "FAILED" : "OK" ) << endl;        


        int i1 = uni_atoi("-100");
        cout << "**** check uni_atoi: '-100' " << ( ( i1 != -100 ) ? "FAILED" : "OK" ) << endl;

        int i2 = uni_atoi("20");
        cout << "**** check uni_atoi: '20' " << ( ( i2 != 20 ) ? "FAILED" : "OK" ) << endl;
        
        xmlNode* cnode = conf->getNode("testnode");
        if( cnode == NULL )                                                                                                                                                         
        {                                                                                                                                                                           
            cerr << "<testnode name='testnode'> not found" << endl;                                                                                                                                 
            return 1;                                                                                                                                                               
        }
        
        cout << "**** check conf->getNode function [OK] " << endl;
        
        UniXML_iterator it(cnode); 

        int prop2 = conf->getArgInt("--prop-id2",it.getProp("id2"));
        cerr << "**** check conf->getArgInt(arg1,...): " << ( (prop2 == 0) ? "[FAILED]" : "OK" ) << endl;

        int prop3 = conf->getArgInt("--prop-dummy",it.getProp("id2"));
        cerr << "**** check conf->getArgInt(...,arg2): " << ( (prop3 != -100) ? "[FAILED]" : "OK" ) << endl;
        
        int prop1 = conf->getArgPInt("--prop-id2",it.getProp("id2"),0);
        cerr << "**** check conf->getArgPInt(...): " << ( (prop1 == 0) ? "[FAILED]" : "OK" ) << endl;

        int prop4 = conf->getArgPInt("--prop-dummy",it.getProp("dummy"),20);
        cerr << "**** check conf->getArgPInt(...,...,defval): " << ( (prop4 != 20) ? "[FAILED]" : "OK" ) << endl;

        int prop5 = conf->getArgPInt("--prop-dummy",it.getProp("dummy"),0);
        cerr << "**** check conf->getArgPInt(...,...,defval): " << ( (prop5 != 0) ? "[FAILED]" : "OK" ) << endl;
        


        
        return 0;
    }
    catch(SystemError& err)
    {
        cerr << "(conftest): " << err << endl;
    }
    catch(Exception& ex)
    {
        cerr << "(conftest): " << ex << endl;
    }
    catch(...)
    {
        cerr << "(conftest): catch(...)" << endl;
    }
    
    return 1;
}
