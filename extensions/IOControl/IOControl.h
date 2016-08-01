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
#ifndef IOControl_H_
#define IOControl_H_
// -----------------------------------------------------------------------------
#include <vector>
#include <memory>
#include <deque>
#include <string>
#include "UniXML.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "IONotifyController.h"
#include "UniSetObject.h"
#include "Mutex.h"
#include "MessageType.h"
#include "ComediInterface.h"
#include "DigitalFilter.h"
#include "Calibration.h"
#include "SMInterface.h"
#include "SingleProcess.h"
#include "IOController.h"
#include "IOBase.h"
#include "SharedMemory.h"
#include "LogServer.h"
#include "DebugStream.h"
#include "LogAgregator.h"
// -----------------------------------------------------------------------------
/*!
      \page page_IOControl (IOControl) Реализация процесса ввода/вывода

      - \ref sec_IOC_Comm
      - \ref sec_IOC_Conf
      - \ref sec_IOC_ConfList

      \section sec_IOC_Comm Общее описание процесса в/в

      \section sec_IOC_Conf Конфигурирование процесса в/в
    в качестве префикса по умолчанию используется "io", но в конструкторе можно задать другой
    если используется несколько процессов ввода/вывода.

    <br>\b --io-confnode name      - Использовать для настройки указанный xml-узел
    <br>\b --io-name name        - ID процесса. По умолчанию IOController1.
    <br>\b --io-numcards        - Количество карт в/в. По умолчанию 1.
    <br>\b --iodev0 dev        - Использовать для card='0' указанный файл comedi-устройства.
    <br>\b --iodev1 dev        - Использовать для card='1' указанный файл comedi-устройства.
    <br>\b --iodev2 dev        - Использовать для card='2' указанный файл comedi-устройства.
    <br>\b --iodev3 dev        - Использовать для card='3' указанный файл comedi-устройства.
    <br>\b --iodevX dev        - Использовать для card='X' указанный файл comedi-устройства.
                              'X'  не должен быть больше --io-numcards

    <br>\b --iodevX-subdevX-type name    - Настройка типа подустройства для UNIO.
                                 Разрешены: TBI0_24,TBI24_0,TBI16_8

    <br>\b --io-default_cardnum    - Номер карты по умолчанию. По умолчанию -1.
                                 Если задать, то он будет использоватся для датчиков
                                 у которых не задано поле 'card'.

    <br>\b --io-test-lamp        - Для данного узла в качестве датчика кнопки 'ТестЛамп' использовать указанный датчик.
    <br>\b --io-conf-field fname    - Считывать из конф. файла все датчики с полем fname='1'
    <br>\b --io-polltime msec    - Пауза между опросом карт. По умолчанию 200 мсек.
    <br>\b --io-filtersize val    - Размерность фильтра для аналоговых входов.
    <br>\b --io-filterT val    - Постоянная времени фильтра.
    <br>\b --io-s-filter-field    - Идентификатор в configure.xml по которому считывается список относящихся к это процессу датчиков
    <br>\b --io-s-filter-value    - Значение идентификатора по которому считывается список относящихся к это процессу датчиков
    <br>\b --io-blink-time msec    - Частота мигания, мсек. По умолчанию в configure.xml
    <br>\b --io-blink2-time msec    - Вторая частота мигания (lmpBLINK2), мсек. По умолчанию в configure.xml
    <br>\b --io-blink3-time msec    - Вторая частота мигания (lmpBLINK3), мсек. По умолчанию в configure.xml
    <br>\b --io-heartbeat-id    - Данный процесс связан с указанным аналоговым heartbeat-дачиком.
    <br>\b --io-heartbeat-max      - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10.
    <br>\b --io-ready-timeout    - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')
    <br>\b --io-force            - Сохранять значения в SM, независимо от, того менялось ли значение
    <br>\b --io-force-out        - Обновлять выходы принудительно (не по заказу)

    <br>\b --io-skip-init-output    - Не инициализировать 'выходы' при старте
    <br>\b --io-sm-ready-test-sid - Использовать указанный датчик, для проверки готовности SharedMemory

    \par Возможные настройки по каждому входу/выходу

    <br>\b nofilter        - не использовать фильтр
    <br>\b ioignore        - игнорировать данный датчик (позволяет временно отключить вход/выход)
    <br>\b ioinvert        - инвертированная логика (для DI,DO)
    <br>\b default         - значение по умолчанию (при запуске)
    <br>\b noprecision     - игнорировать поле precision (т.е. процесс в/в не будет его использовать,
                      но будет его присылать в SensorMessage)
    <br>\b cal_nocrop     - не обрезать значение по крайним точкам по при калибровке.

    <br>\b breaklim        - пороговое значение для определения обырва датчика (используется для AI).
                      Если значение ниже этого порога, то выставляется признак обрыва датчика.
    <br>\b debouncedelay   - защита от дребезга. Задержка на дребезг, мсек.
    <br>\b ondelay         - задержка на срабатывание, мсек.
    <br>\b offdelay        - задержка на отпускание, мсек.
    <br>\b iofront         - работа по фронту сигнала (для DI).
    <br>                        "01" - срабатывание (и отпускание) по переходу "0 --> 1"
    <br>                        "10" - срабатывание (и отпускание) по переходу "1 --> 0"

    <br>\b safety          - безопасное значение. Значение которое сохраняется в случае аварийного
                      завершения процесса.

    <br>\b iopriority      - приоритет при опросе.
    <br>\b iotype          - тип  входа/выхода [DI|DO|AI|AO]

    <br>\b rmin            - минимальное "сырое" значение
    <br>\b rmax            - максимальное "сырое" значение
    <br>\b cmin            - минимальное "калиброванное" значение
    <br>\b cmax            - максимальное "калиброванное" значение
    <br>\b precision       - Точность. Задаёт количство знаков после запятой.
                      <br>Т.е. при считывании из канала, значение домножается
              <br>на 10^precision и уже таким сохраняется.
              <br>А в SensorMessage присылается присылается precision.

    <br>\b filtermedian    - Значение для "медианного" фильтра
    <br>\b filtersize      - Значение для "усреднения"
    <br>\b filterT         - Постоянная времени фильтра.

    <br>\b caldiagram      - Имя калибровочной диаграммы из секции <Calibrations>.
    <br>\b cal_cachesize   - Размер кэша в калибровочной диаграмме (Calibration.h)
    <br>\b cal_cacheresort - Количество циклов обращения к кэшу, для вызова принудительной пересортировки. (Calibration.h)

    <br>\b threshold_aid   - идентификатор аналогового датчика по которому формируется порог.
                      Используется для DI.
    <br>\b lowlimit        - нижний порого срабатывания.
    <br>\b hilimit         - верхний порого срабатывания.

    <br>\b card            - номер карты
    <br>\b subdev          - номер подустройства
    <br>\b channel [ 0<>32 ] - номер канала
        <br>\b jack [ J1 | J2 | J3 | J4 | J5 ]  - название разъёма. Можно задавать вместо channel
           <br>&nbsp;&nbsp;   J1 - chanenel 0 - 15
           <br>&nbsp;&nbsp;   J2
    <br>\b lamp            - признак, что данный аналоговый датчик является "лампочкой".
              <br>        Т.е. на самом деле дискретный выход, который может иметь состояния:
              <br>UniSetTypes::lmpOFF      - выключен
              <br>UniSetTypes::lmpON       - включен
              <br>UniSetTypes::lmpBLINK    - мигание с частотой 1
              <br>UniSetTypes::lmpBLINK2   - мигание с частотой 2
              <br>UniSetTypes::lmpBLINK3   - мигание с частотой 3

    <br>\b no_iotestlamp  - игнорировать данную лампочку при тесте ламп.
    <br>\b range          - диапазон измерения аналогового входа (см. libcomedi)
    <br>\b aref           - тип подключения (см. libcomedi)

    <br>\b enable_testmode  - включить в работу во время тестового режима tmConfigEnable
    <br>\b disable_testmode  - исключить из работы в тестовом режиме tmConfigDisable.

    \note Помимо этого для конкретного процесса можно переопределять настройки используя "prefix_" (префикс плюс подчёркивание).
     Где "prefix" - это префикс это префикс заданный в конструкторе IOControl.

    \section sec_IOC_ConfList Список датчиков для процесса в/в

    \section sec_IOC_TestMode Тестовый режим
        В IOControl встроена возможнось переводить его в один из тестовых режимов.
    Для этого необходимо указать для IOControl аналоговый датчик в который будет записан "код"
    режима работы. Датчик можно задать либо аргументом командной строки
    --io-test-mode ID либо в конфигурационном файле testmode_as="ID"
    Сейчас поддерживаются следующий режимы (см. IOControl::TestModeID):

    <br>\b "0" - тестовый режим отключён. Обычная работа.
    <br>\b "1" - полностью отключить работу с картами в/в. При этом все выходы будут переведены в безопасное состояние.
    <br>\b "2" - Режим "разрешённых" каналов. В этом режиме отключается работа со всеми каналами, кроме тех, у которых
          указан параметр enable_testmode="1".
    <br>\b "3" - Режим "запрещённых" каналов. В этом режиме отключается работа ТОЛЬКО для каналов, у которых
        указан параметр disable_testmode="1".
    <br>\b "4" - Режим "только входы"
    <br>\b "5" - Режим "только выходы"
*/
// -----------------------------------------------------------------------------
/*! \todo (IOControl): Сделать обработку сигналов завершения */

