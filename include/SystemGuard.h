/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
//--------------------------------------------------------------------------------
/*! \file
 * \brief Реализация SystemGuard
 * \author Pavel Vainerman <pv>
 * \date $Date: 2007/06/17 21:30:55 $
 * \version $Id: SystemGuard.h,v 1.9 2007/06/17 21:30:55 vpashka Exp $
 */
//--------------------------------------------------------------------------------
#ifndef SystemGuard_H_
#define SystemGuard_H_
//--------------------------------------------------------------------------------
#include <omniORB4/CORBA.h>
#include <omnithread.h>
#include "UniSetTypes.h"
#include "PassiveTimer.h"		
#include "ThreadCreator.h"
#include "ObjectsActivator.h"
//--------------------------------------------------------------------------------
/*! \class SystemGuard
 * Предназначен для слежения за исправностью работы процессов. А так же отслеживает наличие
 * связи c узлами и обновляет эту информацию в ListOfNodes.
*/ 
class SystemGuard:
	public ObjectsActivator
{
	public:

		SystemGuard(UniSetTypes::ObjectId id);
		SystemGuard();
		~SystemGuard();

		virtual void run( bool thread=false );
		virtual void stop();
		virtual void oaDestroy(int signo=0);
		
		virtual UniSetTypes::SimpleInfo* getInfo();
		virtual UniSetTypes::ObjectType getType(){ return UniSetTypes::getObjectType("SystemGuard"); }

	protected:
	
		void execute();
		virtual void sigterm( int signo );
		virtual bool pingNode();
		virtual void updateNodeInfo(const UniSetTypes::NodeInfo& newinf){};	

		virtual void watchDogTime();		
		virtual void dumpStateInfo();		
		virtual void autostart();		

	private:

		void init();
		
//		ObjectsActivator* act;	
//		CORBA::ORB_var orb;
		PassiveTimer wdogt;
		PassiveTimer rit;
		PassiveTimer dumpt;
//		PassiveTimer strt;
		friend class ThreadCreator<SystemGuard>;
		ThreadCreator<SystemGuard> *thr;
	
		bool active;
		int expid;
//		omni_mutex omutex;
//		omni_condition ocond;
		UniSetTypes::uniset_mutex actMutex; 
};
//--------------------------------------------------------------------------------
#endif
