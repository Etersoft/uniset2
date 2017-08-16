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
#ifndef _MBSlave_H_
#define _MBSlave_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include "UniSetObject.h"
#include "modbus/ModbusTypes.h"
#include "modbus/ModbusServerSlot.h"
#include "modbus/ModbusTCPServer.h"
#include "modbus/ModbusTCPServerSlot.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "IOBase.h"
#include "VTypes.h"
#include "ThreadCreator.h"
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
	      \page page_ModbusSlave Реализация Modbus slave

	      - \ref sec_MBSlave_Comm
	      - \ref sec_MBSlave_Conf
	      - \ref sec_MBSlave_ConfList
	      - \ref sec_MBSlave_FileTransfer
	      - \ref sec_MBSlave_MEIRDI
		  - \ref sec_MBSlave_DIAG
		  - \ref sec_MBSlave_TCP
		  - \ref sec_MBSlave_REST_API

	      \section sec_MBSlave_Comm Общее описание Modbus slave
	      Класс реализует базовые функции для протокола Modbus в slave режиме. Реализацию Modbus RTU - см. RTUExchange.
	      Реализацию Modbus slave (TCP) - см. MBSlave. Список регистров с которыми работает процесс задаётся в конфигурационном файле
	      в секции \b <sensors>. см. \ref sec_MBSlave_Conf

	      В данной версии поддерживаются следующие функции:
	     - 0x02 - read input status
	     - 0x03 - read register outputs or memories or read word outputs or memories
	     - 0x04 - read input registers or memories or read word outputs or memories
	     - 0x05 - forces a single coil to either ON or OFF
	     - 0x06 - write register outputs or memories
	     - 0x08 - Diagnostics (Serial Line only)
	     - 0x0F - force multiple coils
	     - 0x10 - write register outputs or memories
	     - 0x14 - read file record
	     - 0x15 - write file record
	     - 0x2B - Modbus Encapsulated Interface
	     - 0x50 - set date and time
	     - 0x66 - file transfer

	      \section  sec_MBSlave_Conf Конфигурирование ModbusTCP slave

	      Конфигурирование процесса осуществляется либо параметрами командной строки либо
	      через настроечную секцию.

	      \par Секция с настройками
	      При своём старте, в конфигурационном файле ищётся секция с названием объекта,
	      в которой указываются настроечные параметры по умолчанию.
	      Пример:
	      \code
			<MBSlave1 name="MBSlave1" default_mbaddr="0x31"
	        afterSendPause="0"
	        reg_from_id="0"
	        replyTimeout="60"
	        askcount_id=""
	        respond_invert=""
	        respond_id=""
	        timeout=""
	        heartbeat_id=""
	        initPause=""
	        force=""
	        ...
	      \endcode

		  - \b default_mbaddr - адрес по умолчанию для данного устройства. Если указан адрес 255 - ответ будет на любые сообщения.
	      - \b afterSendPause - принудительная пауза после посылки ответа
	      - \b reg_from_id - номер регистра брать из ID датчика
	      - \b replyTimeout - таймаут на формирование ответа. Если ответ на запрос будет сформирован за большее время, он не будет отослан.
	      - \b askcount_id - идентификатор датчика для счётчика запросов
	      - \b respond_id - идентификатор датчика наличия связи. Выставляется в "1" когда связь есть.
	      - \b respond_invert - инвертировать логику выставления датчика связи (т.е. выставлять "1" - когда нет связи).
	      - \b heartbeat_id - идентификтор датчика "сердцебиения". См. \ref sec_SM_HeartBeat
	      - \b initPause - пауза перед началом работы, после активации. По умолчанию 3000 мсек.
	      - \b force - [1,0] перезаписывать ли значения в SharedMemory каждый раз (а не по изменению).
	      - \b timeout msec - таймаут, для определения отсутствия связи

	        Специфичные для RTU настройки:
	    \code
	            device="/dev/ttyS0" speed="9600" use485F="1" transmitCtl="0">
	      \endcode
	      - \b device -  устройство (порт)
	      - \b speed - скорость обмена
	      - \b use485F - [0,1] - использовать специальный класс для обмена по RS485 на контрллерах фаствел (убирает echo программным путём).
	      - \b transmitCtl - [0,1] - управлять ли приёмопередатчиков (ну программном уровне). Обычно это на аппаратном или драйвером.

	        Специфичные для TCP настройки:
	    \code
	        iaddr="localhost" iport="502"
	    \endcode
	      - \b iaddr - ip адрес данного устройства
	      - \b iport - tcp порт.


	      \par Параметры запуска

	    При создании объекта в конструкторе передаётся префикс для определения параметров командной строки.
	      По умолчанию \b xxx="mbs".
	      Далее приведены основные параметры:

	      \b --xxx-name ID - идентификатор процесса.

		  \b --xxx-default-mbaddr addr1 - slave-адрес по умолчанию для данного устройства. Если указан адрес 255 - ответ будет на любые сообщения.

		  \b --xxx-timeout или \b timeout msec  - таймаут на определение отсутсвия связи.

	      \b --xxx-reply-timeout msec  - таймаут на формирование ответа.

	      \b --xxx-initPause или \b initPause msec - пауза перед началом работы, после активации. По умолчанию 50 мсек.

	      \b --xxx-force или \b force [1|0]
	       - 1 - перечитывать/перезаписывать значения входов из SharedMemory на каждом цикле
	       - 0 - обновлять значения только по изменению

	      \b --xxx-reg-from-id или \b reg_from_id [1|0]
	       - 1 - в качестве регистра использовать идентификатор датчика
	       - 0 - регистр брать из поля tcp_mbreg

		  \b --xxx-default-mbfunc или \b default_mbfunc [0...255] - Функция подставляемая по умолчанию, если не указан параметр mbfunc. Действует только если включён контроль функций (check-mbfunc).
		  \b --xxx-check-mbfunc [0|1] -
		   - 1 - включить контроль (обработку) свойства mbfunc. По умолчанию: отключёна. Если контроль включён то разрешено
		   использовать один и тот же регистр но \b для \b разных \b функций.
		   - 0 - игнорировать свойство mbfunc..

	      \b --xxx-heartbeat-id или \b heartbeat_id ID - идентификатор датчика "сердцебиения" (см. \ref sec_SM_HeartBeat)

	      \b --xxx-heartbeat-max или \b heartbeat_max val - сохраняемое значение счётчика "сердцебиения".

	      \b --xxx-activate-timeout msec . По умолчанию 2000. - время ожидания готовности SharedMemory к работе.

	      \b --xxx-allow-setdatetime 0,1 - Включить функцию 0x50. Выставление даты и времени.

	      \par Настройки протокола RTU:

	      \b --xxx-dev devname  - файл устройства

	      \b --xxx-speed        - Скорость обмена (9600,19920,38400,57600,115200)

	      \par Настройки протокола TCP:

	      \b --xxx-inet-addr [xxx.xxx.xxx.xxx | hostname ]  - this modbus server address

	      \b --xxx-inet-port num - this modbus server port. Default: 502.

	      \section  sec_MBSlave_ConfList Конфигурирование списка регистров для ModbusSlave
	      Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
	      Список обрабатываемых регистров задаётся при помощи двух параметров командной строки

	      \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков

	      \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.

	      \warning Если в результате список будет пустым, процесс завершает работу.

	      Пример конфигурационных параметров:
	  \code
	  <sensors name="Sensors">
	    ...
	    <item name="MySensor_S" textname="my sesnsor" iotype="DI"
			  mbs="1" mbs_mbaddr="0x02" mbs_mbreg="1"
	     />
		<item name="MySensor2_S" textname="my sesnsor 2" iotype="DI"
			  mbs="1" mbs_mbaddr="0x01" mbs_mbreg="1"
		 />
	    ...
	  </sensors>
	\endcode

	   \warning По умолчанию для свойств используется заданный в конструктроре префикс "mbs_".

	  К основным параметрам настройки датчиков относятся следующие (префикс \b mbs_ - для примера):
	   - \b mbs_mbadrr    - адрес к которому относиться данный регистр. Если не используется параметр \b default_mbaddr.
	   - \b mbs_mbreg     - запрашиваемый/записываемый регистр. Если не используется параметр \b reg_from_id.

	   Помимо этого можно задавать следующие параметры:
	   - \b mbs_vtype     - тип переменной. см VTypes::VType.
	   - \b mbs_rawdata   - [0|1]  - игнорировать или нет параметры калибровки (cmin,cmax,rmin,rmax,presicion,caldiagram)
	   - \b mbs_iotype    - [DI,DO,AI,AO] - переназначить тип датчика. По умолчанию используется поле iotype.
	   - \b mbs_nbyte     - [1|2] номер байта. Используется если mbs_vtype="byte".

	   - \b accessmode    - режим доступа к регистру.
	                        "ro" - read only
	                        "wo" - write only
	                        "rw" - read/write. Режим по умолчанию.

	   \warning Регистр должен быть уникальным. И может повторятся только если указан параметр \a nbyte.

	    \section sec_MBSlave_FileTransfer Настройка передачи файлов в ModbusSlave (0x66)
	        Данная реализация позволяет передавать по протоколу Modbus заранее заданные файлы.
	    Настройка происходвится в конфигурационном файле.
	    \code
	            <filelist>
	                <!-- Список файлов разрешённых для передачи по modbus
	            directory - каталог где лежит файл. Можно не задавать
	                'ConfDir' - берётся из настроек (см. начало этого файла)
	                'DataDir' - берётся из настроек (см. начало этого файла)
	                'xxx'  - прямое указание каталога
	        -->
	                <item directory="ConfDir" id="1" name="configure.xml"/>
	                <item directory="ConfDir" id="2" name="VERSION"/>
	                <item directory="/tmp/" id="3" name="configure.xml.gz"/>
	                <item directory="ConfDir" id="4" name="SERIAL"/>
	            </filelist>
	    \endcode
	    - \b id - задаёт идентификтор файла (собственно он и будет запрашиваться.
	    - \b name - название файла
	    - \b directory - каталог где храниться файл.

	    \section sec_MBSlave_MEIRDI Поддержка "MODBUS Encapsulated Interface" (0x2B)[0x0E]
	    \code
	            <MEI>
	                <!-- ВНИМАНИЕ: должен заполняться в соответсвии со стандартом. ObjectID и DeviceID не случайны.. -->
	                <device id="0x01">
	                    <object id="0" comm="VendorName">
	                        <string value="etersoft"/>
	                    </object>
	                    <object id="1" comm="ProductCode">
	                        <string value="uniset"/>
	                    </object>
	                    <object id="2" comm="MajorMinorRevision">
	                        <string value="1.6"/>
	                    </object>
	                </device>
	                <device id="0x02">
	                    <object id="3" comm="VendorURL">
	                        <string value="http://www.etersoft.ru"/>
	                    </object>
	                    <object id="4" comm="ProductName">
	                        <string value="uniset"/>
	                    </object>
	                    <object id="5" comm="ModelName">
	                        <string value="uniset:MBSlave"/>
	                    </object>
	                    <object id="6" comm="UserApplicationName">
	                        <string value="MBSlave1"/>
	                    </object>
	                </device>
	                <device id="0x03">
	                    <object id="128" comm="private objects">
	                        <string id="129" value="etersoft"/>
	                        <string id="130" value="uniset"/>
	                        <string id="131" value="1.6"/>
	                        <string id="132" value="http://www.etersoft.ru"/>
	                        <string id="133" value="MBSlave1"/>
	                    </object>
	                </device>
	            </MEI>
	    \endcode


	    \section sec_MBSlave_DIAG Диагностические функции (0x08)

		\section sec_MBSlave_TCP  Настройка TCPServer
		\code
		<MBTCPPersistentSlave ....sesscount="">
				<clients>
					<item ip="" respond="" invert="1" askcount=""/>
					<item ip="" respond="" invert="1" askcount=""/>
					<item ip="" respond="" invert="1" askcount=""/>
				</clients>
		</MBTCPPersistentSlave>
		\endcode

		По умолчанию если не удалось создать сокет процесс пытается повторять попытки каждые
		tcpRepeatCreateSocketPause и не вылетает. Но если задан параметр tcpBreakIfFailRun="1",
		то процессы завершает работу после первой неудачной попытки.
		Изменить эти параметры можно либо в конфигурационном файле:
		\code
		<MBTCPPersistentSlave ... tcpBreakIfFailRun="1" tcpRepeatCreateSocket="xxx msec"/>
		\endcode

		Либо аргументами командной строки:
		- --prefix--break-if-fail-run 1
		- --prefix-repeat-create-socket msec.


		\section sec_MBSlave_REST_API MBSlave HTTP API


		/help                            - Получение списка доступных команд
		/                                - получение стандартной информации
		/regs?regs=reg1,reg2,reg3,..&addr=addr1,addr2,addr3  - получение списка регистров
									 Не обязательные параметры:
									   regs - выдать информацию только по указанным регистрам
									   addr - выдать информацию только по указанным адресам устройств
	*/
	// -----------------------------------------------------------------------------
	/*! Реализация slave-интерфейса */
	class MBSlave:
		public UniSetObject
	{
		public:
			MBSlave( uniset::ObjectId objId, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "mbs" );
			virtual ~MBSlave();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<MBSlave> init_mbslave(int argc, const char* const* argv,
					uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
					const std::string& prefix = "mbs" );

			/*! глобальная функция для вывода help-а */
			static void help_print( int argc, const char* const* argv );

			static const int NoSafetyState = -1;

			enum AccessMode
			{
				amRW,
				amRO,
				amWO
			};

			std::string amode2str( AccessMode m );

			struct BitRegProperty;

			struct IOProperty:
				public IOBase
			{
				ModbusRTU::ModbusData mbreg;    /*!< регистр */
				AccessMode amode;
				VTypes::VType vtype;    /*!< type of value */
				size_t wnum;               /*!< номер слова (для типов с размеров больше 2х байт */
				size_t nbyte;              /*!< номер байта, который надо "сохранить" из "пришедщего в запросе" слова. [1-2] */
				std::shared_ptr<BitRegProperty> bitreg; /*!< указатель, как признак является ли данный регистр "сборным" из битовых */
				ModbusRTU::RegID regID;

				IOProperty():
					mbreg(0),
					amode(amRW),
					vtype(VTypes::vtUnknown),
					wnum(0),
					nbyte(0),
					regID(0)
				{}

				friend std::ostream& operator<<( std::ostream& os, IOProperty& p );
			};


			struct BitRegProperty
			{
				typedef std::vector<IOProperty> BitSensorMap;

				ModbusRTU::ModbusData mbreg; /*!< к какому регистру относятся биты */
				BitSensorMap bvec;

				BitRegProperty(): mbreg(0), bvec(ModbusRTU::BitsPerData) {}

				/*! проверка привязан ли данный датчик, к какому-либо биту в этом слове */
				bool check( const IOController_i::SensorInfo& si );

				friend std::ostream& operator<<( std::ostream& os, BitRegProperty& p );
				friend std::ostream& operator<<( std::ostream& os, BitRegProperty* p );
			};

			inline long getConnCount()
			{
				return connCount;
			}

			inline std::shared_ptr<LogAgregator> getLogAggregator()
			{
				return loga;
			}
			inline std::shared_ptr<DebugStream> log()
			{
				return mblog;
			}

			virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

#ifndef DISABLE_REST_API
			// http API
			virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override;
			virtual Poco::JSON::Object::Ptr httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override;
#endif

		protected:

			/*! обработка 0x01 */
			ModbusRTU::mbErrCode readCoilStatus( ModbusRTU::ReadCoilMessage& query,
												 ModbusRTU::ReadCoilRetMessage& reply );
			/*! обработка 0x02 */
			ModbusRTU::mbErrCode readInputStatus( ModbusRTU::ReadInputStatusMessage& query,
												  ModbusRTU::ReadInputStatusRetMessage& reply );

			/*! обработка 0x03 */
			ModbusRTU::mbErrCode readOutputRegisters( ModbusRTU::ReadOutputMessage& query,
					ModbusRTU::ReadOutputRetMessage& reply );

			/*! обработка 0x04 */
			ModbusRTU::mbErrCode readInputRegisters( ModbusRTU::ReadInputMessage& query,
					ModbusRTU::ReadInputRetMessage& reply );

			/*! обработка 0x05 */
			ModbusRTU::mbErrCode forceSingleCoil( ModbusRTU::ForceSingleCoilMessage& query,
												  ModbusRTU::ForceSingleCoilRetMessage& reply );

			/*! обработка 0x0F */
			ModbusRTU::mbErrCode forceMultipleCoils( ModbusRTU::ForceCoilsMessage& query,
					ModbusRTU::ForceCoilsRetMessage& reply );


			/*! обработка 0x10 */
			ModbusRTU::mbErrCode writeOutputRegisters( ModbusRTU::WriteOutputMessage& query,
					ModbusRTU::WriteOutputRetMessage& reply );

			/*! обработка 0x06 */
			ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
					ModbusRTU::WriteSingleOutputRetMessage& reply );

			/*! обработка запросов на чтение ошибок */
			//        ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query,
			//                                                            ModbusRTU::JournalCommandRetMessage& reply );

			/*! обработка запроса на установку времени */
			ModbusRTU::mbErrCode setDateTime( ModbusRTU::SetDateTimeMessage& query,
											  ModbusRTU::SetDateTimeRetMessage& reply );

			/*! обработка запроса удалённого сервиса */
			ModbusRTU::mbErrCode remoteService( ModbusRTU::RemoteServiceMessage& query,
												ModbusRTU::RemoteServiceRetMessage& reply );

			ModbusRTU::mbErrCode fileTransfer( ModbusRTU::FileTransferMessage& query,
											   ModbusRTU::FileTransferRetMessage& reply );

			ModbusRTU::mbErrCode diagnostics( ModbusRTU::DiagnosticMessage& query,
											  ModbusRTU::DiagnosticRetMessage& reply );

			ModbusRTU::mbErrCode read4314( ModbusRTU::MEIMessageRDI& query,
										   ModbusRTU::MEIMessageRetRDI& reply );

			// т.к. в функциях (much_real_read,nuch_real_write) рассчёт на отсортированность IOMap
			// то использовать unordered_map нельзя
			typedef std::map<ModbusRTU::RegID, IOProperty> RegMap;

			typedef std::unordered_map<ModbusRTU::ModbusAddr, RegMap> IOMap;

			IOMap iomap;  /*!< список входов/выходов по адресам */

			// т.к. пороговые датчики не связаны напрямую с обменом, создаём для них отдельный список
			// и отдельно его проверяем потом
			typedef std::list<IOBase> ThresholdList;
			ThresholdList thrlist;

			std::shared_ptr<ModbusServerSlot> mbslot;
			std::unordered_set<ModbusRTU::ModbusAddr> vaddr; /*!< адреса данного узла */
			std::string default_mbaddr = { "" };

			xmlNode* cnode = { 0 };
			std::string s_field = { "" };
			std::string s_fvalue = { "" };
			int default_mbfunc = {0}; // функция по умолчанию, для вычисления RegID

			std::shared_ptr<SMInterface> shm;

			virtual void sysCommand( const uniset::SystemMessage* msg ) override;
			virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
			virtual void timerInfo( const uniset::TimerMessage* tm ) override;
			void askSensors( UniversalIO::UIOCommand cmd );
			void waitSMReady();
			virtual void execute_rtu();
			virtual void execute_tcp();
			virtual void updateStatistics();
			virtual void updateTCPStatistics();
			virtual void updateThresholds();
			virtual void postReceiveEvent( ModbusRTU::mbErrCode res );
			void runTCPServer();

			virtual bool activateObject() override;
			virtual bool deactivateObject() override;

			// действия при завершении работы
			virtual void sigterm( int signo ) override;
			virtual void finalThread();

			enum Timer
			{
				tmCheckExchange
			};

			uniset::timeout_t checkExchangeTime = { 10000 }; // контроль "живости" потока обмена, мсек

			virtual void initIterators();
			bool initItem( UniXML::iterator& it );
			bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );

			void readConfiguration();
			bool check_item( UniXML::iterator& it );

			ModbusRTU::mbErrCode real_write( RegMap& rmap, const ModbusRTU::ModbusData regOKOK, ModbusRTU::ModbusData val, const int fn = 0 );
			ModbusRTU::mbErrCode real_write( RegMap& rmap, const ModbusRTU::ModbusData regOKOK, ModbusRTU::ModbusData* dat, size_t& i, size_t count, const int fn = 0  );
			ModbusRTU::mbErrCode real_read( RegMap& rmap, const ModbusRTU::ModbusData regOKOK, ModbusRTU::ModbusData& val, const int fn = 0  );
			ModbusRTU::mbErrCode much_real_read( RegMap& rmap, const ModbusRTU::ModbusData regOKOK, ModbusRTU::ModbusData* dat, size_t count, const int fn = 0  );
			ModbusRTU::mbErrCode much_real_write(RegMap& rmap, const ModbusRTU::ModbusData regOKOK, ModbusRTU::ModbusData* dat, size_t count, const int fn = 0  );

			ModbusRTU::mbErrCode real_read_it( RegMap& rmap, RegMap::iterator& it, ModbusRTU::ModbusData& val );
			ModbusRTU::mbErrCode real_bitreg_read_it( std::shared_ptr<BitRegProperty>& bp, ModbusRTU::ModbusData& val );
			ModbusRTU::mbErrCode real_read_prop( IOProperty* p, ModbusRTU::ModbusData& val );

			ModbusRTU::mbErrCode real_write_it(RegMap& rmap, RegMap::iterator& it, ModbusRTU::ModbusData* dat, size_t& i, size_t count );
			ModbusRTU::mbErrCode real_bitreg_write_it( std::shared_ptr<BitRegProperty>& bp, const ModbusRTU::ModbusData val );
			ModbusRTU::mbErrCode real_write_prop(IOProperty* p, ModbusRTU::ModbusData* dat, size_t& i, size_t count );

