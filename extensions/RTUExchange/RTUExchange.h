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

		enum Timer
		{
			tmExchange
		};

		// --------------------------------------------------------
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
		static std::ostream& print_plist( std::ostream& os, PList& p );

		typedef std::map<ModbusRTU::ModbusData,RegInfo*> RegMap;
		struct RegInfo
		{
			RegInfo():
				mbval(0),mbreg(0),mbfunc(ModbusRTU::fnUnknown),
				mtrType(MTR::mtUnknown),
				rtuJack(RTUStorage::nUnknown),rtuChan(0),
				dev(0),offset(0),
				q_num(0),q_count(1),mb_init(false),sm_init(false),
				mb_init_mbreg(0)
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

			int offset;

			// optimization
			int q_num;		/*! number in query */
			int q_count;	/*! count registers for query */
			
			RegMap::iterator rit;
			bool mb_init;	/*!< init before use */
			bool sm_init;	/*!< SM init value */
			ModbusRTU::ModbusData mb_init_mbreg;	/*!< mb_init register */
		};

		friend std::ostream& operator<<( std::ostream& os, RegInfo& r );



		struct RTUDevice
		{
			RTUDevice():
			speed(ComPort::ComSpeed38400),
			respnond(false),
			mbaddr(0),
			dtype(dtUnknown),
			resp_id(UniSetTypes::DefaultObjectId),
			resp_state(false),
			resp_invert(false),
			resp_real(false),
			resp_init(false),
			rtu(0)
			{
				resp_trTimeout.change(false);
			}
			
			
			ComPort::Speed speed;
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
		ComPort::Speed defSpeed;
		int recv_timeout;
		bool use485F;
		bool transmitCtl;

		virtual void step();
		void poll();
		bool pollRTU( RTUDevice* dev, RegMap::iterator& it );
		
		void updateSM();
		void updateRTU(RegMap::iterator& it);
		void updateMTR(RegMap::iterator& it);
		void updateRTU188(RegMap::iterator& it);
		void updateRSProperty( RSProperty* p, bool write_only=false );

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		virtual void sysCommand( UniSetTypes::SystemMessage *msg );
		virtual void sensorInfo( UniSetTypes::SensorMessage*sm );
		void timerInfo( UniSetTypes::TimerMessage *tm );
		void askSensors( UniversalIO::UIOCommand cmd );	
		void initOutput();

		virtual bool activateObject();
		
		// действия при завершении работы
		virtual void sigterm( int signo );
		
		void initMB( bool reopen=false );
		virtual void initIterators();
		bool initItem( UniXML_iterator& it );
		void initDeviceList();
		void initOffsetList();

		RTUDevice* addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		RegInfo* addReg( RegMap& rmap, ModbusRTU::ModbusData r, UniXML_iterator& it, 
							RTUDevice* dev, RegInfo* rcopy=0 );
		RSProperty* addProp( PList& plist, RSProperty& p );

		bool initMTRitem( UniXML_iterator& it, RegInfo* p );
		bool initRTU188item( UniXML_iterator& it, RegInfo* p );
		bool initRSProperty( RSProperty& p, UniXML_iterator& it );
		bool initRegInfo( RegInfo* r, UniXML_iterator& it, RTUDevice* dev  );
		bool initRTUDevice( RTUDevice* d, UniXML_iterator& it );
		bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		
		void rtuQueryOptimization( RTUDeviceMap& m );

	private:
		RTUExchange();

		UniSetTypes::uniset_mutex pollMutex;
		bool rs_pre_clean;
		bool allNotRespond;
		Trigger trAllNotRespond;
		PassiveTimer ptAllNotRespond;
};
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
