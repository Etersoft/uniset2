// -----------------------------------------------------------------------------
#ifndef SharedMemory_H_
#define SharedMemory_H_
// -----------------------------------------------------------------------------
#include <string>
#include <deque>
#include "IONotifyController_LT.h"
#include "Mutex.h"
#include "PassiveTimer.h"
#include "NCRestorer.h"
#include "WDTInterface.h"
// -----------------------------------------------------------------------------

/*! \page page_SharedMemory Реализация разделямой между процессами памяти (SharedMemory)


      \section sec_SM_Common Задачи решаемые объектом SharedMemory

    Класс SharedMemory расширяет набор задач класса IONotifyController.
    Для ознакомления с базовыми функциями см. \ref page_IONotifyController

    Задачи решаемые SM:
    - \ref sec_SM_Conf
    - \ref sec_SM_Event
    - \ref sec_SM_Depends
    - \ref sec_SM_HeartBeat
    - \ref sec_SM_History
    - \ref sec_SM_Pulsar
    - \ref sec_SM_DBLog

    \section sec_SM_Conf Определение списка регистрируемых датчиков
      SM позволяет определять список датчиков, которые он будет предоставлять
      для работы другим объектам. Помимо этого можно задавать фильтрующие поля
      для списка "заказчиков"(consumer) по каждому датчику, а также
      поля для фильтрования списка зависимостей(depends) по датчикам.
      Все эти параметры задаются в командной строке

     \par Фильтрование списка регистрируемых датчиков
      \code
        --s-filter-field   - задаёт фильтрующее поле для датчиков
        --s-filter-value   - задаёт значение фильтрующего поля. Необязательный параметр.
      \endcode
    Пример файла настроек:
    \code
      <sensors>
        ...
        <item id="12" name="Sensor12" textname="xxx" .... myfilter="m1" ...>
          <consumers>
            <item name="Consumer1" type="object" mycfilter="c1" .../>
          </consumers>
        </item>  
        ...
        <item id="121" name="Sensor121" textname="xxx" .... myfilter="m1" ...>
          <consumers>
            <item name="Consumer1" type="object" mycfilter="c1" .../>
          </consumers>
        </item>  
        ...
      </sensors>
    \endcode
    Для того, чтобы SM зарегистрировало на себя датчики 12 и 121 необходимо
    указать \b --s-filter-field myfilter \b --s-filter-value m1.
    
    \par Фильтрование заказчиков (consumer)
      \code
        --с-filter-field   - задаёт фильтрующее поле для заказчиков (consumer)
        --с-filter-value   - задаёт значение фильтрующего поля. Необязательный параметр.
      \endcode
    Если задать \b --c-filter-field mycfilter \b --c-filter-value c1, то при загрузке
    SM внесёт в список заказчиков, только те объекты, у которых будет указан параметрах
    \a mycfilter="c1".
    
    \par Фильтрование зависимостей (depends)
      \code
        --d-filter-field   - задаёт фильтрующее поле для "зависимостей" (depends)
        --d-filter-value   - задаёт значение фильтрующего поля. Необязательный параметр.
      \endcode

     \par Фильтрование порогов (<thresholds>)
     \code
        --t-filter-field   - задаёт фильтрующее поле для "порогов" (thresholds)
        --t-filter-value   - задаёт значение фильтрующего поля. Необязательный параметр.
     \endcode


    \note Если поле \b --X-filter-value не указано будут загружены все датчики(заказчики,зависимости)
    у которых поле \b --X-filter-field не пустое.
    \note Если не указывать параметры \b --X-filter-field - то в SM будут загружен ВЕСЬ список 
    датчиков(заказчиков, зависимостей) из секции <sensors>.
    
    
    \section sec_SM_Event Уведомление о рестарте SM
    В SM реализован механизм позволяющий задавать список объетов,
    которым присылается уведомление о старте/рестарте SM.
    Параметр определяющий фильтр, по которому формируется список объектов
    задаётся из командной строки \b --e-filter. 
    Параметр \b --e-startup-pause msec - задаёт паузу после старта SM,
    после которой заданным объектам посылается уведомление.
    Чтобы указать объект которому необходимо присылать уведомление,
    необходимо для него в конфигурайионном файле указать поле 
    \b evnt_xxx="1", где \b xxx - имя заданное в качестве параметра 
    \b --e-filter.
    Пример:
    \code
    <objects>
      ...
      <item id="2342" name="MyObject" ... evnt_myfilter="1" ../>
    </objects>
    \endcode
    При этом в параметрах старта SM должно быть указано \b --e-filter myfilter.
    Тогда при своём старте SM пришлёт объекту с идентификатором 2342 уведомление.
    
    В качестве уведомления объектам рассылается сообщение \b SystemMessage::WatchDog.

    \section sec_SM_Depends Механизм зависимостей между датчиками
    В SM реализован механизм позволяющий задавать зависимости между датчиками. Т.е. датчик
    будет равен "0" пока разрешающий датчик не будет равено "1". Ниже показан пример конфигурирования
    зависимости.
    \code
      <item id="20050" iotype="AI" name="Sensor1"" textname="Зависящий датчик 1">
        <consumers>
            <consumer name="Consumer1" type="objects"/>
        </consumers>
        <depends>
            <depend block_invert="1" name="Node_Not_Respond_FS"/>
        </depends>
    </item>
    \endcode
    В данном примере Sensor1 зависит от значения датчика Node_Not_Respond_FS. При этом значение блокировки
    инвертировано (block_invert=1). Т.е. если Node_Not_Respond=0, то Sensor1 - будет равен своему реальному
    значению. Как только Node_Not_Respond_FS станет равен 1, зависящий от него датчик Sensor1 сбросится в "0".
    Описание зависимости производится в секции <depends>. Возможные поля:
    \code
      block_invert - инвертировать "разрешающий" датчик
    \endcode
    На данный момент зависимость можно устанавливать только на дискретные датчики.
    
        
    \section sec_SM_HeartBeat Слежение за "живостью" объектов ("сердцебиение")
    
      Слежение за специальными датчиками (инкремент специальных датчиков),
      а также выставление дискретных датчиков "живости" процессов.
      
      Данный механизм построен на следующей логике:
      \par
      Каждому процессу, за которым необходимо следить, назначается два датчика "сердцебиения"(heartbeat), 
      аналоговый(счётчик) и дискретный. Во время работы, процесс периодически (время задаётся в настройках)
      сохраняет в свой \b аналоговый датчик заданное значение (количество тактов).
      В свою очередь процесс SM, каждый "такт"(время между шагами задаётся в настройках),
      "отнимает" у этого значения 1 (декриментирует). Пока значение счётчика больше нуля, дискретный датчик
      держиться равным "1" (т.е. 1 - процесс "жив"). Если процесс "вылетает" и перестаёт обновлять свой счётчик,
      то через некоторое количество тактов его счётчик становится меньше нуля. Как только это происходит, SM фиксирует,
      "недоступность" процесса, и выставляет дискретный датчик в ноль (т.е. 0 - процесс вылетел(недоступен)).

      При этом, имеется возможность для некоторых процессов (обычно для особо важных, без которых работа невозможна) указать, 
      время ожидания "перезапуска процесса"(heartbeat_reboot_msec) и в случае если для SM настроена работа с WDT-таймером
      и за заданное время процесс не перезапустился (не обновил свой счётчик), происходит
      перезагрузка контроллера (SM перестаёт сбрасывать WDT-таймер).

      У аналоговых датчиков "сердцебиения" в конфигурайионном файле необходимо указывать \b heartbeat="1".
      А также поля:
      <br>\b heartbeat_ds_name - имя дискретного датчика, связанного с данным аналоговым
      <br>\b heartbeat_node="ses" - фильтрующее поле (см. --heartbeat-node)
      <br>\b heartbeat_reboot_msec - время ожидания перезапуска процесса. Это необязательный параметр, задаётся только в случае
                                     необходимости перезапуска контроллера.

      Пример задания датчиков "сердцебияния":
      <br>_31_04_AS - аналоговый (счётчик)
      <br>_41_04_S - дискретный ("доступность процесса")

\code
    <item default="10" heartbeat="1" heartbeat_ds_name="_41_04_S" heartbeat_node="ses" heartbeat_reboot_msec="10000"
          id="103104" iotype="AI" name="_31_04_AS" textname="SES: IO heartbeat"/>

    <item default="1" id="104104" iotype="DI" name="_41_04_S">
            <MessagesList>
                <msg mtype="1" text="КСЭС: отключился ввод\вывод" value="0"/>
            </MessagesList>
    </item>
\endcode


    \section sec_SM_History Механизм аварийного дампа
    "Аварийный дамп" представляет из себя набор циклических буферов
    (размер в количестве точек хранения задаётся через конф. файл),
    в которых сохраняется история изменения заданного набора датчиков.
    В качестве "детонатора" задаётся идентификатор датчика (если
    это аналоговый датчик, то задаётся значение) при котором накопленный аварийный
    дамп должен "сбрасываться". За сохранение накопленного дампа (при "сбросе")
    отвечает разработчик, который может обрабатывать это событие подключившись
    к сигналу SharedMemory::signal_history(). Количество циклических буферов
    не ограничено, размер, а также список датчиков по которым ведётся "история"
    также не ограничены.
    Настройка параметров дампа осуществляется через конф. файл. Для этого
    у настроечной секции объекта SharedMemory должна быть создана подсекция
    "<History>".
    Пример:
    В данном примере задаётся две "истории".
       \code
    <SharedMemory name="SharedMemory" shmID="SharedMemory">
        <History savetime="200">
            <item id="1" fuse_id="AlarmFuse1_S" fuse_invert="1" size="30" filter="a1"/>
            <item id="2" fuse_id="AlarmFuse2_AS" fuse_value="2" size="30" filter="a2"/>
        </History>
    </SharedMemory>    
       \endcode
       где:
       \code
       savetime     - задаёт дискретность сохранения точек истории, в мсек.
       id           - задаёт (внутренний) идентификатор "истории"
       fuse_id      - идентификатор датчика "детонатора"
       fuse_value   - значение срабатывания (для аналогового "детонатора")
       fuse_invert  - ивертировать (для дискретных "детонаторов"). 
                      Т.е. срабатвание на значение "0".

       size         - количество точек в хранимой истории
       filter       - поле используемое в качестве фильтра, определяющего датчики
                      входящие в данную группу (историю). 
    \endcode

       Каждый датчик может входить в любое количество групп (историй).

       Механизм фукнционирует по следующей логике:

       При запуске происходит считывание параметров секции <History>
       и заполнение соответствующих структур хранения. При этом происходит
       проход по секции <sensors> и если встречается "не пустое" поле заданное 
       в качестве фильтра (\b filter), датчик включается в соответствующую историю.

       Далее каждые \b savetime мсек происходит запись очередной точки истории.
       При этом в конец буфера добавляется новое (текущее) значение датчика,
       а одно устаревшее удаляется, тем самым всегда поддерживается буфер не более
       \b size точек.

       Помимо этого в фукнциях изменения датчиков  (saveXXX, setXXX) отслеживается
       изменение состояния "детонаторов". Если срабатывает заданое условие для
       "сброса" дампа, инициируется сигнал, в который передаётся идентификатор истории
       и текущая накопленная история.

       \section sec_SM_Pulsar "Мигание" специальным датчиком
       В SM реализован механизм позволяющий задать специальный дискретный датчик ("пульсар"),
       который будет с заданным периодом менять своё состояние. Идентификатор датчика
       задаётся в настроечной секции параметром \b pulsar_id или из командной строки,
       параметром \b --pulsar-id. Период мигания задаётся параметром \b pulsar_msec
       или в командной строке \b --pulsar-msec. В качестве дискретного датчика можно
       задать любой датчик типа DO или DI. Параметр определяющий тип заданного датчика
       \b pulsar_iotype или в командной строке \b --pulsar-iotype.

       \section sec_SM_DBLog Управление сохранением изменений датчиков в БД
       Для оптимизации, по умолчанию в SM отключено сохранение каждого изменения датчиков в БД
       (реализованное в базовом классе IONotifyController).
       Параметр командной строки \b --db-logging 1 позволяет включить этот механизм
       (в свою очередь работа с БД требует отдельной настройки).
*/
class SharedMemory:
    public IONotifyController_LT
{
    public:
        SharedMemory( UniSetTypes::ObjectId id, const std::string& datafile, const std::string& confname="" );
        virtual ~SharedMemory();

        /*! глобальная функция для инициализации объекта */
        static SharedMemory* init_smemory( int argc, const char* const* argv );
        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char* const* argv );

        // функция определяет "готовность" SM к работе.
        // должна использоваться другими процессами, для того, 
        // чтобы понять, когда можно получать от SM данные.
        virtual CORBA::Boolean exist();

        void addReadItem( Restorer_XML::ReaderSlot sl );


        // ------------  HISTORY  --------------------
        typedef std::deque<long> HBuffer;

        struct HistoryItem
        {
            HistoryItem():id(UniSetTypes::DefaultObjectId){}

            inline void init( unsigned int size, long val )
            {
                if( size > 0 )
                   buf.assign(size,val);
            }

            UniSetTypes::ObjectId id;
            HBuffer buf;

            IOStateList::iterator ioit;

            void add( long val, size_t size )
            {
                // т.е. буфер у нас уже заданного размера
                // то просто удаляем очередную точку в начале
                // и добавляем в конце
                buf.pop_front();
                buf.push_back(val);
            }
        };

        typedef std::list<HistoryItem> HistoryList;

        struct HistoryInfo
        {
            HistoryInfo():
                id(0),
                size(0),filter(""),
                fuse_id(UniSetTypes::DefaultObjectId),
                fuse_invert(false),fuse_use_val(false),fuse_val(0),
                fuse_sec(0),fuse_usec(0)
                {
                      struct timeval tv;
                      struct timezone tz;
                      gettimeofday(&tv,&tz);
                      fuse_sec = tv.tv_sec;
                      fuse_usec = tv.tv_usec;
                }

            long id;                        // ID
            HistoryList hlst;               // history list
            int size;
            std::string filter;             // filter field
            UniSetTypes::ObjectId fuse_id;  // fuse sesnsor
            bool fuse_invert;
            bool fuse_use_val;
            long fuse_val;
            // timestamp
            long fuse_sec;
            long fuse_usec;
        };

        friend std::ostream& operator<<( std::ostream& os, const HistoryInfo& h );

        typedef std::list<HistoryInfo> History;

        // т.к. могуть быть одинаковые "детонаторы" для разных "историй" то, 
        // вводим не просто map, а "map списка историй". 
        // точнее итераторов-историй.
        typedef std::list<History::iterator> HistoryItList;
        typedef std::map<UniSetTypes::ObjectId,HistoryItList> HistoryFuseMap;

        typedef sigc::signal<void,HistoryInfo*> HistorySlot;
        HistorySlot signal_history(); /*!< сигнал о срабатывании условий "сборса" дампа истории */

        inline int getHistoryStep(){ return histSaveTime; } /*!< период между точками "дампа", мсек */

    protected:
        typedef std::list<Restorer_XML::ReaderSlot> ReadSlotList;
        ReadSlotList lstRSlot;

        virtual void sysCommand( const UniSetTypes::SystemMessage *sm ) override;
        virtual void timerInfo( const UniSetTypes::TimerMessage *tm ) override;
        virtual void askSensors( UniversalIO::UIOCommand cmd );
        void sendEvent( UniSetTypes::SystemMessage& sm );

        // действия при завершении работы
        virtual void sigterm( int signo ) override;
        virtual bool activateObject() override;
