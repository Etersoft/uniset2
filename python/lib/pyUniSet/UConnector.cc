#include "UConnector.h"
#include "ORepHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
UConnector::UConnector( UTypes::Params* p, const char* xfile )throw(UException):
conf(0),
ui(0),
xmlfile(xfile)
{
    try
    {
        conf = new UniSetTypes::Configuration(p->argc,p->argv,xmlfile);
        ui = new UInterface(conf);
    }
    catch( UniSetTypes::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException();
    }
}
//---------------------------------------------------------------------------
UConnector::UConnector( int argc, char** argv, const char* xfile )throw(UException):
conf(0),
ui(0),
xmlfile(xfile)
{
    try
    {
        conf = new UniSetTypes::Configuration(argc,argv,xmlfile);
        ui = new UInterface(conf);
    }
    catch( UniSetTypes::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException();
    }
}
// --------------------------------------------------------------------------
UConnector::~UConnector()
{
    delete ui;
    delete conf;
}
// --------------------------------------------------------------------------
const char* UConnector::getConfFileName()
{
//    return xmlfile;
    if( conf )
        return conf->getConfFileName().c_str();

    return "";

}
// --------------------------------------------------------------------------
long UConnector::getValue( long id, long node )throw(UException)
{
    if( !conf || !ui )
      throw USysError();

    if( node == UTypes::DefaultID )
      node = conf->getLocalNode();

    try
    {
         return ui->getValue(id,node);
    }
    catch( UException& ex )
    {
        throw;
    }
    catch( UniSetTypes::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException("(getValue): catch...");
    }

    throw UException("(getValue): unknown error");
}
//---------------------------------------------------------------------------
void UConnector::setValue( long id, long val, long node )throw(UException)
{
    if( !conf || !ui )
      throw USysError();


    if( node == UTypes::DefaultID )
      node = conf->getLocalNode();

    try
    {
        ui->setValue(id,val,node);
    }
    catch( UException& ex )
    {
        throw;
    }
    catch( UniSetTypes::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException("(setValue): catch...");
    }
}
//---------------------------------------------------------------------------
long UConnector::getSensorID( const char* name )
{
    if( conf )
      return conf->getSensorID(name);

    return UTypes::DefaultID;
}
//---------------------------------------------------------------------------
long UConnector::getNodeID( const char* name )
{
    if( conf )
      return conf->getNodeID(name);

    return UTypes::DefaultID;
}
//---------------------------------------------------------------------------
const char* UConnector::getName( long id )
{
    if( conf )
        return conf->oind->getMapName(id).c_str();

    return "";
}
//---------------------------------------------------------------------------
const char* UConnector::getShortName( long id )
{
    if( conf )
        return ORepHelpers::getShortName(conf->oind->getMapName(id)).c_str();

    return "";
}
//---------------------------------------------------------------------------
const char* UConnector::getTextName( long id )
{
    if( conf )
        return conf->oind->getTextName(id).c_str();

    return "";
}
//---------------------------------------------------------------------------
