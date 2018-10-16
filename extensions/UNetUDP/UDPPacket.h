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
// --------------------------------------------------------------------------
namespace uniset
{
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

			"ByteOrder"
			============
			В текущей версии протокола. В UDPHeader содержиться информации о порядке байт.
			Поэтому логика следующая:
			- Узел который посылает, ничего не перекодирует и просто посылает данные так как хранит
			(информация о порядке байт, если специально не выставить, будет выставлена при компиляции, см. конструктор)
			- Узел который принимает данные, декодирует их, если на его узле порядок байт не совпадает.
			Т.е. если все узлы будут иметь одинаковый порядок байт, фактического перекодирования не будет.
		*/

		const uint32_t UNETUDP_MAGICNUM = 0x133EF54; // идентификатор протокола

		struct UDPHeader
		{
			UDPHeader() noexcept;
			uint32_t magic;
			u_char _be_order; // 1 - BE byte order, 0 - LE byte order
			size_t num;
			long nodeID;
			long procID;

			size_t dcount; /*!< количество булевых величин */
			size_t acount; /*!< количество аналоговых величин */

		} __attribute__((packed));

		std::ostream& operator<<( std::ostream& os, UDPHeader& p );
		std::ostream& operator<<( std::ostream& os, UDPHeader* p );

		const size_t MaxPacketNum = std::numeric_limits<size_t>::max();

		struct UDPAData
		{
			UDPAData() noexcept: id(uniset::DefaultObjectId), val(0) {}
			UDPAData(long id, long val) noexcept: id(id), val(val) {}

			long id;
			long val;

		} __attribute__((packed));

		std::ostream& operator<<( std::ostream& os, UDPAData& p );

		// Теоретический размер данных в UDP пакете (исключая заголовки) 65507
		// Фактически желательно не вылезать за размер MTU (обычно 1500) - заголовки = 1432 байта
		// т.е. надо чтобы sizeof(UDPPacket) < 1432
		// с другой стороны в текущей реализации
		// в сеть посылается фактическое количество данных, а не sizeof(UDPPacket).

		// При текущих настройках sizeof(UDPPacket) = 72679 (!)
		static const size_t MaxACount = 2000;
		static const size_t MaxDCount = 5000;
		static const size_t MaxDDataCount = 1 + MaxDCount / 8 * sizeof(unsigned char);

		struct UDPPacket
		{
			UDPPacket() noexcept: len(0) {}

			size_t len;
			uint8_t data[ sizeof(UDPHeader) + MaxDCount * sizeof(long) + MaxDDataCount + MaxACount * sizeof(UDPAData) ];
		} __attribute__((packed));

		static const size_t MaxDataLen = sizeof(UDPPacket);

		struct UDPMessage:
			public UDPHeader
		{
			UDPMessage() noexcept;

			UDPMessage(UDPMessage&& m) noexcept = default;
			UDPMessage& operator=(UDPMessage&&) noexcept = default;

			UDPMessage( const UDPMessage& m ) noexcept = default;
			UDPMessage& operator=(const UDPMessage&) noexcept = default;

			explicit UDPMessage( UDPPacket& p ) noexcept;
			size_t transport_msg( UDPPacket& p ) const noexcept;

			static size_t getMessage( UDPMessage& m, UDPPacket& p ) noexcept;

			// \warning в случае переполнения возвращается MaxDCount
			size_t addDData( long id, bool val ) noexcept;

			//!\return true - successful
			bool setDData( size_t index, bool val ) noexcept;

			//! \return uniset::DefaultObjectId if not found
			long dID( size_t index ) const noexcept;

			//! \return uniset::DefaultObjectId if not found
			bool dValue( size_t index ) const noexcept;

			// функции addAData возвращают индекс, по которому потом можно напрямую писать при помощи setAData(index)
			// \warning в случае переполнения возвращается MaxACount
			size_t addAData( const UDPAData& dat ) noexcept;
			size_t addAData( long id, long val ) noexcept;

			//!\return true - successful
			bool setAData( size_t index, long val ) noexcept;

			long getDataID( ) const noexcept; /*!< получение "уникального" идентификатора данных этого пакета */

			inline bool isAFull() const noexcept
			{
				return (acount >= MaxACount);
			}
			inline bool isDFull() const noexcept
			{
				return (dcount >= MaxDCount);
			}

			inline bool isFull() const noexcept
			{
				return !((dcount < MaxDCount) && (acount < MaxACount));
			}

			inline size_t dsize() const noexcept
			{
				return dcount;
			}

			inline size_t asize() const noexcept
			{
				return acount;
			}

			// размер итогового пакета в байтах
			size_t sizeOf() const noexcept;

			uint16_t getDataCRC() const noexcept;

			// количество байт в пакете с булевыми переменными...
			size_t d_byte() const noexcept
			{
				return dcount * sizeof(long) + dcount;
			}

			UDPAData a_dat[MaxACount]; /*!< аналоговые величины */
			long d_id[MaxDCount];      /*!< список дискретных ID */
			uint8_t d_dat[MaxDDataCount];  /*!< битовые значения */
		};

		std::ostream& operator<<( std::ostream& os, UDPMessage& p );

		uint16_t makeCRC( unsigned char* buf, size_t len ) noexcept;
	}
	// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
