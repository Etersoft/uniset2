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
#include "ThreadCreator.h"
#include "IOBase.h"
#include "VTypes.h"
#include "MTR.h"
// -----------------------------------------------------------------------------
/*!
      \page page_ModbusTCP Реализация ModbusTCP master
      
      - \ref sec_MBTCP_Comm
      - \ref sec_MBTCP_Conf
      - \ref sec_MBTCP_ConfList
      
      \section sec_MBTCP_Comm Общее описание ModbusTCP master
      Класс реализует процесс обмена (опрос/запись) с RTU-устройствами,
      через TCP-шлюз. Список регистров с которыми работает процесс задаётся в конфигурационном файле
      в секции \b <sensors>. см. \ref sec_MBTCP_Conf
      
      \section  sec_MBTCP_Conf Конфигурирование ModbusTCP master

      Конфигурирование процесса осуществляется либо параметрами командной строки либо
      через настроечную секцию. 

      \par Секция с настройками
      При своём старте, в конфигурационном файле ищётся секция с названием объекта,
      в которой указываются настроечные параметры по умолчанию.
      Пример:
      \code
	<MBMaster1 name="MBMaster1" gateway_iaddr="127.0.0.1" gateway_port="30000" polltime="200">
	     <DeviceList>
	         <item addr="0x01" respondSensor="RTU1_Not_Respond_FS" timeout="2000" invert="1"/>
		 <item addr="0x02" respondSensor="RTU2_Respond_FS" timeout="2000" invert="0"/>
	     </DeviceList>
	</MBMaster1>
      \endcode
      Секция <DeviceList> позволяет задать параметры обмена с конкретным RTU-устройством.
      
      - \b addr -  адрес устройства для которого, задаются параметры
      - \b timeout msec - таймаут, для определения отсутствия связи
      - \b invert - инвертировать логику. По умолчанию датчик выставляется в "1" при \b наличии связи.
      - \b respondSensor - идентификатор датчика связи.

      \par Параметры запуска
	
	При создании объекта в конструкторе передаётся префикс для определения параметров командной строки.
      По умолчанию \b xxx="mbtcp".
      Далее приведены основные параметры:

      \b --xxx-name ID - идентификатор процесса.
      
      IP-адрес шлюза задаётся параметром в конфигурационном файле \b gateway_iaddr или
      параметром командной строки \b --xxx-gateway-iaddr.
      
      Порт задаётся в конфигурационном файле параметром \b gateway_port или
      параметром командной строки \b --xxx-gateway-port. По умолчанию используется порт \b 502.
      
      \b --xxx-recv-timeout или \b recv_timeout msec - таймаут на приём одного сообщения. По умолчанию 100 мсек.
      
      \b --xxx-timeout или \b timeout msec  - таймаут на определение отсутсвия связи
                                                   (после этого идёт попытка реинициализировать соединение)
                                                   По умолчанию 5000 мсек.
      
      \b --xxx-no-query-optimization или \b no_query_optimization   - [1|0] отключить оптимизацию запросов
       
       Оптимизация заключается в том, что регистры идущие подряд автоматически запрашиваются/записываются одним запросом.
       В связи с чем, функция указанная в качестве \b mbfunc игнорируется и подменяется на работающую с многими регистрами.
      
      
      \b --xxx-poll-time или \b poll_time msec - пауза между опросами. По умолчанию 100 мсек.
      
      \b --xxx-initPause или \b initPause msec - пауза перед началом работы, после активации. По умолчанию 50 мсек.

      \b --xxx-force или \b force [1|0] 
       - 1 - перечитывать значения входов из SharedMemory на каждом цикле
       - 0 - обновлять значения только по изменению

      \b --xxx-persistent-connection или \b persistent_connection - НЕ закрывать соединение после каждого запроса.

      \b --xxx-force-out или \b force_out [1|0]
       - 1 - перечитывать значения выходов из SharedMemory на каждом цикле
       - 0 - обновлять значения только по изменению

      \b --xxx-reg-from-id или \b reg_from_id [1|0] 
       - 1 - в качестве регистра использовать идентификатор датчика
       - 0 - регистр брать из поля tcp_mbreg
      
      \b --xxx-heartbeat-id или \b heartbeat_id ID - идентификатор датчика "сердцебиения" (см. \ref sec_SM_HeartBeat)

      \b --xxx-heartbeat-max или \b heartbeat_max val - сохраняемое значение счётчика "сердцебиения".
      
      \b --xxx-activate-timeout msec . По умолчанию 2000. - время ожидания готовности SharedMemory к работе.
      
      \section  sec_MBTCP_ConfList Конфигурирование списка регистров для ModbusTCP master
      Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
      Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
      
      \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков
      
      \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

      \b --xxx-statistic-sec sec - при наличии выведет кол-во посланных запросов за этот промежуток времени.

      Если параметры не заданы, будет произведена попытка загрузить все датчики, у которых
      присутствуют необходимые настроечные параметры.
      
      \warning Если в результате список будет пустым, процесс завершает работу.

      Пример конфигурационных параметров:
  \code      
  <sensors name="Sensors">
    ...
    <item name="MySensor_S" textname="my sesnsor" iotype="DI" 
	      tcp_mbtype="rtu" tcp_mbaddr="0x01" tcp_mbfunc="0x04" tcp_mbreg="0x02" my_tcp="1" 
     />
    ...
  </sensors>
\endcode

  К основным параметрам относятся следующие:
   - \b tcp_mbtype    - [rtu] - пока едиственный разрешённый тип.
   - \b tcp_mbaddr    - адрес RTU-устройства.
   - \b tcp_mbreg     - запрашиваемый/записываемый регистр. 
   - \b tcp_mbfunc    - [0x1,0x2,0x3,...] функция опроса/записи. Разрешённые см. ModbusRTU::SlaveFunctionCode.
   
   Помимо этого можно задавать следующие параметры:
   - \b tcp_vtype     - тип переменной. см VTypes::VType.
   - \b tcp_rawdata   - [0|1]  - игнорировать или нет параметры калибровки
   - \b tcp_iotype    - [DI,DO,AI,AO] - переназначить тип датчика. По умолчанию используется поле iotype.
   - \b tcp_nbit      - номер бита в слове. Используется для DI,DO в случае когда для опроса используется
			 функция читающая слова (03
   - \b tcp_nbyte     - [1|2] номер байта. Используется если tcp_vtype="byte".
   - \b tcp_mboffset  - "сдвиг"(может быть отрицательным) при опросе/записи. 
                        Т.е. фактически будет опрошен/записан регистр "mbreg+mboffset".

   Для инициализации "выходов" (регистров которые пишутся) можно использовать поля:
   - \b tcp_preinit      - [0|1] считать регистр перед использованием (при запуске процесса)
   - \b tcp_init_mbfunc  - Номер функции для инициализации. Если не указана, будет определена автоматически исходя из tcp_mbfunc.
   - \b tcp_init_mbreg   - Номер регистра откуда считывать значение для инициализации. Если это поле не указано используется tcp_mbreg.

   Если указано tcp_preinit="1", то прежде чем начать писать регистр в устройство, будет произведено его чтение.

   По умолчанию все "записываемые" регистры инициализируются значением из SM. Т.е. пока не будет первый раз считано значение из SM,
   регистры в устройство писатся не будут. Чтобы отключить это поведение, можно указать параметр
   - \b tcp_sm_initOK    - [0|1] Игнорировать начальную инициализацию из SM (сразу писать в устройство)

   При этом будет записывыться значение "default".

   \warning Регистр должен быть уникальным. И может повторятся только если указан параметр \a nbit или \a nbyte.

*/
// -----------------------------------------------------------------------------
/*!
	\par Реализация Modbus TCP Master для обмена с многими ModbusRTU устройствами
	через один modbus tcp шлюз.

	\par Чтобы не зависеть от таймаутов TCP соединений, которые могут неопределённо зависать
	на создании соединения с недоступным хостом. Обмен вынесен в отдельный поток.
	При этом в этом же потоке обновляются данные в SM. В свою очередь информация о датчиках
	связи обновляется в основном потоке (чтобы не зависеть от TCP).
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

		void execute();
	
		static const int NoSafetyState=-1;

		enum Timer
		{
			tmExchange
		};

		enum DeviceType
		{
			dtUnknown,		/*!< неизвестный */
			dtRTU,			/*!< RTU (default) */
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

		typedef unsigned long RegID;

		typedef std::map<RegID,RegInfo*> RegMap;
		struct RegInfo
		{
			RegInfo():
				mbval(0),mbreg(0),mbfunc(ModbusRTU::fnUnknown),
				id(0),dev(0),mtrType(MTR::mtUnknown),
				q_num(0),q_count(1),mb_initOK(true),sm_initOK(true)
			{}

			ModbusRTU::ModbusData mbval;
			ModbusRTU::ModbusData mbreg;			/*!< регистр */
			ModbusRTU::SlaveFunctionCode mbfunc;	/*!< функция для чтения/записи */
			PList slst;
			RegID id;

			RTUDevice* dev;

			// only for MTR
			MTR::MTRType mtrType;	/*!< тип регистра (согласно спецификации на MTR) */
			
			// optimization
			int q_num;		/*!< number in query */
			int q_count;	/*!< count registers for query */
			
			RegMap::iterator rit;

			// начальная инициалиазция для "записываемых" регистров
			// Механизм:
			// Если tcp_preinit="1", то сперва будет сделано чтение значения из устройства.
			// при этом флаг mb_init=false пока не пройдёт успешной инициализации
			// Если tcp_preinit="0", то флаг mb_init сразу выставляется в true.
			bool mb_initOK;	/*!< инициализировалось ли значение из устройства */

			// Флаг sm_init означает, что писать в устройство нельзя, т.к. значение в "карте регистров"
			// ещё не инициализировано из SM
			bool sm_initOK;	/*!< инициализировалось ли значение из SM */
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
			resp_real(false),
			resp_init(false),
			ask_every_reg(false)
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
			bool ask_every_reg;

			// return TRUE if state changed
			bool checkRespond();

		};

		friend std::ostream& operator<<( std::ostream& os, RTUDevice& d );
		
		typedef std::map<ModbusRTU::ModbusAddr,RTUDevice*> RTUDeviceMap;

		friend std::ostream& operator<<( std::ostream& os, RTUDeviceMap& d );
		void printMap(RTUDeviceMap& d);
