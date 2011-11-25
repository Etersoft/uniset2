// -------------------------------------------------------------------------
#ifndef ModbusTypes_H_
#define ModbusTypes_H_
// -------------------------------------------------------------------------
#include <ostream>
#include <bitset>
#include "ModbusRTUErrors.h"
// -------------------------------------------------------------------------
/* Основные предположения:	
 * - младший и старший байт переворачиваются только в CRC
 * - В случае неправильного формата пакета(запроса), логической ошибки и т.п
 * 		ОТВЕТ просто не посылается, а пакет отбрасывается...
 * - CRC считается по всей посылке (с начальным адресом)
 * - CRC инициализируется значением 0xffff
 * - CRC не переворачивается
 * - Все двухбайтовые слова переворачиваются. Порядок байт: старший младший
*/
// -------------------------------------------------------------------------
namespace ModbusRTU
{
	// Базовые типы 
	typedef unsigned char ModbusByte;	/*!< modbus-байт */
	const int BitsPerByte = 8;
	typedef unsigned char ModbusAddr;	/*!< адрес узла в modbus-сети */
	typedef unsigned short ModbusData;	/*!< размер данных в modbus-сообщениях */
	const int BitsPerData = 16;
	typedef unsigned short ModbusCRC;	/*!< размер CRC16 в modbus-сообщениях */

	// ---------------------------------------------------------------------
	/*! Коды используемых функций (согласно описанию modbus) */
	enum SlaveFunctionCode
	{
		fnUnknown				= 0x00,
		fnReadCoilStatus		= 0x01, /*!< read coil status */
		fnReadInputStatus		= 0x02, /*!< read input status */
		fnReadOutputRegisters	= 0x03, /*!< read register outputs or memories or read word outputs or memories */
		fnReadInputRegisters	= 0x04, /*!< read input registers or memories or read word outputs or memories */
		fnForceSingleCoil		= 0x05, /*!< forces a single coil to either ON or OFF */
		fnWriteOutputSingleRegister = 0x06,	/*!< write register outputs or memories */
		fnDiagnostics			= 0x08, /*!< Diagnostics (Serial Line only) */
		fnForceMultipleCoils	= 0x0F,	/*!< force multiple coils */
		fnWriteOutputRegisters	= 0x10,	/*!< write register outputs or memories */
		fnReadFileRecord		= 0x14,	/*!< read file record */
		fnWriteFileRecord		= 0x15,	/*!< write file record */
		fnSetDateTime			= 0x50, /*!< set date and time */
		fnRemoteService			= 0x53,	/*!< call remote service */
		fnJournalCommand		= 0x65,	/*!< read,write,delete alarm journal */
		fnFileTransfer			= 0x66	/*!< file transfer */
	};

	/*! Коды диагностически подфункций (для запроса 0x08) */
	enum DiagnosticsSubFunction
	{
		subEcho = 0x00, 		/*!< (0) Return Query Data (echo) */
		dgRestartComm = 0x01, 	/*!< (1) Restart Communications Option */
		dgDiagReg = 0x02, 		/*!< (2) Return Diagnostic Register */
		dgChangeASCII = 0x03,	/*!< (3) Change ASCII Input Delimiter */
		dgForceListen = 0x04,	/*!< (4) Force Listen Only Mode */
		// 05.. 09 RESERVED
		dgClearCounters = 0x0A, 	/*!< (10)Clear Counters and Diagnostic Register */
		dgBusMsgCount = 0x0B,		/*!< (11) Return Bus Message Count */
		dgBusErrCount = 0x0C,		/*!< (12) Return Bus Communication Error Count */
		dgBusExceptCount = 0x0D,	/*!< (13) Return Bus Exception Error Count */
		dgMsgSlaveCount = 0x0E,		/*!< (14) Return Slave Message Count */
		dgNoNoResponseCount = 0x0F,	/*!< (15) Return Slave No Response Count */
		dgSlaveNAKCount = 0x10,		/*!< (16) Return Slave NAK Count */
		dgSlaveBusyCount = 0x11,		/*!< (17) Return Slave Busy Count */
		dgBusCharOverrunCount = 0x12,	/*!< (18) Return Bus Character Overrun Count */
		 // = 0x13,	/*!<  RESERVED */
		dgClearOverrunCounter = 0x14	/*!< (20) Clear Overrun Counter and FlagN.A. */
		 // 21 ...65535 RESERVED
	};

	// определение размера данных в зависимости от типа сообщения
	// возвращает -1 - если динамический размер сообщения или размер неизвестен
	int szRequestDiagnosticData( DiagnosticsSubFunction f );

	/*! различные базовые константы */
	enum
	{
		/*! максимальное количество данных в пакете (c учётом контрольной суммы) */
		MAXLENPACKET 	= 508,	/*!< максимальная длина пакета 512 - header(2) - CRC(2) */
		BroadcastAddr	= 255,	/*!< адрес для широковещательных сообщений */
		MAXDATALEN		= 127	/*!< максимальное число слов, которое можно запросить.
									Связано с тем, что в ответе есть поле bcnt - количество байт
									Соответственно максимум туда можно записать только 255
								*/
	};

