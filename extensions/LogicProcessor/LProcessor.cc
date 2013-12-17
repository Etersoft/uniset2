#include <iostream>
#include "Configuration.h"
#include "Extensions.h"
#include "PassiveTimer.h"
#include "LProcessor.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------
LProcessor::LProcessor( const std::string& name ):
logname(name)
{
	sleepTime = conf->getArgPInt("--sleepTime", 200);
	smReadyTimeout = conf->getArgInt("--sm-ready-timeout","");
	if( smReadyTimeout == 0 )
		smReadyTimeout = 60000;
	else if( smReadyTimeout < 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;
}

LProcessor::~LProcessor()
{
}
// -------------------------------------------------------------------------
void LProcessor::execute( const string& lfile )
{
	build(lfile);

	while(1)
	{
		try
		{
			step();
		}
		catch( LogicException& ex )
		{
			dlog.crit() << logname << "(execute): " << ex << endl;
		}
		catch( Exception& ex )
		{
			dlog.crit() << logname << "(execute): " << ex << endl;
		}
		catch(...)
		{
			dlog.crit() << logname << "(execute): catch...\n";
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
	sch.read(lfile);
	
	// составляем карту внешних входов
	// считая, что в поле name записано название датчика
	for( Schema::EXTiterator it=sch.extBegin(); it!=sch.extEnd(); ++it )
	{
		UniSetTypes::ObjectId sid = conf->getSensorID(it->name);
		if( sid == DefaultObjectId )
		{
			dlog.crit() << "НЕ НАЙДЕН ИДЕНТИФИКАТОР ДАТЧИКА: " << it->name << endl;
			continue;	
		}
		
		EXTInfo ei;
		ei.sid = sid;
		ei.state = false;
		ei.lnk = &(*it);
		ei.iotype = conf->getIOType(sid);
		if( ei.iotype == UniversalIO::UnknownIOType )
		{
			dlog.crit() << "Unkown iotype for sid=" << sid << "(" << it->name << ")" << endl;
			continue;
		}
		extInputs.push_front(ei);
	}
	
	for( Schema::OUTiterator it=sch.outBegin(); it!=sch.outEnd(); ++it )
	{
		UniSetTypes::ObjectId sid = conf->getSensorID(it->name);
		if( sid == DefaultObjectId )
		{
			dlog.crit() << "НЕ НАЙДЕН ИДЕНТИФИКАТОР ВЫХОДА: " << it->name << endl;
			continue;
		}

		EXTOutInfo ei;
		ei.sid = sid;
		ei.lnk = &(*it);
		ei.iotype = conf->getIOType(sid);
		if( ei.iotype == UniversalIO::UnknownIOType )
		{
			dlog.crit() << "Unkown iotype for sid=" << sid << "(" << it->name << ")" << endl;
			continue;
		}

		extOuts.push_front(ei);
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
	for( EXTList::iterator it=extInputs.begin(); it!=extInputs.end(); ++it )
	{
//		try
//		{
			it->state = (bool)ui.getValue(it->sid);
//		}
	}
}
// -------------------------------------------------------------------------
void LProcessor::processing()
{
	// выcтавляем все внешние входы
	for( EXTList::iterator it=extInputs.begin(); it!=extInputs.end();++it )
		it->lnk->to->setIn(it->lnk->numInput,it->state);

	// проходим по всем элементам
	for( Schema::iterator it=sch.begin(); it!=sch.end(); ++it )
		it->second->tick();
}
// -------------------------------------------------------------------------
void LProcessor::setOuts()
{
	// выcтавляем выходы
	for( OUTList::iterator it=extOuts.begin(); it!=extOuts.end(); ++it )
	{
		try
		{
			ui.setValue(it->sid,it->lnk->from->getOut(),DefaultObjectId);
		}
		catch( Exception& ex )
		{
			dlog.crit() << "(LProcessor::setOuts): " << ex << endl;
		}
		catch(...)
		{
			dlog.crit() << "(LProcessor::setOuts): catch...\n";
		}
	}
}
// -------------------------------------------------------------------------
