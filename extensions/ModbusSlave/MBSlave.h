// -----------------------------------------------------------------------------
#ifndef _MBSlave_H_
#define _MBSlave_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "UniSetObject_LT.h"
#include "modbus/ModbusTypes.h"
#include "modbus/ModbusServerSlot.h"
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "IOBase.h"
#include "VTypes.h"
#include "ThreadCreator.h"
// -----------------------------------------------------------------------------
/*! Реализация slave-интерфейса */
class MBSlave:
	public UniSetObject_LT
{
	public:
		MBSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0, std::string prefix="mbs" );
		virtual ~MBSlave();
	
		/*! глобальная функция для инициализации объекта */
		static MBSlave* init_mbslave( int argc, const char* const* argv,
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
											std::string prefix="mbs" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		static const int NoSafetyState=-1;

		enum AccessMode
		{
			amRW,
			amRO,
			amWO
		};

		struct IOProperty:
			public IOBase
		{
			ModbusRTU::ModbusData mbreg;	/*!< регистр */
			AccessMode amode;
			VTypes::VType vtype;	/*!< type of value */
			int wnum;				/*!< номер слова (для типов с размеров больше 2х байт */

			IOProperty():
				mbreg(0),
				amode(amRW),
				vtype(VTypes::vtUnknown),
				wnum(0)
			{}

			friend std::ostream& operator<<( std::ostream& os, IOProperty& p );
		};

		inline long getAskCount(){ return askCount; }

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
//		ModbusRTU::mbErrCode journalCommand( ModbusRTU::JournalCommandMessage& query, 
//															ModbusRTU::JournalCommandRetMessage& reply );

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
		
		/*! Проверка корректности регистра перед сохранением.
			Вызывается для каждого регистра не зависимо от используемой функции (06 или 10)
		*/
		virtual ModbusRTU::mbErrCode checkRegister( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData& val )
		{ return ModbusRTU::erNoError; }

		typedef std::map<ModbusRTU::ModbusData,IOProperty> IOMap;
		IOMap iomap;			/*!< список входов/выходов */

		ModbusServerSlot* mbslot;
		ModbusRTU::ModbusAddr addr;			/*!< адрес данного узла */

		UniSetTypes::uniset_rwmutex mbMutex;

		xmlNode* cnode;
		std::string s_field;
		std::string s_fvalue;

		SMInterface* shm;

		virtual void processingMessage( UniSetTypes::VoidMessage *msg );
		void sysCommand( UniSetTypes::SystemMessage *msg );
		void sensorInfo( UniSetTypes::SensorMessage* sm );
		void askSensors( UniversalIO::UIOCommand cmd );	
		void waitSMReady();
		void execute_rtu();
		void execute_tcp();

		virtual bool activateObject();
		
		// действия при завершении работы
		virtual void sigterm( int signo );

		void initIterators();
		bool initItem( UniXML_iterator& it );
		bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );

		void readConfiguration();
		bool check_item( UniXML_iterator& it );

		ModbusRTU::mbErrCode real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData val );
		ModbusRTU::mbErrCode real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData& val );
		ModbusRTU::mbErrCode much_real_read( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, int count );
		ModbusRTU::mbErrCode much_real_write( ModbusRTU::ModbusData reg, ModbusRTU::ModbusData* dat, int count );
		
		ModbusRTU::mbErrCode real_read_it( IOMap::iterator& it, ModbusRTU::ModbusData& val );
		ModbusRTU::mbErrCode real_write_it( IOMap::iterator& it, ModbusRTU::ModbusData& val );
	private:
		MBSlave();
		bool initPause;
		UniSetTypes::uniset_rwmutex mutex_start;
		ThreadCreator<MBSlave>* thr;

		PassiveTimer ptHeartBeat;
		UniSetTypes::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::AIOStateList::iterator aitHeartBeat;
		UniSetTypes::ObjectId test_id;

		IOController::AIOStateList::iterator aitAskCount;
		UniSetTypes::ObjectId askcount_id;

		IOController::DIOStateList::iterator ditRespond;
		UniSetTypes::ObjectId respond_id;
		bool respond_invert;

		PassiveTimer ptTimeout;
		ModbusRTU::mbErrCode prev;
		long askCount;
		typedef std::map<ModbusRTU::mbErrCode,unsigned int> ExchangeErrorMap;
		ExchangeErrorMap errmap; 	/*!< статистика обмена */
		
		bool activated;
		int activateTimeout;
		bool pingOK;
		timeout_t wait_msec;
		bool force;		/*!< флаг означающий, что надо сохранять в SM, даже если значение не менялось */

		bool mbregFromID;

		typedef std::map<int,std::string> FileList;
		FileList flist;
		std::string prefix;
		
		ModbusRTU::ModbusData buf[ModbusRTU::MAXLENPACKET/2+1];  /*!< буфер для формирования ответов */

		// данные для ответа на запрос 0x2B(43)/0x0E(14)
		// 'MEI' - modbus encapsulated interface
		// 'RDI' - read device identification
		typedef std::map<int,std::string> MEIValMap;
		typedef std::map<int,MEIValMap> MEIObjIDMap;
		typedef std::map<int,MEIObjIDMap> MEIDevIDMap;

		MEIDevIDMap meidev;
};
// -----------------------------------------------------------------------------
#endif // _MBSlave_H_
// -----------------------------------------------------------------------------
