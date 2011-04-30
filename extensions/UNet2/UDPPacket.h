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
	struct UDPHeader
	{
		UDPHeader():num(0),nodeID(0),procID(0),dcount(0){}
		unsigned long num;
		long nodeID;
		long procID;
		size_t dcount;
		
		friend std::ostream& operator<<( std::ostream& os, UDPHeader& p );
	}__attribute__((packed));

	static unsigned long MaxPacketNum = std::numeric_limits<unsigned long>::max();
	
	struct UDPData
	{
		UDPData():id(UniSetTypes::DefaultObjectId),val(0){}
		UDPData(long id, long val):id(id),val(val){}

		long id;
		long val;
		
		friend std::ostream& operator<<( std::ostream& os, UDPData& p );
	}__attribute__((packed));

	static const int MaxDataLen = 8192; // ~ 1000 параметров
	static const int MaxDataCount = ( MaxDataLen - sizeof(UniSetUDP::UDPHeader) ) / sizeof(UDPData);

	 struct DataPacket
	 {
		UDPHeader header;
		UDPData dat[MaxDataCount];
	 }__attribute__((packed));


	struct UDPMessage:
		public UDPHeader
	{
		UDPMessage();

		bool addData( const UDPData& dat );
		bool addData( long id, long val );
		bool setData( unsigned int index, long val );

		inline bool isFull(){ return count<MaxDataCount; }
		inline int size(){ return count; }
		inline int byte_size(){ return count*sizeof(UDPData); }
		
		DataPacket msg;
		int count;
		
		friend std::ostream& operator<<( std::ostream& os, UDPMessage& p );
	};
}
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