// ----------------------------------
		static RegID genRegID( const ModbusRTU::ModbusData r, const int fn );

		
	protected:
		struct InitRegInfo
		{
			InitRegInfo():
			dev(0),mbreg(0),
			mbfunc(ModbusRTU::fnUnknown),
			initOK(false),ri(0)
			{}
			RSProperty p;
			RTUDevice* dev;
			ModbusRTU::ModbusData mbreg;
			ModbusRTU::SlaveFunctionCode mbfunc;
			bool initOK;
			RegInfo* ri;
		};
		typedef std::list<InitRegInfo> InitList;

		void firstInitRegisters();
		bool preInitRead( InitList::iterator& p );
		bool initSMValue( ModbusRTU::ModbusData* data, int count, RSProperty* p );
		bool allInitOK;
	  
		RTUDeviceMap rmap;
		InitList initRegList;	/*!< список регистров для инициализации */
		
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
		void poll_thread();
		void poll();
		bool pollRTU( RTUDevice* dev, RegMap::iterator& it );
		
		void updateSM();
		void updateRTU(RegMap::iterator& it);
		void updateMTR(RegMap::iterator& it);
		void updateRSProperty( RSProperty* p, bool write_only=false );
		void updateRespondSensors();

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
		RegInfo* addReg( RegMap& rmap, RegID id, ModbusRTU::ModbusData r, UniXML_iterator& it,
							RTUDevice* dev, RegInfo* rcopy=0 );
		RSProperty* addProp( PList& plist, RSProperty& p );

		bool initMTRitem( UniXML_iterator& it, RegInfo* p );
		bool initRSProperty( RSProperty& p, UniXML_iterator& it );
		bool initRegInfo( RegInfo* r, UniXML_iterator& it, RTUDevice* dev  );
		bool initRTUDevice( RTUDevice* d, UniXML_iterator& it );
		bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
		
		void rtuQueryOptimization( RTUDeviceMap& m );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

		bool checkProcActive();
		void setProcActive( bool st );

	 private:
		MBTCPMaster();
		bool initPause;
		UniSetTypes::uniset_mutex mutex_start;

		bool force;		/*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
		bool force_out;	/*!< флаг означающий, принудительного чтения выходов */
		bool mbregFromID;
		int polltime;	/*!< переодичность обновления данных, [мсек] */
		timeout_t sleepPause_usec;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		UniSetTypes::uniset_mutex actMutex;
		bool activated;
		int activateTimeout;
		
		bool noQueryOptimization;
		bool force_disconnect;
		
		std::string prefix;
		
		bool no_extimer;

		timeout_t stat_time; 		/*!< время сбора статистики обмена */
		int poll_count;
		PassiveTimer ptStatistic;   /*!< таймер для сбора статистики обмена */

		// т.к. TCP может "зависнуть" на подключении к недоступному узлу
		// делаем опрос в отдельном потоке
		ThreadCreator<MBTCPMaster>* pollThread; /*!< поток опроса */
		bool pollActivated;
		UniSetTypes::uniset_mutex pollMutex;

		// определение timeout для соединения
		PassiveTimer ptTimeout;
		UniSetTypes::uniset_mutex tcpMutex;
};
// -----------------------------------------------------------------------------
#endif // _MBTCPMaster_H_
// -----------------------------------------------------------------------------
