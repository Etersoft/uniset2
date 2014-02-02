#ifndef _MBExchange_H_
#define _MBExchange_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "IONotifyController.h"
#include "UniSetObject_LT.h"
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
#include "RTUStorage.h"
#include "modbus/ModbusClient.h"
// -----------------------------------------------------------------------------
/*!
    \par Базовый класс для реализация обмена по протоколу Modbus [RTU|TCP].
*/
class MBExchange:
    public UniSetObject_LT
{
    public:
        MBExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
                        const std::string& prefix="mb" );
        virtual ~MBExchange();

        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char* const* argv );

        static const int NoSafetyState=-1;

        /*! Режимы работы процесса обмена */
        enum ExchangeMode
        {
            emNone=0,         /*!< нормальная работа (по умолчанию) */
            emWriteOnly=1,     /*!< "только посылка данных" (работают только write-функции) */
            emReadOnly=2,        /*!< "только чтение" (работают только read-функции) */
            emSkipSaveToSM=3,    /*!< не писать данные в SM (при этом работают и read и write функции */
            emSkipExchange=4  /*!< отключить обмен */
        };

        friend std::ostream& operator<<( std::ostream& os, const ExchangeMode& em );

        enum DeviceType
        {
            dtUnknown,        /*!< неизвестный */
            dtRTU,            /*!< RTU (default) */
            dtMTR,            /*!< MTR (DEIF) */
            dtRTU188        /*!< RTU188 (Fastwell) */
        };

        static DeviceType getDeviceType( const std::string& dtype );
        friend std::ostream& operator<<( std::ostream& os, const DeviceType& dt );

        struct RTUDevice;
        struct RegInfo;

        struct RSProperty:
            public IOBase
        {
            // only for RTU
            short nbit;                /*!< bit number) */
            VTypes::VType vType;    /*!< type of value */
            short rnum;                /*!< count of registers */
            short nbyte;            /*!< byte number (1-2) */

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
                id(0),dev(0),
                rtuJack(RTUStorage::nUnknown),rtuChan(0),
                mtrType(MTR::mtUnknown),
                q_num(0),q_count(1),mb_initOK(true),sm_initOK(true)
            {}

            ModbusRTU::ModbusData mbval;
            ModbusRTU::ModbusData mbreg;            /*!< регистр */
            ModbusRTU::SlaveFunctionCode mbfunc;    /*!< функция для чтения/записи */
            PList slst;
            RegID id;

            RTUDevice* dev;

            // only for RTU188
            RTUStorage::RTUJack rtuJack;
            int rtuChan;

            // only for MTR
            MTR::MTRType mtrType;    /*!< тип регистра (согласно спецификации на MTR) */

            // optimization
            int q_num;        /*!< number in query */
            int q_count;    /*!< count registers for query */

            RegMap::iterator rit;

            // начальная инициалиазция для "записываемых" регистров
            // Механизм:
            // Если tcp_preinit="1", то сперва будет сделано чтение значения из устройства.
            // при этом флаг mb_init=false пока не пройдёт успешной инициализации
            // Если tcp_preinit="0", то флаг mb_init сразу выставляется в true.
            bool mb_initOK;    /*!< инициализировалось ли значение из устройства */

            // Флаг sm_init означает, что писать в устройство нельзя, т.к. значение в "карте регистров"
            // ещё не инициализировано из SM
            bool sm_initOK;    /*!< инициализировалось ли значение из SM */
        };

        friend std::ostream& operator<<( std::ostream& os, RegInfo& r );
        friend std::ostream& operator<<( std::ostream& os, RegInfo* r );

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
            ask_every_reg(false),
            mode_id(UniSetTypes::DefaultObjectId),
            mode(emNone),
            speed(ComPort::ComSpeed38400),
            rtu(0)
            {
                resp_trTimeout.change(false);
            }

            bool respnond;
            ModbusRTU::ModbusAddr mbaddr;    /*!< адрес устройства */
            RegMap regmap;

            DeviceType dtype;    /*!< тип устройства */

            UniSetTypes::ObjectId resp_id;
            IOController::IOStateList::iterator resp_it;
            PassiveTimer resp_ptTimeout;
            Trigger resp_trTimeout;
            bool resp_state;
            bool resp_invert;
            bool resp_real;
            bool resp_init;
            bool ask_every_reg;
            UniSetTypes::ObjectId mode_id;
            IOController::IOStateList::iterator mode_it;
            long mode; // режим работы с устройством (см. ExchangeMode)

            // return TRUE if state changed
            bool checkRespond();

            // специфические поля для RS
            ComPort::Speed speed;
            RTUStorage* rtu;
        };

        friend std::ostream& operator<<( std::ostream& os, RTUDevice& d );

        typedef std::map<ModbusRTU::ModbusAddr,RTUDevice*> RTUDeviceMap;

        friend std::ostream& operator<<( std::ostream& os, RTUDeviceMap& d );
        void printMap(RTUDeviceMap& d);

        // ----------------------------------
        static RegID genRegID( const ModbusRTU::ModbusData r, const int fn );

        enum Timer
        {
            tmExchange
        };

        void execute();

    protected:
        virtual void step();
        virtual void sysCommand( const UniSetTypes::SystemMessage *msg );
        virtual void sensorInfo( const UniSetTypes::SensorMessage*sm );
        virtual void timerInfo( const UniSetTypes::TimerMessage *tm );
        virtual void askSensors( UniversalIO::UIOCommand cmd );
        virtual void initOutput();
        virtual void sigterm( int signo );
        virtual bool activateObject();
        virtual void initIterators();

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
        InitList initRegList;    /*!< список регистров для инициализации */
        UniSetTypes::uniset_mutex pollMutex;

        virtual ModbusClient* initMB( bool reopen=false )= 0;

        virtual void poll();
        bool pollRTU( RTUDevice* dev, RegMap::iterator& it );

        void updateSM();
        void updateRTU(RegMap::iterator& it);
        void updateMTR(RegMap::iterator& it);
        void updateRTU188(RegMap::iterator& it);
        void updateRSProperty( RSProperty* p, bool write_only=false );
        virtual void updateRespondSensors();

        bool checkUpdateSM( bool wrFunc, long devMode );
        bool checkPoll( bool wrFunc );

        bool checkProcActive();
        void setProcActive( bool st );
        void waitSMReady();

        void readConfiguration();
        bool readItem( const UniXML& xml, UniXML_iterator& it, xmlNode* sec );
        bool initItem( UniXML_iterator& it );
        void initDeviceList();
        void initOffsetList();

        RTUDevice* addDev( RTUDeviceMap& dmap, ModbusRTU::ModbusAddr a, UniXML_iterator& it );
        RegInfo* addReg( RegMap& rmap, RegID id, ModbusRTU::ModbusData r, UniXML_iterator& it,
                            RTUDevice* dev, RegInfo* rcopy=0 );
        RSProperty* addProp( PList& plist, RSProperty& p );

        bool initMTRitem( UniXML_iterator& it, RegInfo* p );
        bool initRTU188item( UniXML_iterator& it, RegInfo* p );
        bool initRSProperty( RSProperty& p, UniXML_iterator& it );
        bool initRegInfo( RegInfo* r, UniXML_iterator& it, RTUDevice* dev  );
        bool initRTUDevice( RTUDevice* d, UniXML_iterator& it );
        virtual bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it );

        void rtuQueryOptimization( RTUDeviceMap& m );

        xmlNode* cnode;
        std::string s_field;
        std::string s_fvalue;

        SMInterface* shm;

        bool initPause;
        UniSetTypes::uniset_rwmutex mutex_start;

        bool force;        /*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */
        bool force_out;    /*!< флаг означающий, принудительного чтения выходов */
        bool mbregFromID;
        int polltime;    /*!< переодичность обновления данных, [мсек] */
        timeout_t sleepPause_usec;

        PassiveTimer ptHeartBeat;
        UniSetTypes::ObjectId sidHeartBeat;
        int maxHeartBeat;
        IOController::IOStateList::iterator itHeartBeat;
        UniSetTypes::ObjectId test_id;

        UniSetTypes::ObjectId sidExchangeMode; /*!< иденидентификатор для датчика режима работы */
        IOController::IOStateList::iterator itExchangeMode;
        long exchangeMode; /*!< режим работы см. ExchangeMode */

        UniSetTypes::mutex_atomic_t activated;
        int activateTimeout;
        bool noQueryOptimization;
        bool no_extimer;

        std::string prefix;

        timeout_t stat_time;         /*!< время сбора статистики обмена */
        int poll_count;
        PassiveTimer ptStatistic;   /*!< таймер для сбора статистики обмена */

        std::string prop_prefix;  /*!< префикс для считывания параметров обмена */

        ModbusClient* mb;

        // определение timeout для соединения
        PassiveTimer ptTimeout;
        bool pollActivated;
        int recv_timeout;

        int aftersend_pause;

        PassiveTimer ptReopen; /*!< таймер для переоткрытия соединения */
        Trigger trReopen;

        // т.к. пороговые датчики не связаны напрямую с обменом, создаём для них отдельный список
        // и отдельно его проверяем потом
        typedef std::list<IOBase> ThresholdList;
        ThresholdList thrlist;

     private:
        MBExchange();

};
// -----------------------------------------------------------------------------
#endif // _MBExchange_H_
// -----------------------------------------------------------------------------
