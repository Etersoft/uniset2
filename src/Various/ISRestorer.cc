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
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------

#include <sstream>
#include <fstream>
#include "Debug.h"
#include "Configuration.h"
#include "ISRestorer.h"
#include "Exceptions.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
ISRestorer::ISRestorer()
{

}

ISRestorer::~ISRestorer()
{
}
// ------------------------------------------------------------------------------------------
void ISRestorer::addlist( InfoServer* is, const UniSetTypes::MessageCode code,
						InfoServer::ConsumerList& lst )
{
	is->askList[code]=lst;
}
// ------------------------------------------------------------------------------------------
