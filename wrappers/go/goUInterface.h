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
#ifndef goUInterface_H_
#define goUInterface_H_
// --------------------------------------------------------------------------
#include <string>
#include "UTypes.h"
#include "UExceptions.h"
// --------------------------------------------------------------------------
namespace goUInterface
{
	UTypes::ResultBool uniset_init_params( UTypes::Params* p, const std::string& xmlfile ) noexcept;
	UTypes::ResultBool uniset_init( int argc, char** argv, const std::string& xmlfile ) noexcept;

	//---------------------------------------------------------------------------
	UTypes::ResultValue getValue( long id ) noexcept;
	UTypes::ResultBool setValue( long id, long val, long supplier = UTypes::DefaultSupplerID ) noexcept;

	// return DefaultObjectId if not found
	long getSensorID( const std::string& name ) noexcept;
	long getObjectID( const std::string& name ) noexcept;

	std::string getShortName( long id ) noexcept;
	std::string getName( long id ) noexcept;
	std::string getTextName( long id ) noexcept;

	std::string getConfFileName() noexcept;
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
