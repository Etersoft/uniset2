#include "UDPPacket.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetUDP;
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPHeader& p )
{
	return os << "nodeID=" << p.nodeID 
				<< " procID=" << p.procID 
				<< " dcount=" << p.dcount
				<< " pnum=" << p.num;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPData& p )
{
	return os << "id=" << p.id << " val=" << p.val;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPMessage& p )
{
	return os;
}
// -----------------------------------------------------------------------------
UDPMessage::UDPMessage():
count(0)
{
}
// -----------------------------------------------------------------------------
bool UDPMessage::addData( const UniSetUDP::UDPData& dat )
{
	if( count >= MaxDataCount )
		return false;

	msg.dat[count] = dat;
	count++;
	msg.header.dcount = count;
	return true;
}
// -----------------------------------------------------------------------------
bool UDPMessage::addData( long id, long val)
{
	UDPData d(id,val);
	return addData(d);
}
// -----------------------------------------------------------------------------
bool UDPMessage::setData( unsigned int index, long val )
{
	if( index < MaxDataCount )
	{
		msg.dat[index].val = val;
		return true;
	}
	
	return false;
}
// -----------------------------------------------------------------------------
