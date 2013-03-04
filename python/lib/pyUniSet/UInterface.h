#ifndef UInterface_H_
#define UInterface_H_
// -------------------------------------------------------------------------- 
#include <string>
#include "UTypes.h"
#include "UExceptions.h"
// -------------------------------------------------------------------------- 
namespace UInterface
{
	void uniset_init_params( UTypes::Params* p, const char* xmlfile )throw(UException);
	void uniset_init( int argc, char** argv, const char* xmlfile )throw(UException);

	//---------------------------------------------------------------------------
	long getValue( long id )throw(UException);
	void setValue( long id, long val )throw(UException);

	long getSensorID( const char* );
	
	const char* getShortName( long id );
	const char* getName( long id );
	const char* getTextName( long id );
	
	const char* getConfFileName();
	
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
