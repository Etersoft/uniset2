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

#include <sstream>
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>

#include "UInterface.h"
#include "IONotifyController.h"
#include "ORepHelpers.h"
#include "Debug.h"
#include "IOConfig.h"

// ------------------------------------------------------------------------------------------
using namespace UniversalIO;
using namespace uniset;
using namespace std;
// ------------------------------------------------------------------------------------------
IONotifyController::IONotifyController():
	askIOMutex("askIOMutex"),
	trshMutex("trshMutex"),
	maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
	sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{

}

IONotifyController::IONotifyController(const string& name, const string& section, std::shared_ptr<IOConfig> d ):
	IOController(name, section),
	restorer(d),
	askIOMutex(name + "askIOMutex"),
	trshMutex(name + "trshMutex"),
	maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
	sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
	conUndef = signal_change_undefined_state().connect(sigc::mem_fun(*this, &IONotifyController::onChangeUndefinedState));
	conInit = signal_init().connect(sigc::mem_fun(*this, &IONotifyController::initItem));
}

IONotifyController::IONotifyController( ObjectId id, std::shared_ptr<IOConfig> d ):
	IOController(id),
	restorer(d),
	askIOMutex(string(uniset_conf()->oind->getMapName(id)) + "_askIOMutex"),
	trshMutex(string(uniset_conf()->oind->getMapName(id)) + "_trshMutex"),
	maxAttemtps(uniset_conf()->getPIntField("ConsumerMaxAttempts", 10)),
	sendAttemtps(uniset_conf()->getPIntField("ConsumerSendAttempts", 3))
{
	conUndef = signal_change_undefined_state().connect(sigc::mem_fun(*this, &IONotifyController::onChangeUndefinedState));
	conInit = signal_init().connect(sigc::mem_fun(*this, &IONotifyController::initItem));
}

