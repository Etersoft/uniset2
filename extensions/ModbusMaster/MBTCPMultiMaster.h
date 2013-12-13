#ifndef _MBTCPMultiMaster_H_
#define _MBTCPMultiMaster_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "MBExchange.h"
#include "modbus/ModbusTCPMaster.h"
// -----------------------------------------------------------------------------
/*!
      \page page_ModbusTCPMulti Реализация ModbusTCP 'multi' master
      
      - \ref sec_MBTCPM_Comm
      - \ref sec_MBTCPM_Conf
      - \ref sec_MBTCPM_ConfList
	  - \ref sec_MBTCPM_ExchangeMode
      
      \section sec_MBTCPM_Comm Общее описание ModbusTCPMultiMaster
      Класс реализует процесс обмена (опрос/запись) с RTU-устройствами,
      через TCP-шлюз. Список регистров с которыми работает процесс задаётся в конфигурационном файле
      в секции \b <sensors>. см. \ref sec_MBTCPM_Conf
		
	  При этом для шлюза можно задавать несколько ip-адресов (см. <GateList>), если связь пропадает по
	одному каналу (ip), то происходит переключение на другой канал (через timeout мсек), если пропадает
	с этим каналом, то переключается на следующий и так по кругу (в порядке уменьшения приоритета, задаваемого
	для каждого канала (cм. <GateList> \a priority).

      \section  sec_MBTCPM_Conf Конфигурирование ModbusTCPMultiMaster

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
		 <GateList>
			<item ip="" port="" respond_id="" priority=""/>
			<item ip="" port="" respond_id="" priority="" respond_invert="1"/>
		 <GateList>
	</MBMaster1>
      \endcode
      Секция <DeviceList> позволяет задать параметры обмена с конкретным RTU-устройством.
      
      - \b addr -  адрес устройства для которого, задаются параметры
      - \b timeout msec - таймаут, для определения отсутствия связи
      - \b invert - инвертировать логику. По умолчанию датчик выставляется в "1" при \b наличии связи.
      - \b respondSensor - идентификатор датчика связи (DI).
      - \b modeSensor - идентификатор датчика режима работы (см. MBExchange::ExchangeMode).
	  - \b ask_every_reg - 1 - опрашивать ВСЕ регистры подряд, не обращая внимания на timeout. По умолчанию - "0" Т.е. опрос устройства (на текущем шаге цикла опроса), прерывается на первом же регистре, при опросе которого возникнет timeout.

      Секция <GateList> позволяет задать несколько каналов связи со Slave-устройством. Это удобно для случая, когда Slave имеет
	более одного канала связи с ним (основной и резервный например).
      
      - \b ip -  ip-адрес 
      - \b port - порт
      - \b respond - датчик связи по данному каналу (помимо обобщённого)
      - \b priority - приоритет канала (чем больше число, тем выше приоритет)
      - \b respond_invert - инвертировать датчик связи (DI)

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
      
      
      \b --xxx-polltime или \b polltime msec - пауза между опросами. По умолчанию 100 мсек.
	  \b --xxx-checktime или \b checktime msec - пауза между проверками связи по разным каналам. По умолчанию 5000 мсек.
        Если задать <=0, то каналы будут просто переключаться по кругу (по timeout-у) в соответсвии с приоритетом (см. <GateList>).
		Если >0, то происходит проверка связи (раз в checktime) по всем каналам (см. <GateList>) и в случае потери связи, 
		происходит переключение на следующий канал, по которому связь есть.
      
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
      
      \section  sec_MBTCPM_ConfList Конфигурирование списка регистров для ModbusTCP master
      Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
      Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
      
      \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков
      
      \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

      \b --xxx-statistic-sec sec - при наличии выведет кол-во посланных запросов за этот промежуток времени.

      \b --xxx-set-prop-prefix [str] - Использовать 'str' в качестве префикса для свойств.
                                      Если не указать 'str' будет использован пустой префикс.

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

   \warning По умолчанию для свойств используется префикс "tcp_". Но если задано поле \b filter-field,
	то для свойств будет использован префикс <b>"filter-fileld"_</b>.
	При этом при помощи --xxx-set-prop-prefix val можно принудительно задать префикс.
	Если просто указать ключ --xxx-set-prop-prefix - будет использован "пустой" префикс (свойства без префикса).

  К основным параметрам относятся следующие (префикс \b tcp_ - для примера):
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


	\section sec_MBTCPM_ExchangeMode Управление режимом работы MBTCPMultiMaster
		В MBTCPMultiMaster заложена возможность управлять режимом работы процесса. Поддерживаются
	следующие режимы:
	- \b emNone - нормальная работа (по умолчанию)
    - \b emWriteOnly - "только посылка данных" (работают только write-функции)
	- \b emReadOnly - "только чтение" (работают только read-функции)
	- \b emSkipSaveToSM - "не записывать данные в SM", это особый режим, похожий на \b emWriteOnly,
			но отличие в том, что при этом режиме ведётся полноценый обмен (и read и write),
	только реально данные не записываются в SharedMemory(SM).
	- \b emSkipExchnage - отключить обмен (при этом данные "из SM" обновляются).

	Режимы переключаются при помощи датчика, который можно задать либо аргументом командной строки
	\b --prefix-exchange-mode-id либо в конф. файле параметром \b echangeModeID="". Константы определяющие режимы объявлены в MBTCPMultiMaster::ExchangeMode.

*/
// -----------------------------------------------------------------------------
/*!
	\par Реализация Modbus TCP Multi Master для обмена с многими ModbusRTU устройствами
	через один modbus tcp шлюз, доступный по нескольким ip-адресам.

	\par Чтобы не зависеть от таймаутов TCP соединений, которые могут неопределённо зависать
	на создании соединения с недоступным хостом. Обмен вынесен в отдельный поток.
	При этом в этом же потоке обновляются данные в SM. В свою очередь информация о датчиках
	связи обновляется в основном потоке (чтобы не зависеть от TCP).
*/
class MBTCPMultiMaster:
	public MBExchange
{
	public:
		MBTCPMultiMaster( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
						const std::string prefix="mbtcp" );
		virtual ~MBTCPMultiMaster();
	
		/*! глобальная функция для инициализации объекта */
		static MBTCPMultiMaster* init_mbmaster( int argc, const char* const* argv, 
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
											const std::string prefix="mbtcp" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

	protected:
		virtual void sysCommand( UniSetTypes::SystemMessage *sm );
		virtual void initIterators();
		virtual ModbusClient* initMB( bool reopen=false );
		void poll_thread();
		void check_thread();

		UniSetTypes::uniset_rwmutex mbMutex;
		int recv_timeout;
		bool force_disconnect;
		int checktime;

	 private:
		MBTCPMultiMaster();

		struct MBSlaveInfo
		{
			MBSlaveInfo():ip(""),port(0),mbtcp(0),priority(0),
				respond(false),respond_id(UniSetTypes::DefaultObjectId),respond_invert(false),
				recv_timeout(200),aftersend_pause(0),sleepPause_usec(100),
				force_disconnect(true),
				myname(""),initOK(false){}

			std::string ip;
			int port;
			ModbusTCPMaster* mbtcp;
			int priority;
			
			bool respond;
			UniSetTypes::ObjectId respond_id;
			IOController::DIOStateList::iterator respond_dit;
			bool respond_invert;

  			inline bool operator < ( const MBSlaveInfo& mbs ) const
			{
				return priority < mbs.priority;
			}

			bool init();
			bool check();

			int recv_timeout;
			int aftersend_pause;
			int sleepPause_usec;
			bool force_disconnect;
			
			std::string myname;

			bool initOK;
		};

		typedef std::list<MBSlaveInfo> MBGateList;

		MBGateList mblist;
		MBGateList::reverse_iterator mbi;

		// т.к. TCP может "зависнуть" на подключении к недоступному узлу
		// делаем опрос в отдельном потоке
		ThreadCreator<MBTCPMultiMaster>* pollThread; /*!< поток опроса */
		UniSetTypes::uniset_rwmutex tcpMutex;

		ThreadCreator<MBTCPMultiMaster>* checkThread; /*!< поток проверки связи по другим каналам */
};
// -----------------------------------------------------------------------------
#endif // _MBTCPMultiMaster_H_
// -----------------------------------------------------------------------------
