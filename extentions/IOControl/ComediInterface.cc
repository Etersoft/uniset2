// $Id: ComediInterface.cc,v 1.2 2009/01/16 23:16:41 vpashka Exp $
// -----------------------------------------------------------------------------
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
std::vector<lsampl_t> ComediInterface::getAnalogPacket( int subdev, int channel, int range, int aref )
							throw(UniSetTypes::Exception)
{
	lsampl_t* data = new lsampl_t[1024]; /* FIFO size, maximum possible samples */
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_READ;
	insn.n = 1024;
	insn.data = data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(channel, range, aref);

	int ret = comedi_do_insn(card, &insn);
	if( ret < 0 )
	{
		delete[] data;
	
		ostringstream err;
		err << "(ComediInterface:getAnalogPacket): can`t read data from subdev=" << subdev
			<< " channel=" << channel << " range=" << range <<" aref="<< aref
			<< " err: " << ret << " (" << strerror(ret) << ")";
		throw Exception(err.str());
//		return std::vector<lsampl_t>(0);
	}

	std::vector<lsampl_t> result(ret);
	if(ret > 0)
		memcpy(&result[0], data, ret * sizeof(lsampl_t));

	delete[] data;

	return result;
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
void ComediInterface::instrChannel( int subdev, int channel, const std::string instr,
				    std::vector<lsampl_t> args, int range, int aref )
				    throw(UniSetTypes::Exception)
{
	lsampl_t ins = instr2type(instr);
	comedi_insn insn;

	if(ins < 0)
	{
		ostringstream err;
		err << "(ComediInterface:instrChannel): unknown instruction "
			<< " subdev=" << subdev << " channel=" << channel << " instruction=" << instr;
		throw Exception(err.str());
	}

	args.insert(args.begin(), ins);

	memset(&insn,0,sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = args.size();
	insn.data = &args[0];
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(channel,range,aref);
	if( comedi_do_insn(card,&insn) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:instrChannel): can`t execute the instruction "
			<< " subdev=" << subdev << " channel=" << channel << " instruction=" << instr;
		throw Exception(err.str());
	}
	return;
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
	static const unsigned char chans[4] = {0, 8, 16, 20}; /* We can configure only one channel per 8-bit port (4-bit for CL and CH). */
	lsampl_t cmd[4]; /* Ports A, B, CL, CH */
	comedi_insn insn;

	switch(t)
	{
		case TBI24_0:
			cmd[0] = cmd[1] = cmd[2] = cmd[3] = INSN_CONFIG_DIO_INPUT;
			break;
		case TBI0_24:
		default:
			cmd[0] = cmd[1] = cmd[2] = cmd[3] = INSN_CONFIG_DIO_OUTPUT;
			break;
		case TBI16_8:
			cmd[0] = cmd[1] = INSN_CONFIG_DIO_INPUT;
			cmd[2] = cmd[3] = INSN_CONFIG_DIO_OUTPUT;
			break;
	}

	for(int i = 0; i < 4; i++) {
		memset(&insn,0,sizeof(insn));
		insn.insn = INSN_CONFIG;
		insn.n = 1;
		insn.data = &cmd[i];
		insn.subdev = subdev;
		insn.chanspec = CR_PACK(chans[i], 0, 0);
		if( comedi_do_insn(card,&insn) < 0 )
		{
			ostringstream err;
			err << "(ComediInterface:configureSubdev): can`t configure subdev "
				<< " subdev=" << subdev << " type=" << t;
			throw Exception(err.str());
		}
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
ComediInterface::EventType ComediInterface::event2type( const std::string s )
{
	if( s == "Front" )
		return Front;
		
	if( s == "Rear" )
		return Rear;
	
	if( s == "FrontThenRear" )
		return FrontThenRear;
		
	return No;
}
// -----------------------------------------------------------------------------
lsampl_t ComediInterface::instr2type( const std::string s )
{
	if( s == "AVERAGING" || s == "BOUNCE_SUPPRESSION" ) /* This are the same instructions, one for AI, another for DI */
		return INSN_CONFIG_AVERAGING;

	if( s == "TIMER" )
		return INSN_CONFIG_TIMER_1;

	if( s == "INPUT_MASK" )
		return INSN_CONFIG_INPUT_MASK;

	if( s == "FILTER" )
		return INSN_CONFIG_FILTER;

	if( s == "0mA" )
		return INSN_CONFIG_0MA;

	if( s == "COMPL" )
		return INSN_CONFIG_COMPL;

	if( s == "COUNTER" )
		return INSN_CONFIG_COUNTER;

	if( s == "DI_MODE" )
		return INSN_CONFIG_DI_MODE;

	return -1;
}
// -----------------------------------------------------------------------------