	const unsigned char MBErrMask = 0x80;
	// ---------------------------------------------------------------------
	unsigned short SWAPSHORT(unsigned short x);
	// ---------------------------------------------------------------------
	/*! Расчёт контрольной суммы */
	ModbusCRC checkCRC( ModbusByte* start, int len );
	const int szCRC = sizeof(ModbusCRC); /*!< размер данных для контрольной суммы */
	// ---------------------------------------------------------------------
	/*! вывод сообщения */
	std::ostream& mbPrintMessage( std::ostream& os, ModbusByte* b, int len );
	// -------------------------------------------------------------------------
	ModbusAddr str2mbAddr( const std::string val );
	ModbusData str2mbData( const std::string val );
	std::string dat2str( const ModbusData dat );
	std::string addr2str( const ModbusAddr addr );
	std::string b2str( const ModbusByte b );
	// -------------------------------------------------------------------------
	float dat2f( const ModbusData dat1, const ModbusData dat2 );
	// -------------------------------------------------------------------------
	bool isWriteFunction( SlaveFunctionCode c );
	// -------------------------------------------------------------------------
	/*! Заголовок сообщений */
	struct ModbusHeader
	{
		ModbusAddr addr;		/*!< адрес в сети */
		ModbusByte func;		/*!< код функции */

		ModbusHeader():addr(0),func(0){}
	}__attribute__((packed));

	const int szModbusHeader = sizeof(ModbusHeader);
	std::ostream& operator<<(std::ostream& os, ModbusHeader& m );
	std::ostream& operator<<(std::ostream& os, ModbusHeader* m );
	// -----------------------------------------------------------------------

	/*! Базовое (сырое) сообщение 
		\todo Может переименовать ModbusMessage в TransportMessage?
	*/
	struct ModbusMessage:
		public ModbusHeader
	{
		ModbusMessage();
		ModbusByte data[MAXLENPACKET+szCRC]; 	/*!< данные */

		// Это поле вспомогательное и игнорируется при пересылке
		int len;	/*!< фактическая длина */
	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ModbusMessage& m );
	std::ostream& operator<<(std::ostream& os, ModbusMessage* m );
	// -----------------------------------------------------------------------
	/*! Ответ сообщающий об ошибке */	
	struct ErrorRetMessage:
		public ModbusHeader
	{
		ModbusByte ecode;
		ModbusCRC crc;
		
		// ------- from slave -------
		ErrorRetMessage( ModbusMessage& m );
		ErrorRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		
		// ------- to master -------
		ErrorRetMessage( ModbusAddr _from, ModbusByte _func, ModbusByte ecode );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();

		/*! размер данных(после заголовка) у данного типа сообщения 
			Для данного типа он постоянный..
		*/
		inline static int szData(){ return sizeof(ModbusByte)+szCRC; }
	};

	std::ostream& operator<<(std::ostream& os, ErrorRetMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ErrorRetMessage* m ); 
	// -----------------------------------------------------------------------
	struct DataBits
	{	
		DataBits( ModbusByte b );
		DataBits( std::string s ); // example "10001111"
		DataBits();
		
		const DataBits& operator=(const ModbusByte& r);

		operator ModbusByte();
		ModbusByte mbyte();
		
		bool operator[]( const int i ){ return b[i]; }
		
		std::bitset<BitsPerByte> b;
	};

	std::ostream& operator<<(std::ostream& os, DataBits& m );
	std::ostream& operator<<(std::ostream& os, DataBits* m ); 
	// -----------------------------------------------------------------------
	struct DataBits16
	{	
		DataBits16( ModbusData d );
		DataBits16( std::string s ); // example "1000111110001111"
		DataBits16();
		
		const DataBits16& operator=(const ModbusData& r);

		operator ModbusData();
		ModbusData mdata();
		
		bool operator[]( const int i ){ return b[i]; }
		void set( int n, bool s ){ b.set(n,s); }
		
		std::bitset<BitsPerData> b;
	};

