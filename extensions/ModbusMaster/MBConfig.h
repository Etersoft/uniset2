/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#ifndef _MBConfig_H_
#define _MBConfig_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include "IONotifyController.h"
#include "Calibration.h"
#include "DelayTimer.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "IOBase.h"
#include "VTypes.h"
#include "MTR.h"
#include "RTUStorage.h"
#include "modbus/ModbusClient.h"
#include "Configuration.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*! Конфигурация для ModbusMaster */
    class MBConfig
    {
        public:
            MBConfig(const std::shared_ptr<uniset::Configuration>& conf
                     , xmlNode* cnode
                     , std::shared_ptr<SMInterface> _shm );

            ~MBConfig();

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
                int8_t nbit;         /*!< bit number (-1 - not used) */
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
            std::ostream& print_plist( std::ostream& os, const PList& p );

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

                // начальная инициализация для "записываемых" регистров
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
                uniset::ObjectId safemode_id = { uniset::DefaultObjectId }; /*!< идентификатор для датчика безопасного режима */
                IOController::IOStateList::iterator safemode_it;
                long safemode_value = { 1 };

                // return TRUE if state changed
                bool checkRespond( std::shared_ptr<DebugStream>& log );

                // специфические поля для RS
                ComPort::Speed speed = { ComPort::ComSpeed38400 };
                std::shared_ptr<RTUStorage> rtu188;
                ComPort::Parity parity = { ComPort::NoParity };
                ComPort::CharacterSize csize = { ComPort::CSize8 };
                ComPort::StopBits stopBits = { ComPort::OneBit };

                std::string getShortInfo() const;
            };

            friend std::ostream& operator<<( std::ostream& os, RTUDevice& d );

            typedef std::unordered_map<ModbusRTU::ModbusAddr, std::shared_ptr<RTUDevice>> RTUDeviceMap;

            friend std::ostream& operator<<( std::ostream& os, RTUDeviceMap& d );
            static void printMap(RTUDeviceMap& d);

            typedef std::list<IOBase> ThresholdList;

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

            static void rtuQueryOptimization( RTUDeviceMap& m, size_t maxQueryCount );
            static void rtuQueryOptimizationForDevice( const std::shared_ptr<RTUDevice>& d, size_t maxQueryCount );
            static void rtuQueryOptimizationForRegMap( const std::shared_ptr<RegMap>& regmap, size_t maxQueryCount );

            // т.к. пороговые датчики не связаны напрямую с обменом, создаём для них отдельный список
            // и отдельно его проверяем потом
            ThresholdList thrlist;
            RTUDeviceMap devices;
            InitList initRegList;    /*!< список регистров для инициализации */

            void loadConfig( const std::shared_ptr<uniset::UniXML>& xml, UniXML::iterator sensorsSection );
            void initDeviceList( const std::shared_ptr<UniXML>& xml );
            bool initItem( UniXML::iterator& it );

            std::string s_field;
            std::string s_fvalue;

            // определение timeout для соединения
            timeout_t recv_timeout = { 500 }; // msec
            timeout_t default_timeout = { 5000 }; // msec
            timeout_t aftersend_pause = { 0 };
            timeout_t polltime = { 100 };    /*!< периодичность обновления данных, [мсек] */
            timeout_t sleepPause_msec = { 10 };

            size_t maxQueryCount = { ModbusRTU::MAXDATALEN }; /*!< максимальное количество регистров для одного запроса */
            xmlNode* cnode = { 0 };
            std::shared_ptr<DebugStream> mblog;
            std::string myname;
            std::string prefix;
            std::string prop_prefix;  /*!< префикс для считывания параметров обмена */
            std::string defaultMBtype;
            std::string defaultMBaddr;
            bool mbregFromID = { false };
            bool defaultMBinitOK = { false }; // флаг определяющий нужно ли ждать "первого обмена" или при запуске сохранять в SM значение default.
            bool noQueryOptimization = { false };
            std::shared_ptr<uniset::Configuration> conf;
            std::shared_ptr<SMInterface> shm;

            void cloneParams( const std::shared_ptr<MBConfig>& conf );
            std::string getShortInfo() const;

        protected:

            bool initSMValue( ModbusRTU::ModbusData* data, int count, RSProperty* p );

            void readConfiguration(const std::shared_ptr<uniset::UniXML>& xml, UniXML::iterator sensorsSection );
            void initOffsetList();

            std::shared_ptr<RTUDevice> addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML::iterator& it );
            std::shared_ptr<RegInfo> addReg(std::shared_ptr<RegMap>& devices, ModbusRTU::RegID id, ModbusRTU::ModbusData r, UniXML::iterator& it, std::shared_ptr<RTUDevice> dev );
            RSProperty* addProp( PList& plist, RSProperty&& p );

            bool initMTRitem(UniXML::iterator& it, std::shared_ptr<RegInfo>& p );
            bool initRTU188item(UniXML::iterator& it, std::shared_ptr<RegInfo>& p );
            bool initRSProperty( RSProperty& p, UniXML::iterator& it );
            bool initRegInfo(std::shared_ptr<RegInfo>& r, UniXML::iterator& it, std::shared_ptr<RTUDevice>& dev  );
            bool initRTUDevice( std::shared_ptr<RTUDevice>& d, UniXML::iterator& it );
            bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it );
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MBConfig_H_
// -----------------------------------------------------------------------------
