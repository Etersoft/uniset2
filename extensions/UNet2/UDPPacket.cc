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
	for( size_t i=0; i<p.dcount; i++ )
		os << "[" << i << "]={" << p.dID(i) << "," << p.dValue(i) << "}" << endl;
	
	os << "ANALOG:" << endl;
	for( size_t i=0; i<p.acount; i++ )
		os << "[" << i << "]={" << p.a_dat[i].id << "," << p.a_dat[i].val << "}" << endl;
	
	return os;
}
// -----------------------------------------------------------------------------
UDPMessage::UDPMessage()
{
}
// -----------------------------------------------------------------------------
size_t UDPMessage::addAData( const UniSetUDP::UDPAData& dat )
{
	if( acount >= MaxACount )
		return MaxACount;

	a_dat[acount] = dat;
	acount++;
	return acount-1;
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
		a_dat[index].val = val;
		return true;
	}
	
	return false;
}
// -----------------------------------------------------------------------------
size_t UDPMessage::addDData( long id, bool val )
{
	if( dcount >= MaxDCount )
		return MaxDCount;

	// сохраняем ID
	d_id[dcount] = id;
	
	bool res = setDData( dcount, val );
	if( res )
	{
		dcount++;
		return dcount-1;
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
	unsigned char d = d_dat[nbyte];
	if( val )
		d |= (1<<nbit);
	else 
		d &= ~(1<<nbit);

	d_dat[nbyte] = d;
	return true;	
}
// -----------------------------------------------------------------------------
long UDPMessage::dID( size_t index )
{
	if( index >= MaxDCount )
		return UniSetTypes::DefaultObjectId;

	return d_id[index];
}
// -----------------------------------------------------------------------------
bool UDPMessage::dValue( size_t index )
{
	if( index >= MaxDCount )
		return UniSetTypes::DefaultObjectId;

	size_t nbyte = index / sizeof(unsigned char);
	size_t nbit =  index % sizeof(unsigned char);

	return ( d_dat[nbyte] & (1<<nbit) );
}
// -----------------------------------------------------------------------------

