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

    size_t nbyte = index / 8*sizeof(unsigned char);
    size_t nbit =  index % 8*sizeof(unsigned char);

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

    size_t nbyte = index / 8*sizeof(unsigned char);
    size_t nbit =  index % 8*sizeof(unsigned char);

    return ( d_dat[nbyte] & (1<<nbit) );
}
// -----------------------------------------------------------------------------
size_t UDPMessage::transport_msg( UDPPacket& p )
{
    memset(&p,0,sizeof(UDPPacket));

    size_t i = 0;
    memcpy(&(p.data[i]),this,sizeof(UDPHeader));
    i += sizeof(UDPHeader);

    // копируем аналоговые данные
    size_t sz = acount*sizeof(UDPAData);
    memcpy(&(p.data[i]),a_dat,sz);
    i += sz;

    // копируем булевые индексы
    sz = dcount*sizeof(long);
    memcpy(&(p.data[i]),d_id,sz);
    i += sz;

    // копируем булевые данные
    size_t nbyte = dcount / 8*sizeof(unsigned char);
    size_t nbit =  dcount % 8*sizeof(unsigned char);
    sz = nbit > 0 ? nbyte + 1 : nbyte;
    memcpy(&(p.data[i]),d_dat,sz);
    i += sz;

    p.len = i;
    return i;
}
// -----------------------------------------------------------------------------
UDPMessage::UDPMessage( UDPPacket& p )
{
    getMessage(*this,p);
}
// -----------------------------------------------------------------------------
size_t UDPMessage::getMessage( UDPMessage& m, UDPPacket& p )
{
    memset(&m,0,sizeof(m));

    size_t i = 0;
    memcpy(&m,&(p.data[i]),sizeof(UDPHeader));
    i += sizeof(UDPHeader);

    // копируем аналоговые данные
    size_t sz = m.acount*sizeof(UDPAData);
    if( sz > sizeof(m.a_dat) )
        sz = sizeof(m.a_dat);

    memcpy(m.a_dat,&(p.data[i]),sz);
    i += sz;

    // копируем булевые индексы
    sz = m.dcount*sizeof(long);
    if( sz > sizeof(m.d_id) )
        sz = sizeof(m.d_id);

    memcpy(m.d_id,&(p.data[i]),sz);
    i += sz;

    // копируем булевые данные
    size_t nbyte = m.dcount / 8*sizeof(unsigned char);
    size_t nbit =  m.dcount % 8*sizeof(unsigned char);
    sz = nbit > 0 ? nbyte + 1 : nbyte;

    if( sz > sizeof(m.d_dat) )
        sz = sizeof(m.d_dat);
    memcpy(m.d_dat,&(p.data[i]),sz);

    return i+sz;
}
// -----------------------------------------------------------------------------
