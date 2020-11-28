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
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "IORFile.h"
#include "Exceptions.h"
#include "ORepHelpers.h"

// -----------------------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// -----------------------------------------------------------------------------------------
IORFile::IORFile( const std::string& dir ):
	iordir(dir)
{

}
// -----------------------------------------------------------------------------------------
IORFile::~IORFile()
{

}
// -----------------------------------------------------------------------------------------
string IORFile::getIOR( const ObjectId id )
{
	const string fname( getFileName(id) );
	ifstream ior_file(fname.c_str());
	string sior;
	ior_file >> sior;

	return sior;
}
// -----------------------------------------------------------------------------------------
void IORFile::setIOR( const ObjectId id, const string& sior )
{
	const string fname( getFileName(id) );
	ofstream ior_file(fname.c_str(), ios::out | ios::trunc);

	if( !ior_file )
		throw ORepFailed("(IORFile): не смог создать ior-файл " + fname);

	ior_file << sior << endl;
	ior_file.close();
}
// -----------------------------------------------------------------------------------------
void IORFile::unlinkIOR( const ObjectId id )
{
	const string fname( getFileName(id) );
	unlink(fname.c_str());
}
// -----------------------------------------------------------------------------------------
string IORFile::getFileName( const ObjectId id )
{
	ostringstream fname;
	fname << iordir << id;
	return fname.str();
}
// -----------------------------------------------------------------------------------------
