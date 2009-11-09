// $Id: IOControl.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// -----------------------------------------------------------------------------
#ifndef IOControl_H_
#define IOControl_H_
// -----------------------------------------------------------------------------
#include <vector>
#include <list>
#include <string>
#include "UniXML.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "IONotifyController.h"
#include "UniSetObject_LT.h"
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
// -----------------------------------------------------------------------------
/*!
      \page page_IOControl (IOControl) Реализация процесса ввода/вывода
      
      - \ref sec_IOC_Comm
      - \ref sec_IOC_Conf
      - \ref sec_IOC_ConfList

      \section sec_IOC_Comm Общее описание процесса в/в

      \section sec_IOC_Conf Конфигурирование процесса в/в
      
      \section sec_IOC_ConfList Список датчиков для процесса в/в

*/
// -----------------------------------------------------------------------------
#warning Сделать обработку сигналов завершения....

class CardList:
	public std::vector<ComediInterface*>
{
	public:

		CardList(int size) : std::vector<ComediInterface*>(size) { }

		~CardList() {
			for( unsigned int i=0; i<size(); i++ )
				delete (*this)[i];
		}

		inline ComediInterface* getCard(int ncard) {
			if( ncard >= 0 && ncard < (int)size() )
				return (*this)[ncard];
			return NULL;
	}

};

/*! 
	Процесс работы с картами в/в.
	Задачи:
	- опрос дискретных и аналоговых входов, выходов
	- задержка на страбатывание
	- задержка на отпскание (для АПС-ых сигналов)
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
		IOControl( UniSetTypes::ObjectId id, UniSetTypes::ObjectId icID, SharedMemory* ic=0, int numcards=2 );
		virtual ~IOControl();

		/*! глобальная функция для инициализации объекта */
		static IOControl* init_iocontrol( int argc, const char* const* argv,
											UniSetTypes::ObjectId icID, SharedMemory* ic=0 );
		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

//		inline std::string getName(){ return myname; }

		/*! Информация о входе/выходе */
		struct IOInfo:
			public IOBase
		{
			IOInfo():
				subdev(DefaultSubdev),channel(DefaultChannel),
				ncard(-1),
				aref(0),
				range(0),
				lamp(false),
				no_testlamp(false)
			{}


			int subdev;		/*!< (UNIO) подустройство (см. comedi_test для конкретной карты в/в) */
			int channel;	/*!< (UNIO) канал [0...23] */
			int ncard;		/*!< номер карты [1|2]. 0 - не определена. FIXME from Lav: -1 - не определена? */

			/*! Вид поключения
				0	- analog ref = analog ground
				1	- analog ref = analog common
				2	- analog ref = differential
				3	- analog ref = other (undefined)
			*/
			int aref;

			/*! Измерительный диапазон
				0	-  -10В - 10В
				1	-  -5В - 5В
				2	-  -2.5В - 2.5В
				3	-  -1.25В - 1.25В
			*/
			int range;

			bool lamp;		/*!< признак, что данный выход является лампочкой (или сигнализатором) */
			bool no_testlamp; /*!< флаг исключения из 'проверки ламп' */
			
			friend std::ostream& operator<<(std::ostream& os, IOInfo& inf );
		};

		struct IOPriority
		{
			IOPriority(int p, int i):
				priority(p),index(i){}
				
			int priority;
			int index;
		};

		void execute();

	protected:

		void iopoll(); /*!< опрос карт в/в */
		void ioread( IOInfo* it );
		void check_testlamp();
		void blink();
	
		// действия при завершении работы
		virtual void processingMessage( UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm );
		virtual void timerInfo( UniSetTypes::TimerMessage* tm );
		virtual void sigterm( int signo );
		virtual bool activateObject();
		
		// начальная инициализация выходов
		void initOutputs();

		// инициализация карты (каналов в/в)
		void initIOCard();

		// чтение файла конфигурации
		void readConfiguration();
		bool initIOItem( UniXML_iterator& it );
		bool check_item( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		void waitSM();

		bool checkCards( const std::string func="" );

//		std::string myname;
		xmlNode* cnode;			/*!< xml-узел в настроечном файле */

		int polltime;			/*!< переодичность обновления данных (опроса карт в/в), [мсек] */
		CardList cards;			/*!< список карт - массив созданных ComediInterface */
		bool noCards;


		typedef std::vector<IOInfo> IOMap;
		IOMap iomap;			/*!< список входов/выходов */
		
		typedef std::list<IOPriority> PIOMap;
		PIOMap pmap;	/*!< список приоритетных входов/выходов */

		unsigned int maxItem;	/*!< количество элементов (используется на момент инициализации) */
		unsigned int maxHalf;
		int filtersize;
		float filterT;

		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;
		UniversalInterface ui;
		UniSetTypes::ObjectId myid;

		typedef std::list<IOInfo*> BlinkList;

		void addBlink( IOInfo* it, BlinkList& lst );
		void delBlink( IOInfo* it, BlinkList& lst );
		void blink( BlinkList& lst, bool& bstate );

		// обычное мигание
		BlinkList lstBlink;
		PassiveTimer ptBlink;
		bool blink_state;
		
		// мигание с двойной частотой
		BlinkList lstBlink2;
		PassiveTimer ptBlink2;
		bool blink2_state;

		// мигание с тройной частотой
		BlinkList lstBlink3;
		PassiveTimer ptBlink3;
		bool blink3_state;
		
		UniSetTypes::ObjectId testLamp_S;
		Trigger trTestLamp;
		bool isTestLamp;
		IOController::DIOStateList::iterator ditTestLamp;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;

		bool force;			/*!< флаг, означающий, что надо сохранять в SM, даже если значение не менялось */
		bool force_out;		/*!< флаг, включающий принудительное чтения выходов */
		int smReadyTimeout; 	/*!< время ожидания готовности SM к работе, мсек */
		int defCardNum;		/*!< номер карты по умолчанию */
		
		UniSetTypes::uniset_mutex iopollMutex;
		bool activated;
		bool readconf_ok;
		int activateTimeout;
		UniSetTypes::ObjectId sidTestSMReady;
		bool term;

	private:
};
// -----------------------------------------------------------------------------
#endif // IOControl_H_
// -----------------------------------------------------------------------------
