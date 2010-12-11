// $Id: UNetPacket.h,v 1.1 2009/02/10 20:38:27 vpashka Exp $
// -----------------------------------------------------------------------------
#ifndef UNetPacket_H_
#define UNetPacket_H_
// -----------------------------------------------------------------------------
#include <list>
#include <ostream>
#include "UniSetTypes.h"
// -----------------------------------------------------------------------------
namespace UniSetUNet
{
	struct UNetHeader
	{
		long nodeID;
		long procID;
		long dcount;
		
		friend std::ostream& operator<<( std::ostream& os, UNetHeader& p );
	}__attribute__((packed));
	
	struct UNetData
	{
		UNetData():id(UniSetTypes::DefaultObjectId),val(0){}
		UNetData(long id, long val):id(id),val(val){}

		long id;
		long val;
		
		friend std::ostream& operator<<( std::ostream& os, UNetData& p );
	}__attribute__((packed));
	
	struct UNetMessage:
		public UNetHeader
	{
		UNetMessage();

		void addData( const UNetData& dat );
		void addData( long id, long val );
		
		inline int size(){ return dlist.size(); }
		
		typedef std::list<UNetData> UNetDataList;
		UNetDataList dlist;
		
		friend std::ostream& operator<<( std::ostream& os, UNetMessage& p );
	};
}
// -----------------------------------------------------------------------------
#endif // UNetPacket_H_
// -----------------------------------------------------------------------------
