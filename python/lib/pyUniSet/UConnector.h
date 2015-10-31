#ifndef UConnector_H_
#define UConnector_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include "Configuration.h"
#include "UInterface.h"
#include "UTypes.h"
#include "UExceptions.h"
// --------------------------------------------------------------------------
class UConnector
{
	public:
		UConnector( int argc, char** argv, const std::string& xmlfile )throw(UException);
		UConnector( UTypes::Params* p, const std::string& xmlfile )throw(UException);
		~UConnector();

		inline std::string getUIType()
		{
			return string("uniset");
		}

		std::string getConfFileName();
		long getValue( long id, long node )throw(UException);
		void setValue( long id, long val, long node )throw(UException);

		long getSensorID( const std::string& name );
		long getNodeID(  const std::string& name );

		std::string getShortName( long id );
		std::string getName( long id );
		std::string getTextName( long id );

	private:
		std::shared_ptr<UniSetTypes::Configuration> conf;
		std::shared_ptr<UInterface> ui;
		std::string xmlfile;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
