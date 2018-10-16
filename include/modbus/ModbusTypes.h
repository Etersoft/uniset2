// -------------------------------------------------------------------------
#ifndef ModbusTypes_H_
#define ModbusTypes_H_
// -------------------------------------------------------------------------
#include <ostream>
#include <cstdint>
#include <bitset>
#include <string>
#include <list>
#include "ModbusRTUErrors.h"
// -------------------------------------------------------------------------
/* Основные предположения:
 * - младший и старший байт переворачиваются только в CRC
 * - В случае неправильного формата пакета(запроса), логической ошибки и т.п
 *         ОТВЕТ просто не посылается, а пакет отбрасывается...
 * - CRC считается по всей посылке (с начальным адресом)
 * - CRC инициализируется значением 0xffff
 * - CRC не переворачивается
 * - Все двухбайтовые слова переворачиваются. Порядок байт: старший младший
*/
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	namespace ModbusRTU
	{
		// Базовые типы
		typedef uint8_t ModbusByte;    /*!< modbus-байт */
		const size_t BitsPerByte = 8;
		typedef uint8_t ModbusAddr;    /*!< адрес узла в modbus-сети */
		typedef uint16_t ModbusData;    /*!< размер данных в modbus-сообщениях */
		const uint8_t BitsPerData = 16;
		typedef uint16_t ModbusCRC;    /*!< размер CRC16 в modbus-сообщениях */

		// ---------------------------------------------------------------------
		/*! Коды используемых функций (согласно описанию modbus) */
		enum SlaveFunctionCode
		{
			fnUnknown                = 0x00,
			fnReadCoilStatus         = 0x01, /*!< read coil status */
			fnReadInputStatus        = 0x02, /*!< read input status */
			fnReadOutputRegisters    = 0x03, /*!< read register outputs or memories or read word outputs or memories */
			fnReadInputRegisters     = 0x04, /*!< read input registers or memories or read word outputs or memories */
			fnForceSingleCoil        = 0x05, /*!< forces a single coil to either ON or OFF */
			fnWriteOutputSingleRegister = 0x06,    /*!< write register outputs or memories */
			fnDiagnostics            = 0x08, /*!< Diagnostics (Serial Line only) */
			fnForceMultipleCoils     = 0x0F,    /*!< force multiple coils */
			fnWriteOutputRegisters   = 0x10,    /*!< write register outputs or memories */
			fnReadFileRecord         = 0x14,    /*!< read file record */
			fnWriteFileRecord        = 0x15,    /*!< write file record */
			fnMEI                    = 0x2B, /*!< Modbus Encapsulated Interface */
			fnSetDateTime            = 0x50, /*!< set date and time */
			fnRemoteService          = 0x53,    /*!< call remote service */
			fnJournalCommand         = 0x65,    /*!< read,write,delete alarm journal */
			fnFileTransfer           = 0x66    /*!< file transfer */
		};

		/*! Коды диагностически подфункций (для запроса 0x08) */
		enum DiagnosticsSubFunction
		{
			subEcho = 0x00,          /*!< (0) Return Query Data (echo) */
			dgRestartComm = 0x01,    /*!< (1) Restart Communications Option */
			dgDiagReg = 0x02,        /*!< (2) Return Diagnostic Register */
			dgChangeASCII = 0x03,    /*!< (3) Change ASCII Input Delimiter */
			dgForceListen = 0x04,    /*!< (4) Force Listen Only Mode */
			// 05.. 09 RESERVED
			dgClearCounters = 0x0A,  /*!< (10)Clear Counters and Diagnostic Register */
			dgBusMsgCount = 0x0B,    /*!< (11) Return Bus Message Count */
			dgBusErrCount = 0x0C,    /*!< (12) Return Bus Communication Error Count */
			dgBusExceptCount = 0x0D, /*!< (13) Return Bus Exception Error Count */
			dgMsgSlaveCount = 0x0E,        /*!< (14) Return Slave Message Count */
			dgNoNoResponseCount = 0x0F,    /*!< (15) Return Slave No Response Count */
			dgSlaveNAKCount = 0x10,        /*!< (16) Return Slave NAK Count */
			dgSlaveBusyCount = 0x11,       /*!< (17) Return Slave Busy Count */
			dgBusCharOverrunCount = 0x12,  /*!< (18) Return Bus Character Overrun Count */
			// = 0x13,    /*!<  RESERVED */
			dgClearOverrunCounter = 0x14   /*!< (20) Clear Overrun Counter and FlagN.A. */
									// 21 ...65535 RESERVED
		};


		using RegID = size_t;

		/*! Получение уникального ID (hash?) на основе номера функции и регистра
		 *  Требования к данной функции:
		 *  1. ID > диапазона возможных регистров (>65535)
		 *  2. одинаковые регистры, но разные функции должны давать разный ID
		 *  3. регистры идущие подряд, должны давать ID идущие тоже подряд
		*/
		RegID genRegID( const ModbusRTU::ModbusData r, const uint8_t fn );

		// определение размера данных в зависимости от типа сообщения
		// возвращает -1 - если динамический размер сообщения или размер неизвестен
		ssize_t szRequestDiagnosticData( DiagnosticsSubFunction f );

		/*! Read Device Identification ObjectID (0x2B/0xE) */
		enum RDIObjectID
		{
			rdiVendorName = 0x0,
			rdiProductCode = 0x1,
			rdiMajorMinorRevision = 0x2,
			rdiVendorURL = 0x3,
			rdiProductName = 0x4,
			rdiModelName = 0x5,
			rdiUserApplicationName = 0x6
									 // 0x07 .. 0x7F - reserved
									 // 0x80 .. 0xFF - optionaly defined (product dependant)
		};

		/*! Read Device Identification ObjectID (0x2B/0xE) */
		enum RDIRequestDeviceID
		{
			rdevMinNum = 0,
			rdevBasicDevice = 0x1,   // request to get the basic device identification (stream access)
			rdevRegularDevice = 0x2, // request to get the regular device identification (stream access)
			rdevExtentedDevice = 0x3, // request to get the extended device identification (stream access)
			rdevSpecificDevice = 0x4, // request to get the extended device identification (stream access)
			rdevMaxNum = 0x5
		};

		std::string rdi2str( int id );
		// -----------------------------------------------------------------------

		/*! различные базовые константы */
		enum
		{
			/*! максимальное количество данных в пакете (c учётом контрольной суммы) */
			MAXLENPACKET     = 508, /*!< максимальная длина пакета 512 - header(2) - CRC(2) */
			BroadcastAddr    = 0, /*!< адрес для широковещательных сообщений */
			MAXPDULEN       = 253, // 255 - 2(CRC)
			MAXDATALEN       = 125  /*!< максимальное число слов, которое можно запросить.
                                    Связано с тем, что в ответе есть поле bcnt - количество байт
                                    Соответственно максимум туда можно записать только 255
                                */
		};

		const uint8_t MBErrMask = 0x80;
		// ---------------------------------------------------------------------
		uint16_t SWAPSHORT( uint16_t x );
		// ---------------------------------------------------------------------
		/*! Расчёт контрольной суммы */
		ModbusCRC checkCRC( ModbusByte* start, size_t len );
		const size_t szCRC = sizeof(ModbusCRC); /*!< размер данных для контрольной суммы */
		// ---------------------------------------------------------------------
		/*! вывод сообщения */
		std::ostream& mbPrintMessage(std::ostream& os, ModbusByte* b, size_t len );
		// -------------------------------------------------------------------------
		ModbusAddr str2mbAddr( const std::string& val );
		ModbusData str2mbData( const std::string& val );
		std::string dat2str( const ModbusData dat );
		std::string addr2str( const ModbusAddr addr );
		std::string b2str( const ModbusByte b );
		// -------------------------------------------------------------------------
		float dat2f( const ModbusData dat1, const ModbusData dat2 );
		size_t numBytes( const size_t nbits ); // сколько байт нужно для указанного количества бит
		// -------------------------------------------------------------------------
		bool isWriteFunction( SlaveFunctionCode c );
		bool isReadFunction( SlaveFunctionCode c );
		// -------------------------------------------------------------------------
		/*! Заголовок сообщений */
		struct ModbusHeader
		{
			ModbusAddr addr;        /*!< адрес в сети */
			ModbusByte func;        /*!< код функции */

			ModbusHeader(): addr(0), func(0) {}
		} __attribute__((packed));

		const size_t szModbusHeader = sizeof(ModbusHeader);

		std::ostream& operator<<(std::ostream& os, const ModbusHeader& m );
		std::ostream& operator<<(std::ostream& os, const ModbusHeader* m );
		// -----------------------------------------------------------------------
		struct MBAPHeader
		{
			ModbusRTU::ModbusData tID; /*!< transaction ID */
			ModbusRTU::ModbusData pID; /*!< protocol ID */
			ModbusRTU::ModbusData len; /*!< lenght */

			MBAPHeader(): tID(0), pID(0), len(0) {}

			void swapdata();

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, const MBAPHeader& m );
		// -----------------------------------------------------------------------

		/*! Базовое (сырое) сообщение
		    \todo Может переименовать ModbusMessage в TransportMessage?
		*/
		struct ModbusMessage
		{
			ModbusMessage();

			ModbusMessage( ModbusMessage&& ) = default;
			ModbusMessage& operator=(ModbusMessage&& ) = default;
			ModbusMessage( const ModbusMessage& ) = default;
			ModbusMessage& operator=(const ModbusMessage& ) = default;

			inline ModbusByte func() const
			{
				return pduhead.func;
			}
			inline ModbusAddr addr() const
			{
				return pduhead.addr;
			}
			inline ModbusRTU::ModbusData tID() const
			{
				return mbaphead.tID;
			}
			inline ModbusRTU::ModbusData pID() const
			{
				return mbaphead.pID;
			}
			inline ModbusRTU::ModbusData aduLen() const
			{
				return mbaphead.len;
			}

			u_int8_t* buf();
			ModbusRTU::ModbusData len() const;
			void swapHead();
			void makeMBAPHeader( ModbusRTU::ModbusData tID, bool noCRC = true, ModbusRTU::ModbusData pID = 0 );

			ModbusRTU::ModbusData pduLen() const;
			ModbusCRC pduCRC( size_t len ) const;
			static size_t maxSizeOfMessage();

			void clear();

			MBAPHeader mbaphead;
			ModbusHeader pduhead;
			ModbusByte data[MAXLENPACKET + szCRC];   /*!< данные */

			// Это поле вспомогательное и игнорируется при пересылке
			size_t dlen = { 0 };  /*!< фактическая длина сообщения */
		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, const ModbusMessage& m );
		std::ostream& operator<<(std::ostream& os, const ModbusMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ сообщающий об ошибке */
		struct ErrorRetMessage:
			public ModbusHeader
		{
			ModbusByte ecode = { erNoError };
			ModbusCRC crc = { 0 };

			// ------- from slave -------
			ErrorRetMessage( const ModbusMessage& m );
			ErrorRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// ------- to master -------
			ErrorRetMessage( ModbusAddr _from, ModbusByte _func, ModbusByte ecode );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			/*! размер данных(после заголовка) у данного типа сообщения
			    Для данного типа он постоянный..
			*/
			inline static size_t szData()
			{
				return sizeof(ModbusByte) + szCRC;
			}
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

			bool operator[]( const size_t i )
			{
				return b[i];
			}
			void set( int n, bool s )
			{
				b.set(n, s);
			}

			std::bitset<BitsPerByte> b;
		};

		std::ostream& operator<<(std::ostream& os, DataBits& m );
		std::ostream& operator<<(std::ostream& os, DataBits* m );
		// -----------------------------------------------------------------------
		struct DataBits16
		{
			DataBits16( ModbusData d );
			DataBits16( const std::string& s ); // example "1000111110001111"
			DataBits16();

			const DataBits16& operator=(const ModbusData& r);

			operator ModbusData();
			ModbusData mdata() const;

			bool operator[]( const size_t i )
			{
				return b[i];
			}
			void set( int n, bool s )
			{
				b.set(n, s);
			}

			std::bitset<BitsPerData> b;
		};

		std::ostream& operator<<(std::ostream& os, DataBits16& m );
		std::ostream& operator<<(std::ostream& os, DataBits16* m );
		// -----------------------------------------------------------------------
		/*! Запрос 0x01 */
		struct ReadCoilMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };
			ModbusData count = { 0 };
			ModbusCRC crc = { 0 };

			// ------- to slave -------
			ReadCoilMessage( ModbusAddr addr, ModbusData start, ModbusData count );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			ReadCoilMessage( const ModbusMessage& m );
			ReadCoilMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, ReadCoilMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadCoilMessage* m );

		// -----------------------------------------------------------------------

		/*! Ответ на 0x01 */
		struct ReadCoilRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };          /*!< numbers of bytes */
			ModbusByte data[MAXLENPACKET];    /*!< данные */

			// ------- from slave -------
			ReadCoilRetMessage( const ModbusMessage& m );
			ReadCoilRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline int szHead()
			{
				return sizeof(ModbusByte); // bcnt
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

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
			bool setBit( uint8_t dnum, uint8_t bnum, bool state );

			/*! получение данных.
			 * \param bnum  - номер байта(0..MAXLENPACKET)
			 * \param d     - найденные данные
			 * \return TRUE - если есть
			 * \return FALSE - если НЕ найдено
			*/
			bool getData( uint8_t bnum, DataBits& d ) const;

			/*! очистка данных */
			void clear();

			/*! проверка на переполнение */
			inline bool isFull() const
			{
				return ( (int)bcnt >= MAXPDULEN );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

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
			ModbusData start = { 0 };
			ModbusData count = { 0 };
			ModbusCRC crc = { 0 };

			// ------- to slave -------
			ReadInputStatusMessage( ModbusAddr addr, ModbusData start, ModbusData count );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			ReadInputStatusMessage( const ModbusMessage& m );
			ReadInputStatusMessage& operator=( const ModbusMessage& m );

			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, ReadInputStatusMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadInputStatusMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ на 0x02 */
		struct ReadInputStatusRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };          /*!< numbers of bytes */
			ModbusByte data[MAXLENPACKET];    /*!< данные */

			// ------- from slave -------
			ReadInputStatusRetMessage( const ModbusMessage& m );
			ReadInputStatusRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusByte); // bcnt
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

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
			bool setBit( uint8_t dnum, uint8_t bnum, bool state );

			/*! получение данных.
			 * \param dnum  - номер байта (0..MAXLENPACKET)
			 * \param d     - найденные данные
			 * \return TRUE - если есть
			 * \return FALSE - если НЕ найдено
			*/
			bool getData( uint8_t dnum, DataBits& d ) const;

			/*! очистка данных */
			void clear();

			/*! проверка на переполнение */
			inline bool isFull() const
			{
				return ( (int)bcnt >= MAXPDULEN );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

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
			ModbusData start = { 0 };
			ModbusData count = { 0 };
			ModbusCRC crc = { 0 };

			// ------- to slave -------
			ReadOutputMessage( ModbusAddr addr, ModbusData start, ModbusData count );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			ReadOutputMessage( const ModbusMessage& m );
			ReadOutputMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, ReadOutputMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadOutputMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ для 0x03 */
		struct ReadOutputRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };                             /*!< numbers of bytes */
			ModbusData data[MAXLENPACKET / sizeof(ModbusData)];  /*!< данные */

			// ------- from slave -------
			ReadOutputRetMessage( const ModbusMessage& m );
			ReadOutputRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );
			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				// bcnt
				return sizeof(ModbusByte);
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

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
			inline bool isFull() const
			{
				return ( count * sizeof(ModbusData) >= MAXLENPACKET );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// Это поле не входит в стандарт modbus
			// оно вспомогательное и игнорируется при
			// преобразовании в ModbusMessage.
			// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно.
			// Используйте специальную функцию transport_msg()
			size_t count = { 0 };    /*!< фактическое количество данных в сообщении */
		};

		std::ostream& operator<<(std::ostream& os, ReadOutputRetMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadOutputRetMessage* m );
		// -----------------------------------------------------------------------
		/*! Запрос 0x04 */
		struct ReadInputMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };
			ModbusData count = { 0 };
			ModbusCRC crc = { 0 };

			// ------- to slave -------
			ReadInputMessage( ModbusAddr addr, ModbusData start, ModbusData count );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			ReadInputMessage( const ModbusMessage& m );
			ReadInputMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, ReadInputMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadInputMessage* m );
		// -----------------------------------------------------------------------

		/*! Ответ для 0x04 */
		struct ReadInputRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };                                    /*!< numbers of bytes */
			ModbusData data[MAXLENPACKET / sizeof(ModbusData)];  /*!< данные */

			// ------- from slave -------
			ReadInputRetMessage( const ModbusMessage& m );
			ReadInputRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );
			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				// bcnt
				return sizeof(ModbusByte);
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

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
			inline bool isFull() const
			{
				return ( count * sizeof(ModbusData) >= MAXLENPACKET );
			}

			void swapData();

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData();

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// Это поле не входит в стандарт modbus
			// оно вспомогательное и игнорируется при
			// преобразовании в ModbusMessage.
			// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно.
			// Используйте специальную функцию transport_msg()
			size_t count = { 0 };    /*!< фактическое количество данных в сообщении */
		};

		std::ostream& operator<<(std::ostream& os, ReadInputRetMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadInputRetMessage* m );
		// -----------------------------------------------------------------------
		/*! Запрос на запись 0x0F */
		struct ForceCoilsMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };    /*!< стартовый адрес записи */
			ModbusData quant = { 0 };    /*!< количество записываемых битов */
			ModbusByte bcnt = { 0 };    /*!< количество байт данных */
			/*! данные */
			ModbusByte data[MAXLENPACKET - sizeof(ModbusData) * 2 - sizeof(ModbusByte)];
			ModbusCRC crc = { 0 };        /*!< контрольная сумма */

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

			bool setBit( uint8_t nbit, bool state );

			inline size_t last() const
			{
				return quant;
			}

			/*! получение данных.
			 * \param dnum  - номер байта
			 * \param d     - найденные данные
			 * \return TRUE - если есть
			 * \return FALSE - если НЕ найдено
			*/
			bool getData( uint8_t dnum, DataBits& d );

			void clear();
			inline bool isFull() const
			{
				return ( (int)bcnt >= MAXPDULEN );
			}

			// ------- from master -------
			ForceCoilsMessage( const ModbusMessage& m );
			ForceCoilsMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				// start + quant + count
				return sizeof(ModbusData) * 2 + sizeof(ModbusByte);
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );

			/*! проверка корректности данных
			    что quant и bcnt - совпадают...
			*/
			bool checkFormat() const;

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, ForceCoilsMessage& m );
		std::ostream& operator<<(std::ostream& os, ForceCoilsMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ для запроса на запись 0x0F */
		struct ForceCoilsRetMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };    /*!< записанный начальный адрес */
			ModbusData quant = { 0 };    /*!< количество записанных битов */
			ModbusCRC crc = { 0 };

			// ------- from slave -------
			ForceCoilsRetMessage( const ModbusMessage& m );
			ForceCoilsRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// ------- to master -------
			/*!
			 * \param _from - адрес отправителя
			 * \param start    - записанный регистр
			 * \param quant    - количество записанных слов
			*/
			ForceCoilsRetMessage( ModbusAddr _from, ModbusData start = 0, ModbusData quant = 0 );

			/*! записать данные */
			void set( ModbusData start, ModbusData quant );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			/*! размер данных(после заголовка) у данного типа сообщения
			    Для данного типа он постоянный..
			*/
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + sizeof(ModbusCRC);
			}
		};

		std::ostream& operator<<(std::ostream& os, ForceCoilsRetMessage& m );
		std::ostream& operator<<(std::ostream& os, ForceCoilsRetMessage* m );
		// -----------------------------------------------------------------------

		/*! Запрос на запись 0x10 */
		struct WriteOutputMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };    /*!< стартовый адрес записи */
			ModbusData quant = { 0 };    /*!< количество слов данных */
			ModbusByte bcnt = { 0 };    /*!< количество байт данных */
			/*! данные */
			ModbusData data[MAXLENPACKET / sizeof(ModbusData) - sizeof(ModbusData) * 2 - sizeof(ModbusByte)];
			ModbusCRC crc = { 0 };        /*!< контрольная сумма */

			// ------- to slave -------
			WriteOutputMessage( ModbusAddr addr, ModbusData start );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			bool addData( ModbusData d );
			void clear();
			inline bool isFull() const
			{
				return ( quant >= MAXDATALEN );
			}

			// ------- from master -------
			WriteOutputMessage( const ModbusMessage& m );
			WriteOutputMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				// start + quant + count
				return sizeof(ModbusData) * 2 + sizeof(ModbusByte);
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );

			/*! проверка корректности данных
			    что quant и bcnt - совпадают...
			*/
			bool checkFormat() const;

		} __attribute__((packed));


		std::ostream& operator<<(std::ostream& os, WriteOutputMessage& m );
		std::ostream& operator<<(std::ostream& os, WriteOutputMessage* m );

		/*! Ответ для запроса на запись 0x10 */
		struct WriteOutputRetMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };     /*!< записанный начальный адрес */
			ModbusData quant = { 0 };    /*!< количество записанных слов данных */

			// ------- from slave -------
			WriteOutputRetMessage( const ModbusMessage& m );
			WriteOutputRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

			// ------- to master -------
			/*!
			 * \param _from - адрес отправителя
			 * \param start    - записанный регистр
			 * \param quant    - количество записанных слов
			*/
			WriteOutputRetMessage( ModbusAddr _from, ModbusData start = 0, ModbusData quant = 0 );

			/*! записать данные */
			void set( ModbusData start, ModbusData quant );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			/*! размер данных(после заголовка) у данного типа сообщения
			    Для данного типа он постоянный..
			*/
			inline static size_t szData()
			{
				return sizeof(ModbusData) * 2 + sizeof(ModbusCRC);
			}
		};

		std::ostream& operator<<(std::ostream& os, WriteOutputRetMessage& m );
		std::ostream& operator<<(std::ostream& os, WriteOutputRetMessage* m );
		// -----------------------------------------------------------------------
		/*! Запрос 0x05 */
		struct ForceSingleCoilMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };   /*!< стартовый адрес записи */
			ModbusData data = { 0 };    /*!< команда ON - true | OFF - false */
			ModbusCRC crc = { 0 };      /*!< контрольная сумма */

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
			ForceSingleCoilMessage( const ModbusMessage& m );
			ForceSingleCoilMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusData);
			}

			/*! узнать длину данных следующий за
			    предварительным заголовком ( в байтах )
			*/
			static size_t getDataLen( const ModbusMessage& m );

			/*! проверка корректности данных
			    что quant и bcnt - совпадают...
			*/
			bool checkFormat() const;
		} __attribute__((packed));


		std::ostream& operator<<(std::ostream& os, ForceSingleCoilMessage& m );
		std::ostream& operator<<(std::ostream& os, ForceSingleCoilMessage* m );
		// -----------------------------------------------------------------------

		/*! Ответ для запроса 0x05 */
		struct ForceSingleCoilRetMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };   /*!< записанный начальный адрес */
			ModbusData data = { 0 };    /*!< данные */
			ModbusCRC crc = { 0 };

			/*! получить значение команды */
			inline bool cmd() const
			{
				return (data & 0xFF00);
			}

			// ------- from slave -------
			ForceSingleCoilRetMessage( const ModbusMessage& m );
			ForceSingleCoilRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// ------- to master -------
			/*!
			 * \param _from - адрес отправителя
			 * \param start    - записанный регистр
			*/
			ForceSingleCoilRetMessage( ModbusAddr _from );

			/*! записать данные */
			void set( ModbusData start, bool cmd );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			/*! размер данных(после заголовка) у данного типа сообщения
			    Для данного типа он постоянный..
			*/
			inline static size_t szData()
			{
				return 2 * sizeof(ModbusData) + sizeof(ModbusCRC);
			}
		};

		std::ostream& operator<<(std::ostream& os, ForceSingleCoilRetMessage& m );
		std::ostream& operator<<(std::ostream& os, ForceSingleCoilRetMessage* m );
		// -----------------------------------------------------------------------

		/*! Запрос на запись одного регистра 0x06 */
		struct WriteSingleOutputMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };    /*!< стартовый адрес записи */
			ModbusData data = { 0 };    /*!< данные */
			ModbusCRC crc = { 0 };        /*!< контрольная сумма */


			// ------- to slave -------
			WriteSingleOutputMessage( ModbusAddr addr, ModbusData reg = 0, ModbusData data = 0 );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			WriteSingleOutputMessage( const ModbusMessage& m );
			WriteSingleOutputMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusData);
			}

			/*! узнать длину данных следующий за
			    предварительным заголовком ( в байтах )
			*/
			static size_t getDataLen( const ModbusMessage& m );

			/*! проверка корректности данных
			    что quant и bcnt - совпадают...
			*/
			bool checkFormat();
		} __attribute__((packed));


		std::ostream& operator<<(std::ostream& os, WriteSingleOutputMessage& m );
		std::ostream& operator<<(std::ostream& os, WriteSingleOutputMessage* m );
		// -----------------------------------------------------------------------

		/*! Ответ для запроса на запись */
		struct WriteSingleOutputRetMessage:
			public ModbusHeader
		{
			ModbusData start = { 0 };     /*!< записанный начальный адрес */
			ModbusData data = { 0 };     /*!< записанные данные */
			ModbusCRC crc = { 0 };


			// ------- from slave -------
			WriteSingleOutputRetMessage( const ModbusMessage& m );
			WriteSingleOutputRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// ------- to master -------
			/*!
			 * \param _from - адрес отправителя
			 * \param start    - записанный регистр
			*/
			WriteSingleOutputRetMessage( ModbusAddr _from, ModbusData start = 0 );

			/*! записать данные */
			void set( ModbusData start, ModbusData data );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			/*! размер данных(после заголовка) у данного типа сообщения
			    Для данного типа он постоянный..
			*/
			inline static size_t szData()
			{
				return 2 * sizeof(ModbusData) + sizeof(ModbusCRC);
			}
		};

		std::ostream& operator<<(std::ostream& os, WriteSingleOutputRetMessage& m );
		std::ostream& operator<<(std::ostream& os, WriteSingleOutputRetMessage* m );
		// -----------------------------------------------------------------------
		/*! Запрос 0x08 */
		struct DiagnosticMessage:
			public ModbusHeader
		{
			ModbusData subf = { 0 };
			ModbusData data[MAXLENPACKET / sizeof(ModbusData)];  /*!< данные */

			// ------- from slave -------
			DiagnosticMessage( const ModbusMessage& m );
			DiagnosticMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );
			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusData); // subf
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );
			ModbusCRC crc = { 0 };

			// ------- to master -------
			DiagnosticMessage( ModbusAddr _from, DiagnosticsSubFunction subf, ModbusData d = 0 );

			/*! добавление данных.
			 * \return TRUE - если удалось
			 * \return FALSE - если НЕ удалось
			*/
			bool addData( ModbusData d );

			/*! очистка данных */
			void clear();

			/*! проверка на переполнение */
			inline bool isFull() const
			{
				// (1)subf + data count
				return ( 1 + count >= MAXDATALEN );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// Это поле не входит в стандарт modbus
			// оно вспомогательное и игнорируется при
			// преобразовании в ModbusMessage.
			// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно.
			// Используйте специальную функцию transport_msg()
			size_t count = { 0 };    /*!< фактическое количество данных в сообщении */
		};
		std::ostream& operator<<(std::ostream& os, DiagnosticMessage& m );
		std::ostream& operator<<(std::ostream& os, DiagnosticMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ для 0x08 */
		struct DiagnosticRetMessage:
			public DiagnosticMessage
		{
			DiagnosticRetMessage( const ModbusMessage& m );
			DiagnosticRetMessage( const DiagnosticMessage& m );
			DiagnosticRetMessage( ModbusAddr a, DiagnosticsSubFunction subf, ModbusData d = 0 );
		};

		std::ostream& operator<<(std::ostream& os, DiagnosticRetMessage& m );
		std::ostream& operator<<(std::ostream& os, DiagnosticRetMessage* m );
		// -----------------------------------------------------------------------
		/*! Modbus Encapsulated Interface (0x2B). Read Device Identification (0x0E) */
		struct MEIMessageRDI:
			public ModbusHeader
		{
			ModbusByte type;     /*!< for RDI must be 0x0E */
			ModbusByte devID;     /*!< Read Device ID code */
			ModbusByte objID;     /*!< Object Id */

			ModbusCRC crc = { 0 };        /*!< контрольная сумма */

			// ------- to slave -------
			MEIMessageRDI( ModbusAddr addr, ModbusByte devID, ModbusByte objID );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			MEIMessageRDI( const ModbusMessage& m );
			MEIMessageRDI& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusByte) * 3;
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			static inline size_t szData()
			{
				return sizeof(ModbusByte) * 3 + szCRC;
			}

			// вспомогательные функции
			bool checkFormat() const;

		} __attribute__((packed));
		// -----------------------------------------------------------------------
		std::ostream& operator<<(std::ostream& os, MEIMessageRDI& m );
		std::ostream& operator<<(std::ostream& os, MEIMessageRDI* m );
		// -----------------------------------------------------------------------

		struct RDIObjectInfo
		{
			RDIObjectInfo(): id(0), val("") {}
			RDIObjectInfo( ModbusByte id, const std::string& v ): id(id), val(v) {}
			RDIObjectInfo( ModbusByte id, const ModbusByte* dat, ModbusByte len );

			ModbusByte id;
			std::string val;
		};

		using RDIObjectList = std::list<RDIObjectInfo>;

		/*! Ответ для 0x2B/0x0E */
		struct MEIMessageRetRDI:
			public ModbusHeader
		{
			ModbusByte type;     /*!< 0x0E */
			ModbusByte devID;     /*!< Read Device ID code */
			ModbusByte conformity; /*!< Conformity level (0x01 or 0x02 or 0x03 or 0x81 or 0x82 or 0x83) */
			ModbusByte mf;         /*!< More Follows (00/FF) */
			ModbusByte objID;     /*!< Object ID number */
			ModbusByte objNum;     /*!< Number of objects */

			RDIObjectList dlist;
			ModbusCRC crc = { 0 };

			// ------- from slave -------
			MEIMessageRetRDI();
			MEIMessageRetRDI( const ModbusMessage& m );
			MEIMessageRetRDI& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// предварительная инициализации, только заголовочной части, без данных
			void pre_init( const ModbusMessage& m );

			/*! размер предварительного заголовка (после основного до фактических данных) */
			static inline size_t szHead()
			{
				return sizeof(ModbusByte) * 6;
			}

			//        /*! узнать длину данных следующих за предварительным заголовком ( в байтах ) */
			//        static int getDataLen( ModbusMessage& m );

			// ------- to master -------
			MEIMessageRetRDI( ModbusAddr _from, ModbusByte devID, ModbusByte conformity, ModbusByte mf, ModbusByte objID );

			/*! добавление данных.
			 * \return TRUE - если удалось
			 * \return FALSE - если НЕ удалось
			*/
			bool addData( ModbusByte id, const std::string& value );
			bool addData( RDIObjectInfo& dat );

			/*! очистка данных */
			void clear();

			/*! проверка на переполнение */
			inline bool isFull() const
			{
				return ( bcnt >= MAXPDULEN );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			size_t bcnt = { 0 }; /*! размер данных в байтах, внутреннее служебное поле */
		};

		std::ostream& operator<<(std::ostream& os, MEIMessageRetRDI& m );
		std::ostream& operator<<(std::ostream& os, MEIMessageRetRDI* m );
		std::ostream& operator<<(std::ostream& os, RDIObjectList& dl );
		std::ostream& operator<<(std::ostream& os, RDIObjectList* dl );
		// -----------------------------------------------------------------------
		// -----------------------------------------------------------------------

		/*! Чтение информации об ошибке */
		struct JournalCommandMessage:
			public ModbusHeader
		{
			ModbusData cmd = { 0 };            /*!< код операции */
			ModbusData num = { 0 };            /*!< номер записи */
			ModbusCRC crc = { 0 };

			// -------------
			JournalCommandMessage( const ModbusMessage& m );
			JournalCommandMessage& operator=( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusByte) * 4 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, JournalCommandMessage& m );
		std::ostream& operator<<(std::ostream& os, JournalCommandMessage* m );
		// -----------------------------------------------------------------------
		/*! Ответ для запроса на чтение ошибки */
		struct JournalCommandRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };                    /*!< numbers of bytes */
			//        ModbusByte data[MAXLENPACKET-1];    /*!< данные */

			// В связи со спецификой реализации ответной части (т.е. modbus master)
			// данные приходится делать не байтовым потоком, а "словами"
			// которые в свою очередь будут перевёрнуты при посылке...
			ModbusData data[MAXLENPACKET / sizeof(ModbusData)];  /*!< данные */

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
			inline bool isFull() const
			{
				return ( count >= MAXDATALEN );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// Это поле не входит в стандарт modbus
			// оно вспомогательное и игнорируется при
			// преобразовании в ModbusMessage.
			// Делать что-типа memcpy(buf,this,sizeof(*this)); будет не верно.
			// Используйте специальную функцию transport_msg()
			size_t count = { 0 };    /*!< фактическое количество данных в сообщении */
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
			ModbusByte hour = { 0 };    /*!< часы [0..23] */
			ModbusByte min = { 0 };        /*!< минуты [0..59] */
			ModbusByte sec = { 0 };        /*!< секунды [0..59] */
			ModbusByte day = { 1 };        /*!< день [1..31] */
			ModbusByte mon = { 1 };        /*!< месяц [1..12] */
			ModbusByte year = { 0 };    /*!< год [0..99] */
			ModbusByte century = { 20 };    /*!< столетие [19-20] */

			ModbusCRC crc = { 0 };

			// ------- to slave -------
			SetDateTimeMessage( ModbusAddr addr );
			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// ------- from master -------
			SetDateTimeMessage( const ModbusMessage& m );
			SetDateTimeMessage& operator=( const ModbusMessage& m );
			SetDateTimeMessage();

			bool checkFormat() const;

			/*! размер данных(после заголовка) у данного типа сообщения */
			inline static size_t szData()
			{
				return sizeof(ModbusByte) * 7 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, SetDateTimeMessage& m );
		std::ostream& operator<<(std::ostream& os, SetDateTimeMessage* m );
		// -----------------------------------------------------------------------

		/*! Ответ (просто повторяет запрос) */
		struct SetDateTimeRetMessage:
			public SetDateTimeMessage
		{

			// ------- from slave -------
			SetDateTimeRetMessage( const ModbusMessage& m );
			SetDateTimeRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			// ------- to master -------
			SetDateTimeRetMessage( ModbusAddr _from );
			SetDateTimeRetMessage( const SetDateTimeMessage& query );
			static void cpy( SetDateTimeRetMessage& reply, const SetDateTimeMessage& query );

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();
		};
		// -----------------------------------------------------------------------

		/*! Вызов удалённого сервиса */
		struct RemoteServiceMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };    /*!< количество байт */

			/*! данные */
			ModbusByte data[MAXLENPACKET - sizeof(ModbusByte)];
			ModbusCRC crc = { 0 };        /*!< контрольная сумма */

			// -----------
			RemoteServiceMessage( const ModbusMessage& m );
			RemoteServiceMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusByte);    // bcnt
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, RemoteServiceMessage& m );
		std::ostream& operator<<(std::ostream& os, RemoteServiceMessage* m );
		// -----------------------------------------------------------------------
		struct RemoteServiceRetMessage:
			public ModbusHeader
		{
			ModbusByte bcnt = { 0 };    /*!< количество байт */
			/*! данные */
			ModbusByte data[MAXLENPACKET - sizeof(ModbusByte)];

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
			inline bool isFull() const
			{
				return ( count >= sizeof(data) );
			}

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();

			// Это поле не входит в стандарт modbus
			// оно вспомогательное и игнорируется при
			// преобразовании в ModbusMessage.
			size_t count = { 0 };    /*!< фактическое количество данных в сообщении */
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
			} __attribute__((packed));

			ModbusByte bcnt = { 0 };    /*!< количество байт 0x07 to 0xF5 */

			/*! данные */
			SubRequest data[MAXLENPACKET / sizeof(SubRequest) - sizeof(ModbusByte)];
			ModbusCRC crc = { 0 };        /*!< контрольная сумма */

			// -----------
			ReadFileRecordMessage( const ModbusMessage& m );
			ReadFileRecordMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! размер предварительного заголовка
			 * (после основного до фактических данных)
			*/
			static inline size_t szHead()
			{
				return sizeof(ModbusByte);    // bcnt
			}

			/*! узнать длину данных следующий за предварительным заголовком ( в байтах ) */
			static size_t getDataLen( const ModbusMessage& m );

			/*! проверка корректности данных */
			bool checkFormat() const;

			// это поле служебное и не используется в релальном обмене
			size_t count = { 0 }; /*!< фактическое количество данных */
		};

		std::ostream& operator<<(std::ostream& os, ReadFileRecordMessage& m );
		std::ostream& operator<<(std::ostream& os, ReadFileRecordMessage* m );
		// -----------------------------------------------------------------------

		struct FileTransferMessage:
			public ModbusHeader
		{
			ModbusData numfile = { 0 };     /*!< file number 0x0000 to 0xFFFF */
			ModbusData numpacket = { 0 };   /*!< number of packet */
			ModbusCRC crc = { 0 };          /*!< контрольная сумма */

			// ------- to slave -------
			FileTransferMessage( ModbusAddr addr, ModbusData numfile, ModbusData numpacket );
			ModbusMessage transport_msg();     /*!< преобразование для посылки в сеть */

			// ------- from master -------
			FileTransferMessage( const ModbusMessage& m );
			FileTransferMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );

			/*! размер данных(после заголовка) у данного типа сообщения */
			static inline size_t szData()
			{
				return sizeof(ModbusData) * 2 + szCRC;
			}

		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, FileTransferMessage& m );
		std::ostream& operator<<(std::ostream& os, FileTransferMessage* m );
		// -----------------------------------------------------------------------

		struct FileTransferRetMessage:
			public ModbusHeader
		{
			// 255 - max of bcnt...(1 byte)
			//        static const int MaxDataLen = 255 - szCRC - szModbusHeader - sizeof(ModbusData)*3 - sizeof(ModbusByte)*2;
			static const size_t MaxDataLen = MAXLENPACKET - sizeof(ModbusData) * 3 - sizeof(ModbusByte) * 2;

			ModbusByte bcnt;        /*!< общее количество байт в ответе */
			ModbusData numfile;     /*!< file number 0x0000 to 0xFFFF */
			ModbusData numpacks;     /*!< all count packages (file size) */
			ModbusData packet;      /*!< number of packet */
			ModbusByte dlen;        /*!< количество байт данных в ответе */
			ModbusByte data[MaxDataLen];


			// ------- from slave -------
			FileTransferRetMessage( const ModbusMessage& m );
			FileTransferRetMessage& operator=( const ModbusMessage& m );
			void init( const ModbusMessage& m );
			ModbusCRC crc = { 0 };
			static size_t szHead()
			{
				return sizeof(ModbusByte);
			}
			static size_t getDataLen( const ModbusMessage& m );

			// ------- to master -------
			FileTransferRetMessage( ModbusAddr _from );

			/*! Добавление данных
			    \warning Старые данные будут затёрты
			*/
			bool set( ModbusData numfile, ModbusData file_num_packets, ModbusData packet, ModbusByte* b, ModbusByte len );

			/*! очистка данных */
			void clear();

			/*! размер данных(после заголовка) у данного типа сообщения */
			size_t szData() const;

			/*! преобразование для посылки в сеть */
			ModbusMessage transport_msg();
		};

		std::ostream& operator<<(std::ostream& os, FileTransferRetMessage& m );
		std::ostream& operator<<(std::ostream& os, FileTransferRetMessage* m );
		// -----------------------------------------------------------------------
	} // end of ModbusRTU namespace
	// -------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif // ModbusTypes_H_
// ---------------------------------------------------------------------------