IONotifyController::~IONotifyController()
{
	conUndef.disconnect();
	conInit.disconnect();
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsumer( ostringstream& inf, const std::string& consumer )
{
	ObjectId consumer_id = uniset_conf()->getObjectID(consumer);

	if( consumer_id == DefaultObjectId )
		consumer_id = uniset_conf()->getControllerID(consumer);

	if( consumer_id == DefaultObjectId )
		consumer_id = uniset_conf()->getServiceID(consumer);

	if( consumer_id == DefaultObjectId )
	{
		inf << "not found consumer '" << consumer << "'" << endl;
		return;
	}

	// Формируем статистику по каждому датчику..
	struct StatInfo
	{
		StatInfo( ObjectId id, const ConsumerInfoExt& c ): inf(c), sid(id) {}

		const ConsumerInfoExt inf;
		ObjectId sid;
	};

	std::list<StatInfo> stat;

	// общее количество SensorMessage полученное этим заказчиком
	size_t smCount = 0;

	{
		// lock askIOMutex

		// выводим информацию по конкретному объекту
		uniset_rwmutex_rlock lock(askIOMutex);

		for( auto && a : askIOList )
		{
			auto& i = a.second;

			uniset_rwmutex_rlock lock(i.mut);

			if( i.clst.empty() )
				continue;

			// ищем среди заказчиков
			for( const auto& c : i.clst )
			{
				if( c.id == consumer_id )
				{
					stat.emplace_back(a.first, c);
					smCount += c.smCount;
					break;
				}
			}
		}
	} // unlock askIOMutex

	{
		// выводим информацию по конкретному объекту
		uniset_rwmutex_rlock lock(trshMutex);

		for( auto && a : askTMap )
		{
			uniset_rwmutex_rlock lock2(a.second.mut);

			for( const auto& c : a.second.clst )
			{
				if( c.id == consumer_id )
				{
					if( a.first->sid != DefaultObjectId )
						stat.emplace_back(a.first->sid, c);
					else
						stat.emplace_back(DefaultObjectId, c);

					smCount += c.smCount;
					break;
				}
			}
		}
	}

	// печатаем отчёт
	inf << "Statisctic for consumer '" << consumer << "'(smCount=" << smCount << "):"
		<< endl
		<< "--------------------------------------------"
		<< endl;

	if( stat.empty() )
	{
		inf << "NOT FOUND STATISTIC FOR '" << consumer << "'" << endl;
	}
	else
	{
		std::ios_base::fmtflags old_flags = inf.flags();

		inf << std::right;

		auto oind = uniset_conf()->oind;

		for( const auto& s : stat )
		{
			inf << "        " << "(" << setw(6) << s.sid << ") "
				<< setw(35) << ORepHelpers::getShortName(oind->getMapName(s.sid))
				<< " ["
				<< " lostEvents: " << setw(3) << s.inf.lostEvents
				<< " attempt: " << setw(3) << s.inf.attempt
				<< " smCount: " << setw(5) << s.inf.smCount
				<< " ]"
				<< endl;
		}

		inf.setf(old_flags);
	}

	inf << "--------------------------------------------" << endl;

}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForLostConsumers( ostringstream& inf )
{
	std::lock_guard<std::mutex> lock(lostConsumersMutex);

	if( lostConsumers.empty() )
	{
		inf << "..empty lost consumers list..." << endl;
		return;
	}

	inf << "Statistics 'consumers with lost events':"
		<< endl
		<< "----------------------------------------"
		<< endl;

	auto oind = uniset_conf()->oind;

	for( const auto& l : lostConsumers )
	{
		inf << "        " << "(" << setw(6) << l.first << ") "
			<< setw(35) << std::left << ORepHelpers::getShortName(oind->getMapName(l.first))
			<< " lostCount=" << l.second.count
			<< endl;
	}
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsusmers( ostringstream& inf )
{
	uniset_rwmutex_rlock lock(askIOMutex);

	auto oind = uniset_conf()->oind;

	for( auto && a : askIOList )
	{
		auto& i = a.second;

		uniset_rwmutex_rlock lock(i.mut);

		// отображаем только датчики с "не пустым" списком заказчиков
		if( i.clst.empty() )
			continue;

		inf << "(" << setw(6) << a.first << ")[" << oind->getMapName(a.first) << "]" << endl;

		for( const auto& c : i.clst )
		{
			inf << "        " << "(" << setw(6) << c.id << ")"
				<< setw(35) << ORepHelpers::getShortName(oind->getMapName(c.id))
				<< " ["
				<< " lostEvents=" << c.lostEvents
				<< " attempt=" << c.attempt
				<< " smCount=" << c.smCount
				<< "]"
				<< endl;
		}
	}
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForConsumersWithLostEvent( ostringstream& inf )
{
	uniset_rwmutex_rlock lock(askIOMutex);

	auto oind = uniset_conf()->oind;
	bool empty = true;

	for( auto && a : askIOList )
	{
		auto& i = a.second;

		uniset_rwmutex_rlock lock(i.mut);

		// отображаем только датчики с "не пустым" списком заказчиков
		if( i.clst.empty() )
			continue;

		// Т.к. сперва выводится имя датчика, а только потом его заказчики
		// то если надо выводить только тех, у кого есть "потери"(lostEvent>0)
		// для предварительно смотрим список есть ли там хоть один с "потерями", а потом уже выводим

		bool lost = false;

		for( const auto& c : i.clst )
		{
			if( c.lostEvents > 0 )
			{
				lost = true;
				break;
			}
		}

		if( !lost )
			continue;

		empty = false;
		// выводим тех у кого lostEvent>0
		inf << "(" << setw(6) << a.first << ")[" << oind->getMapName(a.first) << "]" << endl;
		inf << "--------------------------------------------------------------------" << endl;

		for( const auto& c : i.clst )
		{
			if( c.lostEvents > 0 )
			{
				inf << "        " << "(" << setw(6) << c.id << ")"
					<< setw(35) << ORepHelpers::getShortName(oind->getMapName(c.id))
					<< " ["
					<< " lostEvents=" << c.lostEvents
					<< " attempt=" << c.attempt
					<< " smCount=" << c.smCount
					<< "]"
					<< endl;
			}
		}
	}

	if( empty )
		inf << "...not found consumers with lost event..." << endl;
	else
		inf << "--------------------------------------------------------------------" << endl;
}
// ------------------------------------------------------------------------------------------
void IONotifyController::showStatisticsForSensor( ostringstream& inf, const string& name )
{
	auto conf = uniset_conf();
	auto oind = conf->oind;

	ObjectId sid = conf->getSensorID(name);

	if( sid == DefaultObjectId )
	{
		inf << "..not found ID for sensor '" << name << "'" << endl;
		return;
	}

	ConsumerListInfo* clist = nullptr;

	{
		uniset_rwmutex_rlock lock(askIOMutex);
		auto s = askIOList.find(sid);

		if( s == askIOList.end() )
		{
			inf << "..not found consumers for sensor '" << name << "'" << endl;
			return;
		}

		clist = &(s->second);
	}

	inf << "Statisctics for sensor "
		<< "(" << setw(6) << sid << ")[" << name << "]: " << endl
		<< "--------------------------------------------------------------------" << endl;

	uniset_rwmutex_rlock lock2(clist->mut);

	for( const auto& c : clist->clst )
	{
		inf << "        (" << setw(6) << c.id << ")"
			<< setw(35) << ORepHelpers::getShortName(oind->getMapName(c.id))
			<< " ["
			<< " lostEvents=" << c.lostEvents
			<< " attempt=" << c.attempt
			<< " smCount=" << c.smCount
			<< "]"
			<< endl;
	}

	inf << "--------------------------------------------------------------------" << endl;
}
// ------------------------------------------------------------------------------------------
SimpleInfo* IONotifyController::getInfo( const char* userparam )
{
	uniset::SimpleInfo_var i = IOController::getInfo(userparam);

	//! \todo Назвать параметры нормально
	//!
	std::string param(userparam);

	ostringstream inf;

	inf << i->info << endl;

	auto oind = uniset_conf()->oind;

	if( param.empty() )
	{
		inf << "-------------------------- lost consumers list [maxAttemtps=" << maxAttemtps << "] ------------------" << endl;
		showStatisticsForLostConsumers(inf);
		inf << "----------------------------------------------------------------------------------" << endl;
	}

	if( param == "consumers" )
	{
		inf << "------------------------------- consumers list ------------------------------" << endl;
		showStatisticsForConsusmers(inf);
		inf << "-----------------------------------------------------------------------------" << endl << endl;
	}
	else if( param == "lost" )
	{
		inf << "------------------------------- consumers list (lost event)------------------" << endl;
		showStatisticsForConsumersWithLostEvent(inf);
		inf << "-----------------------------------------------------------------------------" << endl << endl;
	}
	else if( !param.empty() )
	{
		auto query = uniset::explode_str(param, ':');

		if( query.empty() || query.size() == 1 )
			showStatisticsForConsumer(inf, param);
		else if( query.size() > 1 )
		{
			if( query[0] == "consumer" )
				showStatisticsForConsumer(inf, query[1]);
			else if( query[0] == "sensor" )
				showStatisticsForSensor(inf, query[1]);
			else
				inf << "Unknown command: " << param << endl;
		}
	}
	else
	{
		inf << "IONotifyController::UserParam help: " << endl
			<< "  Default         - Common info" << endl
			<< "  consumers       - Consumers list " << endl
			<< "  lost            - Consumers list with lostEvent > 0" << endl
			<< "  consumer:name   - Statistic for consumer 'name'" << endl
			<< "  sensor:name     - Statistic for sensor 'name'"
			<< endl;
	}

	i->info = inf.str().c_str();

	return i._retn();
}

// ------------------------------------------------------------------------------------------
/*!
 *    \param lst - список в который необходимо внести потребителя
 *    \param name - имя вносимого потребителя
 *    \note Добавление произойдет только если такого потребителя не существует в списке
*/
bool IONotifyController::addConsumer( ConsumerListInfo& lst, const ConsumerInfo& ci )
{
	uniset_rwmutex_wrlock l(lst.mut);

	for( auto && it :  lst.clst )
	{
		if( it.id == ci.id && it.node == ci.node )
		{
			// при перезаказе датчиков количество неудачных попыток послать сообщение
			// считаем что "заказчик" опять на связи
			it.attempt = maxAttemtps;

			// выставляем флаг, что заказчик опять "на связи"
			std::lock_guard<std::mutex> lock(lostConsumersMutex);
			auto c = lostConsumers.find(ci.id);

			if( c != lostConsumers.end() )
				c->second.lost = false;

			return false;
		}
	}

	ConsumerInfoExt cinf(ci, 0, maxAttemtps);

	// получаем ссылку
	try
	{
		uniset::ObjectVar op = ui->resolve(ci.id, ci.node);
		cinf.ref = UniSetObject_i::_narrow(op);
	}
	catch(...) {}

	lst.clst.emplace_front( std::move(cinf) );

	// выставляем флаг, что клиент опять "на связи"
	std::lock_guard<std::mutex> lock(lostConsumersMutex);
	auto c = lostConsumers.find(ci.id);

	if( c != lostConsumers.end() )
		c->second.lost = false;

	return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    \param lst - указатель на список из которго происходит удаление потребителя
 *    \param name - имя удаляемого потребителя
*/
bool IONotifyController::removeConsumer( ConsumerListInfo& lst, const ConsumerInfo& cons )
{
	uniset_rwmutex_wrlock l(lst.mut);

	for( auto li = lst.clst.begin(); li != lst.clst.end(); ++li )
	{
		if( li->id == cons.id && li->node == cons.node  )
		{
			lst.clst.erase(li);
			return true;
		}
	}

	return false;
}
// ------------------------------------------------------------------------------------------
/*!
 *    \param si         - информация о датчике
 *    \param ci         - информация о заказчике
 *    \param cmd        - команда см. UniversalIO::UIOCommand
*/
void IONotifyController::askSensor(const uniset::ObjectId sid,
								   const uniset::ConsumerInfo& ci, UniversalIO::UIOCommand cmd )
{
	ulog2 << "(askSensor): поступил " << ( cmd == UIODontNotify ? "отказ" : "заказ" ) << " от "
		  << uniset_conf()->oind->getNameById(ci.id) << "@" << ci.node
		  << " на аналоговый датчик "
		  << uniset_conf()->oind->getNameById(sid) << endl;

	auto li = myioEnd();

	try
	{
		// если такого аналогового датчика нет, здесь сработает исключение...
		localGetValue(li, sid);
	}
	catch( IOController_i::Undefined& ex ) {}

	{
		uniset_rwmutex_wrlock lock(askIOMutex);
		ask(askIOList, sid, ci, cmd);
	}

	auto usi = li->second;

	// посылка первый раз состояния
	if( cmd == UniversalIO::UIONotify || (cmd == UIONotifyFirstNotNull && usi->value) )
	{
		ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

		if( lst )
		{
			uniset::uniset_rwmutex_rlock lock(usi->val_lock);
			SensorMessage smsg( usi->makeSensorMessage(false) );
			send(*lst, smsg, &ci);
		}
	}
}
// ------------------------------------------------------------------------------------------
void IONotifyController::ask( AskMap& askLst, const uniset::ObjectId sid,
							  const uniset::ConsumerInfo& cons, UniversalIO::UIOCommand cmd)
{
	// поиск датчика в списке
	auto askIterator = askLst.find(sid);

	switch (cmd)
	{
		case UniversalIO::UIONotify: // заказ
		case UniversalIO::UIONotifyChange:
		case UniversalIO::UIONotifyFirstNotNull:
		{
			if( askIterator != askLst.end() )
				addConsumer(askIterator->second, cons);
			else
			{
				ConsumerListInfo newlst; // создаем новый список
				addConsumer(newlst, cons);
				askLst.emplace(sid, std::move(newlst));
			}

			break;
		}

		case UniversalIO::UIODontNotify:     // отказ
		{
			if( askIterator != askLst.end() )
				removeConsumer(askIterator->second, cons);

			break;
		}

		default:
			break;
	}

	if( askIterator == askLst.end() )
		askIterator = askLst.find(sid);

	if( askIterator != askLst.end() )
	{
		auto s = myiofind(sid);

		if( s != myioEnd() )
			s->second->setUserData(udataConsumerList, &(askIterator->second));
		else
			s->second->setUserData(udataConsumerList, nullptr);
	}
}
// ------------------------------------------------------------------------------------------
long IONotifyController::localSetValue( std::shared_ptr<IOController::USensorInfo>& usi,
										CORBA::Long value, uniset::ObjectId sup_id )
{
	// оптимизация:
	// if( !usi ) - не проверяем, т.к. считаем что это внутренние функции и несуществующий указатель передать не могут

	CORBA::Long prevValue = value;
	{
		uniset_rwmutex_rlock lock(usi->val_lock);
		prevValue = usi->value;
	}

	CORBA::Long curValue = IOController::localSetValue(usi, value, sup_id);

	// Рассылаем уведомления только в случае изменения значения
	// --------
	if( prevValue == curValue )
		return curValue;


	{
		// с учётом того, что параллельно с этой функцией может
		// выполняться askSensor, то
		// посылать сообщение надо "заблокировав" доступ к value...

		uniset::uniset_rwmutex_rlock lock(usi->val_lock);

		SensorMessage sm(usi->makeSensorMessage(false));

		try
		{
			if( !usi->dbignore )
				logging(sm);
		}
		catch(...) {}

		ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

		if( lst )
			send(*lst, sm);
	}

	// проверка порогов
	try
	{
		checkThreshold(usi, true);
	}
	catch(...) {}

	return curValue;
}
// -----------------------------------------------------------------------------------------
/*!
    \note В случае зависания в функции push, будут остановлены рассылки другим объектам.
    Возможно нужно ввести своего агента на удалённой стороне, который будет заниматься
    только приёмом сообщений и локальной рассылкой. Lav
*/
void IONotifyController::send( ConsumerListInfo& lst, const uniset::SensorMessage& sm, const uniset::ConsumerInfo* ci  )
{
	TransportMessage tmsg(sm.transport_msg());

	uniset_rwmutex_wrlock l(lst.mut);

	for( ConsumerList::iterator li = lst.clst.begin(); li != lst.clst.end(); ++li )
	{
		if( ci )
		{
			if( ci->id != li->id || ci->node != li->node )
				continue;
		}

		for( int i = 0; i < sendAttemtps; i++ )
		{
			try
			{
				if( CORBA::is_nil(li->ref) )
				{
					CORBA::Object_var op = ui->resolve(li->id, li->node);
					li->ref = UniSetObject_i::_narrow(op);
				}

				tmsg.consumer = li->id;
				li->ref->push( tmsg );
				li->smCount++;
				li->attempt = maxAttemtps; // reinit attempts
				break;
			}
			catch( const CORBA::SystemException& ex )
			{
				uwarn << myname << "(IONotifyController::send): attempt=" << (maxAttemtps - li->attempt + 1)
					  << " from " << maxAttemtps << " "
					  << uniset_conf()->oind->getNameById(li->id) << "@" << li->node << " (CORBA::SystemException): "
					  << ex.NP_minorString() << endl;
			}
			catch( const std::exception& ex )
			{
				uwarn << myname << "(IONotifyController::send): attempt=" <<  (maxAttemtps - li->attempt + 1) << " "
					  << " from " << maxAttemtps << " "
					  << ex.what()
					  << " for " << uniset_conf()->oind->getNameById(li->id) << "@" << li->node << endl;
			}
			catch(...)
			{
				ucrit << myname << "(IONotifyController::send): attempt=" <<  (maxAttemtps - li->attempt + 1) << " "
					  << " from " << maxAttemtps << " "
					  << uniset_conf()->oind->getNameById(li->id) << "@" << li->node
					  << " catch..." << endl;
			}

			// фиксируем только после первой попытки послать
			if( i > 0 )
				li->lostEvents++;

			try
			{
				if( maxAttemtps > 0 && --(li->attempt) <= 0 )
				{
					uwarn << myname << "(IONotifyController::send): ERASE FROM CONSUMERS:  "
						  << uniset_conf()->oind->getNameById(li->id) << "@" << li->node << endl;

					{
						std::lock_guard<std::mutex> lock(lostConsumersMutex);
						auto& c = lostConsumers[li->id];

						// если уже выставлен флаг что "заказчик" пропал, то не надо увеличивать "счётчик"
						// видимо мы уже зафиксировали его пропажу на другом датчике...
						if( !c.lost )
						{
							c.count += 1;
							c.lost = true;
						}
					}

					li = lst.clst.erase(li);
					--li;
					break;
				}


				li->ref = UniSetObject_i::_nil();
			}
			catch( const std::exception& ex )
			{
				uwarn << myname << "(IONotifyController::send): UniSetObject_i::_nil() "
					  << ex.what()
					  << " for " << uniset_conf()->oind->getNameById(li->id) << "@" << li->node << endl;
			}
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::activateObject()
{
	// сперва загружаем датчики и заказчиков..
	readConf();
	// а потом уже собственно активация..
	return IOController::activateObject();
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::sensorsRegistration()
{
	for_iolist([this](std::shared_ptr<USensorInfo>& s)
	{
		ioRegistration(s);
	});
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::readConf()
{
	try
	{
		if( restorer )
			initIOList( std::move(restorer->read()) );
	}
	catch( const std::exception& ex )
	{
		// Если дамп не удалось считать, значит что-то не то в configure.xml
		// и безопаснее "вылететь", чем запустится, т.к. часть датчиков не будет работать
		// как ожидается.
		ucrit << myname << "(IONotifyController::readConf): " << ex.what() << endl << flush;
		//std::terminate(); // std::abort();
		uterminate();
	}
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::initItem( std::shared_ptr<USensorInfo>& usi, IOController* ic )
{
	if( usi->type == UniversalIO::AI || usi->type == UniversalIO::AO )
		checkThreshold( usi, false );
}
// ------------------------------------------------------------------------------------------
void IONotifyController::askThreshold(uniset::ObjectId sid,
									  const uniset::ConsumerInfo& ci,
									  uniset::ThresholdId tid,
									  CORBA::Long lowLimit, CORBA::Long hiLimit,  CORBA::Boolean invert,
									  UniversalIO::UIOCommand cmd )
{
	ulog2 << "(askThreshold): " << ( cmd == UIODontNotify ? "отказ" : "заказ" ) << " от "
		  << uniset_conf()->oind->getNameById(ci.id) << "@" << ci.node
		  << " на порог tid=" << tid
		  << " [" << lowLimit << "," << hiLimit << ",invert=" << invert << "]"
		  << " для датчика "
		  << uniset_conf()->oind->getNameById(sid)
		  << endl;

	if( lowLimit > hiLimit )
		throw IONotifyController_i::BadRange();

	auto li = myioEnd();

	CORBA::Long val = 0;

	try
	{
		// если такого датчика нет здесь сработает исключение...
		val = localGetValue(li, sid);
	}
	catch( const IOController_i::Undefined& ex ) {}

	{
		// lock
		uniset_rwmutex_wrlock lock(trshMutex);

		auto it = findThreshold(sid, tid);

		switch( cmd )
		{
			case UniversalIO::UIONotify: // заказ
			case UniversalIO::UIONotifyChange:
			{
				if( !it )
					it = make_shared<IOController::UThresholdInfo>(tid, lowLimit, hiLimit, invert);

				it = addThresholdIfNotExist(li->second, it);
				addThresholdConsumer(it, ci);

				if( cmd == UniversalIO::UIONotifyChange )
					break;

				// посылка первый раз состояния
				SensorMessage sm(li->second->makeSensorMessage());
				sm.consumer   = ci.id;
				sm.tid        = tid;

				// Проверка нижнего предела
				if( val <= lowLimit )
					sm.threshold = false;
				// Проверка верхнего предела
				else if( val >= hiLimit )
					sm.threshold = true;

				auto clst = askTMap.find(it.get());

				if( clst != askTMap.end() )
					send(clst->second, sm, &ci);
			}
			break;

			case UniversalIO::UIODontNotify:     // отказ
			{
				if( it )
					removeThresholdConsumer(li->second, it, ci);
			}
			break;

			default:
				break;
		}
	} // unlock trshMutex
}
// --------------------------------------------------------------------------------------------------------------
std::shared_ptr<IOController::UThresholdInfo>
IONotifyController::addThresholdIfNotExist( std::shared_ptr<USensorInfo>& usi,
		std::shared_ptr<UThresholdInfo>& ti )
{
	uniset::uniset_rwmutex_wrlock lck(usi->tmut);

	for( auto && t : usi->thresholds )
	{
		if( ti->id == t->id )
			return t;
	}

	struct timespec tm = uniset::now_to_timespec();

	ti->tv_sec  = tm.tv_sec;

	ti->tv_nsec = tm.tv_nsec;

	usi->thresholds.push_back(ti);

	return ti;
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::addThresholdConsumer( std::shared_ptr<IOController::UThresholdInfo>& ti, const ConsumerInfo& ci )
{
	auto i = askTMap.find(ti.get());

	if( i != askTMap.end() )
		return addConsumer(i->second, ci);

	auto ret = askTMap.emplace(ti.get(), ConsumerListInfo());

	return addConsumer( ret.first->second, ci );
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::removeThresholdConsumer( std::shared_ptr<USensorInfo>& usi,
		std::shared_ptr<UThresholdInfo>& ti,
		const uniset::ConsumerInfo& ci )
{
	uniset_rwmutex_wrlock lck(usi->tmut);

	for( auto && t : usi->thresholds )
	{
		if( t->id == ti->id )
		{
			auto it = askTMap.find(ti.get());

			if( it == askTMap.end() )
				return false;

			return removeConsumer(it->second, ci);
		}
	}

	return false;
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( IOController::IOStateList::iterator& li,
		const uniset::ObjectId sid,
		bool send_msg )
{
	if( li == myioEnd() )
		li = myiofind(sid);

	if( li == myioEnd() )
		return; // ???

	checkThreshold(li->second, send_msg);
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::checkThreshold( std::shared_ptr<IOController::USensorInfo>& usi, bool send_msg )
{
	uniset_rwmutex_rlock lock(trshMutex);

	auto& ti = usi->thresholds;

	if( ti.empty() )
		return;

	// обрабатываем текущее состояние датчика обязательно "залочив" значение..
	uniset_rwmutex_rlock vlock(usi->val_lock);

	SensorMessage sm(usi->makeSensorMessage(false));

	// текущее время
	struct timespec tm = uniset::now_to_timespec();

	{
		uniset_rwmutex_rlock l(usi->tmut);

		for( auto && it : ti )
		{
			// Используем здесь значение скопированное в sm.value
			// чтобы не делать ещё раз lock на li->second->value
			IONotifyController_i::ThresholdState state = it->state;

			if( !it->invert )
			{
				// Если логика не инвертированная, то срабатывание это - выход за зону >= hilimit
				if( sm.value <= it->lowlimit  )
					state = IONotifyController_i::NormalThreshold;
				else if( sm.value >= it->hilimit )
					state = IONotifyController_i::HiThreshold;
			}
			else
			{
				// Если логика инвертированная, то срабатывание это - выход за зону <= lowlimit
				if( sm.value >= it->hilimit  )
					state = IONotifyController_i::NormalThreshold;
				else if( sm.value <= it->lowlimit )
					state = IONotifyController_i::LowThreshold;
			}

			// если ничего не менялось..
			if( it->state == state )
				continue;

			it->state = state;

			sm.tid = it->id;

			// если состояние не normal, значит порог сработал,
			// не важно какой.. нижний или верхний (зависит от inverse)
			sm.threshold = ( state != IONotifyController_i::NormalThreshold );

			// запоминаем время изменения состояния
			it->tv_sec     = tm.tv_sec;
			it->tv_nsec    = tm.tv_nsec;
			sm.sm_tv   = tm;

			// если порог связан с датчиком, то надо его выставить
			if( it->sid != uniset::DefaultObjectId )
			{
				try
				{
					localSetValueIt(it->sit, it->sid, (sm.threshold ? 1 : 0), usi->supplier);
				}
				catch( uniset::Exception& ex )
				{
					ucrit << myname << "(checkThreshold): " << ex << endl;
				}
			}

			// отдельно посылаем сообщения заказчикам по данному "порогу"
			if( send_msg )
			{
				uniset_rwmutex_rlock lck(trshMutex);
				auto i = askTMap.find(it.get());

				if( i != askTMap.end() )
					send(i->second, sm);
			}
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
std::shared_ptr<IOController::UThresholdInfo> IONotifyController::findThreshold( const uniset::ObjectId sid, const uniset::ThresholdId tid )
{
	auto it = myiofind(sid);

	if( it != myioEnd() )
	{
		auto usi = it->second;
		uniset_rwmutex_rlock lck(usi->tmut);

		for( auto && t : usi->thresholds )
		{
			if( t->id == tid )
				return t;
		}
	}

	return nullptr;
}
// --------------------------------------------------------------------------------------------------------------

IONotifyController_i::ThresholdInfo IONotifyController::getThresholdInfo( uniset::ObjectId sid, uniset::ThresholdId tid )
{
	uniset_rwmutex_rlock lock(trshMutex);
	auto it = findThreshold(sid, tid);

	if( !it )
	{
		ostringstream err;
		err << myname << "(getThresholds): Not found sensor (" << sid << ") "
			<< uniset_conf()->oind->getNameById(sid);

		uinfo << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	return IONotifyController_i::ThresholdInfo(*it);
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdList* IONotifyController::getThresholds( uniset::ObjectId sid )
{
	auto it = myiofind(sid);

	if( it == myioEnd() )
	{
		ostringstream err;
		err << myname << "(getThresholds): Not found sensor (" << sid << ") "
			<< uniset_conf()->oind->getNameById(sid);

		uinfo << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	auto& usi = it->second;
	IONotifyController_i::ThresholdList* res = new IONotifyController_i::ThresholdList();

	try
	{
		res->si     = usi->si;
		res->value  = IOController::localGetValue(usi);
		res->type   = usi->type;
	}
	catch( const uniset::Exception& ex )
	{
		uwarn << myname << "(getThresholds): для датчика "
			  << uniset_conf()->oind->getNameById(usi->si.id)
			  << " " << ex << endl;
	}

	uniset_rwmutex_rlock lck(usi->tmut);
	res->tlist.length( usi->thresholds.size() );

	size_t k = 0;

	for( const auto& it2 : usi->thresholds )
	{
		res->tlist[k].id       = it2->id;
		res->tlist[k].hilimit  = it2->hilimit;
		res->tlist[k].lowlimit = it2->lowlimit;
		res->tlist[k].state    = it2->state;
		res->tlist[k].tv_sec   = it2->tv_sec;
		res->tlist[k].tv_nsec  = it2->tv_nsec;
		k++;
	}

	return res;
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* IONotifyController::getThresholdsList()
{
	std::list< std::shared_ptr<USensorInfo> > slist;

	// ищем все датчики, у которых не пустой список порогов
	for_iolist([&slist]( std::shared_ptr<USensorInfo>& usi )
	{

		if( !usi->thresholds.empty() )
			slist.push_back(usi);
	});

	IONotifyController_i::ThresholdsListSeq* res = new IONotifyController_i::ThresholdsListSeq();

	res->length( slist.size() );

	if( !slist.empty() )
	{
		size_t i = 0;

		for( auto && it : slist )
		{
			try
			{
				(*res)[i].si    = it->si;
				(*res)[i].value = IOController::localGetValue(it);
				(*res)[i].type  = it->type;
			}
			catch( const std::exception& ex )
			{
				uwarn << myname << "(getThresholdsList): for sid="
					  << uniset_conf()->oind->getNameById(it->si.id)
					  << " " << ex.what() << endl;
				continue;
			}
			catch( const IOController_i::NameNotFound& ex )
			{
				uwarn << myname << "(getThresholdsList): IOController_i::NameNotFound.. for sid="
					  << uniset_conf()->oind->getNameById(it->si.id)
					  << endl;

				continue;
			}

			uniset_rwmutex_rlock lck(it->tmut);
			(*res)[i].tlist.length( it->thresholds.size() );

			size_t k = 0;

			for( const auto& it2 : it->thresholds )
			{
				(*res)[i].tlist[k].id       = it2->id;
				(*res)[i].tlist[k].hilimit  = it2->hilimit;
				(*res)[i].tlist[k].lowlimit = it2->lowlimit;
				(*res)[i].tlist[k].state    = it2->state;
				(*res)[i].tlist[k].tv_sec   = it2->tv_sec;
				(*res)[i].tlist[k].tv_nsec  = it2->tv_nsec;
				k++;
			}

			i++;
		}
	}

	return res;
}
// -----------------------------------------------------------------------------
void IONotifyController::onChangeUndefinedState( std::shared_ptr<USensorInfo>& usi, IOController* ic )
{
	uniset_rwmutex_rlock vlock(usi->val_lock);
	SensorMessage sm( usi->makeSensorMessage(false) );

	try
	{
		if( !usi->dbignore )
			logging(sm);
	}
	catch(...) {}

	ConsumerListInfo* lst = static_cast<ConsumerListInfo*>(usi->getUserData(udataConsumerList));

	if( lst )
		send(*lst, sm);
}

// -----------------------------------------------------------------------------
IDSeq* IONotifyController::askSensorsSeq( const uniset::IDSeq& lst,
		const uniset::ConsumerInfo& ci,
		UniversalIO::UIOCommand cmd)
{
	uniset::IDList badlist; // cписок не найденных идентификаторов

	size_t size = lst.length();
	ObjectId sid;

	for( size_t i = 0; i < size; i++ )
	{
		sid = lst[i];

		try
		{
			askSensor(sid, ci, cmd);
		}
		catch(...)
		{
			badlist.add(sid);
		}
	}

	return badlist.getIDSeq();
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
Poco::JSON::Object::Ptr IONotifyController::httpHelp(const Poco::URI::QueryParameters& p)
{
	uniset::json::help::object myhelp(myname, IOController::httpHelp(p));

	{
		// 'consumers'
		uniset::json::help::item cmd("consumers", "get consumers list");
		cmd.param("sensor1,sensor2,sensor3", "get consumers for sensors");
		myhelp.add(cmd);
	}

	{
		// 'lost'
		uniset::json::help::item cmd("lost", "get lost consumers list");
		myhelp.add(cmd);
	}

	return myhelp;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
	if( req == "consumers" )
		return request_consumers(req, p);

	if( req == "lost" )
		return request_lost(req, p);

	return IOController::httpRequest(req, p);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::request_consumers( const string& req, const Poco::URI::QueryParameters& p )
{
	Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
	Poco::JSON::Array::Ptr jdata = uniset::json::make_child_array(json, "sensors");
	auto my = httpGetMyInfo(json);

	auto oind = uniset_conf()->oind;

	std::list<ParamSInfo> slist;

	ConsumerListInfo emptyList;

	if( !p.empty() )
	{
		if( !p[0].first.empty() )
			slist = uniset::getSInfoList( p[0].first, uniset_conf() );

		if( slist.empty() )
		{
			ostringstream err;
			err << "(request_consumers): Bad request parameter: '" << p[0].first << "'";
			throw uniset::SystemError(err.str());
		}
	}

	uniset_rwmutex_rlock lock(askIOMutex);

	// Проход по списку заданных..
	if( !slist.empty() )
	{
		for( const auto& s : slist )
		{
			auto a = askIOList.find(s.si.id);

			if( a == askIOList.end() )
				jdata->add( getConsumers(s.si.id, emptyList, false) );
			else
				jdata->add( getConsumers(a->first, a->second, false) );
		}
	}
	else // Проход по всему списку
	{
		for( auto && a : askIOList )
		{
			// добавляем только датчики только с непустым списком заказчиков
			auto jret = getConsumers(a.first, a.second, true);

			if( jret )
				jdata->add(jret);
		}
	}

	return json;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::getConsumers(ObjectId sid, ConsumerListInfo& ci, bool ifNotEmpty )
{
	/* Создаём json
	 * { {"sensor":
	 *            {"id": "xxxx"},
	 *            {"name": "xxxx"}
	 *	 },
	 *   "consumers": [
	 *            {..consumer1 info },
	 *            {..consumer2 info },
	 *            {..consumer3 info },
	 *            {..consumer4 info }
	 *	 ]
	 * }
	 */

	Poco::JSON::Object::Ptr jret = new Poco::JSON::Object();

	uniset_rwmutex_rlock lock(ci.mut);

	if( ci.clst.empty() && ifNotEmpty )
		return jret;

	auto oind = uniset_conf()->oind;
	auto jsens = uniset::json::make_child(jret, "sensor");
	jsens->set("id", sid);
	jsens->set("name", ORepHelpers::getShortName(oind->getMapName(sid)));

	auto jcons = uniset::json::make_child_array(jret, "consumers");

	for( const auto& c : ci.clst )
	{
		Poco::JSON::Object::Ptr consumer = new Poco::JSON::Object();
		consumer->set("id", c.id);
		consumer->set("name", ORepHelpers::getShortName(oind->getMapName(c.id)));
		consumer->set("node", c.node);
		consumer->set("node_name", oind->getNodeName(c.node));
		consumer->set("lostEvents", c.lostEvents);
		consumer->set("attempt", c.attempt);
		consumer->set("smCount", c.smCount);
		jcons->add(consumer);
	}

	return jret;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IONotifyController::request_lost( const string& req, const Poco::URI::QueryParameters& p )
{
	Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
	Poco::JSON::Array::Ptr jdata = uniset::json::make_child_array(json, "lost consumers");
	auto my = httpGetMyInfo(json);

	auto oind = uniset_conf()->oind;

	std::lock_guard<std::mutex> lock(lostConsumersMutex);

	for( const auto& c : lostConsumers )
	{
		Poco::JSON::Object::Ptr jcons = new Poco::JSON::Object();
		jcons->set("id", c.first);
		jcons->set("name",  ORepHelpers::getShortName(oind->getMapName(c.first)));
		jcons->set("lostCount",  c.second.count);
		jcons->set("lost", c.second.lost);
		jdata->add(jcons);
	}

	return json;
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
