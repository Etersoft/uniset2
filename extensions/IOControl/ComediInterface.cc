/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <sstream>
#include <cstdlib>
#include <cstring>
#include "ComediInterface.h"
// -----------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------
ComediInterface::ComediInterface( const std::string& dev, const std::string& cname ):
	card(0),
	dname(dev),
	name(cname)
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
int ComediInterface::getAnalogChannel( int subdev, int channel, int range, int aref, int adelay ) const
{
	lsampl_t data = 0;
	int ret = comedi_data_read_delayed(card, subdev, channel, range, aref, &data, adelay);

	if( ret < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:getAnalogChannel): can`t read data from subdev=" << subdev
			<< " channel=" << channel << " range=" << range << " aref=" << aref
			<< " dev=" << dname
			<< " err: " << ret << " (" << strerror(ret) << ")";
		throw uniset::Exception(err.str());
	}

	return data;
}
// -----------------------------------------------------------------------------
void ComediInterface::setAnalogChannel( int subdev, int channel, int data, int range, int aref ) const
{
	if( comedi_data_write(card, subdev, channel, range, aref, data) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:setAnalogChannel): can`t write data=" << data
			<< " TO subdev=" << subdev
			<< " channel=" << channel << " range=" << range
			<< " dev=" << dname;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
bool ComediInterface::getDigitalChannel( int subdev, int channel ) const
{
	lsampl_t data = 0;

	if( comedi_dio_read(card, subdev, channel, &data) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:getDigitalChannel): can`t read bit from subdev=" << subdev
			<< " channel=" << channel << " dev=" << dname;
		throw Exception(err.str());
	}

	return ((bool)(data));
}
// -----------------------------------------------------------------------------
void ComediInterface::setDigitalChannel( int subdev, int channel, bool bit ) const
{
	if( comedi_dio_write(card, subdev, channel, bit) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:setDigitalChannel): can`t write bit=" << bit
			<< " TO subdev=" << subdev << " channel=" << channel << " dev=" << dname;
		throw Exception(err.str());
	}
}
// -----------------------------------------------------------------------------
void ComediInterface::configureChannel( int subdev, int channel, ChannelType t,
										int range, int aref ) const
{
	switch( t )
	{
		case ComediInterface::DI:
		case ComediInterface::DO:
		{
			if( comedi_dio_config(card, subdev, channel, t) < 0 )
			{
				ostringstream err;
				err << "(ComediInterface:configureChannel): can`t configure (DIO) "
					<< " subdev=" << subdev << " channel=" << channel << " type=" << t
					<< " dev=" << dname;
				throw Exception(err.str());
			}

			return;
		}
		break;

		case ComediInterface::AI:
		case ComediInterface::AO:
		{
			lsampl_t data[2];
			comedi_insn insn;
			memset(&insn, 0, sizeof(insn));
			insn.insn = INSN_CONFIG;
			insn.n = 2;
			insn.data = data;
			insn.subdev = subdev;
			insn.chanspec = CR_PACK(channel, range, aref);
			data[0] = t;
			data[1] = t;

			if( comedi_do_insn(card, &insn) < 0 )
			{
				ostringstream err;
				err << "(ComediInterface:configureChannel): can`t configure (AIO) "
					<< " subdev=" << subdev << " channel=" << channel << " type=" << t
					<< " dev=" << dname;
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
		<< " subdev=" << subdev << " channel=" << channel << " type=" << t
		<< " dev=" << dname;
	throw Exception(err.str());
}
// -----------------------------------------------------------------------------
void ComediInterface::configureSubdev( int subdev, SubdevType type ) const
{
	lsampl_t data[2];
	comedi_insn insn;
	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 2;
	insn.data     = data;
	insn.subdev = subdev;
	insn.chanspec = 0;

	data[0] = 102;
	data[1] = type;

	if( comedi_do_insn(card, &insn) < 0 )
	{
		ostringstream err;
		err << "(ComediInterface:configureSubdev): can`t configure subdev "
			<< " subdev=" << subdev << " type=" << type
			<< " dev=" << dname;
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

		case GRAYHILL:
			return "GRAYHILL";

		default:
			break;
	}

	return "";
}
// -----------------------------------------------------------------------------
ComediInterface::SubdevType ComediInterface::str2type( const std::string& s )
{
	if( s == "TBI24_0" )
		return TBI24_0;

	if( s == "TBI0_24" )
		return TBI0_24;

	if( s == "TBI16_8" )
		return TBI16_8;

	if( s == "GRAYHILL" )
		return GRAYHILL;

	return Unknown;
}
// -----------------------------------------------------------------------------