class CardList:
	public std::vector<ComediInterface*>
{
	public:

		explicit CardList( size_t size ) : std::vector<ComediInterface * >(size) { }

		~CardList()
		{
			for( size_t i = 0; i < size(); i++ )
				delete (*this)[i];
		}

		inline ComediInterface* getCard(int ncard) const
		{
			if( ncard >= 0 && ncard < (int)size() )
				return (*this)[ncard];

			return NULL;
		}

};

/*!
    Процесс работы с картами в/в.
    Задачи:
    - опрос дискретных и аналоговых входов, выходов
    - задержка на срабатывание
    - задержка на отпускание
    - защита от дребезга
    - программное фильтрование аналоговых сигналов
    - калибровка аналоговых значений
    - инвертирование логики дискретных сигналов
    - выставление безопасного состояния выходов (при аварийном завершении)
    - определение обрыва провода (для аналоговых сигналов)
    - мигание лампочками
    - тест ламп
*/
class IOControl:
	public UniSetObject
{
	public:
		IOControl( UniSetTypes::ObjectId id, UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory>& shm = nullptr, int numcards = 2, const std::string& prefix = "io" );
		virtual ~IOControl();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<IOControl> init_iocontrol( int argc, const char* const* argv,
				UniSetTypes::ObjectId icID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "io" );
		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		/*! Информация о входе/выходе */
		struct IOInfo:
			public IOBase
		{
			// т.к. IOBase содержит rwmutex с запрещённым конструктором копирования
			// приходится здесь тоже объявлять разрешенными только операции "перемещения"
			IOInfo( const IOInfo& r ) = delete;
			IOInfo& operator=(const IOInfo& r) = delete;
			IOInfo( IOInfo&& r ) = default;
			IOInfo& operator=(IOInfo&& r) = default;

			IOInfo():
				subdev(DefaultSubdev), channel(DefaultChannel),
				ncard(-1),
				aref(0),
				range(0),
				lamp(false),
				no_testlamp(false),
				enable_testmode(false),
				disable_testmode(false)
			{}


			int subdev;     /*!< (UNIO) подустройство (см. comedi_test для конкретной карты в/в) */
			int channel;    /*!< (UNIO) канал [0...23] */
			int ncard;      /*!< номер карты [1|2]. -1 - не определена. */

			/*! Вид поключения
			    0    - analog ref = analog ground
			    1    - analog ref = analog common
			    2    - analog ref = differential
			    3    - analog ref = other (undefined)
			*/
			int aref;

			/*! Измерительный диапазон
			    0    -  -10В - 10В
			    1    -  -5В - 5В
			    2    -  -2.5В - 2.5В
			    3    -  -1.25В - 1.25В
			*/
			int range;

			bool lamp;             /*!< признак, что данный выход является лампочкой (или сигнализатором) */
			bool no_testlamp;      /*!< флаг исключения из 'проверки ламп' */
			bool enable_testmode;  /*!< флаг для режима тестирования tmConfigEnable */
			bool disable_testmode; /*!< флаг для режима тестирования tmConfigDisable */

			friend std::ostream& operator<<(std::ostream& os, IOInfo& inf );
		};

		struct IOPriority
		{
			IOPriority(size_t  p, size_t i):
				priority(p), index(i) {}

			size_t  priority;
			size_t index;
		};

		enum TestModeID
		{
			tmNone       = 0,       /*!< тестовый режим отключён */
			tmOffPoll    = 1,        /*!< отключить опрос */
			tmConfigEnable  = 2,   /*!< специальный режим, в соответствии с настройкой 'enable_testmode' */
			tmConfigDisable = 3,  /*!< специальный режим, в соответствии с настройкой 'disable_testmode' */
			tmOnlyInputs    = 4,     /*!< включены только входы */
			tmOnlyOutputs   = 5     /*!< включены только выходы */
		};

		void execute();

	protected:

		void iopoll(); /*!< опрос карт в/в */
		void ioread( IOInfo* it );
		void check_testlamp();
		void check_testmode();
		void blink();

		// действия при завершении работы
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sigterm( int signo ) override;
		virtual bool activateObject() override;

		// начальная инициализация выходов
		void initOutputs();

		// инициализация карты (каналов в/в)
		void initIOCard();

		// чтение файла конфигурации
		void readConfiguration();
		bool initIOItem( UniXML::iterator& it );
		bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );
		void buildCardsList();

		void waitSM();

		xmlNode* cnode = { 0 }; /*!< xml-узел в настроечном файле */

		int polltime = { 150 };   /*!< переодичность обновления данных (опроса карт в/в), [мсек] */
		CardList cards; /*!< список карт - массив созданных ComediInterface */
		bool noCards = { false };

		typedef std::vector<IOInfo> IOMap;
		IOMap iomap;    /*!< список входов/выходов */

		typedef std::deque<IOPriority> PIOMap;
		PIOMap pmap;    /*!< список приоритетных входов/выходов */

		unsigned int maxItem = { 0 };    /*!< количество элементов (используется на момент инициализации) */
		unsigned int maxHalf = { 0 };
		int filtersize = { 0 };
		float filterT = { 0.0 };

		std::string s_field;
		std::string s_fvalue;

		std::shared_ptr<SMInterface> shm;
		UniSetTypes::ObjectId myid = { UniSetTypes::DefaultObjectId };
		std::string prefix;

		typedef std::list<IOInfo*> BlinkList;

		void addBlink( IOInfo* it, BlinkList& lst );
		void delBlink( IOInfo* it, BlinkList& lst );
		void blink( BlinkList& lst, bool& bstate );

		// обычное мигание
		BlinkList lstBlink;
		PassiveTimer ptBlink;
		bool blink_state = { false };

		// мигание с двойной частотой
		BlinkList lstBlink2;
		PassiveTimer ptBlink2;
		bool blink2_state = { false };

		// мигание с тройной частотой
		BlinkList lstBlink3;
		PassiveTimer ptBlink3;
		bool blink3_state = { false };

		UniSetTypes::ObjectId testLamp_s = { UniSetTypes::DefaultObjectId };
		Trigger trTestLamp;
		bool isTestLamp = { false };
		IOController::IOStateList::iterator itTestLamp;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat = { 10 };
		IOController::IOStateList::iterator itHeartBeat;

		bool force = { false };            /*!< флаг, означающий, что надо сохранять в SM, даже если значение не менялось */
		bool force_out = { false };        /*!< флаг, включающий принудительное чтения выходов */
		timeout_t smReadyTimeout = { 15000 };    /*!< время ожидания готовности SM к работе, мсек */
		int defCardNum = { -1 };        /*!< номер карты по умолчанию */
		int maxCardNum = { 10 };        /*! максимально разрешённый номер для карты */

		std::mutex iopollMutex;
		std::atomic_bool activated = { false };
		bool readconf_ok = { false };
		int activateTimeout;
		UniSetTypes::ObjectId sidTestSMReady = { UniSetTypes::DefaultObjectId };
		std::atomic_bool term = { false };

		UniSetTypes::ObjectId testMode_as = { UniSetTypes::DefaultObjectId };
		IOController::IOStateList::iterator itTestMode;
		long testmode = { false };
		long prev_testmode = { false };

		std::shared_ptr<LogAgregator> loga;
		std::shared_ptr<DebugStream> iolog;
		std::shared_ptr<LogServer> logserv;
		std::string logserv_host = {""};
		int logserv_port = {0};

	private:
};
// -----------------------------------------------------------------------------
#endif // IOControl_H_
// -----------------------------------------------------------------------------
