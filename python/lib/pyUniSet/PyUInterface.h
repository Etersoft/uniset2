#ifndef pyUInterface_H_
#define pyUInterface_H_
// --------------------------------------------------------------------------
#include <string>
#include "UTypes.h"
#include "UExceptions.h"
// --------------------------------------------------------------------------
namespace pyUInterface
{
	void uniset_init_params( UTypes::Params* p, const std::string& xmlfile )throw(UException);
	void uniset_init( int argc, char** argv, const std::string& xmlfile )throw(UException);

	//---------------------------------------------------------------------------
	long getValue( long id )throw(UException);
	void setValue( long id, long val )throw(UException);

	long getSensorID(  const std::string& name );

	std::string getShortName( long id );
	std::string getName( long id );
	std::string getTextName( long id );

	std::string getConfFileName();

}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
