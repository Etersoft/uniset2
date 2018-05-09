/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -----------------------------------------------------------------------------
#ifndef _MBExchange_H_
#define _MBExchange_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include "IONotifyController.h"
#include "UniSetObject.h"
#include "PassiveTimer.h"
#include "DelayTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "Calibration.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "IOBase.h"
#include "VTypes.h"
#include "MTR.h"
#include "RTUStorage.h"
#include "modbus/ModbusClient.h"
#include "LogAgregator.h"
#include "LogServer.h"
#include "LogAgregator.h"
#include "VMonitor.h"
// -----------------------------------------------------------------------------
#ifndef vmonit
#define vmonit( var ) vmon.add( #var, var )
#endif
// -------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	/*!
	    \par Базовый класс для реализация обмена по протоколу Modbus [RTU|TCP].
	*/
	class MBExchange:
		public UniSetObject
	{
		public:
			MBExchange( uniset::ObjectId objId, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
						const std::string& prefix = "mb" );
			virtual ~MBExchange();

			/*! глобальная функция для вывода help-а */
			static void help_print( int argc, const char* const* argv );

			/*! Режимы работы процесса обмена */
			enum ExchangeMode
			{
				emNone = 0,       /*!< нормальная работа (по умолчанию) */
				emWriteOnly = 1,  /*!< "только посылка данных" (работают только write-функции) */
				emReadOnly = 2,   /*!< "только чтение" (работают только read-функции) */
				emSkipSaveToSM = 3, /*!< не писать данные в SM (при этом работают и read и write функции) */
				emSkipExchange = 4 /*!< отключить обмен */
			};
			friend std::ostream& operator<<( std::ostream& os, const ExchangeMode& em );

			/*! Режимы работы процесса обмена */
			enum SafeMode
			{
				safeNone = 0,               /*!< не использовать безопасный режим (по умолчанию) */
				safeResetIfNotRespond = 1,  /*!< выставлять безопасное значение, если пропала связь с устройством */
				safeExternalControl = 2     /*!< управление сбросом по внешнему датчику */
			};


			friend std::string to_string( const SafeMode& m );
			friend std::ostream& operator<<( std::ostream& os, const SafeMode& m );

			enum DeviceType
			{
				dtUnknown,      /*!< неизвестный */
				dtRTU,          /*!< RTU (default) */
				dtMTR,          /*!< MTR (DEIF) */
				dtRTU188        /*!< RTU188 (Fastwell) */
			};

			static DeviceType getDeviceType( const std::string& dtype ) noexcept;
			friend std::ostream& operator<<( std::ostream& os, const DeviceType& dt );

			struct RTUDevice;
			struct RegInfo;

			struct RSProperty:
				public IOBase
			{
				// only for RTU
				int8_t nbit;		 /*!< bit number (-1 - not used) */
				VTypes::VType vType; /*!< type of value */
				uint16_t rnum;   /*!< count of registers */
				uint8_t nbyte;   /*!< byte number (1-2) */

				RSProperty():
					nbit(-1), vType(VTypes::vtUnknown),
					rnum(VTypes::wsize(VTypes::vtUnknown)),
					nbyte(0)
				{}

				// т.к. IOBase содержит rwmutex с запрещённым конструктором копирования
				// приходится здесь тоже объявлять разрешенными только операции "перемещения"
				RSProperty( const RSProperty& r ) = delete;
				RSProperty& operator=(const RSProperty& r) = delete;
				RSProperty( RSProperty&& r ) = default;
				RSProperty& operator=(RSProperty&& r) = default;

				std::shared_ptr<RegInfo> reg;
			};

			friend std::ostream& operator<<( std::ostream& os, const RSProperty& p );

			typedef std::list<RSProperty> PList;
			static std::ostream& print_plist( std::ostream& os, const PList& p );

			typedef std::map<ModbusRTU::RegID, std::shared_ptr<RegInfo>> RegMap;
			struct RegInfo
			{
				// т.к. RSProperty содержит rwmutex с запрещённым конструктором копирования
				// приходится здесь тоже объявлять разрешенными только операции "перемещения"
				RegInfo( const RegInfo& r ) = default;
				RegInfo& operator=(const RegInfo& r) = delete;
				RegInfo( RegInfo&& r ) = delete;
				RegInfo& operator=(RegInfo&& r) = default;
				RegInfo() = default;

				ModbusRTU::ModbusData mbval = { 0 };
				ModbusRTU::ModbusData mbreg = { 0 }; /*!< регистр */
				ModbusRTU::SlaveFunctionCode mbfunc = { ModbusRTU::fnUnknown };    /*!< функция для чтения/записи */
				PList slst;
				ModbusRTU::RegID regID = { 0 };

				std::shared_ptr<RTUDevice> dev;

				// only for RTU188
				RTUStorage::RTUJack rtuJack = { RTUStorage::nUnknown };
				int rtuChan = { 0 };

				// only for MTR
				MTR::MTRType mtrType = { MTR::mtUnknown };    /*!< тип регистра (согласно спецификации на MTR) */

				// optimization
				size_t q_num = { 0 };      /*!< number in query */
				size_t q_count = { 1 };    /*!< count registers for query */

				RegMap::iterator rit;

				// начальная инициалиазция для "записываемых" регистров
				// Механизм:
				// Если tcp_preinit="1", то сперва будет сделано чтение значения из устройства.
				// при этом флаг mb_init=false пока не пройдёт успешной инициализации
				// Если tcp_preinit="0", то флаг mb_init сразу выставляется в true.
				bool mb_initOK = { false };    /*!< инициализировалось ли значение из устройства */

				// Флаг sm_init означает, что писать в устройство нельзя, т.к. значение в "карте регистров"
				// ещё не инициализировано из SM
				bool sm_initOK = { false };    /*!< инициализировалось ли значение из SM */
			};

			friend std::ostream& operator<<( std::ostream& os, const RegInfo& r );
			friend std::ostream& operator<<( std::ostream& os, const RegInfo* r );

			struct RTUDevice
			{
				ModbusRTU::ModbusAddr mbaddr = { 0 };    /*!< адрес устройства */
				std::unordered_map<size_t, std::shared_ptr<RegMap>> pollmap;

				DeviceType dtype = { dtUnknown };    /*!< тип устройства */

				// resp - respond..(контроль наличия связи)
				uniset::ObjectId resp_id = { uniset::DefaultObjectId };
				IOController::IOStateList::iterator resp_it;
				DelayTimer resp_Delay; // таймер для формирования задержки на отпускание (пропадание связи)
				PassiveTimer resp_ptInit; // таймер для формирования задержки на инициализацию связи (задержка на выставление датчика связи после запуска)
				bool resp_state = { false };
				bool resp_invert = { false };
				bool resp_force = { false };
				Trigger trInitOK; // триггер для "инициализации"
				std::atomic<size_t> numreply = { 0 }; // количество успешных запросов..
				std::atomic<size_t> prev_numreply = { 0 };

				//
				bool ask_every_reg = { false }; /*!< опрашивать ли каждый регистр, независимо от результата опроса предыдущего. По умолчанию false - прервать опрос при первом же timeout */

				// режим работы
				uniset::ObjectId mode_id = { uniset::DefaultObjectId };
				IOController::IOStateList::iterator mode_it;
				long mode = { emNone }; // режим работы с устройством (см. ExchangeMode)

				// safe mode
				long safeMode = { safeNone }; /*!< режим безопасного состояния см. SafeMode */
				uniset::ObjectId safemode_id = { uniset::DefaultObjectId }; /*!< иденидентификатор для датчика безопасного режима */
				IOController::IOStateList::iterator safemode_it;
				long safemode_value = { 1 };

				// return TRUE if state changed
				bool checkRespond( std::shared_ptr<DebugStream>& log );

				// специфические поля для RS
				ComPort::Speed speed = { ComPort::ComSpeed38400 };
				std::shared_ptr<RTUStorage> rtu188;

				std::string getShortInfo() const;
			};

			friend std::ostream& operator<<( std::ostream& os, RTUDevice& d );

			typedef std::unordered_map<ModbusRTU::ModbusAddr, std::shared_ptr<RTUDevice>> RTUDeviceMap;

			friend std::ostream& operator<<( std::ostream& os, RTUDeviceMap& d );
			void printMap(RTUDeviceMap& d);

			// ----------------------------------
			enum Timer
			{
				tmExchange
			};

			void execute();

			inline std::shared_ptr<LogAgregator> getLogAggregator()
			{
				return loga;
			}
			inline std::shared_ptr<DebugStream> log()
			{
				return mblog;
			}

			virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

		protected:
			virtual void step();
			virtual void sysCommand( const uniset::SystemMessage* msg ) override;
			virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
			virtual void timerInfo( const uniset::TimerMessage* tm ) override;
			virtual void askSensors( UniversalIO::UIOCommand cmd );
			virtual void initOutput();
			virtual bool deactivateObject() override;
			virtual bool activateObject() override;
			virtual void initIterators();
			virtual void initValues();

			struct InitRegInfo
			{
				InitRegInfo():
					dev(0), mbreg(0),
					mbfunc(ModbusRTU::fnUnknown),
					initOK(false)
				{}
				RSProperty p;
				std::shared_ptr<RTUDevice> dev;
				ModbusRTU::ModbusData mbreg;
				ModbusRTU::SlaveFunctionCode mbfunc;
				bool initOK;
				std::shared_ptr<RegInfo> ri;
			};
			typedef std::list<InitRegInfo> InitList;

			void firstInitRegisters();
			bool preInitRead( InitList::iterator& p );
			bool initSMValue( ModbusRTU::ModbusData* data, int count, RSProperty* p );
			bool allInitOK;

			RTUDeviceMap devices;
			InitList initRegList;    /*!< список регистров для инициализации */

			virtual std::shared_ptr<ModbusClient> initMB( bool reopen = false ) = 0;

			virtual bool poll();
			bool pollRTU( std::shared_ptr<RTUDevice>& dev, RegMap::iterator& it );

			void updateSM();

			// в функции передаётся итератор,
			// т.к. в них идёт итерирование в случае если запрос в несколько регистров
			void updateRTU(RegMap::iterator& it);
			void updateMTR(RegMap::iterator& it);
			void updateRTU188(RegMap::iterator& it);
			void updateRSProperty( RSProperty* p, bool write_only = false );
			virtual void updateRespondSensors();

			bool isUpdateSM( bool wrFunc, long devMode ) const noexcept;
			bool isPollEnabled( bool wrFunc ) const noexcept;
			bool isSafeMode( std::shared_ptr<RTUDevice>& dev ) const noexcept;

			bool isProcActive() const;
			void setProcActive( bool st );
			bool waitSMReady();

			void readConfiguration();
			bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );
			bool initItem( UniXML::iterator& it );
			void initDeviceList();
			void initOffsetList();

			std::shared_ptr<RTUDevice> addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML::iterator& it );
			std::shared_ptr<RegInfo> addReg(std::shared_ptr<RegMap>& devices, ModbusRTU::RegID id, ModbusRTU::ModbusData r, UniXML::iterator& it, std::shared_ptr<RTUDevice> dev );
			RSProperty* addProp( PList& plist, RSProperty&& p );

			bool initMTRitem(UniXML::iterator& it, std::shared_ptr<RegInfo>& p );
			bool initRTU188item(UniXML::iterator& it, std::shared_ptr<RegInfo>& p );
			bool initRSProperty( RSProperty& p, UniXML::iterator& it );
			bool initRegInfo(std::shared_ptr<RegInfo>& r, UniXML::iterator& it, std::shared_ptr<RTUDevice>& dev  );
			bool initRTUDevice( std::shared_ptr<RTUDevice>& d, UniXML::iterator& it );
			virtual bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it );

			std::string initPropPrefix( const std::string& def_prop_prefix = "" );

			void rtuQueryOptimization( RTUDeviceMap& m );
			void rtuQueryOptimizationForDevice( const std::shared_ptr<RTUDevice>& d );
			void rtuQueryOptimizationForRegMap( const std::shared_ptr<RegMap>& regmap );

			xmlNode* cnode = { 0 };
			std::string s_field;
			std::string s_fvalue;

			std::shared_ptr<SMInterface> shm;

			timeout_t initPause = { 3000 };
			uniset::uniset_rwmutex mutex_start;

			bool force =  { false };        /*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
			bool force_out = { false };    /*!< флаг означающий, принудительного чтения выходов */
			bool mbregFromID = { false };
			timeout_t polltime = { 100 };    /*!< переодичность обновления данных, [мсек] */
			timeout_t sleepPause_msec = { 10 };
			size_t maxQueryCount = { ModbusRTU::MAXDATALEN }; /*!< максимальное количество регистров для одного запроса */

			PassiveTimer ptHeartBeat;
			uniset::ObjectId sidHeartBeat = { uniset::DefaultObjectId };
			long maxHeartBeat = { 10 };
			IOController::IOStateList::iterator itHeartBeat;
			uniset::ObjectId test_id = { uniset::DefaultObjectId };

			uniset::ObjectId sidExchangeMode = { uniset::DefaultObjectId }; /*!< иденидентификатор для датчика режима работы */
			IOController::IOStateList::iterator itExchangeMode;
			long exchangeMode = { emNone }; /*!< режим работы см. ExchangeMode */

			std::atomic_bool activated = { false };
			std::atomic_bool canceled = { false };
			timeout_t activateTimeout = { 20000 }; // msec
			bool noQueryOptimization = { false };
			bool notUseExchangeTimer = { false };

			std::string prefix;

			timeout_t stat_time = { 0 };      /*!< время сбора статистики обмена, 0 - отключена */
			size_t poll_count = { 0 };
			PassiveTimer ptStatistic; /*!< таймер для сбора статистики обмена */
			std::string statInfo = { "" };

			std::string prop_prefix;  /*!< префикс для считывания параметров обмена */

			std::shared_ptr<ModbusClient> mb;

			// определение timeout для соединения
			timeout_t recv_timeout = { 500 }; // msec
			timeout_t default_timeout = { 5000 }; // msec

			timeout_t aftersend_pause = { 0 };

			PassiveTimer ptReopen; /*!< таймер для переоткрытия соединения */
			Trigger trReopen;

			PassiveTimer ptInitChannel; /*!< задержка не инициализацию связи */

			// т.к. пороговые датчики не связаны напрямую с обменом, создаём для них отдельный список
			// и отдельно его проверяем потом
			typedef std::list<IOBase> ThresholdList;
			ThresholdList thrlist;

			std::string defaultMBtype;
			std::string defaultMBaddr;
			bool defaultMBinitOK = { false }; // флаг определяющий нужно ли ждать "первого обмена" или при запуске сохранять в SM значение default.

			std::shared_ptr<LogAgregator> loga;
			std::shared_ptr<DebugStream> mblog;
			std::shared_ptr<LogServer> logserv;
			std::string logserv_host = {""};
			int logserv_port = {0};
			const std::shared_ptr<SharedMemory> ic;

			VMonitor vmon;

			size_t ncycle = { 0 }; /*!< текущий номер цикла опроса */

		private:
			MBExchange();

	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MBExchange_H_
// -----------------------------------------------------------------------------
