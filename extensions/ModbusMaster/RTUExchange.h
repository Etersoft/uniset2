#ifndef _RTUEXCHANGE_H_
#define _RTUEXCHANGE_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "MBExchange.h"
#include "modbus/ModbusRTUMaster.h"
#include "RTUStorage.h"
// -----------------------------------------------------------------------------
class RTUExchange:
	public MBExchange
{
	public:
		RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID,
					  SharedMemory* ic=0, const std::string prefix="rs" );
		virtual ~RTUExchange();

		/*! глобальная функция для инициализации объекта */
		static RTUExchange* init_rtuexchange( int argc, const char* const* argv,
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
											const std::string prefix="rs" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

	protected:
		ModbusRTUMaster* mbrtu;
		UniSetTypes::uniset_mutex mbMutex;
		std::string devname;
		ComPort::Speed defSpeed;
		bool use485F;
		bool transmitCtl;

		virtual void step();
		virtual void poll();

		virtual ModbusClient* initMB( bool reopen=false );
		virtual bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );

	private:
		RTUExchange();

		UniSetTypes::uniset_mutex pollMutex;
		bool rs_pre_clean;
		bool allNotRespond;
		Trigger trAllNotRespond;
//		PassiveTimer ptAllNotRespond;
};
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
