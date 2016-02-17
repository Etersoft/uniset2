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
 *  \brief Программа просмотра состояния датчиков
 *  \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "SViewer.h"
#include "ORepHelpers.h"
#include "UniSetObject_i.hh"
#include "UniSetTypes.h"
#include "ObjectIndex_Array.h"

// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace UniversalIO;
using namespace std;
// --------------------------------------------------------------------------
SViewer::SViewer(const string& csec, bool sn):
	csec(csec),
	rep(UniSetTypes::uniset_conf()),
	cache(500, 15),
	isShort(sn)
{
	ui = make_shared<UInterface>(UniSetTypes::uniset_conf());
}

SViewer::~SViewer()
{
}

void SViewer::on_SViewer_destroy()
{
	//    activator->oakill(SIGINT);
	//    msleep(500);
	//    activator->oakill(SIGKILL);
}
// --------------------------------------------------------------------------
void SViewer::monitor( timeout_t timeMS )
{
	for(;;)
	{
		view();
		msleep(timeMS);
	}
}

// --------------------------------------------------------------------------
void SViewer::view()
{
	try
	{
		readSection(csec, "");
	}
	catch( const Exception& ex )
	{
		cerr << ex << endl;
	}
}

// ---------------------------------------------------------------------------

void SViewer::readSection( const string& section, const string& secRoot )
{
	ListObjectName lst;
	string curSection;

	try
	{
		if ( secRoot.empty() )
			curSection = section;
		else
			curSection = secRoot + "/" + section;

		//        cout << " read sectionlist ..."<< endl;
		if( !rep.listSections(curSection, &lst, 1000) )
		{
			cout << "(readSection): получен не полный список" << endl;
		}
	}
	catch( ORepFailed ) {}
	catch(...)
	{
		cout << "(readSection): get sectionlist ... catch..." << endl;
	}

	if( !lst.empty() )
	{
		for ( ListObjectName::const_iterator li = lst.begin(); li != lst.end(); ++li)
		{
			readSection((*li), curSection);
		}
	}
	else
	{
		string secName(curSection);
		ListObjectName lstObj;
		ListObjectName::const_iterator li;

		try
		{
			if( !rep.list(secName, &lstObj, 1000) )
				cout << "(readSection): получен не полный список" << endl;
		}
		catch( ORepFailed )
		{
			cout << "(readSection):.. catch ORepFailed" << endl;
			return;
		}
		catch(...)
		{
			cout << "(readSection): catch..." << endl;
			return;
		}

		//        cout << " read objectlist ok."<< endl;

		if ( !lstObj.empty() )
		{
			for ( li = lstObj.begin(); li != lstObj.end(); ++li )
			{
				try
				{
					string ob(*li);
					string fname(curSection + "/" + ob);
					ObjectId id = uniset_conf()->oind->getIdByName( fname );

					if( id == DefaultObjectId )
						cout << "(readSection): ID?! для " << fname << endl;
					else
						getInfo(id);
				}
				catch( const Exception& ex )
				{
					cout << "(readSection): " << ex << endl;
				}

			}
		}
		else
			cout << " секция " << secRoot << "/" << section << " не содержит объектов " << endl;
	}
}
// ---------------------------------------------------------------------------
void SViewer::getInfo( ObjectId id )
{
	CORBA::Object_var oref;

	try
	{
		try
		{
			oref = cache.resolve(id, uniset_conf()->getLocalNode());
		}
		catch( NameNotFound ) {}

		if( CORBA::is_nil(oref) )
		{
			oref = ui->resolve(id);
			cache.cache(id, uniset_conf()->getLocalNode(), oref);
		}

		IONotifyController_i_var ioc = IONotifyController_i::_narrow(oref);

		if( CORBA::is_nil(ioc) )
		{
			cout << "(getInfo): nil references" << endl;
			return;
		}


		try
		{
			IOController_i::SensorInfoSeq_var amap = ioc->getSensorsMap();
			updateSensors(amap, id);
		}
		catch( IOController_i::NameNotFound ) {}
		catch(...) {}

		try
		{
			IONotifyController_i::ThresholdsListSeq_var tlst = ioc->getThresholdsList();
			updateThresholds(tlst, id);
		}
		catch( IOController_i::NameNotFound ) {}
		catch(...) {}

		return;
	}
	catch( const Exception& ex )
	{
		cout << "(getInfo):" << ex << endl;
	}
	catch(...)
	{
		cout << "(getInfo): catch ..." << endl;
	}

	cache.erase(id, uniset_conf()->getLocalNode());
}

