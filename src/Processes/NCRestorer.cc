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
void NCRestorer::addlist( IONotifyController* ic, std::shared_ptr<IOController::USensorInfo>& inf, IONotifyController::ConsumerListInfo&& lst, bool force )
{
	// Проверка зарегистрирован-ли данный датчик
	// если такого дискретного датчика нет, то здесь сработает исключение...
	if( !force )
	{
		try
		{
			ic->getIOType(inf->si.id);
		}
		catch(...)
		{
			// Регистрируем (если не найден)
			switch(inf->type)
			{
				case UniversalIO::DI:
				case UniversalIO::DO:
				case UniversalIO::AI:
				case UniversalIO::AO:
					ic->ioRegistration(inf);
					break;

				default:
					ucrit << ic->getName() << "(askDumper::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА! -> "
						  << uniset_conf()->oind->getNameById(inf->si.id) << endl;
					return;
					break;

			}
		}
	}

	switch(inf->type)
	{
		case UniversalIO::DI:
		case UniversalIO::AI:
		case UniversalIO::DO:
		case UniversalIO::AO:
			ic->askIOList[inf->si.id] = std::move(lst);
			break;

		default:
			ucrit << ic->getName() << "(NCRestorer::addlist): НЕИЗВЕСТНЫЙ ТИП ДАТЧИКА!-> "
				  << uniset_conf()->oind->getNameById(inf->si.id) << endl;
			break;
	}
}
// ------------------------------------------------------------------------------------------
void NCRestorer::addthresholdlist( IONotifyController* ic, std::shared_ptr<IOController::USensorInfo>& inf, IONotifyController::ThresholdExtList&& lst, bool force )
{
	// Проверка зарегистрирован-ли данный датчик
	// если такого дискретного датчика нет сдесь сработает исключение...
	if( !force )
	{
		try
		{
			ic->getIOType(inf->si.id);
		}
		catch(...)
		{
			// Регистрируем (если не найден)
			switch(inf->type)
			{
				case UniversalIO::DI:
				case UniversalIO::DO:
				case UniversalIO::AI:
				case UniversalIO::AO:
					ic->ioRegistration(inf);
					break;

				default:
					break;
			}
		}
	}

	// default init iterators
	for( auto& it : lst )
		it.sit = ic->myioEnd();

	try
	{
		auto i =  ic->myiofind(inf->si.id);
		ic->askTMap[inf->si.id].usi = i->second;
		if( i->second )
			i->second->userdata[IONotifyController::udataThresholdList] = &(ic->askTMap[inf->si.id]);
	}
	catch(...) {}

	ic->askTMap[inf->si.id].si   = inf->si;
	ic->askTMap[inf->si.id].type = inf->type;
	ic->askTMap[inf->si.id].list = std::move(lst);
}
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo::SInfo( const IOController_i::SensorIOInfo& inf )
{
	(*this) = inf;
}
// ------------------------------------------------------------------------------------------
NCRestorer::SInfo& NCRestorer::SInfo::operator=( const IOController_i::SensorIOInfo& inf )
{
	this->si          = inf.si;
	this->type        = inf.type;
	this->priority    = inf.priority;
	this->default_val = inf.default_val;
	this->real_value = inf.real_value;
	this->ci         = inf.ci;
	this->undefined = inf.undefined;
	this->blocked = inf.blocked;
	this->dbignore = inf.dbignore;

	for( size_t i=0; i<IOController::USensorInfo::MaxUserData; i++ )
		this->userdata[i] = nullptr;

	return *this;
}
// ------------------------------------------------------------------------------------------
#if 0
NCRestorer::SInfo& NCRestorer::SInfo::operator=( const std::shared_ptr<IOController_i::SensorIOInfo>& inf )
{
	this->si          = inf->si;
	this->type        = inf->type;
	this->priority    = inf->priority;
	this->default_val = inf->default_val;
	this->real_value = inf->real_value;
	this->ci         = inf->ci;
	this->undefined = inf->undefined;
	this->blocked = inf->blocked;
	this->dbignore = inf->dbignore;
	this->any = 0;
	return *this;
}
#endif
// ------------------------------------------------------------------------------------------
void NCRestorer::init_depends_signals( IONotifyController* ic )
{
	for( auto it = ic->ioList.begin(); it != ic->ioList.end(); ++it )
	{
		// обновляем итераторы...
		it->second->it = it->second;

		if( it->second->d_si.id == DefaultObjectId )
			continue;

		uinfo << ic->getName() << "(NCRestorer::init_depends_signals): "
			  << " init depend: '" << uniset_conf()->oind->getMapName(it->second->si.id) << "'"
			  << " dep_name=(" << it->second->d_si.id << ")'" << uniset_conf()->oind->getMapName(it->second->d_si.id) << "'"
			  << endl;

		ic->signal_change_value(it->second->d_si.id).connect( sigc::mem_fun( it->second.get(), &IOController::USensorInfo::checkDepend) );
	}
}
// -----------------------------------------------------------------------------
