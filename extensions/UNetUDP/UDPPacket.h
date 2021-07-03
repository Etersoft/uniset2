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
#include "proto/unet.pb.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    namespace UniSetUDP
    {
        const uint32_t UNETUDP_MAGICNUM = 0x13463C4; // идентификатор протокола

        const size_t MaxPacketNum = std::numeric_limits<size_t>::max();
        // Теоретический размер данных в UDP пакете (исключая заголовки) 65507
        // Желательно не вылезать за размер MTU (обычно 1500) - заголовки = 1432 байта
        // т.е. надо чтобы sizeof(UDPPacket) < 1432
        static const size_t MaxACount = 5000;
        static const size_t MaxDCount = 5000;
        static const size_t MessageBufSize = 34700;

        struct UDPMessage
        {
            UDPMessage();
            bool initFromBuffer( uint8_t* buf, size_t sz );
            std::string serializeAsString() const noexcept;
            size_t serializeToArray( uint8_t* buf, int sz ) const noexcept;

            bool isOk() const noexcept;
            uint32_t magic() const noexcept;

            void setNum( long num ) noexcept;
            long num()  const noexcept;

            void setNodeID( long num ) noexcept;
            long nodeID()  const noexcept;

            void setProcID( long num ) noexcept;
            long procID()  const noexcept;

            // \warning в случае переполнения возвращается MaxDCount
            size_t addDData( long id, bool val ) noexcept;

            //!\return true - successful
            bool setDData( size_t index, bool val ) noexcept;

            //! \return uniset::DefaultObjectId if not found
            long dID( size_t index ) const noexcept;
            bool dValue( size_t index ) const noexcept;

            //! \return uniset::DefaultObjectId if not found
            long aID(size_t i) const noexcept;
            long aValue(size_t i) const noexcept;

            // функции addAData возвращают индекс, по которому потом можно напрямую писать при помощи setAData(index)
            // \warning в случае переполнения возвращается MaxACount
            size_t addAData( long id, long val ) noexcept;

            //!\return true - successful
            bool setAData( size_t index, long val ) noexcept;

            long getDataID( ) const noexcept; /*!< получение "уникального" идентификатора данных этого пакета */

            bool isAFull() const noexcept;
            bool isDFull() const noexcept;
            bool isFull() const noexcept;

            size_t dsize() const noexcept;
            size_t asize() const noexcept;

            uint16_t dataCRC() const noexcept;
            uint16_t dataCRCWithBuf( uint8_t* buf, size_t sz ) const noexcept;

            unet::UNetPacket pb;
        };

        std::ostream& operator<<( std::ostream& os, UDPMessage& p );

        uint16_t makeCRC( unsigned char* buf, size_t len ) noexcept;
    }
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
