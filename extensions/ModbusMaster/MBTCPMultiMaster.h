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
#ifndef _MBTCPMultiMaster_H_
#define _MBTCPMultiMaster_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "MBExchange.h"
#include "modbus/ModbusTCPMaster.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	/*!
	      \page page_ModbusTCPMulti Реализация ModbusTCP 'multi' master

	      - \ref sec_MBTCPM_Comm
	      - \ref sec_MBTCPM_Conf
	      - \ref sec_MBTCPM_ConfList
	      - \ref sec_MBTCPM_ExchangeMode
		  - \ref sec_MBTCPM_CheckConnection

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
		<MBMaster1 name="MBMaster1" polltime="200" channelTimeout="..." exchangeModeID="..">
	         <DeviceList>
				 <item addr="0x01" respondSensor="RTU1_Not_Respond_FS" force="0" timeout="2000" invert="1"/>
	         <item addr="0x02" respondSensor="RTU2_Respond_FS" timeout="2000" invert="0"/>
	         </DeviceList>
	         <GateList>
				<item ip="" port="" respond_id="" priority="" force=""/>
	            <item ip="" port="" respond_id="" priority="" respond_invert="1"/>
	         <GateList>
	    </MBMaster1>
	      \endcode
		  - \b channelTimeout - умолчательный timeout для переключения каналов. По умолчанию: берётся общий defaultTimeout.

	      Секция <DeviceList> позволяет задать параметры обмена с конкретным RTU-устройством.

	      - \b addr -  адрес устройства для которого, задаются параметры
	      - \b timeout msec - таймаут, для определения отсутствия связи
	      - \b invert - инвертировать логику. По умолчанию датчик выставляется в "1" при \b наличии связи.
	      - \b respondSensor - идентификатор датчика связи (DI).
		  - \b force [1,0] - "1" - обновлять значение датчика связи в SM принудительно на каждом цикле проверки ("0" - только по изменению).
	      - \b exchangeModeID - идентификатор датчика режима работы (см. MBExchange::ExchangeMode).
	      - \b ask_every_reg - 1 - опрашивать ВСЕ регистры подряд, не обращая внимания на timeout. По умолчанию - "0" Т.е. опрос устройства (на текущем шаге цикла опроса), прерывается на первом же регистре, при опросе которого возникнет timeout.
		  - \b safemodeXXX - см. \ref sec_MBTCP_SafeMode

	      Секция <GateList> позволяет задать несколько каналов связи со Slave-устройством. Это удобно для случая, когда Slave имеет
	    более одного канала связи с ним (основной и резервный например).

	      - \b ip -  ip-адрес
	      - \b port - порт
	      - \b respond - датчик связи по данному каналу (помимо обобщённого)
	      - \b priority - приоритет канала (чем больше число, тем выше приоритет)
	      - \b respond_invert - инвертировать датчик связи (DI)
		  - \b force [1,0] - "1" - обновлять значение датчика связи в SM принудительно на каждом цикле проверки ("0" - только по изменению).
		  - \b timeout - таймаут на определение отсутсвия связи для данного канала. По умолчанию берётся глобальный.
		  - \b checkFunc - Номер функции для проверки соединения
		  - \b checkAddr - Адрес устройства для проверки соединения
		  - \b checkReg - Регистр для проверки соединения

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

		  \b --xxx-reinit-timeout или \b reinit_timeout msec  - таймаут на реинициализацию канала связи (после потери связи)
													   По умолчанию timeout мсек.

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

		  \b --xxx-check-func [1,2,3,4]   - Номер функции для проверки соединения
		  \b --xxx-check-addr [1..255 ]   - Адрес устройства для проверки соединения
		  \b --xxx-check-reg [1..65535]   - Регистр для проверки соединения
		  \b --xxx-check-init-from-regmap - Взять адрес, функцию и регистр для проверки связи из списка опроса

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
		Префикс должен задаваться "полным", т.е. включая _(подчёркивание), если оно используется в свойствах
		(например для "tcp_mbreg" должен быть задан --xxx-set-prop-prefix tcp_ ).

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
	   - \b tcp_pollfactor - [0...n] Частота опроса. n задаёт "часоту" опроса. т.е. опрос каждые 1...n циклов  (зависит от общего polltime)

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
	    \b --prefix-exchange-mode-id либо в конф. файле параметром \b exchangeModeID="". Константы определяющие режимы объявлены в MBTCPMultiMaster::ExchangeMode.


		\section sec_MBTCPM_CheckConnection Проверка соединения
		Для контроля состояния связи по "резервным" каналам создаётся специальный поток (check_thread), в котором
		происходит периодическая проверка связи по всем "пассивным"(резервным) в данный момент каналам. Это используется
		как для общей диагностики в системе, так и при выборе на какой канал переключаться в случае пропажи связи в основном канале.
		Т.е. будет выбран ближайший приоритетный канал у которого выставлен признак что есть связь.
		Период проверки связи по "резервным" каналам задаётся при помощи --prefix-checktime или параметром checktime="" в конфигурационном файле.
		В MBTCPMultiMaster реализовано два механизма проверки связи.

		- По умолчанию используется простая установка соединения и тут же его разрыв. Т.е. данные никакие не посылаются,
		но проверяется что host и port доступны для подключения.
		- Второй способ: это проверка соединения с посылкой modbus-запроса. Для этого имеется два способа
		указать адрес устройства, регистр и функция опроса для проверки.
		Либо в секции <GateList> для каждого канала можно указать:
		 - адрес устройства \b checkAddr=""
		 - функцию проверки \b checkFunc=""  - функция может быть только [01,02,03,04] (т.е. функции чтения).
		 - регистр \b checkReg
		Либо в командной строке \b задать параметры --prefix-check-addr,  --prefix-check-func,  --prefix-check-reg,
		которые будут одинаковыми для \b ВСЕХ \b КАНАЛОВ.
		Помимо этого если указать в командной строке аргумент --prefix-check-init-from-regmap, то для тестирования
		соединения будет взят первый попавшийся регистр из списка обмена.

		\warning Способ проверки при помощи "modbus-запроса" имеет ряд проблем: Если фактически производится
		обмен с несколькими устройствами (несколько mbaddr) через TCP-шлюз, то может быть "ложное" срабатвание,
		т.к. фактически состояние канала будет определяться только под связи с каким-то одним конкретным устройством.
		И получается, что если обмен ведётся например с тремя устройствами, но
		проверка канала происходит только по связи с первым, то если оно перестанет отвечать, это будет считаться
		сбоем всего канала и этот канал будет исключён из обмена (!). Если ведётся обмен только с одним устройством,
		такой проблеммы не возникает.
		Но к плюсам данного способа проверки связи ("modbus-запросом") является то, что соедиенение поддерживается
		постоянным, в отличие от "первого способа" при котором оно создаётся и сразу рвётся и если проверка
		настроена достаточно часто ( < TIME_WAIT для сокетов), то при длительной работе могут закончится дескрипторы
		на создание сокетов.
	*/
	// -----------------------------------------------------------------------------
	/*!
		\par Реализация Modbus TCP MultiMaster для обмена с многими ModbusRTU устройствами
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
			MBTCPMultiMaster( uniset::ObjectId objId, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
							  const std::string& prefix = "mbtcp" );
			virtual ~MBTCPMultiMaster();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<MBTCPMultiMaster> init_mbmaster(int argc, const char* const* argv,
					uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
					const std::string& prefix = "mbtcp" );

			/*! глобальная функция для вывода help-а */
			static void help_print( int argc, const char* const* argv );

			virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

		protected:
			virtual void sysCommand( const uniset::SystemMessage* sm ) override;
			virtual void initIterators() override;
			virtual std::shared_ptr<ModbusClient> initMB( bool reopen = false ) override;
			virtual bool deactivateObject() override;
			void initCheckConnectionParameters();

			void poll_thread();
			void check_thread();
			void final_thread();

			uniset::uniset_rwmutex mbMutex;
			bool force_disconnect;
			timeout_t checktime;

		private:
			MBTCPMultiMaster();

			struct MBSlaveInfo
			{
				MBSlaveInfo(): ip(""), port(0), mbtcp(0), priority(0),
					respond(false), respond_id(uniset::DefaultObjectId), respond_invert(false),
					recv_timeout(200), aftersend_pause(0), sleepPause_usec(100),
					force_disconnect(true),
					myname(""), use(false), initOK(false), ignore(false) {}

				std::string ip;
				int port;
				std::shared_ptr<ModbusTCPMaster> mbtcp;
				int priority;

				// параметры для проверки соединения..
				ModbusRTU::SlaveFunctionCode checkFunc = { ModbusRTU::fnUnknown };
				ModbusRTU::ModbusAddr checkAddr = { 0x00 };
				ModbusRTU::ModbusData checkReg = { 0 };

				bool respond;
				uniset::ObjectId respond_id;
				IOController::IOStateList::iterator respond_it;
				bool respond_invert;
				bool respond_init = { false };
				bool respond_force = { false }; // флаг означающий принудительно обновлять значение датчика связи на каждом цикле проверки
				DelayTimer respondDelay;
				timeout_t channel_timeout = { 0 };

				inline bool operator < ( const MBSlaveInfo& mbs ) const noexcept
				{
					return priority < mbs.priority;
				}

				bool init( std::shared_ptr<DebugStream>& mblog );
				bool check();
				void setUse( bool st );

				timeout_t recv_timeout;
				timeout_t aftersend_pause;
				timeout_t sleepPause_usec;
				bool force_disconnect;

				std::string myname;

				bool use = { false }; // флаг используется ли в данный момент этот канал
				bool initOK = { false };
				bool ignore = { false }; // игнорировать данное соединение (обычно флаг выставляется на время ignoreTimeout, если узел не отвечает, хотя связь есть.
				PassiveTimer ptIgnoreTimeout;

				const std::string getShortInfo() const;

				std::mutex mutInit;
			};

			typedef std::list<std::shared_ptr<MBSlaveInfo>> MBGateList;

			MBGateList mblist;
			MBGateList::reverse_iterator mbi;

			// т.к. TCP может "зависнуть" на подключении к недоступному узлу
			// делаем опрос в отдельном потоке
			std::unique_ptr< ThreadCreator<MBTCPMultiMaster> > pollThread; /*!< поток опроса */
			std::unique_ptr< ThreadCreator<MBTCPMultiMaster> > checkThread; /*!< поток проверки связи по другим каналам */
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MBTCPMultiMaster_H_
// -----------------------------------------------------------------------------
