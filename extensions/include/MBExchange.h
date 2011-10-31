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
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
/*!
	\par Базовый класс для реализация обмена по протоколу Modbus [RTU|TCP].
*/
class MBExchange:
	public UniSetObject_LT
{
	public:
		MBExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
						const std::string prefix="mb" );
		virtual ~MBExchange();
	
		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		static const int NoSafetyState=-1;

		/*! Режимы работы процесса обмена */
		enum ExchangeMode
		{
			emNone, 		/*!< нормальная работа (по умолчанию) */
			emWriteOnly, 	/*!< "только посылка данных" (работают только write-функции) */
			emReadOnly,		/*!< "только чтение" (работают только read-функции) */
			emSkipSaveToSM	/*!< не писать данные в SM (при этом работают и read и write функции */
		};

		friend std::ostream& operator<<( std::ostream& os, const ExchangeMode& em );
		
		enum DeviceType
		{
			dtUnknown,		/*!< неизвестный */
			dtRTU,			/*!< RTU (default) */
			dtMTR,			/*!< MTR (DEIF) */
			dtRTU188		/*!< RTU188 (Fastwell) */
		};

		static DeviceType getDeviceType( const std::string dtype );
		friend std::ostream& operator<<( std::ostream& os, const DeviceType& dt );
	
	protected:
		
		virtual void step();
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm );
		virtual void sigterm( int signo );
		virtual bool initItem( UniXML_iterator& it ){ return false; }
		virtual void initIterators();
		
		bool checkUpdateSM( bool wrFunc );
		bool checkPoll( bool wrFunc );
		
		bool checkProcActive();
		void setProcActive( bool st );
		void waitSMReady();

		void readConfiguration();
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;
		
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

		UniSetTypes::ObjectId sidExchangeMode; /*!< иденидентификатор для датчика режима работы */
		IOController::AIOStateList::iterator aitExchangeMode;
		long exchangeMode; /*!< режим работы см. ExchangeMode */

		UniSetTypes::uniset_mutex actMutex;
		bool activated;
		int activateTimeout;

		bool noQueryOptimization;
		
		std::string prefix;
		
		timeout_t stat_time; 		/*!< время сбора статистики обмена */
		int poll_count;
		PassiveTimer ptStatistic;   /*!< таймер для сбора статистики обмена */

	 private:
		MBExchange();

};
// -----------------------------------------------------------------------------
#endif // _MBExchange_H_
// -----------------------------------------------------------------------------
