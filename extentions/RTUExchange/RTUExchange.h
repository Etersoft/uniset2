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
#include "VTypes.h"
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

		static DeviceType getDeviceType( const std::string dtype );
		friend std::ostream& operator<<( std::ostream& os, const DeviceType& dt );
// -------------------------------------------------------------------------------
		struct RTUDevice;
		struct RegInfo;

		struct RSProperty:
			public IOBase
		{
			// only for RTU
			short nbit;				/*!< bit number (for func=[0x01,0x02]) */
			VTypes::VType vType;	/*!< type of value */
			short rnum;				/*!< count of registers */
			
			RSProperty():
				nbit(-1),vType(VTypes::vtUnknown),
				rnum(VTypes::wsize(VTypes::vtUnknown)),
				reg(0)
			{}

			RegInfo* reg;
		};

		friend std::ostream& operator<<( std::ostream& os, const RSProperty& p );

		typedef std::list<RSProperty> PList;
		
		struct RegInfo
		{
			RegInfo():
				mbval(0),mbreg(0),mbfunc(ModbusRTU::fnUnknown),
				mtrType(MTR::mtUnknown),
				rtuJack(RTUStorage::nUnknown),rtuChan(0),
				dev(0),
				q_num(0),q_count(1)
			{}

			ModbusRTU::ModbusData mbval;
			ModbusRTU::ModbusData mbreg;			/*!< регистр */
			ModbusRTU::SlaveFunctionCode mbfunc;	/*!< функция для чтения/записи */
			PList slst;

			// only for MTR
			MTR::MTRType mtrType;	/*!< тип регистра (согласно спецификации на MTR) */

			// only for RTU188
			RTUStorage::RTUJack rtuJack;
			int rtuChan;
			
			RTUDevice* dev;

			// optimization
			int q_num;		/*! number in query */
			int q_count;	/*! count registers for query */
		};

		friend std::ostream& operator<<( std::ostream& os, RegInfo& r );

		typedef std::map<ModbusRTU::ModbusData,RegInfo*> RegMap;

		struct RTUDevice
		{
			RTUDevice():
			respnond(false),
			mbaddr(0),
			dtype(dtUnknown),
			resp_id(UniSetTypes::DefaultObjectId),
			resp_state(false),
			resp_invert(false),
			resp_real(false),
			rtu(0)
			{}
			
			bool respnond;
			ModbusRTU::ModbusAddr mbaddr;	/*!< адрес устройства */
			RegMap regmap;

			DeviceType dtype;	/*!< тип устройства */

			UniSetTypes::ObjectId resp_id;
			IOController::DIOStateList::iterator resp_dit;
			PassiveTimer resp_ptTimeout;
			Trigger resp_trTimeout;
			bool resp_state;
			bool resp_invert;
			bool resp_real;

			RTUStorage* rtu;

			// return TRUE if state changed
			bool checkRespond();

		};

		friend std::ostream& operator<<( std::ostream& os, RTUDevice& d );
		
		typedef std::map<ModbusRTU::ModbusAddr,RTUDevice*> RTUDeviceMap;

		friend std::ostream& operator<<( std::ostream& os, RTUDeviceMap& d );
		void printMap(RTUDeviceMap& d);
// ----------------------------------
	protected:

		RTUDeviceMap rmap;

		ModbusRTUMaster* mb;
		UniSetTypes::uniset_mutex mbMutex;
		std::string devname;
		std::string speed;
		int recv_timeout;

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		void step();
		void poll();
		void pollRTU( RTUDevice* dev, RegMap::iterator& it );
		
		void updateSM();
		void updateRTU(RegMap::iterator& it);
		void updateMTR(RegMap::iterator& it);
		void updateRTU188(RegMap::iterator& it);

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
		
		void initMB();
		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );


		RTUDevice* addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		RegInfo* addReg( RegMap& rmap, ModbusRTU::ModbusData r, UniXML_iterator& it, 
							RTUDevice* dev, RegInfo* rcopy=0 );
		RSProperty* addProp(PList& plist, RSProperty& p );

		bool initMTRitem( UniXML_iterator& it, RegInfo* p );
		bool initRTU188item( UniXML_iterator& it, RegInfo* p );
		bool initRSProperty( RSProperty& p, UniXML_iterator& it );
		bool initRegInfo( RegInfo* r, UniXML_iterator& it, RTUDevice* dev  );
		bool initRTUDevice( RTUDevice* d, UniXML_iterator& it );
		bool initRespondInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		
		void rtuQueryOptimization( RTUDeviceMap& m );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

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

		bool activated;
		int activateTimeout;
};
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