	std::ostream& operator<<(std::ostream& os, DataBits16& m );
	std::ostream& operator<<(std::ostream& os, DataBits16* m ); 
	// -----------------------------------------------------------------------
	/*! Запрос 0x01 */	
	struct ReadCoilMessage:
		public ModbusHeader
	{
		ModbusData start;
		ModbusData count;
		ModbusCRC crc;
		
		// ------- to slave -------
		ReadCoilMessage( ModbusAddr addr, ModbusData start, ModbusData count );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		// ------- from master -------
		ReadCoilMessage( ModbusMessage& m );
		ReadCoilMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusData)*2 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ReadCoilMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ReadCoilMessage* m ); 

	// -----------------------------------------------------------------------
	
	/*! Ответ на 0x01 */	
	struct ReadCoilRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;				/*!< numbers of bytes */
		ModbusByte data[MAXLENPACKET];	/*!< данные */

		// ------- from slave -------
		ReadCoilRetMessage( ModbusMessage& m );
		ReadCoilRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			return sizeof(ModbusByte); // bcnt
		}

		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );
		ModbusCRC crc;
		
		// ------- to master -------
		ReadCoilRetMessage( ModbusAddr _from );

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( DataBits d );

		/*! установить бит.
		 * \param dnum  - номер байта (0..MAXLENPACKET)
		 * \param bnum  - номер бита (0..7)
		 * \param state - состояние
		 * \return TRUE - если есть
		 * \return FALSE - если НЕ найдено
		*/
		bool setBit( unsigned char dnum, unsigned char bnum, bool state );

		/*! получение данных.
		 * \param bnum  - номер байта(0..MAXLENPACKET)
		 * \param d     - найденные данные
		 * \return TRUE - если есть
		 * \return FALSE - если НЕ найдено
		*/
		bool getData( unsigned char bnum, DataBits& d );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull()
		{
			return ( bcnt >= MAXLENPACKET );
		}

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();
	};

	std::ostream& operator<<(std::ostream& os, ReadCoilRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ReadCoilRetMessage* m );
	// -----------------------------------------------------------------------
	/*! Запрос 0x02 */	
	struct ReadInputStatusMessage:
		public ModbusHeader
	{
		ModbusData start;
		ModbusData count;
		ModbusCRC crc;
		
		// ------- to slave -------
		ReadInputStatusMessage( ModbusAddr addr, ModbusData start, ModbusData count );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		// ------- from master -------
		ReadInputStatusMessage( ModbusMessage& m );
		ReadInputStatusMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusData)*2 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ReadInputStatusMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ReadInputStatusMessage* m ); 
	// -----------------------------------------------------------------------
	/*! Ответ на 0x02 */	
	struct ReadInputStatusRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;				/*!< numbers of bytes */
		ModbusByte data[MAXLENPACKET];	/*!< данные */

		// ------- from slave -------
		ReadInputStatusRetMessage( ModbusMessage& m );
		ReadInputStatusRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			return sizeof(ModbusByte); // bcnt
		}

		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );
		ModbusCRC crc;
		
		// ------- to master -------
		ReadInputStatusRetMessage( ModbusAddr _from );

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( DataBits d );

		/*! установить бит.
		 * \param dnum  - номер байта (0..MAXLENPACKET)
		 * \param bnum  - номер бита (0...7)
		 * \param state - состояние
		 * \return TRUE - если есть
		 * \return FALSE - если НЕ найдено
		*/
		bool setBit( unsigned char dnum, unsigned char bnum, bool state );

		/*! получение данных.
		 * \param dnum  - номер байта (0..MAXLENPACKET)
		 * \param d     - найденные данные
		 * \return TRUE - если есть
		 * \return FALSE - если НЕ найдено
		*/
		bool getData( unsigned char dnum, DataBits& d );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull() 		
		{
			return ( bcnt >= MAXLENPACKET );
		}

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
	};

	std::ostream& operator<<(std::ostream& os, ReadInputStatusRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ReadInputStatusRetMessage* m );
	// -----------------------------------------------------------------------

	/*! Запрос 0x03 */	
	struct ReadOutputMessage:
		public ModbusHeader
	{
		ModbusData start;
		ModbusData count;
		ModbusCRC crc;
		
		// ------- to slave -------
		ReadOutputMessage( ModbusAddr addr, ModbusData start, ModbusData count );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		// ------- from master -------
		ReadOutputMessage( ModbusMessage& m );
		ReadOutputMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusData)*2 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ReadOutputMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ReadOutputMessage* m ); 
	// -----------------------------------------------------------------------
	/*! Ответ для 0x03 */	
	struct ReadOutputRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;									/*!< numbers of bytes */
		ModbusData data[MAXLENPACKET/sizeof(ModbusData)];	/*!< данные */

		// ------- from slave -------
		ReadOutputRetMessage( ModbusMessage& m );
		ReadOutputRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			// bcnt
			return sizeof(ModbusByte);
		}

		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );
		ModbusCRC crc;
		
		// ------- to master -------
		ReadOutputRetMessage( ModbusAddr _from );

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( ModbusData d );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull() 		
		{
			return ( count*sizeof(ModbusData) >= MAXLENPACKET );
		}

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		// Это поле не входит в стандарт modbus
		// оно вспомогательное и игнорируется при 
		// преобразовании в ModbusMessage.
		// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно. 
		// Используйте специальную функцию transport_msg()
		int	count;	/*!< фактическое количество данных в сообщении */
	};

	std::ostream& operator<<(std::ostream& os, ReadOutputRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ReadOutputRetMessage* m );
	// -----------------------------------------------------------------------
	/*! Запрос 0x04 */	
	struct ReadInputMessage:
		public ModbusHeader
	{
		ModbusData start;
		ModbusData count;
		ModbusCRC crc;
		
		// ------- to slave -------
		ReadInputMessage( ModbusAddr addr, ModbusData start, ModbusData count );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		// ------- from master -------
		ReadInputMessage( ModbusMessage& m );
		ReadInputMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusData)*2 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ReadInputMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ReadInputMessage* m ); 
	// -----------------------------------------------------------------------

	/*! Ответ для 0x04 */
	struct ReadInputRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;									/*!< numbers of bytes */
		ModbusData data[MAXLENPACKET/sizeof(ModbusData)];	/*!< данные */

		// ------- from slave -------
		ReadInputRetMessage( ModbusMessage& m );
		ReadInputRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			// bcnt
			return sizeof(ModbusByte);
		}

		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );
		ModbusCRC crc;
		
		// ------- to master -------
		ReadInputRetMessage( ModbusAddr _from );

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( ModbusData d );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull()
		{
			return ( count*sizeof(ModbusData) >= MAXLENPACKET );
		}
		
		void swapData();

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();
		
		// Это поле не входит в стандарт modbus
		// оно вспомогательное и игнорируется при 
		// преобразовании в ModbusMessage.
		// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно. 
		// Используйте специальную функцию transport_msg()
		int	count;	/*!< фактическое количество данных в сообщении */
	};

	std::ostream& operator<<(std::ostream& os, ReadInputRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ReadInputRetMessage* m );
	// -----------------------------------------------------------------------
	/*! Запрос на запись 0x0F */	
	struct ForceCoilsMessage:
		public ModbusHeader
	{
		ModbusData start;	/*!< стартовый адрес записи */
		ModbusData quant;	/*!< количество записываемых битов */ 
		ModbusByte bcnt;	/*!< количество байт данных */
		/*! данные */
		ModbusByte data[MAXLENPACKET-sizeof(ModbusData)*2-sizeof(ModbusByte)];
		ModbusCRC crc;		/*!< контрольная сумма */

		// ------- to slave -------
		ForceCoilsMessage( ModbusAddr addr, ModbusData start );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( DataBits d );
		
		// return number of bit
		// -1 - error
		int addBit( bool state );
		
		bool setBit( int nbit, bool state );
		
		inline int last(){ return quant; }

		/*! получение данных.
		 * \param dnum  - номер байта
		 * \param d     - найденные данные
		 * \return TRUE - если есть
		 * \return FALSE - если НЕ найдено
		*/
		bool getData( unsigned char dnum, DataBits& d );
		
		bool getBit( unsigned char bnum );

		void clear();
		inline bool isFull()
		{
			return ( bcnt >= MAXLENPACKET );
		}

		// ------- from master -------	
		ForceCoilsMessage( ModbusMessage& m );
		ForceCoilsMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			// start + quant + count
			return sizeof(ModbusData)*2+sizeof(ModbusByte);
		}
		
		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );

		/*! проверка корректности данных 
			что quant и bcnt - совпадают...
		*/
		bool checkFormat();
		
	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, ForceCoilsMessage& m );
	std::ostream& operator<<(std::ostream& os, ForceCoilsMessage* m );
	// -----------------------------------------------------------------------
	/*! Ответ для запроса на запись 0x0F */	
	struct ForceCoilsRetMessage:
		public ModbusHeader
	{
		ModbusData start; 	/*!< записанный начальный адрес */
		ModbusData quant;	/*!< количество записанных битов */
		ModbusCRC crc;

		// ------- from slave -------
		ForceCoilsRetMessage( ModbusMessage& m );
		ForceCoilsRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		
		// ------- to master -------
		/*! 
		 * \param _from - адрес отправителя
		 * \param start	- записанный регистр
		 * \param quant	- количество записанных слов
		*/
		ForceCoilsRetMessage( ModbusAddr _from, ModbusData start=0, ModbusData quant=0 );

		/*! записать данные */
		void set( ModbusData start, ModbusData quant );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		/*! размер данных(после заголовка) у данного типа сообщения 
			Для данного типа он постоянный..
		*/
		inline static int szData(){ return sizeof(ModbusData)*2+sizeof(ModbusCRC); }
	};
	
	std::ostream& operator<<(std::ostream& os, ForceCoilsRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ForceCoilsRetMessage* m );
	// -----------------------------------------------------------------------

	/*! Запрос на запись 0x10 */	
	struct WriteOutputMessage:
		public ModbusHeader
	{
		ModbusData start;	/*!< стартовый адрес записи */
		ModbusData quant;	/*!< количество слов данных */ 
		ModbusByte bcnt;	/*!< количество байт данных */
		/*! данные */
		ModbusData data[MAXLENPACKET/sizeof(ModbusData)-sizeof(ModbusData)*2-sizeof(ModbusByte)];
		ModbusCRC crc;		/*!< контрольная сумма */

		// ------- to slave -------
		WriteOutputMessage( ModbusAddr addr, ModbusData start );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		bool addData( ModbusData d );
		void clear();
		inline bool isFull() 		
		{
			return ( quant*sizeof(ModbusData) >= MAXLENPACKET );
		}

		// ------- from master -------	
		WriteOutputMessage( ModbusMessage& m );
		WriteOutputMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			// start + quant + count
			return sizeof(ModbusData)*2+sizeof(ModbusByte);
		}
		
		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );

		/*! проверка корректности данных 
			что quant и bcnt - совпадают...
		*/
		bool checkFormat();
		
	}__attribute__((packed));


	std::ostream& operator<<(std::ostream& os, WriteOutputMessage& m );
	std::ostream& operator<<(std::ostream& os, WriteOutputMessage* m );

	/*! Ответ для запроса на запись 0x10 */	
	struct WriteOutputRetMessage:
		public ModbusHeader
	{
		ModbusData start; 	/*!< записанный начальный адрес */
		ModbusData quant;	/*!< количество записанных слов данных */

		// ------- from slave -------
		WriteOutputRetMessage( ModbusMessage& m );
		WriteOutputRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		ModbusCRC crc;

		// ------- to master -------
		/*! 
		 * \param _from - адрес отправителя
		 * \param start	- записанный регистр
		 * \param quant	- количество записанных слов
		*/
		WriteOutputRetMessage( ModbusAddr _from, ModbusData start=0, ModbusData quant=0 );

		/*! записать данные */
		void set( ModbusData start, ModbusData quant );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		/*! размер данных(после заголовка) у данного типа сообщения 
			Для данного типа он постоянный..
		*/
		inline static int szData(){ return sizeof(ModbusData)*2+sizeof(ModbusCRC); }
	};
	
	std::ostream& operator<<(std::ostream& os, WriteOutputRetMessage& m );
	std::ostream& operator<<(std::ostream& os, WriteOutputRetMessage* m );
	// -----------------------------------------------------------------------
	/*! Запрос 0x05 */	
	struct ForceSingleCoilMessage:
		public ModbusHeader
	{
		ModbusData start;	/*!< стартовый адрес записи */
		ModbusData data;	/*!< команда ON - true | OFF - false */
		ModbusCRC crc;		/*!< контрольная сумма */

		/*! получить значение команды */
		inline bool cmd()
		{
			return (data & 0xFF00);
		}


		// ------- to slave -------
		ForceSingleCoilMessage( ModbusAddr addr, ModbusData reg, bool state );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();
	
		// ------- from master -------
		ForceSingleCoilMessage( ModbusMessage& m );
		ForceSingleCoilMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			return sizeof(ModbusData);
		}
		
		/*! узнать длину данных следующий за 
			предварительным заголовком ( в байтах ) 
		*/
		static int getDataLen( ModbusMessage& m );

		/*! проверка корректности данных 
			что quant и bcnt - совпадают...
		*/
		bool checkFormat();
	}__attribute__((packed));


	std::ostream& operator<<(std::ostream& os, ForceSingleCoilMessage& m );
	std::ostream& operator<<(std::ostream& os, ForceSingleCoilMessage* m );
	// -----------------------------------------------------------------------

	/*! Ответ для запроса 0x05 */	
	struct ForceSingleCoilRetMessage:
		public ModbusHeader
	{
		ModbusData start; 	/*!< записанный начальный адрес */
		ModbusData data; 	/*!< данные */
		ModbusCRC crc;


		// ------- from slave -------
		ForceSingleCoilRetMessage( ModbusMessage& m );
		ForceSingleCoilRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		// ------- to master -------
		/*! 
		 * \param _from - адрес отправителя
		 * \param start	- записанный регистр
		*/
		ForceSingleCoilRetMessage( ModbusAddr _from );

		/*! записать данные */
		void set( ModbusData start, bool cmd );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		/*! размер данных(после заголовка) у данного типа сообщения 
			Для данного типа он постоянный..
		*/
		inline static int szData(){ return 2*sizeof(ModbusData)+sizeof(ModbusCRC); }
	};
	
	std::ostream& operator<<(std::ostream& os, ForceSingleCoilRetMessage& m );
	std::ostream& operator<<(std::ostream& os, ForceSingleCoilRetMessage* m );
	// -----------------------------------------------------------------------

	/*! Запрос на запись одного регистра 0x06 */	
	struct WriteSingleOutputMessage:
		public ModbusHeader
	{
		ModbusData start;	/*!< стартовый адрес записи */
		ModbusData data;	/*!< данные */
		ModbusCRC crc;		/*!< контрольная сумма */


		// ------- to slave -------
		WriteSingleOutputMessage( ModbusAddr addr, ModbusData reg=0, ModbusData data=0 );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();
	
		// ------- from master -------
		WriteSingleOutputMessage( ModbusMessage& m );
		WriteSingleOutputMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{
			return sizeof(ModbusData);
		}
		
		/*! узнать длину данных следующий за 
			предварительным заголовком ( в байтах ) 
		*/
		static int getDataLen( ModbusMessage& m );

		/*! проверка корректности данных 
			что quant и bcnt - совпадают...
		*/
		bool checkFormat();
	}__attribute__((packed));


	std::ostream& operator<<(std::ostream& os, WriteSingleOutputMessage& m );
	std::ostream& operator<<(std::ostream& os, WriteSingleOutputMessage* m );
	// -----------------------------------------------------------------------

	/*! Ответ для запроса на запись */	
	struct WriteSingleOutputRetMessage:
		public ModbusHeader
	{
		ModbusData start; 	/*!< записанный начальный адрес */
		ModbusData data; 	/*!< записанный начальный адрес */
		ModbusCRC crc;


		// ------- from slave -------
		WriteSingleOutputRetMessage( ModbusMessage& m );
		WriteSingleOutputRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		// ------- to master -------
		/*! 
		 * \param _from - адрес отправителя
		 * \param start	- записанный регистр
		*/
		WriteSingleOutputRetMessage( ModbusAddr _from, ModbusData start=0 );

		/*! записать данные */
		void set( ModbusData start, ModbusData data );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		/*! размер данных(после заголовка) у данного типа сообщения 
			Для данного типа он постоянный..
		*/
		inline static int szData(){ return 2*sizeof(ModbusData)+sizeof(ModbusCRC); }
	};
	
	std::ostream& operator<<(std::ostream& os, WriteSingleOutputRetMessage& m );
	std::ostream& operator<<(std::ostream& os, WriteSingleOutputRetMessage* m );
	// -----------------------------------------------------------------------
	/*! Запрос 0x08 */
	struct DiagnosticMessage:
		public ModbusHeader
	{
		ModbusData subf;
		ModbusData data[MAXLENPACKET/sizeof(ModbusData)];	/*!< данные */

		// ------- from slave -------
		DiagnosticMessage( ModbusMessage& m );
		DiagnosticMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		/*! размер предварительного заголовка
		 * (после основного до фактических данных)
		*/
		static inline int szHead()
		{
			return sizeof(ModbusData); // subf
		}

		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );
		ModbusCRC crc;

		// ------- to master -------
		DiagnosticMessage( ModbusAddr _from, DiagnosticsSubFunction subf, ModbusData d=0 );

		/*! добавление данных.
		 * \return TRUE - если удалось
		 * \return FALSE - если НЕ удалось
		*/
		bool addData( ModbusData d );

		/*! очистка данных */
		void clear();

		/*! проверка на переполнение */
		inline bool isFull()
		{
			return ( sizeof(subf)+count*sizeof(ModbusData) >= MAXLENPACKET );
		}

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();

		// Это поле не входит в стандарт modbus
		// оно вспомогательное и игнорируется при
		// преобразовании в ModbusMessage.
		// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно.
		// Используйте специальную функцию transport_msg()
		int	count;	/*!< фактическое количество данных в сообщении */
	};
	std::ostream& operator<<(std::ostream& os, DiagnosticMessage& m );
	std::ostream& operator<<(std::ostream& os, DiagnosticMessage* m );
	// -----------------------------------------------------------------------
	/*! Ответ для 0x08 */
	struct DiagnosticRetMessage:
		public DiagnosticMessage
	{
		DiagnosticRetMessage( ModbusMessage& m );
		DiagnosticRetMessage( DiagnosticMessage& m );
		DiagnosticRetMessage( ModbusAddr a, DiagnosticsSubFunction subf, ModbusData d=0 );
	};

	std::ostream& operator<<(std::ostream& os, DiagnosticRetMessage& m );
	std::ostream& operator<<(std::ostream& os, DiagnosticRetMessage* m );
	// -----------------------------------------------------------------------

	/*! Чтение информации об ошибке */	
	struct JournalCommandMessage:
		public ModbusHeader
	{
		ModbusData cmd;			/*!< код операции */
		ModbusData num;			/*!< номер записи */
		ModbusCRC crc;
		
		// -------------
		JournalCommandMessage( ModbusMessage& m );
		JournalCommandMessage& operator=( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusByte)*4 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, JournalCommandMessage& m ); 
	std::ostream& operator<<(std::ostream& os, JournalCommandMessage* m ); 
	// -----------------------------------------------------------------------
	/*! Ответ для запроса на чтение ошибки */	
	struct JournalCommandRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;					/*!< numbers of bytes */
