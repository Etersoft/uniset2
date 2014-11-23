#include <ostream>
#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "Configuration.h"
#include "UniSetTypes.h"
#include "PyUInterface.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static UInterface* ui=0;
//---------------------------------------------------------------------------
void pyUInterface::uniset_init_params( UTypes::Params* p, const char* xmlfile )throw(UException)
{
    pyUInterface::uniset_init(p->argc,p->argv,xmlfile);
}
//---------------------------------------------------------------------------

void pyUInterface::uniset_init( int argc, char* argv[], const char* xmlfile )throw(UException)
{
    try
    {
        UniSetTypes::uniset_init(argc,argv,xmlfile);
        ui = new UInterface();
        return;
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
long pyUInterface::getValue( long id )throw(UException)
{
    auto conf = UniSetTypes::uniset_conf();
    if( !conf || !ui )
      throw USysError();
    
    UniversalIO::IOType t = conf->getIOType(id);
    try
    {
        switch(t)
        {
            case UniversalIO::DI:
            case UniversalIO::DO:
            case UniversalIO::AI:
            case UniversalIO::AO:
                return ui->getValue(id);
            break;
                
            default:
            {
              ostringstream e;
              e << "(getValue): Unknown iotype for id=" << id;
              throw UException(e.str());
            }
        }
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
void pyUInterface::setValue( long id, long val )throw(UException)
{
    auto conf = UniSetTypes::uniset_conf();

    if( !conf || !ui )
      throw USysError();
    
    UniversalIO::IOType t = conf->getIOType(id);
    try
    {
        switch(t)
        {
            case UniversalIO::DI:
            case UniversalIO::DO:
            case UniversalIO::AI:
            case UniversalIO::AO:
                ui->setValue(id,val);
            break;
                
            default:
            {
              ostringstream e;
              e << "(setValue): Unknown iotype for id=" << id;
              throw UException(e.str());
            }
        }
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
long pyUInterface::getSensorID( const char* name )
{
    auto conf = UniSetTypes::uniset_conf();
    if( conf )
      return conf->getSensorID(name);
    
    return UniSetTypes::DefaultObjectId;
}
//---------------------------------------------------------------------------
const char* pyUInterface::getName( long id )
{
    auto conf = UniSetTypes::uniset_conf();
    if( conf )
        return conf->oind->getMapName(id).c_str();
        
    return "";
}
//---------------------------------------------------------------------------
const char* pyUInterface::getShortName( long id )
{
    auto conf = UniSetTypes::uniset_conf();
    if( conf )
        return ORepHelpers::getShortName(conf->oind->getMapName(id)).c_str();
        
    return "";
}
//---------------------------------------------------------------------------
const char* pyUInterface::getTextName( long id )
{
    auto conf = UniSetTypes::uniset_conf();
    if( conf )
        return conf->oind->getTextName(id).c_str();
        
    return "";
}
//---------------------------------------------------------------------------
const char* pyUInterface::getConfFileName()
{
    auto conf = UniSetTypes::uniset_conf();
    if( conf )
        return conf->getConfFileName().c_str();
        
    return "";

}
//---------------------------------------------------------------------------
