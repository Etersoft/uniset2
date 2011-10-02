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
				<< " acount=" << p.acount
				<< " pnum=" << p.num;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPHeader* p )
{
	return os << (*p);
}
// -----------------------------------------------------------------------------

std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPAData& p )
{
	return os << "id=" << p.id << " val=" << p.val;
}
// -----------------------------------------------------------------------------
std::ostream& UniSetUDP::operator<<( std::ostream& os, UniSetUDP::UDPMessage& p )
{
	os << (UDPHeader*)(&p) << endl;

	os << "DIGITAL:" << endl;
	for( size_t i=0; i<p.msg.header.dcount; i++ )
		os << "[" << i << "]={" << p.dID(i) << "," << p.dValue(i) << "}" << endl;

	os << "ANALOG:" << endl;
	for( size_t i=0; i<p.msg.header.acount; i++ )
		os << "[" << i << "]={" << p.msg.a_dat[i].id << "," << p.msg.a_dat[i].val << "}" << endl;

	return os;
}
// -----------------------------------------------------------------------------
UDPMessage::UDPMessage()
{
}
// -----------------------------------------------------------------------------
size_t UDPMessage::addAData( const UniSetUDP::UDPAData& dat )
{
	if( msg.header.acount >= MaxACount )
		return MaxACount;

	msg.a_dat[msg.header.acount] = dat;
	msg.header.acount++;
	return msg.header.acount-1;
}
// -----------------------------------------------------------------------------
size_t UDPMessage::addAData( long id, long val)
{
	UDPAData d(id,val);
	return addAData(d);
}
// -----------------------------------------------------------------------------
bool UDPMessage::setAData( size_t index, long val )
{
	if( index < MaxACount )
	{
		msg.a_dat[index].val = val;
		return true;
	}

	return false;
}
// -----------------------------------------------------------------------------
size_t UDPMessage::addDData( long id, bool val )
{
	if( msg.header.dcount >= MaxDCount )
		return MaxDCount;

	// сохраняем ID
	msg.d_id[msg.header.dcount] = id;

	bool res = setDData( msg.header.dcount, val );
	if( res )
	{
		msg.header.dcount++;
		return msg.header.dcount-1;
	}

	return MaxDCount;
}
// -----------------------------------------------------------------------------
bool UDPMessage::setDData( size_t index, bool val )
{
	if( index >= MaxDCount )
		return false;

	size_t nbyte = index / sizeof(unsigned char);
	size_t nbit =  index % sizeof(unsigned char);

	// выставляем бит
	unsigned char d = msg.d_dat[nbyte];
	if( val )
		d |= (1<<nbit);
	else
		d &= ~(1<<nbit);

	msg.d_dat[nbyte] = d;
	return true;
}
// -----------------------------------------------------------------------------
long UDPMessage::dID( size_t index )
{
	if( index >= MaxDCount )
		return UniSetTypes::DefaultObjectId;

	return msg.d_id[index];
}
// -----------------------------------------------------------------------------
bool UDPMessage::dValue( size_t index )
{
	if( index >= MaxDCount )
		return UniSetTypes::DefaultObjectId;

	size_t nbyte = index / sizeof(unsigned char);
	size_t nbit =  index % sizeof(unsigned char);

	return ( msg.d_dat[nbyte] & (1<<nbit) );
}
// -----------------------------------------------------------------------------

