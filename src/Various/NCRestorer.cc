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

#include "Debug.h"
#include "Configuration.h"
#include "NCRestorer.h"
#include "Exceptions.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
NCRestorer::NCRestorer()
{

}

NCRestorer::~NCRestorer()
{
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addlist( IONotifyController* ic, SInfo& inf, IONotifyController::ConsumerList& lst, bool force )
{
	UniSetTypes::KeyType k( key(inf.si.id,inf.si.node) );

	// Проверка зарегистрирован-ли данный датчик	
	// если такого дискретного датчика нет, то здесь сработает исключение...
	if( !force )
	{
		try
		{
			ic->getIOType(inf.si);
		}
		catch(...)
		{
			// Регистрируем (если не найден)
			switch(inf.type)
			{
				case UniversalIO::DigitalInput:
				case UniversalIO::DigitalOutput:
					ic->dsRegistration(inf);
				break;
	
				case UniversalIO::AnalogInput:
				case UniversalIO::AnalogOutput:
					ic->asRegistration(inf);
				break;
	
				default:
					unideb[Debug::CRIT] << ic->getName() << "(askDumper::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА! -> "
									<< conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
					return;
				break;
	
			}
		}
	}
	
	switch(inf.type)
	{
		case UniversalIO::DigitalInput:
			ic->askDIOList[k]=lst;
		break;

		case UniversalIO::AnalogInput:
			ic->askAIOList[k]=lst;
		break;
		
		case UniversalIO::DigitalOutput:
			ic->askDOList[k]=lst;
		break;

		case UniversalIO::AnalogOutput:
			ic->askAOList[k]=lst;
		break;

		default:
			unideb[Debug::CRIT] << ic->getName() << "(askDumper::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА!-> "
								<< conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
		break;
	}
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addthresholdlist( IONotifyController* ic, SInfo& inf, IONotifyController::ThresholdExtList& lst, bool force )
{
	// Проверка зарегистрирован-ли данный датчик	
	// если такого дискретного датчика нет сдесь сработает исключение...
	if( !force )
	{
		try
		{
			ic->getIOType(inf.si);
		}
		catch(...)
		{
			// Регистрируем (если не найден)
			switch(inf.type)
			{
				case UniversalIO::DigitalInput:
				case UniversalIO::DigitalOutput:
					ic->dsRegistration(inf);
				break;
	
				case UniversalIO::AnalogInput:
				case UniversalIO::AnalogOutput:
					ic->asRegistration(inf);
				break;
				
				default:
					break;
			}
		}
	}

	// default init iterators
	for( IONotifyController::ThresholdExtList::iterator it=lst.begin(); it!=lst.end(); ++it )
		it->itSID = ic->mydioEnd();

	UniSetTypes::KeyType k( key(inf.si.id,inf.si.node) );
	ic->askTMap[k].si	= inf.si;
	ic->askTMap[k].type	= inf.type;
	ic->askTMap[k].list	= lst;
	ic->askTMap[k].ait 	= ic->myaioEnd();

	try
	{
		switch( inf.type )
		{
			case UniversalIO::DigitalInput:
			case UniversalIO::DigitalOutput:
			case UniversalIO::AnalogOutput:
			break;

			case UniversalIO::AnalogInput:
			{
				IOController::AIOStateList::iterator it(ic->myaioEnd());
				ic->checkThreshold(it,inf.si,false);
			}
			break;
			
			default:
				break;
		}
	}
	catch(Exception& ex)
	{
	   	unideb[Debug::WARN] << ic->getName() << "(NCRestorer::addthresholdlist): " << ex
				<< " для " << conf->oind->getNameById(inf.si.id, inf.si.node) << endl;
		throw;
	}
	catch( CORBA::SystemException& ex )
	{
	  	unideb[Debug::WARN] << ic->getName() << "(NCRestorer::addthresholdlist): " 
				<< conf->oind->getNameById(inf.si.id,inf.si.node) << " недоступен!!(CORBA::SystemException): "
				<< ex.NP_minorString() << endl;
		throw;
	}
}								
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo& NCRestorer::SInfo::operator=(IOController_i::DigitalIOInfo& inf)
{
	this->si = inf.si;
	this->type = inf.type;
	this->priority = inf.priority;
	this->default_val = inf.default_val;
	this->real_value = inf.real_state ? 1 : 0;
	this->ci.minRaw = 0;
	this->ci.maxRaw = 0;
	this->ci.minCal = 0;
	this->ci.maxCal = 0;
	this->ci.sensibility = 0;
	this->db_ignore = false;
	this->undefined = false;
	this->any = 0;
	return *this;
}
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo& NCRestorer::SInfo::operator=(IOController_i::AnalogIOInfo& inf)
{
	this->si 		= inf.si;
	this->type 		= inf.type;
	this->priority 	= inf.priority;
	this->default_val = inf.default_val;
	this->real_value = inf.real_value;
	this->ci 		= inf.ci;
	this->undefined = false;
	this->db_ignore = false;
	this->any = 0;
	return *this;
}
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo::operator IOController::UniDigitalIOInfo()
{
	IOController::UniDigitalIOInfo ret;

	ret.state		= this->value ? true : false;
	ret.si			= this->si;
	ret.type 		= this->type;
	ret.priority 	= this->priority;
	ret.default_val = this->default_val ? true : false;
	ret.any 		= this->any;
	ret.undefined 	= this->undefined;
	ret.db_ignore 	= this->db_ignore;
	return ret;
}
// ------------------------------------------------------------------------------------------