//		ModbusByte data[MAXLENPACKET-1];	/*!< данные */

		// В связи со спецификой реализации ответной части (т.е. modbus master)
		// данные приходится делать не байтовым потоком, а "словами"
		// которые в свою очередь будут перевёрнуты при посылке...
		ModbusData data[MAXLENPACKET/sizeof(ModbusData)];	/*!< данные */

		// -------------
		JournalCommandRetMessage( ModbusAddr _from );

		/*! Добавление данных
			\warning Старые данные будут затёрты
			\warning Используется указатель ModbusByte*
				т.к. копируемые сюда данные могут быть
				не выровнены по словам!
		*/
		bool setData( ModbusByte* b, int len );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull() 		
		{
			return ( count*sizeof(ModbusData) >= MAXLENPACKET );
		}

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		// Это поле не входит в стандарт modbus
		// оно вспомогательное и игнорируется при 
		// преобразовании в ModbusMessage.
		// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно. 
		// Используйте специальную функцию transport_msg()
		int	count;	/*!< фактическое количество данных в сообщении */
	};

	std::ostream& operator<<(std::ostream& os, JournalCommandRetMessage& m );
	std::ostream& operator<<(std::ostream& os, JournalCommandRetMessage* m );
	// -----------------------------------------------------------------------
	/*! ответ в случае необходимости подтверждения команды 
		(просто пакует в JournalCommandRetMessage код команды и ошибки )
	*/
	struct JournalCommandRetOK:
		public JournalCommandRetMessage
	{
		// -------------
		JournalCommandRetOK( ModbusAddr _from );
		void set( ModbusData cmd, ModbusData ecode );
		static void set( JournalCommandRetMessage& m, ModbusData cmd, ModbusData ecode );
	};

	std::ostream& operator<<(std::ostream& os, JournalCommandRetOK& m ); 
	std::ostream& operator<<(std::ostream& os, JournalCommandRetOK* m ); 
	// -----------------------------------------------------------------------

	/*! Установка времени */	
	struct SetDateTimeMessage:
		public ModbusHeader
	{
		ModbusByte hour;	/*!< часы [0..23] */
		ModbusByte min;		/*!< минуты [0..59] */
		ModbusByte sec;		/*!< секунды [0..59] */
		ModbusByte day;		/*!< день [1..31] */
		ModbusByte mon;		/*!< месяц [1..12] */
		ModbusByte year;	/*!< год [0..99] */
		ModbusByte century;	/*!< столетие [19-20] */

		ModbusCRC crc;
		
		// ------- to slave -------
		SetDateTimeMessage( ModbusAddr addr );
		/*! преобразование для посылки в сеть */
		ModbusMessage transport_msg();
		
		// ------- from master -------
		SetDateTimeMessage( ModbusMessage& m );
		SetDateTimeMessage& operator=( ModbusMessage& m );
		SetDateTimeMessage();

		bool checkFormat();

		/*! размер данных(после заголовка) у данного типа сообщения */
		inline static int szData(){ return sizeof(ModbusByte)*7 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, SetDateTimeMessage& m ); 
	std::ostream& operator<<(std::ostream& os, SetDateTimeMessage* m ); 
	// -----------------------------------------------------------------------

	/*! Ответ (просто повторяет запрос) */
	struct SetDateTimeRetMessage:
		public SetDateTimeMessage
	{

		// ------- from slave -------
		SetDateTimeRetMessage( ModbusMessage& m );
		SetDateTimeRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		// ------- to master -------
		SetDateTimeRetMessage( ModbusAddr _from );
		SetDateTimeRetMessage( const SetDateTimeMessage& query );
		static void cpy( SetDateTimeRetMessage& reply, SetDateTimeMessage& query );

		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
	};
	// -----------------------------------------------------------------------

	/*! Вызов удалённого сервиса */	
	struct RemoteServiceMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;	/*!< количество байт */

		/*! данные */
		ModbusByte data[MAXLENPACKET-sizeof(ModbusByte)];
		ModbusCRC crc;		/*!< контрольная сумма */
	
		// -----------
		RemoteServiceMessage( ModbusMessage& m );
		RemoteServiceMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{ return sizeof(ModbusByte); } // bcnt
		
		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, RemoteServiceMessage& m ); 
	std::ostream& operator<<(std::ostream& os, RemoteServiceMessage* m ); 
	// -----------------------------------------------------------------------
	struct RemoteServiceRetMessage:
		public ModbusHeader
	{
		ModbusByte bcnt;	/*!< количество байт */
		/*! данные */
		ModbusByte data[MAXLENPACKET-sizeof(ModbusByte)];

		RemoteServiceRetMessage( ModbusAddr _from );

		/*! Добавление данных
			\warning Старые данные будут затёрты
			\warning Используется указатель ModbusByte*
				т.к. копируемые сюда данные могут быть
				не выровнены по словам!
		*/
		bool setData( ModbusByte* b, int len );

		/*! очистка данных */
		void clear();
		
		/*! проверка на переполнение */	
		inline bool isFull() 		
			{ return ( count >= sizeof(data) ); }

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
		
		// Это поле не входит в стандарт modbus
		// оно вспомогательное и игнорируется при 
		// преобразовании в ModbusMessage.
		unsigned int	count;	/*!< фактическое количество данных в сообщении */
	};
	// -----------------------------------------------------------------------

	struct ReadFileRecordMessage:
		public ModbusHeader
	{
		struct SubRequest
		{
			ModbusByte reftype; /*!< referens type 06 */
			ModbusData numfile; /*!< file number 0x0000 to 0xFFFF */
			ModbusData numrec;  /*!< record number 0x0000 to 0x270F */
			ModbusData reglen;  /*!< registers length */
		}__attribute__((packed));	

		ModbusByte bcnt;	/*!< количество байт 0x07 to 0xF5 */
 
		/*! данные */
		SubRequest data[MAXLENPACKET/sizeof(SubRequest)-sizeof(ModbusByte)];
		ModbusCRC crc;		/*!< контрольная сумма */
	
		// -----------
		ReadFileRecordMessage( ModbusMessage& m );
		ReadFileRecordMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();

		/*! размер предварительного заголовка 
		 * (после основного до фактических данных) 
		*/
		static inline int szHead()
		{ return sizeof(ModbusByte); } // bcnt
		
		/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
		static int getDataLen( ModbusMessage& m );

		/*! проверка корректности данных */
		bool checkFormat();
		
		// это поле служебное и не используется в релальном обмене
		int count; /*!< фактическое количество данных */
	};

	std::ostream& operator<<(std::ostream& os, ReadFileRecordMessage& m ); 
	std::ostream& operator<<(std::ostream& os, ReadFileRecordMessage* m ); 
	// -----------------------------------------------------------------------

	struct FileTransferMessage:
		public ModbusHeader
	{
		ModbusData numfile; 	/*!< file number 0x0000 to 0xFFFF */
		ModbusData numpacket;  	/*!< number of packet */
		ModbusCRC crc;			/*!< контрольная сумма */
	
		// ------- to slave -------
		FileTransferMessage( ModbusAddr addr, ModbusData numfile, ModbusData numpacket );
		ModbusMessage transport_msg(); 	/*!< преобразование для посылки в сеть */
	
		// ------- from master -------
		FileTransferMessage( ModbusMessage& m );
		FileTransferMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );

		/*! размер данных(после заголовка) у данного типа сообщения */
		static inline int szData()
		{	return sizeof(ModbusData)*2 + szCRC; }

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, FileTransferMessage& m ); 
	std::ostream& operator<<(std::ostream& os, FileTransferMessage* m ); 
	// -----------------------------------------------------------------------

	struct FileTransferRetMessage:
		public ModbusHeader
	{
		// 255 - max of bcnt...(1 byte)		
//		static const int MaxDataLen = 255 - szCRC - szModbusHeader - sizeof(ModbusData)*3 - sizeof(ModbusByte)*2;
		static const int MaxDataLen = MAXLENPACKET - sizeof(ModbusData)*3 - sizeof(ModbusByte)*2;

		ModbusByte bcnt;		/*!< общее количество байт в ответе */
		ModbusData numfile; 	/*!< file number 0x0000 to 0xFFFF */
		ModbusData numpacks; 	/*!< all count packages (file size) */
		ModbusData packet;  	/*!< number of packet */
		ModbusByte dlen;		/*!< количество байт данных в ответе */
		ModbusByte data[MaxDataLen];

	
		// ------- from slave -------
		FileTransferRetMessage( ModbusMessage& m );
		FileTransferRetMessage& operator=( ModbusMessage& m );
		void init( ModbusMessage& m );
		ModbusCRC crc;
		static int szHead(){ return sizeof(ModbusByte); }
		static int getDataLen( ModbusMessage& m );
		
		// ------- to master -------
		FileTransferRetMessage( ModbusAddr _from );

		/*! Добавление данных
			\warning Старые данные будут затёрты
		*/
		bool set( ModbusData numfile, ModbusData file_num_packets, ModbusData packet, ModbusByte* b, ModbusByte len );

		/*! очистка данных */
		void clear();
		
		/*! размер данных(после заголовка) у данного типа сообщения */
		int szData();
		
		/*! преобразование для посылки в сеть */	
		ModbusMessage transport_msg();
	};

	std::ostream& operator<<(std::ostream& os, FileTransferRetMessage& m ); 
	std::ostream& operator<<(std::ostream& os, FileTransferRetMessage* m );
	// -----------------------------------------------------------------------
} // end of ModbusRTU namespace
// ---------------------------------------------------------------------------
namespace ModbusTCP
{
	struct MBAPHeader
	{
		ModbusRTU::ModbusData	tID; /*!< transaction ID */
		ModbusRTU::ModbusData	pID; /*!< protocol ID */
		ModbusRTU::ModbusData	len; /*!< lenght */
/*		ModbusRTU::ModbusByte	uID; */ /*!< unit ID */ /* <------- see ModbusHeader */

		MBAPHeader():tID(0),pID(0) /*,uID(0) */{}
		
		void swapdata();

	}__attribute__((packed));

	std::ostream& operator<<(std::ostream& os, MBAPHeader& m ); 

  // -----------------------------------------------------------------------
} // end of namespace ModbusTCP
// ---------------------------------------------------------------------------
#endif // ModbusTypes_H_
// ---------------------------------------------------------------------------