//        virtual void logging(UniSetTypes::SensorMessage& sm){}
//        virtual void dumpToDB(){}
        bool readItem( const UniXML& xml, UniXML_iterator& it, xmlNode* sec );

        void buildEventList( xmlNode* cnode );
        void readEventList( const std::string& oname );

        UniSetTypes::uniset_rwmutex mutex_start;

        struct HeartBeatInfo
        {
            HeartBeatInfo():
                a_sid(UniSetTypes::DefaultObjectId),
                d_sid(UniSetTypes::DefaultObjectId),
                reboot_msec(UniSetTimer::WaitUpTime),
                timer_running(false),
                ptReboot(UniSetTimer::WaitUpTime)
            {}

            UniSetTypes::ObjectId a_sid; // аналоговый счётчик
            UniSetTypes::ObjectId d_sid; // дискретный датчик состояния процесса
            IOStateList::iterator ioit;

            int reboot_msec; /*!< Время в течение которого, процесс обязан подтвердить своё существование,
                                иначе будет произведена перезагрузка контроллера по WDT (в случае если он включён).
                                Если данный параметр не указывать, то "не живость" процесса просто игнорируется
                                (т.е. только отображение на GUI). */

            bool timer_running;
            PassiveTimer ptReboot;
        };

        enum Timers
        {
            tmHeartBeatCheck,
            tmEvent,
            tmHistory,
            tmPulsar
        };

        int heartbeatCheckTime;
        std::string heartbeat_node;
        int histSaveTime;

        void checkHeartBeat();

        typedef std::list<HeartBeatInfo> HeartBeatList;
        HeartBeatList hlist; // список датчиков "сердцебиения"
        WDTInterface* wdt;
        UniSetTypes::mutex_atomic_t activated;
        UniSetTypes::mutex_atomic_t workready;

        typedef std::list<UniSetTypes::ObjectId> EventList;
        EventList elst;
        std::string e_filter;
        int evntPause;
        int activateTimeout;

        virtual void loggingInfo( UniSetTypes::SensorMessage& sm ) override;
        virtual void dumpOrdersList( const UniSetTypes::ObjectId sid, const IONotifyController::ConsumerListInfo& lst ) override {};
        virtual void dumpThresholdList( const UniSetTypes::ObjectId sid, const IONotifyController::ThresholdExtList& lst ) override {}

        bool dblogging;

        History hist;
        HistoryFuseMap histmap;  /*!< map для оптимизации поиска */

        virtual void updateHistory( IOStateList::iterator& it, IOController* );
        virtual void saveHistory();

        void buildHistoryList( xmlNode* cnode );
        void checkHistoryFilter( UniXML_iterator& it );
        bool isActivated();

        IOStateList::iterator itPulsar;
        UniSetTypes::ObjectId sidPulsar;
        int msecPulsar;

    private:
        HistorySlot m_historySignal;
};
// -----------------------------------------------------------------------------
#endif // SharedMemory_H_
// -----------------------------------------------------------------------------
