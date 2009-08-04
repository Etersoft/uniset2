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
 *  \date   $Date: 2007/11/18 19:13:35 $
 *  \version $Id: SystemGuard.cc,v 1.11 2007/11/18 19:13:35 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <fstream>

#include "ORepHelpers.h"
#include "Configuration.h"
#include "SystemGuard.h"

//---------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//---------------------------------------------------------------------------
static int SleepTickMS		= 500;	// минимальный интервал [мсек]
static int PingNodeTimeOut	= 5;	// интервал проверки связи с узлами [сек]
static int WatchDogTimeOut	= 15;	// интервал посылки WathDog message [мин]
static int AutoStartUpTimeOut=1;	// интервал перед посылкой StartUp-а
static int DumpStateTime	= 0;	// интервал записи внутреннего состояния объектов

//---------------------------------------------------------------------------
SystemGuard::SystemGuard():
 	thr(0),
	active(false)
{
	thread(false);
	init();
}
//---------------------------------------------------------------------------

SystemGuard::SystemGuard( ObjectId id ): //, ObjectsActivator* a ):
	ObjectsActivator(id),
	thr(0),
	active(false)
{
	thread(true);
	init();
}

//---------------------------------------------------------------------------

void SystemGuard::init()
{
	WatchDogTimeOut = conf->getIntField("WatchDogTime") * 60; 
	PingNodeTimeOut = conf->getIntField("PingNodeTime");
	AutoStartUpTimeOut = conf->getIntField("AutoStartUpTime");
	DumpStateTime = conf->getIntField("DumpStateTime");
	SleepTickMS	= conf->getIntField("SleepTickMS");
//	act->addManager( static_cast<ObjectsManager*>(this) );
}

// -------------------------------------------------------------------------
SystemGuard::~SystemGuard()
{
	unideb[Debug::WARN] << myname << "(" << ui.timeToString(time(0),":") << "): destroy.."<< endl;
	if( active )
	{
		active = false;
		uniset_mutex_lock ml(actMutex, 2000);
	}

	if( thr )
	{
		delete thr;
		thr=0;
	}
}

// -------------------------------------------------------------------------
void SystemGuard::execute()
{
	active = true;
	unideb[Debug::SYSTEM] << myname << ": guard processing run..." << endl;

	expid = getpid();

	// Сперва инициализируем сеть...
	pingNode();


	if( SleepTickMS <= 0 )
		SleepTickMS = 200;

//	int SleepTick = SleepTickMS/1000;
//	int SleepTickNS = (SleepTickMS%1000)*1000000;

	PassiveTimer ast;
	if( AutoStartUpTimeOut )
		ast.setTiming(AutoStartUpTimeOut*1000);
	else
		ast.setTiming(UniSetTimer::WaitUpTime);

	if( PingNodeTimeOut )
		rit.setTiming(PingNodeTimeOut*1000);
	else
		rit.setTiming(UniSetTimer::WaitUpTime);

	if( WatchDogTimeOut )
		wdogt.setTiming(WatchDogTimeOut*1000);
	else
		wdogt.setTiming(UniSetTimer::WaitUpTime);

	
	if( DumpStateTime )
	{
		dumpStateInfo();	
		dumpt.setTiming(DumpStateTime*1000);
	}
	else
		dumpt.setTiming(UniSetTimer::WaitUpTime);

//	omutex.lock();		
	actMutex.lock();
	while( active )
	{
//		cout << myname << "(execute): begin "<< endl << flush;
		try
		{
			if( ast.checkTime() )
			{
				autostart();
				ast.setTiming(UniSetTimer::WaitUpTime);
			}

			if( wdogt.checkTime() )
			{
				watchDogTime();	
				wdogt.reset();									
			}
				
			if( rit.checkTime() )
			{
				pingNode();
				rit.reset();
			}

			if( dumpt.checkTime() )
			{
				dumpStateInfo();
				dumpt.reset();
			}

		}
		catch(...)
		{
			unideb[Debug::CRIT] << myname << ": cacth ..." << endl;	
		}

//		cout << myname << "(execute): end (" << SleepTickMS << ")" << endl << flush;				
		msleep(SleepTickMS);
//	    unsigned long s, n;
//	  	omni_thread::get_time(&s, &n, SleepTick, SleepTickNS);
//	    ocond.timedwait(s,n);
	}

	actMutex.unlock();
//	omutex.unlock();		
	unideb[Debug::SYSTEM] << myname << "(" << ui.timeToString(time(0),":") << "): guard processing  stop..." << endl;	
}
// -------------------------------------------------------------------------
void SystemGuard::watchDogTime()
{
	// Рассылаем менеджерам
	SystemMessage sm(SystemMessage::WatchDog); 
	broadcast( sm.transport_msg() );
}
// -------------------------------------------------------------------------
bool SystemGuard::pingNode()
{
//	unideb[Debug::SYSTEM] << myname << "(initNode)..." << endl;
	for( UniSetTypes::ListOfNode::iterator it = conf->lnodes.begin();
			it != conf->lnodes.end(); ++it )
	{
		bool prev = it->connected;
		try
		{
			unideb[Debug::SYSTEM] << myname << "(pingNode): проверяем " << conf->oind->getMapName(it->id) << endl << flush;
			CosNaming::NamingContext_var ctx = ORepHelpers::getRootNamingContext( getORB(), 
							conf->oind->getRealNodeName(it->id) );

			it->connected = true;	
			unideb[Debug::SYSTEM] << myname << "(pingNode): c узлом "<< conf->oind->getMapName(it->id)<< " связь есть ..." << endl;
		}
		catch(...)
		{
			it->connected = false;	
		}

		try
		{
			updateNodeInfo((*it));
		}
		catch(...){}

		// Рассылаем уведомление об изменении состояния в сети
		if( prev != it->connected )
		{
			SystemMessage sm(SystemMessage::NetworkInfo); 
			sm.data[0] = it->id;
			sm.data[1] = it->connected;
			broadcast( sm.transport_msg() );
			if(!it->connected )
				unideb[Debug::SYSTEM] << myname <<"(pingNode): узел "<< conf->oind->getMapName(it->id)<< " НЕДОСТУПЕН!!!!" << endl;
		}
	}
	
	return true;
}
// -------------------------------------------------------------------------

