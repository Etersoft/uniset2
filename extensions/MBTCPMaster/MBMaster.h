// $Id: MBMaster.h,v 1.7 2009/03/03 10:33:27 pv Exp $
// -----------------------------------------------------------------------------
#ifndef _MBMaster_H_
#define _MBMaster_H_
// -----------------------------------------------------------------------------
#include <string>
#include <vector>
#include "UniSetObject_LT.h"
#include "IONotifyController.h"
#include "modbus/ModbusTCPMaster.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SharedMemory.h"
#include "IOBase.h"
#include "SMInterface.h"
// -----------------------------------------------------------------------------
class MBMaster:
	public UniSetObject_LT
{
	public:
		MBMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, SharedMemory* ic=0,
				 std::string prefix="mbtcp" );
		virtual ~MBMaster();
	
		/*! глобальная функция для инициализации объекта */
		static MBMaster* init_mbmaster( int argc, const char** argv, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
										std::string prefix="mbtcp" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char** argv );

		static const int NoSafetyState=-1;

		enum Timer
		{
			tmExchange
		};

		struct MBProperty:
			public IOBase
		{
			ModbusRTU::ModbusAddr mbaddr;			/*!< адрес устройства */
			ModbusRTU::ModbusData mbreg;			/*!< регистр */
			ModbusRTU::SlaveFunctionCode mbfunc;	/*!< функция для чтения/записи */
			short nbit;		/*!< bit number (for func=[0x01,0x02]) */

			MBProperty():	
				mbaddr(0),mbreg(0),
				mbfunc(ModbusRTU::fnUnknown),nbit(0)
			{}

			friend std::ostream& operator<<( std::ostream& os, MBProperty& p );
		};

	protected:	

		typedef std::vector<MBProperty> MBMap;
		MBMap mbmap;			/*!< список входов/выходов */
		unsigned int maxItem;	/*!< количество элементов (используется на момент инициализации) */

		ModbusTCPMaster* mb;
		UniSetTypes::uniset_mutex mbMutex;
		std::string iaddr;
		int port;
		int recv_timeout;
		ModbusRTU::ModbusAddr myaddr;

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;
		SMInterface* shm;

		void step();
		void poll();

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage*sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );	
		void initOutput();
		void waitSMReady();
		long readReg( MBMap::iterator& p );
		bool writeReg( MBMap::iterator& p, long val );

		virtual bool activateObject();
		
		// действия при завершении работы
		virtual void sigterm( int signo );

		void init_mb();
		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

	private:
		MBMaster();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		bool mbregFromID;
		bool force;		/*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
		bool force_out;	/*!< флаг означающий, принудительного чтения выходов */
		int polltime;	/*!< переодичность обновления данных, [мсек] */
		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;
		
		UniSetTypes::uniset_mutex pollMutex;
		Trigger trTimeout;
		PassiveTimer ptTimeout;
		
		
		bool activated;
		int activateTimeout;
		std::string prefix;
};
// -----------------------------------------------------------------------------
#endif // _MBMaster_H_
// -----------------------------------------------------------------------------
