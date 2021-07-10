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
        /*! С учётом того, что ID могут идти не подряд. Сделан следующий формат:
            Для аналоговых величин передаётся массив пар "id-value"(UDPAData).
            Для булевых величин - отдельно массив ID и отдельно битовый массив со значениями,
            (по количеству битов такого же размера).
            \warning Пакет UDPMessage передаётся всегда полностью, независимо от того, насколько он наполнен датчиками.
            \warning ТЕКУЩАЯ ВЕРСИЯ ПРОТОКОЛА НЕ БУДЕТ РАБОТАТЬ МЕЖДУ 32-битными и 64-битными системами (из-за отличия в типе long).
            т.к. это не сильно актуально, пока не переделываю.

            "ByteOrder"
            ============
            В текущей версии протокола. В UDPHeader содержится информации о порядке байт.
            Поэтому логика следующая:
            - Узел который посылает, ничего не перекодирует и просто посылает данные так как хранит
            (информация о порядке байт, если специально не выставить, будет выставлена при компиляции, см. конструктор)
            - Узел который принимает данные, декодирует их, если на его узле порядок байт не совпадает.
            Т.е. если все узлы будут иметь одинаковый порядок байт, фактического перекодирования не будет.
        */

        const uint32_t UNETUDP_MAGICNUM = 0x1343EFD; // идентификатор протокола

        struct UDPHeader
        {
            UDPHeader() noexcept;
            uint32_t magic;
            uint8_t _be_order; // 1 - BE byte order, 0 - LE byte order
            size_t num;
            int64_t nodeID;
            int64_t procID;
            size_t dcount; /*!< количество булевых величин */
            size_t acount; /*!< количество аналоговых величин */
        } __attribute__((packed));

        std::ostream& operator<<( std::ostream& os, UDPHeader& p );
        std::ostream& operator<<( std::ostream& os, UDPHeader* p );

        const size_t MaxPacketNum = std::numeric_limits<size_t>::max();

        struct UDPAData
        {
            UDPAData() noexcept: id(uniset::DefaultObjectId), val(0) {}
            UDPAData(int64_t id, int64_t val) noexcept: id(id), val(val) {}

            int64_t id;
            int64_t val;

        } __attribute__((packed));

        std::ostream& operator<<( std::ostream& os, UDPAData& p );

        // Теоретический размер данных в UDP пакете (исключая заголовки) 65507
        // Фактически желательно не вылезать за размер MTU (обычно 1500) - заголовки = 1432 байта
        // т.е. надо чтобы sizeof(UDPPacket) < 1432
        // При текущих настройках sizeof(UDPPacket) = 56421 (!)
        static const size_t MaxACount = 2000;
        static const size_t MaxDCount = 3000;
        static const size_t MaxDDataCount = 1 + MaxDCount / 8 * sizeof(uint8_t);

        struct UDPMessage
        {
            // net to host
            void ntoh() noexcept;
            bool isOk() noexcept;

            // \warning в случае переполнения возвращается MaxDCount
            size_t addDData( int64_t id, bool val ) noexcept;

            //!\return true - successful
            bool setDData( size_t index, bool val ) noexcept;

            //! \return uniset::DefaultObjectId if not found
            long dID( size_t index ) const noexcept;

            //! \return uniset::DefaultObjectId if not found
            bool dValue( size_t index ) const noexcept;

            // функции addAData возвращают индекс, по которому потом можно напрямую писать при помощи setAData(index)
            // \warning в случае переполнения возвращается MaxACount
            size_t addAData( const UDPAData& dat ) noexcept;
            size_t addAData( int64_t id, int64_t val ) noexcept;

            //!\return true - successful
            bool setAData( size_t index, int64_t val ) noexcept;

            long getDataID( ) const noexcept; /*!< получение "уникального" идентификатора данных этого пакета */

            inline bool isAFull() const noexcept
            {
                return (header.acount >= MaxACount);
            }
            inline bool isDFull() const noexcept
            {
                return (header.dcount >= MaxDCount);
            }

            inline bool isFull() const noexcept
            {
                return !((header.dcount < MaxDCount) && (header.acount < MaxACount));
            }

            inline size_t dsize() const noexcept
            {
                return header.dcount;
            }

            inline size_t asize() const noexcept
            {
                return header.acount;
            }

            uint16_t getDataCRC() const noexcept;

            UDPHeader header;
            UDPAData a_dat[MaxACount]; /*!< аналоговые величины */
            int64_t d_id[MaxDCount];      /*!< список дискретных ID */
            uint8_t d_dat[MaxDDataCount];  /*!< битовые значения */
        } __attribute__((packed));

        std::ostream& operator<<( std::ostream& os, UDPMessage& p );

        uint16_t makeCRC( unsigned char* buf, size_t len ) noexcept;
    }
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
