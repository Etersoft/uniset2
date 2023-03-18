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
#ifndef _MBTCPMaster_H_
#define _MBTCPMaster_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "MBExchange.h"
#include "modbus/ModbusTCPMaster.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
    \page page_ModbusTCP Реализация ModbusTCP master

    - \ref sec_MBTCP_Comm
    - \ref sec_MBTCP_Conf
    - \ref sec_MBTCP_ConfList
    - \ref sec_MBTCP_ExchangeMode
    - \ref sec_MBTCP_ReloadConfig
    - \ref sec_MBTCP_REST_API

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
     <MBMaster1 name="MBMaster1" gateway_iaddr="127.0.0.1" gateway_port="30000" polltime="200" exchangeModeID="...">
       <DeviceList>
         <item addr="0x01" respondSensor="RTU1_Not_Respond_FS" timeout="2000" invert="1"/>
         <item addr="0x02" respondSensor="RTU2_Respond_FS" timeout="2000" invert="0"/>
       </DeviceList>
     </MBMaster1>
    \endcode
    Где
    - \b exchangeModeID - датчик(name) определяющий режим работы (см. MBExchange::ExchangeMode).

    Секция <DeviceList> позволяет задать параметры обмена с конкретным RTU-устройством.
    - \b addr -  адрес устройства для которого, задаются параметры
    - \b timeout msec - таймаут, для определения отсутствия связи
    - \b invert - инвертировать логику. По умолчанию датчик выставляется в "1" при \b наличии связи.
    - \b respondSensor - название(name) датчика связи.
    - \b respondInitTimeout - msec, время на инициализацию связи после запуска процесса. Т.е. только после этого времени будет выставлен(обновлён) датчик наличия связи. По умолчанию время равно timeout.
    - \b ask_every_reg - 1 - опрашивать ВСЕ регистры подряд, не обращая внимания на timeout. По умолчанию - "0" Т.е. опрос устройства (на текущем шаге цикла опроса), прерывается на первом же регистре, при опросе которого возникнет timeout.
    - \b safemodeXXX - см. \ref sec_MBTCP_SafeMode

    \par Параметры запуска

     При создании объекта в конструкторе передаётся префикс для определения параметров командной строки.
    По умолчанию \b xxx="mbtcp".
    Далее приведены основные параметры:
     - \b --xxx-name ID - идентификатор(name) процесса.
       IP-адрес шлюза задаётся параметром в конфигурационном файле \b gateway_iaddr или параметром командной строки \b --xxx-gateway-iaddr.
      Порт задаётся в конфигурационном файле параметром \b gateway_port или параметром командной строки \b --xxx-gateway-port. По умолчанию используется порт \b 502.

     - \b --xxx-recv-timeout или \b recv_timeout msec - таймаут на приём одного сообщения. По умолчанию 100 мсек.
     - \b --xxx-timeout или \b timeout msec  - таймаут на определение отсутствия связи (после этого идёт попытка реинициализировать соединение). По умолчанию 5000 мсек.

     - \b --xxx-reinit-timeout или \b reinit_timeout msec  - таймаут на реинициализацию канала связи (после потери связи). По умолчанию timeout.
     - \b --xxx-no-query-optimization или \b no_query_optimization- [1|0] отключить оптимизацию запросов
     Оптимизация заключается в том, что регистры идущие подряд автоматически запрашиваются/записываются одним запросом.
     В связи с чем, функция указанная в качестве \b mbfunc игнорируется и подменяется на работающую с многими регистрами.

     - \b --xxx-poll-time или \b poll_time msec - пауза между опросами. По умолчанию 100 мсек.
     - \b --xxx-initPause или \b initPause msec - пауза перед началом работы, после активации. По умолчанию 50 мсек.
     - \b --xxx-force или \b force [1|0]
       - 1 - перечитывать значения входов из SharedMemory на каждом цикле
       - 0 - обновлять значения только по изменению
     - \b --xxx-persistent-connection или \b persistent_connection - НЕ закрывать соединение после каждого запроса.
     - \b --xxx-force-out или \b force_out [1|0]
       - 1 - перечитывать значения выходов из SharedMemory на каждом цикле
       - 0 - обновлять значения только по изменению
     - \b --xxx-reg-from-id или \b reg_from_id [1|0]
       - 1 - в качестве регистра использовать идентификатор датчика
       - 0 - регистр брать из поля tcp_mbreg
     - \b --xxx-heartbeat-id или \b heartbeat_id ID - название для датчика "сердцебиения" (см. \ref sec_SM_HeartBeat)
     - \b --xxx-heartbeat-max или \b heartbeat_max val - сохраняемое значение счётчика "сердцебиения".
     - \b --xxx-activate-timeout msec . По умолчанию 2000. - время ожидания готовности SharedMemory к работе.

    \section  sec_MBTCP_ConfList Конфигурирование списка регистров для ModbusTCP master
    Конфигурационные параметры задаются в секции <sensors> конфигурационного файла.
    Список обрабатываемых регистров задаётся при помощи двух параметров командной строки
     - \b --xxx-filter-field  - задаёт фильтрующее поле для датчиков
     - \b --xxx-filter-value  - задаёт значение фильтрующего поля. Необязательный параметр.
     - \b --xxx-statistic-sec sec - при наличии выведет кол-во посланных запросов за этот промежуток времени.
     - \b --xxx-set-prop-prefix [str] - Использовать 'str' в качестве префикса для свойств.
    Если не указать 'str' будет использован пустой префикс.

     Если параметры не заданы, будет произведена попытка загрузить все датчики, у которых
    присутствуют необходимые настроечные параметры.

    \warning Если в результате список будет пустым, процесс завершает работу.

    Пример конфигурационных параметров:
    \code
    <sensors name="Sensors">
     ...
     <item name="MySensor_S" textname="my sesnsor" iotype="DI" tcp_mbtype="rtu" tcp_mbaddr="0x01" tcp_mbfunc="0x04" tcp_mbreg="0x02" my_tcp="1"/>
     ...
    </sensors>
    \endcode

    \warning При помощи --xxx-set-prop-prefix val можно принудительно задать префикс.
     Если просто указать ключ --xxx-set-prop-prefix - будет использован "пустой" префикс (свойства без префикса).
     Префикс должен задаваться "полным", т.е. включая _(подчёркивание), если оно используется в свойствах
     (например для "tcp_mbreg" должен быть задан --xxx-set-prop-prefix tcp_ ).

    К основным параметрам относятся следующие (префикс \b tcp_ - для примера):
    - \b tcp_mbtype - [rtu] - пока едиственный разрешённый тип.
    - \b tcp_mbaddr - адрес RTU-устройства.
    - \b tcp_mbreg  - запрашиваемый/записываемый регистр.
    - \b tcp_mbfunc - [0x1,0x2,0x3,...] функция опроса/записи. Разрешённые см. ModbusRTU::SlaveFunctionCode.

    Помимо этого можно задавать следующие параметры:
    - \b tcp_vtype  - тип переменной. см VTypes::VType.
    - \b tcp_rawdata- [0|1]  - игнорировать или нет параметры калибровки (cmin,cmax,rmin,rmax,presicion,caldiagram)
    - \b tcp_iotype - [DI,DO,AI,AO] - переназначить тип датчика. По умолчанию используется поле iotype.
    - \b tcp_nbit- номер бита в слове. Используется для DI,DO в случае когда для опроса используется функция читающая слова (03,04)
    - \b tcp_nbyte  - [1|2] номер байта. Используется если tcp_vtype="byte".
    - \b tcp_mboffset  - "сдвиг"(может быть отрицательным) при опросе/записи.
    Т.е. фактически будет опрошен/записан регистр "mbreg+mboffset".
    - \b tcp_pollfactor - [0...n] Частота опроса. n задаёт "частоту" опроса. т.е. опрос каждые 1...n циклов
    - \b tcp_mbmask     - "битовая маска" (uint16_t). Позволяет задать маску для значения. Действует как на значения читаемые из
     регистров, так и записываемые. При этом разрешается привязывать разные датчики к одному и томуже modbus-регистру указывая
     разные маски. Применяется для ресгитров читаемых словами (функции 03, 04).

    Для инициализации "выходов" (регистров которые пишутся) можно использовать поля:
    - \b tcp_preinit- [0|1] считать регистр перед использованием (при запуске процесса)
    - \b tcp_init_mbfunc  - Номер функции для инициализации. Если не указана, будет определена автоматически исходя из tcp_mbfunc.
    - \b tcp_init_mbreg- Номер регистра откуда считывать значение для инициализации. Если это поле не указано используется tcp_mbreg.

    Если указано tcp_preinit="1", то прежде чем начать писать регистр в устройство, будет произведено его чтение.

    По умолчанию все "записываемые" регистры инициализируются значением из SM. Т.е. пока не будет первый раз считано значение из SM,
    регистры в устройство писатся не будут. Чтобы отключить это поведение, можно указать параметр
    - \b tcp_sm_initOK - [0|1] Игнорировать начальную инициализацию из SM (сразу писать в устройство)

    При этом будет записывыться значение "default".

    \warning Регистр должен быть уникальным. И может повторятся только если указан параметр \a nbit или \a nbyte.


     \section sec_MBTCP_ExchangeMode Управление режимом работы MBTCPMaster
    В MBTCPMaster заложена возможность управлять режимом работы процесса. Поддерживаются
     следующие режимы:
     - \b emNone - нормальная работа (по умолчанию)
     - \b emWriteOnly - "только посылка данных" (работают только write-функции)
     - \b emReadOnly - "только чтение" (работают только read-функции)
     - \b emSkipSaveToSM - "не записывать данные в SM", это особый режим, похожий на \b emWriteOnly,
    но отличие в том, что при этом режиме ведётся полноценый обмен (и read и write),
     только реально данные не записываются в SharedMemory(SM).
     - \b emSkipExchnage - отключить обмен (при этом данные "из SM" обновляются).

     Режимы переключаются при помощи датчика, который можно задать либо аргументом командной строки
     - \b --prefix-exchange-mode-id либо в конф. файле параметром \b exchangeModeID="". Константы определяющие режимы объявлены в MBTCPMaster::ExchangeMode.

     \section sec_MBTCP_SafeMode Управление режимом "безопасного состояния"
    В MBTCPMaster заложена возможность управлять режимом выставления безопасного состояния входов и выходов.
     Возможны следующие режимы:
     - \b safeNone - режим отключён (по умолчанию)
     - \b safeExternalControl - управление при помощи внешнего датчика
     - \b safeResetIfNotRespond - выставление безопасных значение, если пропала связь с устройством.

     Суть этого режима, в том, что все входы и выходы у которых в настройках указан параметр safeval=""
     выставляются в это значение, при срабатывании внешнего датчика (режим "safeExternalControl") или
     при отсутсвии связи с устройством (режим "safeResetIfNotRespond").

     Режим задаётся в секции <DeviceList> для каждого устройства отдельно.
     \code
     <DeviceList>
       <item addr="01" .. safemodeSensor="Slave1_SafemodeSensor_S" safemodeValue="42"/>
       <item addr="02" .. safemodeResetIfNotRespond="1"/>
     </DeviceList>
     \endcode
     Если указан параметр \a safemodeSensor="..", то используется режим \b safeExternalControl.
     При этом можно указать конкретное значение датчика \a safemodeSensorValue="..",
     при котором будет сделан сброс значений в безопасное состояние.

     Если указан параметр safemodeResetIfNotRespond="1", то будет использован режим \b safeResetIfNotRespond.
     Если указан и параметр \a safemodeSensor=".." и \a safemodeResetIfNotRespond="1", то будет использован
     режим \b safeExternalControl (как более приоритетный).

     \section sec_MBTCP_ReloadConfig Переконфигурирование "на ходу"
     В процессе реализована возможность перечитать конфигурацию "на ходу". Для этого достаточно процессу
     послать команду SystemMessage::Reconfigure или воспользоваться HTTP API, где можно указать файл из
     которого произвести загрузку (см. \ref sec_MBTCP_REST_API).

     Послать команду можно при помощи утилиты uniset2-admin
     \code
      uniset2-admin --configure ObjectID
     \endcode

     При этом процесс приостанавливает обмен и перезагружает конфигурационный файл с которым был запущен.
     Переконфигурировать можно регистры, список устройств, адреса и любые свойства. В том числе возможно
     добавление новых регистров в список или уменьшение списка существующих.

     \warning Если во время загрузки новой конфигурации будет найдена какая-то ошибка, то конфигурация не будет применена.
     Ошибку можно будет увидеть в логах процесса.

     \warning Важно понимать, что это перезагрузка только настроек касающихся ModbusMaster, поэтому список датчиков
     и другие базовые настройки должны совпадать с исходным файлом. Т.е. возможно только переопределение параметров
     касающихся обмена, а не всего конфига в целом.

     \section sec_MBTCP_REST_API ModbusMaster HTTP API
     \code
        /help                                  - Получение списка доступных команд
        /                                      - получение стандартной информации
        /reload?confile=/path/to/confile       - Перезагрузить конфигурацию
                                                 confile - абсолютный путь до файла с конфигурацией. Не обязательный параметр.
     \endcode

     \warning Важно иметь ввиду, что если указывается confile, то он должен совпадать с базовым configure.xml
     в идентификаторах датчиков. И как минимум должен содержать соответствующую секцию настроек для процесса
     и те датчики, которые участвуют в обмене. Т.к. реальный config (глобальный) не подменяется, из указанного
     файла только загружаются необходимые для инициализации обмена параметры.
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
        public MBExchange
    {
        public:
            MBTCPMaster( uniset::ObjectId objId, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                         const std::string& prefix = "mbtcp" );
            virtual ~MBTCPMaster();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<MBTCPMaster> init_mbmaster( int argc, const char* const* argv,
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "mbtcp" );

            static void help_print( int argc, const char* const* argv );

            virtual uniset::SimpleInfo* getInfo( const char* userparam = 0 ) override;

        protected:
            virtual void sysCommand( const uniset::SystemMessage* sm ) override;
            virtual std::shared_ptr<ModbusClient> initMB( bool reopen = false ) override;
            virtual bool deactivateObject() override;
            virtual bool reconfigure( const std::shared_ptr<uniset::UniXML>& xml, const std::shared_ptr<uniset::MBConfig>& mbconf ) override;

            std::string iaddr;
            int port;

            void poll_thread();
            void final_thread();
            bool force_disconnect;

        private:
            MBTCPMaster();

            std::shared_ptr<ModbusTCPMaster> mbtcp;

            // т.к. TCP может "зависнуть" на подключении к недоступному узлу
            // делаем опрос в отдельном потоке
            std::unique_ptr<ThreadCreator<MBTCPMaster>> pollThread; /*!< поток опроса */
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MBTCPMaster_H_
// -----------------------------------------------------------------------------
