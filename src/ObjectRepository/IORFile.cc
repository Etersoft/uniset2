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
string IORFile::getIOR( const ObjectId id, const ObjectId node )
{
	string fname( genFName(id,node) );
	ifstream ior_file(fname.c_str());
	string sior;
	ior_file >> sior;

	return sior;
}
// -----------------------------------------------------------------------------------------
void IORFile::setIOR( const ObjectId id, const ObjectId node, const string sior )
{
	string fname( genFName(id,node) );
	ofstream ior_file(fname.c_str(), ios::out | ios::trunc);

	if( !ior_file )
	{
		unideb[Debug::CRIT] << "(IORFile): не смог открыть файл "+fname << endl;
		throw TimeOut("(IORFile): не смог создать ior-файл "+fname);
	}

	ior_file << sior << endl;
	ior_file.close();
}
// -----------------------------------------------------------------------------------------
void IORFile::unlinkIOR( const ObjectId id, const ObjectId node )
{
	string fname( genFName(id,node) );
//	ostringstream cmd;
//	cmd << "unlink " << fname;
//	system(cmd.str().c_str());
	unlink(fname.c_str());
}
// -----------------------------------------------------------------------------------------
string IORFile::genFName( const ObjectId id, const ObjectId node )
{
/*
	string oname(conf->oind->getMapName(id));
	string nodename(conf->oind->getMapName(node));
	oname = ORepHelpers::getShortName(oname);
	nodename = conf->oind->getFullNodeName(nodename);

	ostringstream fname;

	if( oname.empty() )
		fname << conf->getLockDir() << id << "." << node;
	else
		fname << conf->getLockDir() << oname << "." << nodename;
*/
	ostringstream fname;
	fname << conf->getLockDir() << id << "." << node;
	return fname.str();
}
// -----------------------------------------------------------------------------------------
