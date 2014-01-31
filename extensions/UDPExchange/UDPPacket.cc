#include "UDPPacket.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetUDP;
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPHeader& p )
{
	return os << "nodeID=" << p.nodeID
				<< " procID=" << p.procID
				<< " dcount=" << p.dcount;
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
UDPMessage::UDPMessage()
{

}
// -----------------------------------------------------------------------------
void UDPMessage::addData( const UniSetUDP::UDPData& dat )
{
	dlist.push_back(dat);
}
// -----------------------------------------------------------------------------
void UDPMessage::addData( long id, long val)
{
	UDPData d(id,val);
	addData(d);
}
// -----------------------------------------------------------------------------