// ---------------------------------------------------------------------------
void SViewer::updateSensors( IOController_i::SensorInfoSeq_var& amap, UniSetTypes::ObjectId oid )
{
	string owner = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(oid));
	cout << "\n======================================================\n" << owner;
	cout << "\t Датчики";
	cout << "\n------------------------------------------------------" << endl;
	int size = amap->length();

	for(int i = 0; i < size; i++)
	{
		if( amap[i].type == UniversalIO::AI || amap[i].type == UniversalIO::DI )
		{
			string name(uniset_conf()->oind->getNameById(amap[i].si.id));

			if( isShort )
				name = ORepHelpers::getShortName(name);

			string txtname( uniset_conf()->oind->getTextName(amap[i].si.id) );
			printInfo( amap[i].si.id, name, amap[i].value, owner, txtname, (amap[i].type == UniversalIO::AI ? "AI" : "DI") );
		}
	}

	cout << "------------------------------------------------------\n";

	cout << "\n======================================================\n" << owner;
	cout << "\t Выходы";
	cout << "\n------------------------------------------------------" << endl;

	for(int i = 0; i < size; i++)
	{
		if( amap[i].type == UniversalIO::AO || amap[i].type == UniversalIO::DO )
		{
			string name(uniset_conf()->oind->getNameById(amap[i].si.id));

			if( isShort )
				name = ORepHelpers::getShortName(name);

			string txtname( uniset_conf()->oind->getTextName(amap[i].si.id) );
			printInfo( amap[i].si.id, name, amap[i].value, owner, txtname, (amap[i].type == UniversalIO::AO ? "AO" : "DO"));
		}
	}

	cout << "------------------------------------------------------\n";

}
// ---------------------------------------------------------------------------
void SViewer::updateThresholds( IONotifyController_i::ThresholdsListSeq_var& tlst, UniSetTypes::ObjectId oid )
{
	int size = tlst->length();
	string owner = ORepHelpers::getShortName(uniset_conf()->oind->getMapName(oid));
	cout << "\n======================================================\n" << owner;
	cout << "\t Пороговые датчики";
	cout << "\n------------------------------------------------------" << endl;

	for(int i = 0; i < size; i++)
	{
		cout << "(" << setw(5) << tlst[i].si.id << ") | ";

		switch( tlst[i].type  )
		{
			case UniversalIO::AI:
				cout << "AI";
				break;

			case UniversalIO::AO:
				cout << "AO";
				break;

			default:
				cout << "??";
				break;
		}

		string sname(uniset_conf()->oind->getNameById(tlst[i].si.id));

		if( isShort )
			sname = ORepHelpers::getShortName(sname);

		cout << " | " << setw(60) << sname << " | " << setw(5) << tlst[i].value << endl;

		int m = tlst[i].tlist.length();

		for( auto k = 0; k < m; k++ )
		{
			IONotifyController_i::ThresholdInfo* ti = &tlst[i].tlist[k];
			cout << "\t(" << setw(3) << ti->id << ")  |  " << setw(5) << ti->state << "  |  hi: " << setw(5) << ti->hilimit;
			cout << " | low: " << setw(5) << ti->lowlimit;
			cout << endl;
		}
	}
}
// ---------------------------------------------------------------------------

void SViewer::printInfo(UniSetTypes::ObjectId id, const string& sname, long value, const string& owner,
						const string& txtname, const string& iotype)
{
	cout << "(" << setw(5) << id << ")" << " | " << setw(2) << iotype << " | " << setw(60) << sname << "   | " << setw(5) << value << endl; // << "\t | " << setw(40) << owner << "\t | " << txtname << endl;
}
// ---------------------------------------------------------------------------
