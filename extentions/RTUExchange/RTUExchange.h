// $Id: RTUExchange.h,v 1.2 2009/01/11 19:08:45 vpashka Exp $
// -----------------------------------------------------------------------------
#ifndef _RTUEXCHANGE_H_
#define _RTUEXCHANGE_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "IONotifyController.h"
#include "UniSetObject_LT.h"
#include "modbus/ModbusRTUMaster.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "Calibration.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "MTR.h"
#include "RTUStorage.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
class RTUExchange:
	public UniSetObject_LT
{
	public:
		RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );
		virtual ~RTUExchange();
	
		/*! глобальная функция для инициализации объекта */
		static RTUExchange* init_rtuexchange( int argc, char* argv[], 
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, char* argv[] );

		static const int NoSafetyState=-1;

		enum Timer
		{
			tmExchange
		};

		enum DeviceType
		{
			dtUnknown,		/*!< неизвестный */
			dtRTU,			/*!< RTU (default) */
			dtRTU188,		/*!< RTU188 (Fastwell) */
			dtMTR			/*!< MTR (DEIF) */
		};

		friend std::ostream& operator<<( std::ostream& os, const DeviceType& dt );


		struct RSProperty:
			public IOBase
		{
			DeviceType devtype;						/*!< тип устройства */
			ModbusRTU::ModbusAddr mbaddr;			/*!< адрес устройства */
			ModbusRTU::ModbusData mbreg;			/*!< регистр */
			ModbusRTU::SlaveFunctionCode mbfunc;	/*!< функция для чтения/записи */

			// only for RTU
			short nbit;		/*!< bit number (for func=[0x01,0x02]) */

			// only for MTR
			MTR::MTRType mtrType;	/*!< тип регистра (согласно спецификации на MTR) */

			// only for RTU188
			RTUStorage* rtu;
			RTUStorage::RTUJack rtuJack;
			int rtuChan;

			RSProperty():	
				devtype(dtUnknown),
				mbaddr(0),mbreg(0),mbfunc(ModbusRTU::fnUnknown),
				nbit(-1),rtu(0),rtuJack(RTUStorage::nUnknown),rtuChan(0)
			{}

			friend std::ostream& operator<<( std::ostream& os, RSProperty& p );
		};

	protected:	

		typedef std::vector<RSProperty> RSMap;
		RSMap rsmap;			/*!< список входов/выходов */
		unsigned int maxItem;	/*!< количество элементов (используется на момент инициализации) */

		ModbusRTUMaster* mb;
		UniSetTypes::uniset_mutex mbMutex;

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		void step();
		void poll();
		long pollRTU188( RSMap::iterator& p );
		long pollMTR( RSMap::iterator& p );
		long pollRTU( RSMap::iterator& p );

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage*sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );	
		void initOutput();
		void waitSMReady();

		virtual bool activateObject();
		
		// действия при завершении работы
		virtual void sigterm( int signo );


		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );
		bool initCommParam( UniXML_iterator& it, RSProperty& p );
		bool initMTRitem( UniXML_iterator& it, RSProperty& p );
		bool initRTU188item( UniXML_iterator& it, RSProperty& p );
		bool initRTUitem( UniXML_iterator& it, RSProperty& p );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

/*
		struct RTUInfo
		{
			RTUInfo():rtu(0),sid_conn(UniSetTypes::DefaultObjectId){}
			RTUStorage* rtu;
			UniSetTypes::ObjectId sid_conn;
		};
*/
		typedef std::map<int,RTUStorage*> RTUMap;
		RTUMap rtulist;

	private:
		RTUExchange();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		bool force;		/*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
		bool force_out;	/*!< флаг означающий, принудительного чтения выходов */
		bool mbregFromID;
		int polltime;	/*!< переодичность обновления данных, [мсек] */
		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		UniSetTypes::uniset_mutex pollMutex;
		Trigger trTimeout;
		PassiveTimer ptTimeout;
		
		PassiveTimer aiTimer;
		int ai_polltime;

		bool activated;
		int activateTimeout;
};
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
