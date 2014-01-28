#ifndef UConnector_H_
#define UConnector_H_
// --------------------------------------------------------------------------
#include <string>
#include <Configuration.h>
#include <UniversalInterface.h>
#include "UTypes.h"
#include "UExceptions.h"
// -------------------------------------------------------------------------- 
class UConnector
{
	public:
	  UConnector( int argc, char** argv, const char* xmlfile )throw(UException);
	  UConnector( UTypes::Params* p, const char* xmlfile )throw(UException);
	  ~UConnector();

  	  inline const char* getUIType(){ return "uniset"; }

	  const char* getConfFileName();
	  long getValue( long id, long node )throw(UException);
	  void setValue( long id, long val, long node )throw(UException);

	  long getSensorID( const char* );
	  long getNodeID( const char* );
	
	  const char* getShortName( long id );
	  const char* getName( long id );
	  const char* getTextName( long id );

	  
	private:
	  UniSetTypes::Configuration* conf;
	  UniversalInterface* ui;
	  const char* xmlfile;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
