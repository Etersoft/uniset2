// $Id: UniExchange.h,v 1.2 2009/04/07 16:11:23 pv Exp $
// -----------------------------------------------------------------------------
#ifndef UniExchange_H_
#define UniExchange_H_
// -----------------------------------------------------------------------------
#include <list>
#include "UniXML.h"
#include "IOController.h"
#include "SMInterface.h"
#include "SharedMemory.h"
// -----------------------------------------------------------------------------
class UniExchange:
	public IOController
{
	public:
		UniExchange( UniSetTypes::ObjectId id, UniSetTypes::ObjectId shmID, 
						SharedMemory* ic=0, const std::string prefix="unet" );
		virtual ~UniExchange();

		void execute();

		virtual IOController_i::ASensorInfoSeq* getAnalogSensorsMap();
		virtual IOController_i::DSensorInfoSeq* getDigitalSensorsMap();

		static UniExchange* init_exchange( int argc, const char* const* argv,
									UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
									const std::string prefix="unet" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char** argv );

	protected:

		virtual void processingMessage( UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm );
		virtual void timerInfo( UniSetTypes::TimerMessage* tm );
		virtual void sigterm( int signo );

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;
		SMInterface* shm;
		
		struct NetNodeInfo
		{
			CORBA::Object_var oref;
			IONotifyController_i_var shm;
			UniSetTypes::ObjectId id;
			UniSetTypes::ObjectId node;
			UniSetTypes::ObjectId sidConnection; /*!< датчик связи */
		};
		
		typedef std::list<NetNodeInfo> NetNodeList;
		NetNodeList nlst;

		void readConfiguration();
		bool check_item( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );
		bool initItem( UniXML_iterator& it );
		
	private:
};
// -----------------------------------------------------------------------------
#endif // UniExchange_H_