#ifndef DISABLE_REST_API
			// http api
			Poco::JSON::Object::Ptr request_regs( const std::string& req, const Poco::URI::QueryParameters& p );
			Poco::JSON::Object::Ptr get_regs(ModbusRTU::ModbusAddr addr, const RegMap& rmap, const std::vector<std::string>& q_regs );
			Poco::JSON::Object::Ptr get_reginfo( const IOProperty& prop );
#endif
			MBSlave();
			timeout_t initPause = { 3000 };
			uniset::uniset_rwmutex mutex_start;
			std::unique_ptr< ThreadCreator<MBSlave> > thr;

			std::mutex mutexStartNotify;
			std::condition_variable startNotifyEvent;

			PassiveTimer ptHeartBeat;
			uniset::ObjectId sidHeartBeat = { uniset::DefaultObjectId };
			long maxHeartBeat = { 10 };
			IOController::IOStateList::iterator itHeartBeat;
			uniset::ObjectId test_id = { uniset::DefaultObjectId };

			IOController::IOStateList::iterator itAskCount;
			uniset::ObjectId askcount_id = { uniset::DefaultObjectId };

			IOController::IOStateList::iterator itRespond;
			uniset::ObjectId respond_id = { uniset::DefaultObjectId };
			bool respond_invert = { false };

			PassiveTimer ptTimeout;
			long connCount = { 0 };
			long restartTCPServerCount = { 0 };

			std::atomic_bool activated = { false };
			std::atomic_bool cancelled = { false };
			timeout_t activateTimeout = { 20000 }; // msec
			bool smPingOK = { false };
			timeout_t wait_msec = { 3000 };
			bool force = { false };        /*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */

			bool mbregFromID = {0};
			bool checkMBFunc = {0};
			bool noMBFuncOptimize = {0}; // флаг отключающий принудительное преобразование функций (0x06->0x10, 0x05->0x0F) см. initItem()

			int getOptimizeWriteFunction( const int fn ); // функция возвращает оптимизированную функцию (если оптимизация включена)

			typedef std::unordered_map<int, std::string> FileList;
			FileList flist;
			std::string prefix = { "" };
			std::string prop_prefix = { "" };

			ModbusRTU::ModbusData buf[ModbusRTU::MAXLENPACKET / 2 + 1]; /*!< буфер для формирования ответов */

			// данные для ответа на запрос 0x2B(43)/0x0E(14)
			// 'MEI' - modbus encapsulated interface
			// 'RDI' - read device identification
			typedef std::unordered_map<int, std::string> MEIValMap;
			typedef std::unordered_map<int, MEIValMap> MEIObjIDMap;
			typedef std::unordered_map<int, MEIObjIDMap> MEIDevIDMap;

			MEIDevIDMap meidev;

			std::shared_ptr<LogAgregator> loga;
			std::shared_ptr<DebugStream> mblog;
			std::shared_ptr<LogServer> logserv;
			std::string logserv_host = {""};
			int logserv_port = {0};
			VMonitor vmon;
			std::string mbtype = { "" };

			// ----------------------------------------------------------------------------
			// TCPServer section..
			void initTCPClients( UniXML::iterator confnode );

			timeout_t sessTimeout = { 2000 };  /*!< таймаут на сессию */
			timeout_t updateStatTime = { 4000 };
			ModbusTCPServer::Sessions sess; /*!< список открытых сессий */
			std::mutex sessMutex;
			size_t sessMaxNum = { 5 };
			std::shared_ptr<ModbusTCPServerSlot> tcpserver;

			struct ClientInfo
			{
				ClientInfo(): iaddr(""), respond_s(uniset::DefaultObjectId), invert(false),
					askCount(0), askcount_s(uniset::DefaultObjectId)
				{
					ptTimeout.setTiming(0);
				}

				std::string iaddr = { "" };

				uniset::ObjectId respond_s = { uniset::DefaultObjectId };
				IOController::IOStateList::iterator respond_it;
				bool invert = { false };
				PassiveTimer ptTimeout;
				timeout_t tout = { 2000 };

				long askCount = { 0 };
				uniset::ObjectId askcount_s = { uniset::DefaultObjectId };
				IOController::IOStateList::iterator askcount_it;

				inline void initIterators( const std::shared_ptr<SMInterface>& shm )
				{
					shm->initIterator( respond_it );
					shm->initIterator( askcount_it );
				}

				const std::string getShortInfo() const;
			};

			typedef std::unordered_map<std::string, ClientInfo> ClientsMap;
			ClientsMap cmap;

			uniset::ObjectId sesscount_id = { uniset::DefaultObjectId };
			IOController::IOStateList::iterator sesscount_it;

			std::atomic_bool tcpCancelled = { true };

			bool tcpBreakIfFailRun = { false };
			timeout_t tcpRepeatCreateSocketPause = { 30000 }; /*! пауза между попытками открыть сокет */
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MBSlave_H_
// -----------------------------------------------------------------------------
