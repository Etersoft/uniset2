#include <iostream>
#include <sstream>
#include <fstream>
#include "Configuration.h"
#include "Exceptions.h"
#include "ClickHouseInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
static int help( int argc, char** argv );
static int do_dict_sensors( int argc, char** argv );
static int do_dict_nodes( int argc, char** argv );
static int do_dict_objects( int argc, char** argv );
static int do_dict_messages( int argc, char** argv );
// --------------------------------------------------------------------------
struct FileDefer
{
    FileDefer( ofstream& f ): file(f) {}
    ~FileDefer()
    {
        if( file.is_open() )
            file.close();
    }
    ofstream& file;
};
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    if( findArgParam("--help", argc, argv) != -1 || findArgParam("-h", argc, argv) != -1 )
        return help(argc, argv);

    if( findArgParam("--generate-dict-sensors", argc, argv) != -1 )
        return do_dict_sensors(argc, argv);

    if( findArgParam("--generate-dict-nodes", argc, argv) != -1 )
        return do_dict_nodes(argc, argv);

    if( findArgParam("--generate-dict-objects", argc, argv) != -1 )
        return do_dict_objects(argc, argv);

    if( findArgParam("--generate-dict-objects", argc, argv) != -1 )
        return do_dict_objects(argc, argv);

    if( findArgParam("--generate-dict-messages", argc, argv) != -1 )
        return do_dict_messages(argc, argv);

    cerr << "unknown command. Use -h for help" << endl;
    return -1;
}
// --------------------------------------------------------------------------
int help( int argc, char** argv )
{
    cout << "Usage: uniset2-clickhouse-helper --confile configure.xml --[command]" << endl;
    cout << endl;
    cout << "Commands:" << endl;
    cout << "--generate-dict-sensors - Generate dict-sensors data (csv)" << endl;
    cout << "--generate-dict-nodes - Generate dict-nodes data (csv)" << endl;
    cout << "--generate-dict-objects - Generate dict-objects data (csv)" << endl;
    cout << "--generate-dict-messages - Generate dict-messages data (csv)" << endl;
    cout << endl;
    return 0;
}
// --------------------------------------------------------------------------
int do_dict_sensors( int argc, char** argv )
{
    auto conf = uniset_init( argc, argv );

    auto outfilename = conf->getArgParam("--generate-dict-sensors", "dict-sensors.csv");
    xmlNode* snode = conf->getXMLSensorsSection();

    if( !snode )
    {
        cerr << "(do_dict_sensors): Not found section <sensors>" << endl;
        return 1;
    }

    UniXML::iterator it(snode);

    if( !it.goChildren() )
    {
        cerr << "(do_dict_sensors): <sensors> section is empty?!" << endl;
        return 1;
    }

    ofstream outfile(outfilename.c_str(), ios::out | ios::trunc);

    if( !outfile.is_open() )
    {
        cerr << "(do_dict_sensors): can't create file '" << outfilename << "'" << endl;
        return 1;
    }

    FileDefer d(outfile);

    //    outfile << "cityhash64, name, iotype, textname, default" << endl;
    for( ; it; it++ )
    {
        auto name = it.getProp("name");
        outfile << uniset::hash64(name)
                << ",\"" << name << "\""
                << "," << it.getProp("iotype")
                << ",\"" << it.getProp("textname") << "\""
                << "," << it.getIntProp("default")
                << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------
int do_dict_nodes( int argc, char** argv )
{
    auto conf = uniset_init( argc, argv );

    auto outfilename = conf->getArgParam("--generate-dict-nodes", "dict-nodes.csv");
    xmlNode* nnode = conf->getXMLNodesSection();

    if( !nnode )
    {
        cerr << "(do_dict_nodes): Not found section <sensors>" << endl;
        return 1;
    }

    UniXML::iterator it(nnode);

    if( !it.goChildren() )
    {
        cerr << "(do_dict_nodes): <nodes> section is empty?!" << endl;
        return 1;
    }

    ofstream outfile(outfilename.c_str(), ios::out | ios::trunc);

    if( !outfile.is_open() )
    {
        cerr << "(do_dict_nodes): can't create file '" << outfilename << "'" << endl;
        return 1;
    }

    FileDefer d(outfile);

    //    outfile << "cityhash64, name, ip, textname" << endl;
    for( ; it; it++ )
    {
        auto name = it.getProp("name");
        outfile << uniset::hash64(name)
                << ",\"" << name << "\""
                << "," << it.getProp("ip")
                << ",\"" << it.getProp("textname") << "\""
                << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------
int do_dict_objects( int argc, char** argv )
{
    auto conf = uniset_init( argc, argv );

    auto outfilename = conf->getArgParam("--generate-dict-objects", "dict-objects.csv");
    xmlNode* onode = conf->getXMLObjectsSection();
    xmlNode* snode = conf->getXMLServicesSection();
    xmlNode* cnode = conf->getXMLControllersSection();

    if( !onode || !snode || !cnode )
    {
        cerr << "(do_dict_objects): Not found sections <objects> or <controlles> or <services>" << endl;
        return 1;
    }

    UniXML::iterator it1(onode);
    UniXML::iterator it2(snode);
    UniXML::iterator it3(cnode);

    if( !it1.goChildren() )
    {
        cerr << "(do_dict_objects): <objects> section is empty?!" << endl;
        return 1;
    }

    // may be empty
    it2.goChildren();
    it3.goChildren();

    ofstream outfile(outfilename.c_str(), ios::out | ios::trunc);

    if( !outfile.is_open() )
    {
        cerr << "(do_dict_objects): can't create file '" << outfilename << "'" << endl;
        return 1;
    }

    FileDefer d(outfile);

    //    outfile << "cityhash64, name, textname" << endl;
    for( ; it1; it1++ )
    {
        auto name = it1.getProp("name");
        outfile << uniset::hash64(name)
                << ",\"" << name << "\""
                << ",\"" << it1.getProp("comment") << "\""
                << endl;
    }

    for( ; it2; it2++ )
    {
        auto name = it2.getProp("name");
        outfile << uniset::hash64(name)
                << ",\"" << name << "\""
                << ",\"" << it2.getProp("comment") << "\""
                << endl;
    }

    for( ; it3; it3++ )
    {
        auto name = it3.getProp("name");
        outfile << uniset::hash64(name)
                << ",\"" << name << "\""
                << ",\"" << it3.getProp("comment") << "\""
                << endl;
    }

    return 0;
}
// --------------------------------------------------------------------------
int do_dict_messages( int argc, char** argv )
{
    auto conf = uniset_init( argc, argv );

    auto outfilename = conf->getArgParam("--generate-dict-messages", "dict-messages.csv");

    xmlNode* mnode = conf->findNode(conf->getConfXML()->getFirstNode(), "messages");

    if( !mnode )
    {
        cerr << "(do_dict_messages): Not found section <messages>" << endl;
        return 1;
    }

    UniXML::iterator it(mnode);

    if( !it.goChildren() )
    {
        cerr << "(do_dict_messages): <messages> section is empty?!" << endl;
        return 1;
    }

    ofstream outfile(outfilename.c_str(), ios::out | ios::trunc);

    if( !outfile.is_open() )
    {
        cerr << "(do_dict_messages): can't create file '" << outfilename << "'" << endl;
        return 1;
    }

    FileDefer d(outfile);

    //    outfile << "cityhash64, name, iotype, textname, default" << endl;
    for( ; it; it++ )
    {
        auto name = it.getProp("sensorname");
        auto value = it.getProp("value");
        outfile << uniset::hash32(name+value)
                << ",\"" << name << "\""
                << ",\"" << value << "\""
                << ",\"" << it.getProp("message") << "\""
                << ",\"" << it.getProp("mtype") << "\""
                << ",\"" << it.getProp("mgroup") << "\""
                << ",\"" << it.getProp("mcode") << "\""
                << "," << it.getIntProp("default")
                << endl;
    }

    return 0;
}
