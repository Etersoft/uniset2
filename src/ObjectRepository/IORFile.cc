// This file is part of the UniSet project.

/* This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov
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
// -----------------------------------------------------------------------------------------
/*!
    \todo Добавить проверку на предельный номер id
*/
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <fstream>
#include "IORFile.h"
#include "Configuration.h"
#include "ORepHelpers.h"

// -----------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------------------
IORFile::IORFile()
{
}

// -----------------------------------------------------------------------------------------
string IORFile::getIOR( const ObjectId id )
{
	string fname( getFileName(id) );
	ifstream ior_file(fname.c_str());
	string sior;
	ior_file >> sior;

	return std::move(sior);
}
// -----------------------------------------------------------------------------------------
void IORFile::setIOR( const ObjectId id, const string& sior )
{
	string fname( getFileName(id) );
	ofstream ior_file(fname.c_str(), ios::out | ios::trunc);

	if( !ior_file )
	{
		ucrit << "(IORFile): не смог открыть файл " + fname << endl;
		throw TimeOut("(IORFile): не смог создать ior-файл " + fname);
	}

	ior_file << sior << endl;
	ior_file.close();
}
// -----------------------------------------------------------------------------------------
void IORFile::unlinkIOR( const ObjectId id )
{
	string fname( getFileName(id) );
	unlink(fname.c_str());
}
// -----------------------------------------------------------------------------------------
string IORFile::getFileName( const ObjectId id )
{
	ostringstream fname;
	fname << uniset_conf()->getLockDir() << id;
	return std::move( fname.str() );
}
// -----------------------------------------------------------------------------------------
