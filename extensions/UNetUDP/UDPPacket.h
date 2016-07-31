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
#ifndef UDPPacket_H_
#define UDPPacket_H_
// -----------------------------------------------------------------------------
#include <list>
#include <limits>
#include <ostream>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
namespace UniSetUDP
{
	/*! Для оптимизации размера передаваемх данных, но с учётом того, что ID могут идти не подряд.
	    Сделан следующий формат:
	    Для аналоговых величин передаётся массив пар "id-value"(UDPAData).
	    Для булевых величин - отдельно массив ID и отдельно битовый массив со значениями,
	    (по количеству битов такого же размера).

	    \todo Подумать на тему сделать два отдельных вида пакетов для булевых значений и для аналоговых,
	          чтобы уйти от преобразования UDPMessage --> UDPPacket --> UDPMessage.

		\warning ТЕКУЩАЯ ВЕРСИЯ ПРОТОКОЛА НЕ БУДЕТ РАБОТАТЬ МЕЖДУ 32-битными и 64-битными системами (из-за отличия в типе long).
		т.к. это не сильно актуально, пока не переделываю.
	*/

	const uint32_t UNETUDP_MAGICNUM = 0x1337A1D; // идентификатор протокола

	struct UDPHeader
	{
		UDPHeader(): magic(UNETUDP_MAGICNUM), num(0), nodeID(0), procID(0), dcount(0), acount(0) {}
		uint32_t magic;
		size_t num;
		long nodeID;
		long procID;

		size_t dcount; /*!< количество булевых величин */
		size_t acount; /*!< количество аналоговых величин */

		friend std::ostream& operator<<( std::ostream& os, UDPHeader& p );
		friend std::ostream& operator<<( std::ostream& os, UDPHeader* p );
	} __attribute__((packed));

	const size_t MaxPacketNum = std::numeric_limits<size_t>::max();

	struct UDPAData
	{
		UDPAData(): id(UniSetTypes::DefaultObjectId), val(0) {}
		UDPAData(long id, long val): id(id), val(val) {}

		long id;
		long val;

		friend std::ostream& operator<<( std::ostream& os, UDPAData& p );
	} __attribute__((packed));

	// Хотелось бы не вылезать за общий размер посылаемых пакетов 8192. (550,900 --> 8133)
	// ------
	// временное резрешение на A=800,D=5000! DI/DO
	// 1500*8 + 5000*4 + 5000/8 = 32625 байт максимальный размер данных + служебные заголовки

	static const size_t MaxACount = 1500;
	static const size_t MaxDCount = 5000;
	static const size_t MaxDDataCount = 1 + MaxDCount / 8 * sizeof(unsigned char);

	struct UDPPacket
	{
		UDPPacket(): len(0) {}

		size_t len;
		uint8_t data[ sizeof(UDPHeader) + MaxDCount * sizeof(long) + MaxDDataCount + MaxACount * sizeof(UDPAData) ];
	} __attribute__((packed));

	static const size_t MaxDataLen = sizeof(UDPPacket);

	struct UDPMessage:
		public UDPHeader
	{
		UDPMessage();

		explicit UDPMessage( UDPPacket& p );
		size_t transport_msg( UDPPacket& p );
		static size_t getMessage( UDPMessage& m, UDPPacket& p );

		size_t addDData( long id, bool val );
		bool setDData( size_t index, bool val );
		long dID( size_t index ) const;
		bool dValue( size_t index ) const;

		// функции addAData возвращают индекс, по которому потом можно напрямую писать при помощи setAData(index)
		size_t addAData( const UDPAData& dat );
		size_t addAData( long id, long val );
		bool setAData( size_t index, long val );

		long getDataID( ) const; /*!< получение "уникального" идентификатора данных этого пакета */
		inline bool isAFull() const
		{
			return (acount >= MaxACount);
		}
		inline bool isDFull() const
		{
			return (dcount >= MaxDCount);
		}

		inline bool isFull() const
		{
			return !((dcount < MaxDCount) && (acount < MaxACount));
		}
		inline size_t dsize() const
		{
			return dcount;
		}
		inline size_t asize() const
		{
			return acount;
		}

		uint16_t getDataCRC() const;

		// количество байт в пакете с булевыми переменными...
		size_t d_byte() const
		{
			return dcount * sizeof(long) + dcount;
		}

		UDPAData a_dat[MaxACount]; /*!< аналоговые величины */
		long d_id[MaxDCount];      /*!< список дискретных ID */
		uint8_t d_dat[MaxDDataCount];  /*!< битовые значения */

		friend std::ostream& operator<<( std::ostream& os, UDPMessage& p );
	};

	uint16_t makeCRC( unsigned char* buf, size_t len );
}
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
