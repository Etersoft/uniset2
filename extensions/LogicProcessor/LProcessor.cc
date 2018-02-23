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
// -------------------------------------------------------------------------
#include <iostream>
#include <algorithm>
#include "Configuration.h"
#include "Extensions.h"
#include "PassiveTimer.h"
#include "LProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -------------------------------------------------------------------------
LProcessor::LProcessor( const std::string& name ):
	logname(name)
{
	auto conf = uniset_conf();
	sleepTime = conf->getArgPInt("--sleepTime", 200);
	int tout = conf->getArgInt("--sm-ready-timeout", "120000");

	if( tout == 0 )
		smReadyTimeout = conf->getNCReadyTimeout();
	else if( tout < 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;
	else
		smReadyTimeout = tout;

	sch = make_shared<SchemaXML>();
}

LProcessor::~LProcessor()
{
}
// -------------------------------------------------------------------------
void LProcessor::open( const string& lfile )
{
	if( isOpen() )
	{
		ostringstream err;
		err << logname << "(execute): already opened from '" << fSchema << "'" << endl;
		throw SystemError(err.str());
	}

	fSchema = lfile;
	build(lfile);
}
// -------------------------------------------------------------------------
bool LProcessor::isOpen() const
{
	return !fSchema.empty();
}
// -------------------------------------------------------------------------
void LProcessor::execute( const std::string& lfile )
{
	if( !lfile.empty() )
		open(lfile);

	while( !canceled )
	{
		try
		{
			step();
		}
		catch( LogicException& ex )
		{
			dcrit << logname << "(execute): " << ex << endl;
		}
		catch( const uniset::Exception& ex )
		{
			dcrit << logname << "(execute): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			dcrit << logname << "(execute): " << ex.what() << endl;
		}

		msleep(sleepTime);
	}
}
// -------------------------------------------------------------------------
void LProcessor::terminate()
{
	canceled = true;
}
// -------------------------------------------------------------------------
std::shared_ptr<SchemaXML> LProcessor::getSchema()
{
	return sch;
}
// -------------------------------------------------------------------------
timeout_t LProcessor::getSleepTime() const noexcept
{
	return sleepTime;
}
// -------------------------------------------------------------------------
void LProcessor::step()
{
	getInputs();
	processing();
	setOuts();
}
// -------------------------------------------------------------------------
void LProcessor::build( const string& lfile )
{
	auto conf = uniset_conf();

	sch->read(lfile);

	// составляем карту внешних входов
	// считая, что в поле name записано название датчика
	for( auto it = sch->extBegin(); it != sch->extEnd(); ++it )
	{
		uniset::ObjectId sid = conf->getSensorID(it->name);

		if( sid == DefaultObjectId )
		{
			ostringstream err;
			err << "Unknown sensor ID for " << it->name;

			dcrit << err.str() << endl;
			throw SystemError(err.str());
		}

		EXTInfo ei;
		ei.sid = sid;
		ei.value = false;
		ei.el = it->to;
		ei.numInput = it->numInput;

		extInputs.emplace_front( std::move(ei) );
	}

	for( auto it = sch->outBegin(); it != sch->outEnd(); ++it )
	{
		uniset::ObjectId sid = conf->getSensorID(it->name);

		if( sid == DefaultObjectId )
		{
			ostringstream err;
			err << "Unknown sensor ID for " << it->name;

			dcrit << err.str() << endl;
			throw SystemError(err.str());
		}

		EXTOutInfo ei;
		ei.sid = sid;
		ei.el = it->from;

		extOuts.emplace_front(std::move(ei));
	}
}
// -------------------------------------------------------------------------
/*!
    Опрос всех датчиков. Являющхся входами для логических элементов.
Исключение специально НЕ ловится. Т.к. если не удалось опросить хотя бы один
датчик, то проверку вообще лучше прервать. Иначе схема может работать не так, как надо

*/
void LProcessor::getInputs()
{
	for( auto && it : extInputs )
	{
		//        try
		//        {
		it.value = ui.getValue(it.sid);
		//        }
	}
}
// -------------------------------------------------------------------------
void LProcessor::processing()
{
	// выcтавляем все внешние входы
	for( const auto& it : extInputs )
		it.el->setIn(it.numInput, it.value);

	// проходим по всем элементам
	for_each( sch->begin(), sch->end(), [](Schema::ElementMap::value_type it)
	{
		it.second->tick();
	} );
}
// -------------------------------------------------------------------------
void LProcessor::setOuts()
{
	// выcтавляем выходы
	for( const auto& it : extOuts )
	{
		try
		{
			ui.setValue(it.sid, it.el->getOut());
		}
		catch( const uniset::Exception& ex )
		{
			dcrit << "(LProcessor::setOuts): " << ex << endl;
		}
		catch( const std::exception& ex )
		{
			dcrit << "(LProcessor::setOuts): catch: " << ex.what() << endl;
		}
	}
}
// -------------------------------------------------------------------------
