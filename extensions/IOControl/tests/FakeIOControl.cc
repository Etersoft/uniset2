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
#include <iostream>
#include "FakeIOControl.h"
// -----------------------------------------------------------------------------
using namespace std;
// -----------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------

	FakeIOControl::FakeIOControl(uniset::ObjectId id, uniset::ObjectId icID,
						 const std::shared_ptr<SharedMemory>& ic, int numcards, const std::string& prefix_ ):
		IOControl(id,icID,ic,numcards,prefix_)
	{
		fcard = new FakeComediInterface();

		// Подменяем все карты на fake-овые
		for( size_t i=0; i < cards.size(); i++ )
		{
			if( cards[i] )
				delete cards[i];

			cards[i] = fcard;
		}

		noCards = false;
	}
	// --------------------------------------------------------------------------------
	FakeIOControl::~FakeIOControl()
	{
		for( size_t i=0; i < cards.size(); i++ )
		{
			if( cards[i] )
			{
				delete (FakeComediInterface*)cards[i];
				cards[i] = nullptr;
			}
		}
	}

	// --------------------------------------------------------------------------------
	std::shared_ptr<FakeIOControl> FakeIOControl::init_iocontrol(int argc, const char* const* argv,
			uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
			const std::string& prefix )
	{
		auto conf = uniset_conf();
		string name = conf->getArgParam("--" + prefix + "-name", "FakeIOControl1");

		if( name.empty() )
		{
			std::cerr << "(iocontrol): Unknown name. Use --" << prefix << "-name " << std::endl;
			return 0;
		}

		ObjectId ID = conf->getObjectID(name);

		if( ID == uniset::DefaultObjectId )
		{
			std::cerr << "(iocontrol): Unknown ID for " << name
				 << "' Not found in <objects>" << std::endl;
			return 0;
		}

		int numcards = conf->getArgPInt("--" + prefix + "-numcards", 1);

		std::cout << "(iocontrol): name = " << name << "(" << ID << ")" << std::endl;
		return std::make_shared<FakeIOControl>(ID, icID, ic, numcards, prefix);
	}
	// -----------------------------------------------------------------------------
	FakeComediInterface::FakeComediInterface():
		chInputs(maxChannelNum),
		chOutputs(maxChannelNum)
	{

	}
	// -----------------------------------------------------------------------------
	FakeComediInterface::~FakeComediInterface()
	{

	}
	// -----------------------------------------------------------------------------
	int FakeComediInterface::getAnalogChannel(int subdev, int channel, int range, int aref) const
	throw(uniset::Exception)
	{
		if( channel < 0 || channel > maxChannelNum )
		{
			cerr << "(FakeComediInterface::getAnalogChannel): BAD channel num=" << channel
				 << " Must be [0," << maxChannelNum << "]" << endl;
			std::terminate();
		}
		return chInputs[channel];
	}
	// -----------------------------------------------------------------------------
	void FakeComediInterface::setAnalogChannel(int subdev, int channel, int data, int range, int aref) const
	throw(uniset::Exception)
	{
		if( channel < 0 || channel > maxChannelNum )
		{
			cerr << "(FakeComediInterface::setAnalogChannel): BAD channel num=" << channel
				 << " Must be [0," << maxChannelNum << "]" << endl;
			std::terminate();
		}

		chOutputs[channel] = data;
	}
	// -----------------------------------------------------------------------------
	bool FakeComediInterface::getDigitalChannel( int subdev, int channel ) const
	throw(uniset::Exception)
	{
		if( channel < 0 || channel > maxChannelNum )
		{
			cerr << "(FakeComediInterface::getDigitalChannel): BAD channel num=" << channel
				 << " Must be [0," << maxChannelNum << "]" << endl;
			std::terminate();
		}
		return (bool)chInputs[channel];
	}
	// -----------------------------------------------------------------------------
	void FakeComediInterface::setDigitalChannel( int subdev, int channel, bool bit ) const
	throw(uniset::Exception)
	{
		if( channel < 0 || channel > maxChannelNum )
		{
			cerr << "(FakeComediInterface::setDigitalChannel): BAD channel num=" << channel
				 << " Must be [0," << maxChannelNum << "]" << endl;
			std::terminate();
		}

		chOutputs[channel] = (bit ? 1 : 0);
	}
	// -----------------------------------------------------------------------------
	void FakeComediInterface::configureSubdev( int subdev, ComediInterface::SubdevType type ) const
	throw(uniset::Exception)
	{

	}
	// -----------------------------------------------------------------------------
	void FakeComediInterface::configureChannel(int subdev, int channel, ComediInterface::ChannelType type, int range, int aref) const
	throw(uniset::Exception)
	{

	}

	// -----------------------------------------------------------------------------
} // end of namespace uniset
