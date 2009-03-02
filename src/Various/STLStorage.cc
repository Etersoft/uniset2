/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Vitaly Lipatov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#include "StorageInterface.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
// --------------------------------------------------------------------------

STLStorage::STLStorage()
{
}

STLStorage::~STLStorage()
{
}

// --------------------------------------------------------------------------
IOController_i::DigitalIOInfo STLStorage::find( const IOController_i::SensorInfo& si )
{
	throw NameNotFound();
}
// --------------------------------------------------------------------------
void STLStorage::push( IOController_i::DigitalIOInfo& di )
{

}
// --------------------------------------------------------------------------
void STLStorage::remove(const IOController_i::SensorInfo& si)
{

}
// --------------------------------------------------------------------------	
bool STLStorage::getState(const IOController_i::SensorInfo& si)
{
	return false;
}
// --------------------------------------------------------------------------
long STLStorage::getValue(const IOController_i::SensorInfo& si)
{
	return 0;
}
// --------------------------------------------------------------------------
void STLStorage::saveState(const IOController_i::DigitalIOInfo& di,bool st)
{

}
// --------------------------------------------------------------------------
void STLStorage::saveValue(const IOController_i::AnalogIOInfo& ai, long val)
{

}
// --------------------------------------------------------------------------
