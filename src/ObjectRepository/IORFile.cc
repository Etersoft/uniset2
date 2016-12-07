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
// -----------------------------------------------------------------------------------------
/*!
    \todo Добавить проверку на предельный номер id
*/
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "IORFile.h"
#include "Configuration.h"
#include "ORepHelpers.h"

// -----------------------------------------------------------------------------------------
using namespace uniset;
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
		throw ORepFailed("(IORFile): не смог создать ior-файл " + fname);
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
