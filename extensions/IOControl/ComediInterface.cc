#include <sstream>
#include <cstdlib>
#include <cstring>
#include "ComediInterface.h"
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
ComediInterface::ComediInterface( const std::string dev ):
	card(0),
	dname(dev)
{
	card = comedi_open(dev.c_str());
	if( card == 0 )
	{
		ostringstream err;
		err << "(ComediInterface): can`t open comedi device: " << dev;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
ComediInterface::~ComediInterface()
{

}
// -----------------------------------------------------------------------------
int ComediInterface::getAnalogChannel( int subdev, int channel, int range, int aref )
															throw(UniSetTypes::Exception)
{
	lsampl_t data = 0;
	int ret = comedi_data_read(card, subdev, channel, range, aref, &data);
	if( ret < 0 ) 
	{
		ostringstream err;
		err << "(ComediInterface:getAnalogChannel): can`t read data from subdev=" << subdev
			<< " channel=" << channel << " range=" << range <<" aref="<< aref
			<< " err: " << ret << " (" << strerror(ret) << ")";
		throw Exception(err.str());
	}

	return data;
}
// -----------------------------------------------------------------------------
void ComediInterface::setAnalogChannel( int subdev, int channel, int data, int range, int aref )
																throw(UniSetTypes::Exception)
{
	if( comedi_data_write(card, subdev, channel, range, aref, data) < 0 ) 
	{
		ostringstream err;
		err << "(ComediInterface:setAnalogChannel): can`t write data=" << data 
			<< " TO subdev=" << subdev
			<< " channel=" << channel << " range=" << range;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
bool ComediInterface::getDigitalChannel( int subdev, int channel ) throw(UniSetTypes::Exception)
{
	lsampl_t data = 0;

	if( comedi_dio_read(card, subdev, channel, &data) < 0 ) 
	{
		ostringstream err;
		err << "(ComediInterface:getDigitalChannel): can`t read bit from subdev=" << subdev
			<< " channel=" << channel;
		throw Exception(err.str());
	}

	return ((bool)(data));
}
// -----------------------------------------------------------------------------
void ComediInterface::setDigitalChannel( int subdev, int channel, bool bit )
													throw(UniSetTypes::Exception)
{
	if( comedi_dio_write(card, subdev, channel,bit) < 0 ) 
	{
		ostringstream err;
		err << "(ComediInterface:setDigitalChannel): can`t write bit=" << bit
			<< " TO subdev=" << subdev << " channel=" << channel;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
void ComediInterface::configureChannel( int subdev, int channel, ChannelType t,
										int range, int aref )
														throw(UniSetTypes::Exception)
{
	switch( t )
	{
		case ComediInterface::DI:
		case ComediInterface::DO:
		{
			if( comedi_dio_config(card,subdev,channel,t) < 0 )
			{
				ostringstream err;
				err << "(ComediInterface:configureChannel): can`t configure (DIO) "
					<< " subdev=" << subdev << " channel=" << channel << " type=" << t;
				throw Exception(err.str());
			}

			return;
		}
		break;

		case ComediInterface::AI:
		case ComediInterface::AO:
		{
			comedi_insn insn;
			lsampl_t iotype = t;
			memset(&insn,0,sizeof(insn));
			insn.insn = INSN_CONFIG;
			insn.n = 1;
			insn.data = &iotype;
			insn.subdev = subdev;
			insn.chanspec = CR_PACK(channel,range,aref);
			if( comedi_do_insn(card,&insn) < 0 )
			{
				ostringstream err;
				err << "(ComediInterface:configureChannel): can`t configure (AIO) "
					<< " subdev=" << subdev << " channel=" << channel << " type=" << t;
				throw Exception(err.str());
			}
			return;
		}
		break; 
		
		default:
			break;
	}
	
	ostringstream err;
	err << "(ComediInterface:configureChannel): can`t configure (DIO) "
		<< " subdev=" << subdev << " channel=" << channel << " type=" << t;
	throw Exception(err.str());
}
// -----------------------------------------------------------------------------
void ComediInterface::configureSubdev( int subdev, SubdevType t )	
								throw(UniSetTypes::Exception)
{
	lsampl_t cmd = 102;
	comedi_insn insn;
	memset(&insn,0,sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 1;
	insn.data 		= &cmd;
	insn.unused[0] 	= t;
	insn.subdev = subdev;
	insn.chanspec = 0;
	if( comedi_do_insn(card,&insn) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:configureSubdev): can`t configure subdev "
			<< " subdev=" << subdev << " type=" << t;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
std::string ComediInterface::type2str( ComediInterface::SubdevType t )
{
	switch(t)
	{
		case TBI24_0:
			return "TBI24_0";
			
		case TBI0_24:
			return "TBI0_24";
		
		case TBI16_8:
			return "TBI16_8";

		default:
			break;
	}
	
	return "";
}
// -----------------------------------------------------------------------------
ComediInterface::SubdevType ComediInterface::str2type( const std::string s )
{
	if( s == "TBI24_0" )
		return TBI24_0;
		
	if( s == "TBI0_24" )
		return TBI0_24;
	
	if( s == "TBI16_8" )
		return TBI16_8;
		
	return Unknown;
}
// -----------------------------------------------------------------------------
