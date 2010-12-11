#include "UNetPacket.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetUNet;
// -----------------------------------------------------------------------------
std::ostream& UniSetUNet::operator<<( std::ostream& os, UniSetUNet::UNetHeader& p )
{
	return os << "nodeID=" << p.nodeID 
				<< " procID=" << p.procID 
				<< " dcount=" << p.dcount;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUNet::operator<<( std::ostream& os, UniSetUNet::UNetData& p )
{
	return os << "id=" << p.id << " val=" << p.val;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUNet::operator<<( std::ostream& os, UniSetUNet::UNetMessage& p )
{
	return os;
}
// -----------------------------------------------------------------------------
UNetMessage::UNetMessage()
{

}
// -----------------------------------------------------------------------------
void UNetMessage::addData( const UniSetUNet::UNetData& dat )
{
	dlist.push_back(dat);
}
// -----------------------------------------------------------------------------
void UNetMessage::addData( long id, long val)
{
	UNetData d(id,val);
	addData(d);
}
// -----------------------------------------------------------------------------
