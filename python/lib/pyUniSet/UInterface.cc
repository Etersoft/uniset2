#include <ostream>
#include <Exceptions.h>
#include <ORepHelpers.h>
#include <UniversalInterface.h>
#include <Configuration.h>
#include <UniSetTypes.h>
#include "UInterface.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static UniversalInterface* ui=0;
//---------------------------------------------------------------------------
void UInterface::uniset_init_params( UTypes::Params* p, const char* xmlfile )throw(UException)
{
	UInterface::uniset_init(p->argc,p->argv,xmlfile);
}
//---------------------------------------------------------------------------

void UInterface::uniset_init( int argc, char* argv[], const char* xmlfile )throw(UException)
{
	try
	{
		UniSetTypes::uniset_init(argc,argv,xmlfile);
		ui = new UniversalInterface();
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
long UInterface::getValue( long id )throw(UException)
{
	if( !UniSetTypes::conf || !ui )
	  throw USysError();

	UniversalIO::IOTypes t = UniSetTypes::conf->getIOType(id);
	try
	{
		switch(t)
		{
			case UniversalIO::DigitalInput:
			case UniversalIO::DigitalOutput:
			  return (ui->getState(id) ? 1 : 0);
			break;

			case UniversalIO::AnalogInput:
			case UniversalIO::AnalogOutput:
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
		throw ex;
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
void UInterface::setValue( long id, long val )throw(UException)
{
	if( !UniSetTypes::conf || !ui )
	  throw USysError();

	UniversalIO::IOTypes t = UniSetTypes::conf->getIOType(id);
	try
	{
		switch(t)
		{
			case UniversalIO::DigitalInput:
				ui->saveState(id,val,t);
			break;

			case UniversalIO::DigitalOutput:
				ui->setState(id,val);
			break;

			case UniversalIO::AnalogInput:
				ui->saveValue(id,val,t);
			break;

			case UniversalIO::AnalogOutput:
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
		throw ex;
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
long UInterface::getSensorID( const char* name )
{
	if( UniSetTypes::conf )
	  return UniSetTypes::conf->getSensorID(name);

	return -1;
}
//---------------------------------------------------------------------------
const char* UInterface::getName( long id )
{
	if( UniSetTypes::conf )
		return UniSetTypes::conf->oind->getMapName(id).c_str();

	return "";
}
//---------------------------------------------------------------------------
const char* UInterface::getShortName( long id )
{
	if( UniSetTypes::conf )
		return ORepHelpers::getShortName(UniSetTypes::conf->oind->getMapName(id)).c_str();

	return "";
}
//---------------------------------------------------------------------------
const char* UInterface::getTextName( long id )
{
	if( UniSetTypes::conf )
		return UniSetTypes::conf->oind->getTextName(id).c_str();

	return "";
}
//---------------------------------------------------------------------------
const char* getConfFileName()
{
	if( UniSetTypes::conf )
		return UniSetTypes::conf->getConfFileName().c_str();

	return "";

}
//---------------------------------------------------------------------------
