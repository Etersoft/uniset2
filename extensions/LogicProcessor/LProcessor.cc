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
	int tout = conf->getArgInt("--sm-ready-timeout", "");

	if( tout == 0 )
		smReadyTimeout = 60000;
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
			dcrit << "НЕ НАЙДЕН ИДЕНТИФИКАТОР ДАТЧИКА: " << it->name << endl;
			continue;
		}

		EXTInfo ei;
		ei.sid = sid;
		ei.state = false;
		ei.el = it->to;
		ei.iotype = conf->getIOType(sid);
		ei.numInput = it->numInput;

		if( ei.iotype == UniversalIO::UnknownIOType )
		{
			dcrit << "Unkown iotype for sid=" << sid << "(" << it->name << ")" << endl;
			continue;
		}

		extInputs.emplace_front( std::move(ei) );
	}

	for( auto it = sch->outBegin(); it != sch->outEnd(); ++it )
	{
		uniset::ObjectId sid = conf->getSensorID(it->name);

		if( sid == DefaultObjectId )
		{
			dcrit << "НЕ НАЙДЕН ИДЕНТИФИКАТОР ВЫХОДА: " << it->name << endl;
			continue;
		}

		EXTOutInfo ei;
		ei.sid = sid;
		ei.el = it->from;
		ei.iotype = conf->getIOType(sid);

		if( ei.iotype == UniversalIO::UnknownIOType )
		{
			dcrit << "Unkown iotype for sid=" << sid << "(" << it->name << ")" << endl;
			continue;
		}

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
	for( auto& it : extInputs )
	{
		//        try
		//        {
		it.state = (bool)ui.getValue(it.sid);
		//        }
	}
}
// -------------------------------------------------------------------------
void LProcessor::processing()
{
	// выcтавляем все внешние входы
	for( const auto& it : extInputs )
		it.el->setIn(it.numInput, it.state);

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
			ui.setValue(it.sid, it.el->getOut(), DefaultObjectId);
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
