#ifndef _MBTCPMaster_H_
#define _MBTCPMaster_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "IONotifyController.h"
#include "UniSetObject_LT.h"
#include "modbus/ModbusTCPMaster.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "Calibration.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "IOBase.h"
#include "VTypes.h"
// -----------------------------------------------------------------------------
/*!
	п═п╣п╟п╩п╦п╥п╟я├п╦я▐ Modbus TCP Master-п╟ п╢п╩я▐ я─п╟п╠п╬я┌я▀ я│п╬ п╪п╫п╬пЁп╦п╪п╦ ModbusRTU я┐я│я┌я─п╬п╧я│я┌п╡п╟п╪п╦,
	п╫п╬ я┌п╬п╩я▄п╨п╬ я┤п╣я─п╣п╥ п·п■п≤п² ModbusTCP-я┬п╩я▌п╥.
*/
class MBTCPMaster:
	public UniSetObject_LT
{
	public:
		MBTCPMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
						const std::string prefix="mbtcp" );
		virtual ~MBTCPMaster();
	
		/*! глобальная функция для инициализации объекта */
		static MBTCPMaster* init_mbmaster( int argc, const char* const* argv, 
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
											const std::string prefix="mbtcp" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		static const int NoSafetyState=-1;

		enum Timer
		{
			tmExchange
		};

		enum DeviceType
		{
			dtUnknown,		/*!< неизвестный */
			dtRTU			/*!< RTU (default) */
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
			short nbit;				/*!< bit number) */
			VTypes::VType vType;	/*!< type of value */
			short rnum;				/*!< count of registers */
			short nbyte;			/*!< byte number (1-2) */
			
			RSProperty():
				nbit(-1),vType(VTypes::vtUnknown),
				rnum(VTypes::wsize(VTypes::vtUnknown)),
				nbyte(0),reg(0)
			{}

			RegInfo* reg;
		};

		friend std::ostream& operator<<( std::ostream& os, const RSProperty& p );

		typedef std::list<RSProperty> PList;

		typedef std::map<ModbusRTU::ModbusData,RegInfo*> RegMap;
		struct RegInfo
		{
			RegInfo():
				mbval(0),mbreg(0),mbfunc(ModbusRTU::fnUnknown),
				dev(0),offset(0),
				q_num(0),q_count(1)
			{}

			ModbusRTU::ModbusData mbval;
			ModbusRTU::ModbusData mbreg;			/*!< регистр */
			ModbusRTU::SlaveFunctionCode mbfunc;	/*!< функция для чтения/записи */
			PList slst;

			RTUDevice* dev;

			int offset;

			// optimization
			int q_num;		/*! number in query */
			int q_count;	/*! count registers for query */
			
			RegMap::iterator rit;
		};

		friend std::ostream& operator<<( std::ostream& os, RegInfo& r );

		struct RTUDevice
		{
			RTUDevice():
			respnond(false),
			mbaddr(0),
			dtype(dtUnknown),
			resp_id(UniSetTypes::DefaultObjectId),
			resp_state(false),
			resp_invert(false),
			resp_real(true),
			resp_init(false)
			{
				resp_trTimeout.change(false);
			}
			
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
			bool resp_init;

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

		ModbusTCPMaster* mb;
		UniSetTypes::uniset_mutex mbMutex;
		std::string iaddr;
//		ost::InetAddress* ia;
		int port;
		int recv_timeout;

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;
		
		void step();
		void poll();
		bool pollRTU( RTUDevice* dev, RegMap::iterator& it );
		
		void updateSM();
		void updateRTU(RegMap::iterator& it);
		void updateRSProperty( RSProperty* p, bool write_only=false );

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
		
		void initMB( bool reopen=false );
		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );
		void initDeviceList();
		void initOffsetList();


		RTUDevice* addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		RegInfo* addReg( RegMap& rmap, ModbusRTU::ModbusData r, UniXML_iterator& it, 
							RTUDevice* dev, RegInfo* rcopy=0 );
		RSProperty* addProp( PList& plist, RSProperty& p );

		bool initRSProperty( RSProperty& p, UniXML_iterator& it );
		bool initRegInfo( RegInfo* r, UniXML_iterator& it, RTUDevice* dev  );
		bool initRTUDevice( RTUDevice* d, UniXML_iterator& it );
		bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		
		void rtuQueryOptimization( RTUDeviceMap& m );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

	private:
		MBTCPMaster();
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
		
		bool noQueryOptimization;
		
		bool allNotRespond;
		Trigger trAllNotRespond;
		PassiveTimer ptAllNotRespond;
		std::string prefix;
};
// -----------------------------------------------------------------------------
#endif // _MBTCPMaster_H_
// -----------------------------------------------------------------------------
