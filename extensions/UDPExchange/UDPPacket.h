// $Id: UDPPacket.h,v 1.1 2009/02/10 20:38:27 vpashka Exp $
// -----------------------------------------------------------------------------
#ifndef UDPPacket_H_
#define UDPPacket_H_
// -----------------------------------------------------------------------------
#include <list>
#include <ostream>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
namespace UniSetUDP
{
	struct UDPHeader
	{
		long nodeID;
		long procID;
		long dcount;
		
		friend std::ostream& operator<<( std::ostream& os, UDPHeader& p );
	}__attribute__((packed));
	
	struct UDPData
	{
		UDPData():id(UniSetTypes::DefaultObjectId),val(0){}
		UDPData(long id, long val):id(id),val(val){}

		long id;
		long val;
		
		friend std::ostream& operator<<( std::ostream& os, UDPData& p );
	}__attribute__((packed));
	
	struct UDPMessage:
		public UDPHeader
	{
		UDPMessage();

		void addData( const UDPData& dat );
		void addData( long id, long val );
		
		inline int size(){ return dlist.size(); }
		
		typedef std::list<UDPData> UDPDataList;
		UDPDataList dlist;
		
		friend std::ostream& operator<<( std::ostream& os, UDPMessage& p );
	};
}
// -----------------------------------------------------------------------------
#endif // UDPPacket_H_
// -----------------------------------------------------------------------------
