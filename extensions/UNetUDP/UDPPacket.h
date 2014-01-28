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
        Сделан следующие формат:
        Для аналоговых величин передаётся массив пар "id-value".
        Для булевых величин - отдельно массив ID и отдельно битовый массив со значениями,
        (по количеству битов такого же размера).
    */

    struct UDPHeader
    {
        UDPHeader():num(0),nodeID(0),procID(0),dcount(0),acount(0){}
        unsigned long num;
        long nodeID;
        long procID;
        size_t dcount; /*!< количество булевых величин */
        size_t acount; /*!< количество аналоговых величин */

        friend std::ostream& operator<<( std::ostream& os, UDPHeader& p );
        friend std::ostream& operator<<( std::ostream& os, UDPHeader* p );
    }__attribute__((packed));

    static unsigned long MaxPacketNum = std::numeric_limits<unsigned long>::max();

    struct UDPAData
    {
        UDPAData():id(UniSetTypes::DefaultObjectId),val(0){}
        UDPAData(long id, long val):id(id),val(val){}

        long id;
        long val;

        friend std::ostream& operator<<( std::ostream& os, UDPAData& p );
    }__attribute__((packed));

    // Хотелось бы не вылезать за общий размер посылаемых пакетов 8192. (550,900 --> 8133)

    static const size_t MaxACount = 550;
    static const size_t MaxDCount = 900;
    static const size_t MaxDDataCount = 1 + MaxDCount / 8*sizeof(unsigned char);

    struct UDPPacket
    {
        UDPPacket():len(0){}

        int len;
        unsigned char data[ sizeof(UDPHeader) + MaxDCount*sizeof(long) + MaxDDataCount + MaxACount*sizeof(UDPAData) ];
    }__attribute__((packed));

    static const int MaxDataLen = sizeof(UDPPacket);

    struct UDPMessage:
        public UDPHeader
    {
        UDPMessage();

        UDPMessage( UDPPacket& p );
         size_t transport_msg( UDPPacket& p );
        static size_t getMessage( UDPMessage& m, UDPPacket& p );

        size_t addDData( long id, bool val );
        bool setDData( size_t index, bool val );
        long dID( size_t index );
        bool dValue( size_t index );

        size_t addAData( const UDPAData& dat );
        size_t addAData( long id, long val );
        bool setAData( size_t index, long val );

        inline bool isFull(){ return ((dcount<MaxDCount) && (acount<MaxACount)); }
        inline int dsize(){ return dcount; }
        inline int asize(){ return acount; }
//        inline int byte_size(){ return (dcount*sizeof(long)*UDPDData) + acount*sizeof(UDPAData)); }

        // количество байт в пакете с булевыми переменными...
        int d_byte(){ return dcount*sizeof(long) + dcount; }

        UDPAData a_dat[MaxACount]; /*!< аналоговые величины */
        long d_id[MaxDCount];      /*!< список дискретных ID */
        unsigned char d_dat[MaxDDataCount];  /*!< битовые значения */

        friend std::ostream& operator<<( std::ostream& os, UDPMessage& p );
    };
}
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