void SystemGuard::autostart()
{
	unideb[Debug::SYSTEM] << myname << ": Рассылаем StartUp!!!!!"<< endl;
	SystemMessage sm(SystemMessage::StartUp); 
	broadcast( sm.transport_msg() );
	unideb[Debug::SYSTEM] << myname << ": StartUp ok."<< endl;
}

// -------------------------------------------------------------------------
void SystemGuard::sigterm( int signo )
{
	unideb[Debug::CRIT] << myname << "(sigterm): catch sig=" << signo << "("<< strsignal(signo) <<")" << endl;	
	
	wdogt.setTiming(UniSetTimer::WaitUpTime);
	rit.setTiming(UniSetTimer::WaitUpTime);
	dumpt.setTiming(UniSetTimer::WaitUpTime);

	try
	{
		ObjectsActivator::sigterm(signo);
	}
	catch(...){}

//	active = false; // ??? 
}

// -------------------------------------------------------------------------
void SystemGuard::stop()
{
	unideb[Debug::WARN] << myname << "(stop): disactivate..." << endl;	
	ObjectsActivator::stop();
//	active = false;
//	uniset_mutex_lock ml(actMutex, 2000); // ждём отключения
}
// -------------------------------------------------------------------------
void SystemGuard::run( bool thread )
{
	thr = new ThreadCreator<SystemGuard>(this, &SystemGuard::execute);
	if( !thr->start() )
	{
		unideb[Debug::CRIT] << myname << ":  НЕ СМОГЛИ СОЗДАТЬ поток"<<endl;	
		throw SystemError("CREATE SYSTEMGUARD THREAD FAILED");
	}
	ObjectsActivator::run(thread);
}
// -------------------------------------------------------------------------
void SystemGuard::oaDestroy( int signo )
{
	try
	{
		active = false;
		uniset_mutex_lock ml(actMutex, 2000); // ждём отключения
		ObjectsActivator::oaDestroy(signo);
	}
	catch(...)
	{
		unideb[Debug::CRIT] << myname << "(oaDestroy) catch ..." << endl;
	}
}
// -------------------------------------------------------------------------

void SystemGuard::dumpStateInfo()
{
	ostringstream fname;

	string dir(conf->getLogDir());
//	cout << "!!!!!DIR = " << dir << endl;
	string name(ORepHelpers::getShortName(myname));
	if( name.empty() )
	{
		name = conf->getArgv()[0];
	}

	fname << dir << name << ".log";
	fstream fout( fname.str().c_str(), ios::out | ios::trunc );
	if(!fout)
	{
		unideb[Debug::CRIT] << "НЕ СМОГ ОТКРЫТЬ ФАЙЛ " << fname.str() << endl;
		return;
	}

	fout.setf(ios::left, ios::adjustfield);
	fout << ui.dateToString(time(0),"/")<< "\t" << ui.timeToString(time(0),";") << endl;
	fout << "==========================================================================" << endl;
//	fout << "\n--------------------------------------------------------------------------" << endl;
	fout << "\nСеть:\n";
	fout << "--------------------------------------------------------------------------" << endl;	



	for( UniSetTypes::ListOfNode::const_iterator it = conf->listNodesBegin();
			it != conf->listNodesEnd(); ++it )
	{
		fout << setw(40) << conf->oind->getMapName(it->id) << setw(2) << "\t"<< it->connected << endl;
	}


	fout << "\n\n--------------------------------------------------------------------------";
	fout << "\nИнформация по объектам:";
	fout << "\n--------------------------------------------------------------------------" << endl;
//	fout << setw(40)<< "name"<< setw(6) << "\tid"<< setw(8) << "\tmsgTID \textinfo" << endl;	
//	fout << "--------------------------------------------------------------------------\n" << endl;
	SimpleInfoSeq_var res = this->getObjectsInfo();
	int size = res->length();
	
	for(int i=0; i<size; i++)
		fout << "\n" << res[i].info << "\n" << endl;
		
	fout.close();
}
// -------------------------------------------------------------------------
SimpleInfo* SystemGuard::getInfo()
{
	ostringstream info;
	SimpleInfo_var si = ObjectsManager::getInfo();
	info << si->info;
	if( thr )
		info << "\texTID= " << thr->getTID();
	else
		info << "\texpid= " << expid;

	SimpleInfo* res = new SimpleInfo();
	res->info	= info.str().c_str(); // CORBA::string_dup(info.str().c_str());
	res->id 	= getId();
	return res; //._retn();	
					
}
// -------------------------------------------------------------------------
