/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
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
/*! \file
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef StorageInterface_H_
#define StorageInterface_H_
// ---------------------------------------------------------------------------
#include "IOController_i.hh"
// ---------------------------------------------------------------------------
/*! */ 
class StorageInterface
{
	public:
	
		StorageInterface(){};
		virtual ~StorageInterface(){};


		virtual IOController_i::DigitalIOInfo find( const IOController_i::SensorInfo& si )=0;
		
		virtual void push( IOController_i::DigitalIOInfo& di )=0;
		virtual void remove(const IOController_i::SensorInfo& si)=0;
		
		
		virtual bool getState(const IOController_i::SensorInfo& si)=0;
		virtual long getValue(const IOController_i::SensorInfo& si)=0;

		virtual void saveState(const IOController_i::DigitalIOInfo& di,bool st)=0;
		virtual void saveValue(const IOController_i::SensorIOInfo& ai, long val)=0;

	protected:

	private:
};
// ---------------------------------------------------------------------------
class STLStorage:
	public StorageInterface
{
	public:
		STLStorage();
		~STLStorage();


		virtual IOController_i::DigitalIOInfo find( const IOController_i::SensorInfo& si );
		
		virtual void push( IOController_i::DigitalIOInfo& di );
		virtual void remove(const IOController_i::SensorInfo& si);
		
		
		virtual bool getState(const IOController_i::SensorInfo& si);
		virtual long getValue(const IOController_i::SensorInfo& si);

		virtual void saveState(const IOController_i::DigitalIOInfo& di,bool st);
		virtual void saveValue(const IOController_i::SensorIOInfo& ai, long val);

	protected:
	private:
};
// ---------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------
