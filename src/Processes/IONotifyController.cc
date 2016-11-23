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
#include "NCRestorer.h"

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

IONotifyController::IONotifyController(const string& name, const string& section, std::shared_ptr<NCRestorer> d ):
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

IONotifyController::IONotifyController( ObjectId id, std::shared_ptr<NCRestorer> d ):
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
SimpleInfo* IONotifyController::getInfo( const char* userparam )
{
	uniset::SimpleInfo_var i = IOController::getInfo();

	ostringstream inf;

	inf << i->info << endl;

	auto oind = uniset_conf()->oind;

	{
		std::lock_guard<std::mutex> lock(lostConsumersMutex);
		if( lostConsumers.size() > 0 )
		{
			inf << "-------------------------- lost consumers list [maxAttemtps=" << maxAttemtps << "] ------------------" << endl;
			for( const auto& l: lostConsumers )
			{
				inf << "        " << "(" << setw(6) << l.first << ")"
					<< setw(35) << std::left << ORepHelpers::getShortName(oind->getMapName(l.first))
					<< " lostCount=" << l.second.count
					<< endl;
			}
		}
		inf << "----------------------------------------------------------------------------------" << endl;
	}

	const std::string param(userparam);

	if( param == "1" || param == "2" )
	{
		inf << "------------------------------- consumers list ------------------------------" << endl;
		inf << "[userparam=" << userparam << "]" << endl;

		{
			uniset_rwmutex_rlock lock(askIOMutex);

			for( auto && a : askIOList )
			{
				auto& i = a.second;

				uniset_rwmutex_rlock lock(i.mut);

				// отображаем только датчики с "не пустым" списком заказчиков
				if( i.clst.empty() )
					continue;

				// Т.к. сперва выводится имя датчика, а только потом его заказчики
				// то если надо выводить только тех, у кого есть "потери"(lostEvent>0)
				// предварительно смотрим список есть ли там хоть один с "потерями", а потом уже выводим
				if( param == "2" )
				{
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

					// выводим тех у кого lostEvent>0
					inf << "(" << setw(6) << a.first << ")[" << oind->getMapName(a.first) << "]" << endl;

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
				else // просто выводим всех
				{
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
		}
		inf << "-----------------------------------------------------------------------------" << endl << endl;
	}

	inf << "IONotifyController::UserParam help: " << endl
		<< "  0. Common info" << endl
		<< "  1. Consumers list " << endl
		<< "  2. Consumers list with lostEvent > 0" << endl;

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

	for( auto&& it :  lst.clst )
	{
		if( it.id == ci.id && it.node == ci.node )
		{
			// при перезаказе датчиков количество неудачных попыток послать сообщение
			// считаем что "заказчик" опять на связи
			it.attempt = maxAttemtps;

			// выставляем флаг, что заказчик опять "на связи"
			std::lock_guard<std::mutex> lock(lostConsumersMutex);
			auto c = lostConsumers.find(ci.id);
			if( c!= lostConsumers.end() )
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
	if( c!= lostConsumers.end() )
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
	uinfo << "(askSensor): поступил " << ( cmd == UIODontNotify ? "отказ" : "заказ" ) << " от "
		  << uniset_conf()->oind->getNameById(ci.id) << "@" << ci.node
		  << " на аналоговый датчик "
		  << uniset_conf()->oind->getNameById(sid) << endl;

	// если такого аналогового датчика нет, здесь сработает исключение...
	auto li = myioEnd();

	try
	{
		localGetValue(li, sid);
	}
	catch( IOController_i::Undefined& ex ) {}

	{
		// lock
		uniset_rwmutex_wrlock lock(askIOMutex);
		// а раз есть обрабатываем
		ask(askIOList, sid, ci, cmd);
	}    // unlock

	// посылка первый раз состояния
	if( cmd == UniversalIO::UIONotify || (cmd == UIONotifyFirstNotNull && li->second->value) )
	{
		SensorMessage  smsg( std::move(li->second->makeSensorMessage()) );

		try
		{
			ui->send(ci.id, std::move(smsg.transport_msg()), ci.node);
		}
		catch( const uniset::Exception& ex )
		{
			uwarn << myname << "(askSensor): " <<  uniset_conf()->oind->getNameById(sid) << " error: " << ex << endl;
		}
		catch( const CORBA::SystemException& ex )
		{
			uwarn << myname << "(askSensor): " << uniset_conf()->oind->getNameById(ci.id) << "@" << ci.node
				  << " недоступен!!(CORBA::SystemException): "
				  << ex.NP_minorString() << endl;
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
			if( askIterator == askLst.end() )
			{
				ConsumerListInfo lst; // создаем новый список
				addConsumer(lst, cons);
				// более оптимальный способ(при условии вставки первый раз)
				askLst.emplace(sid, std::move(lst));

				try
				{
					dumpOrdersList(sid, lst);
				}
				catch( const uniset::Exception& ex )
				{
					uwarn << myname << " не смогли сделать dump: " << ex << endl;
				}
			}
			else
			{
				if( addConsumer(askIterator->second, cons) )
				{
					try
					{
						dumpOrdersList(sid, askIterator->second);
					}
					catch( const std::exception& ex )
					{
						uwarn << myname << " не смогли сделать dump: " << ex.what() << endl;
					}
					catch(...)
					{
						uwarn << myname << " не смогли сделать dump (catch...)" << endl;
					}
				}
			}

			break;
		}

		case UniversalIO::UIODontNotify:     // отказ
		{
			if( askIterator != askLst.end() )  // существует
			{
				if( removeConsumer(askIterator->second, cons) )
				{
					uniset_rwmutex_wrlock l(askIterator->second.mut);

					if( askIterator->second.clst.empty() )
					{
						//                         не удаляем, т.к. могут поломаться итераторы
						//                         используемые в это время в других потоках..
						//                         askLst.erase(askIterator);
					}
					else
					{
						try
						{
							dumpOrdersList(sid, askIterator->second);
						}
						catch( const std::exception& ex )
						{
							uwarn << myname << " не смогли сделать dump: " << ex.what() << endl;
						}
						catch(...)
						{
							uwarn << myname << " не смогли сделать dump (catch...)" << endl;
						}
					}
				}
			}

			break;
		}

		default:
			break;
	}

	if( askIterator == askLst.end() )
		askIterator = askLst.find(sid);

	if( askIterator != askLst.end() )
	{
		//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
		auto s = myiofind(sid);

		if( s != myioEnd() )
			s->second->userdata[udataConsumerList] = &(askIterator->second);
		else
			s->second->userdata[udataConsumerList] = nullptr;
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

	SensorMessage sm(1); // <-- вызываем dummy конструктор т.к. потом все поля всё-равно сами инициализируем
	sm.id           = usi->si.id;
	sm.node         = usi->si.node; // uniset_conf()->getLocalNode();
	sm.value        = curValue;
	sm.priority     = (Message::Priority)usi->priority;
	sm.supplier     = sup_id; // owner_id
	sm.sensor_type  = usi->type;

	// копируем под lock только изменяемую часть
	// lock
	{
		uniset_rwmutex_rlock lock(usi->val_lock);
		sm.undefined    = usi->undefined;
		sm.sm_tv.tv_sec    = usi->tv_sec;
		sm.sm_tv.tv_nsec   = usi->tv_nsec;
		sm.ci = usi->ci;
	} // unlock value

	sm.tm = sm.sm_tv;

	try
	{
		if( !usi->dbignore )
			logging(sm);
	}
	catch(...) {}

	{
		uniset_rwmutex_rlock lock(askIOMutex);

		//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
		if( usi->userdata[udataConsumerList] != nullptr )
		{
			ConsumerListInfo& lst = *(static_cast<ConsumerListInfo*>(usi->userdata[udataConsumerList]));
			send(lst, sm);
		}
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
void IONotifyController::send( ConsumerListInfo& lst, const uniset::SensorMessage& sm )
{
	TransportMessage tmsg(sm.transport_msg());

	uniset_rwmutex_wrlock l(lst.mut);

	for( ConsumerList::iterator li = lst.clst.begin(); li != lst.clst.end(); ++li )
	{
		for( int i = 0; i < sendAttemtps; i++ ) // на каждый объект по две попытки послать
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
	// сперва вычитаем датчиков и заказчиков..
	readDump();
	// а потом уже собственно активация..
	return IOController::activateObject();
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::readDump()
{
	try
	{
		if( restorer != NULL )
			restorer->read(this);
	}
	catch( const std::exception& ex )
	{
		uwarn << myname << "(IONotifyController::readDump): " << ex.what() << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------
void IONotifyController::initItem( std::shared_ptr<USensorInfo>& usi, IOController* ic )
{
	if( usi->type == UniversalIO::AI || usi->type == UniversalIO::AO )
		checkThreshold( usi, false );
}
// ------------------------------------------------------------------------------------------
void IONotifyController::dumpOrdersList( const uniset::ObjectId sid,
		const IONotifyController::ConsumerListInfo& lst )
{
	if( restorer == NULL )
		return;

	try
	{
		IOController_i::SensorIOInfo ainf( getSensorIOInfo(sid) );
		// делаем через tmp, т.к. у нас определён operator=
		NCRestorer::SInfo tmp = ainf;
		auto sinf = make_shared<NCRestorer::SInfo>( std::move(tmp) );
		restorer->dump(this, sinf, lst);
	}
	catch( const uniset::Exception& ex )
	{
		uwarn << myname << "(IONotifyController::dumpOrderList): " << ex << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::dumpThresholdList( const uniset::ObjectId sid, const IONotifyController::ThresholdExtList& lst )
{
	if( restorer == NULL )
		return;

	try
	{
		IOController_i::SensorIOInfo ainf(getSensorIOInfo(sid));
		auto sinf = make_shared<NCRestorer::SInfo>( std::move(ainf) );
		restorer->dumpThreshold(this, sinf, lst);
	}
	catch( const uniset::Exception& ex )
	{
		uwarn << myname << "(IONotifyController::dumpThresholdList): " << ex << endl;
	}
}
// --------------------------------------------------------------------------------------------------------------

void IONotifyController::askThreshold(uniset::ObjectId sid, const uniset::ConsumerInfo& ci,
									  uniset::ThresholdId tid,
									  CORBA::Long lowLimit, CORBA::Long hiLimit,  CORBA::Boolean invert,
									  UniversalIO::UIOCommand cmd )
{
	if( lowLimit > hiLimit )
		throw IONotifyController_i::BadRange();

	// если такого дискретного датчика нет сдесь сработает исключение...
	auto li = myioEnd();

	CORBA::Long val = 0;

	try
	{
		val = localGetValue(li, sid);
	}
	catch( const IOController_i::Undefined& ex ) {}

	{
		// lock
		uniset_rwmutex_wrlock lock(trshMutex);

		// поиск датчика в списке
		auto it = askTMap.find(sid);

		ThresholdInfoExt ti(tid, lowLimit, hiLimit, invert);
		ti.sit = myioEnd();

		switch( cmd )
		{
			case UniversalIO::UIONotify: // заказ
			case UniversalIO::UIONotifyChange:
			{
				if( it == askTMap.end() )
				{
					ThresholdExtList lst;    // создаем новый список
					ThresholdsListInfo tli;
					tli.si.id   = sid;
					tli.si.node = uniset_conf()->getLocalNode();
					tli.list   = std::move(lst);
					tli.type   = li->second->type;
					tli.usi	   = li->second;

					// после этого вызова ti использовать нельзя
					addThreshold(tli.list, std::move(ti), ci);

					try
					{
						dumpThresholdList(sid, tli.list);
					}
					catch( const uniset::Exception& ex )
					{
						uwarn << myname << " не смогли сделать dump: " << ex << endl;
					}

					// т.к. делаем move... то надо гарантировать, что дальше уже tli не используется..
					askTMap.emplace(sid, std::move(tli));
				}
				else
				{
					if( addThreshold(it->second.list, std::move(ti), ci) )
					{
						try
						{
							dumpThresholdList(sid, it->second.list);
						}
						catch( const uniset::Exception& ex )
						{
							uwarn << myname << "(askThreshold): dump: " << ex << endl;
						}
					}
				}

				if( cmd == UniversalIO::UIONotifyChange )
					break;

				// посылка первый раз состояния
				try
				{
					SensorMessage sm;
					sm.id         = sid;
					sm.node       = uniset_conf()->getLocalNode();
					sm.value      = val;
					sm.undefined  = li->second->undefined;
					sm.sensor_type  = li->second->type;
					sm.priority   = (Message::Priority)li->second->priority;
					sm.consumer   = ci.id;
					sm.tid        = tid;
					sm.ci         = li->second->ci;

					// Проверка нижнего предела
					if( val <= lowLimit )
					{
						sm.threshold = false;
						CORBA::Object_var op = ui->resolve(ci.id, ci.node);
						UniSetObject_i_var ref = UniSetObject_i::_narrow(op);

						if(!CORBA::is_nil(ref))
							ref->push( std::move(sm.transport_msg()) );
					}
					// Проверка верхнего предела
					else if( val >= hiLimit )
					{
						sm.threshold = true;
						CORBA::Object_var op = ui->resolve(ci.id, ci.node);
						UniSetObject_i_var ref = UniSetObject_i::_narrow(op);

						if(!CORBA::is_nil(ref))
							ref->push( std::move(sm.transport_msg()) );
					}
				}
				catch( const uniset::Exception& ex )
				{
					uwarn << myname << "(askThreshod): " << ex << endl;
				}
				catch( const CORBA::SystemException& ex )
				{
					uwarn << myname << "(askThreshod): CORBA::SystemException: "
						  << ex.NP_minorString() << endl;
				}
			}
			break;

			case UniversalIO::UIODontNotify:     // отказ
			{
				if( it != askTMap.end() )
				{
					if( removeThreshold(it->second.list, ti, ci) )
					{
						try
						{
							dumpThresholdList(sid, it->second.list);
						}
						catch( const uniset::Exception& ex )
						{
							uwarn << myname << "(askThreshold): dump: " << ex << endl;
						}
					}
				}
			}
			break;

			default:
				break;
		}

		it = askTMap.find(sid);

		if( li != myioEnd() )
		{
			//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
			if( it == askTMap.end() )
				li->second->userdata[udataThresholdList] = nullptr;
			else
				li->second->userdata[udataThresholdList] = (void*)(&(it->second));
		}

	}    // unlock
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::addThreshold( ThresholdExtList& lst, ThresholdInfoExt&& ti, const uniset::ConsumerInfo& ci )
{
	for( auto it = lst.begin(); it != lst.end(); ++it)
	{
		if( ti == (*it) )
		{
			if( addConsumer(it->clst, ci) )
				return true;

			return false;
		}
	}

	addConsumer(ti.clst, ci);

	// запоминаем начальное время
	struct timespec tm = uniset::now_to_timespec();
	ti.tv_sec  = tm.tv_sec;
	ti.tv_nsec = tm.tv_nsec;

	lst.emplace_back( std::move(ti) );
	return true;
}
// --------------------------------------------------------------------------------------------------------------
bool IONotifyController::removeThreshold( ThresholdExtList& lst, ThresholdInfoExt& ti, const uniset::ConsumerInfo& ci )
{
	for( auto it = lst.begin(); it != lst.end(); ++it)
	{
		if( ti == (*it) )
		{
			if( removeConsumer(it->clst, ci) )
			{
				/*                Даже если список заказчиков по данному датчику стал пуст.
				                  Не удаляем датчик из списка, чтобы не поломать итераторы
				                  которые могут использоваться в этот момент в других потоках */
				//                uniset_rwmutex_wrlock lock(it->clst.mut);
				//                if( it->clst.clst.empty() )
				//                    lst.erase(it);
				return true;
			}
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
	// поиск списка порогов
	ThresholdsListInfo* ti = nullptr;

	{
		uniset_rwmutex_rlock lock(trshMutex);

		//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
		if( usi->userdata[udataThresholdList] == nullptr )
			return;

		//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
		ti = static_cast<ThresholdsListInfo*>(usi->userdata[udataThresholdList]);

		if( ti->list.empty() )
			return;
	}

	SensorMessage sm(std::move(usi->makeSensorMessage()));

	// текущее время
	struct timespec tm = uniset::now_to_timespec();

	{
		uniset_rwmutex_rlock l(ti->mut);

		for( auto it = ti->list.begin(); it != ti->list.end(); ++it )
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
			sm.threshold = ( state != IONotifyController_i::NormalThreshold ) ? true : false;

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
				send(it->clst, sm);
		}
	}
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController::ThresholdExtList::iterator IONotifyController::findThreshold( const uniset::ObjectId sid, const uniset::ThresholdId tid )
{
	{
		// lock
		uniset_rwmutex_rlock lock(trshMutex);
		// поиск списка порогов
		auto lst = askTMap.find(sid);

		if( lst != askTMap.end() )
		{
			for( auto it = lst->second.list.begin(); it != lst->second.list.end(); ++it)
			{
				if( it->id == tid )
					return it;
			}
		}
	}

	return ThresholdExtList::iterator();
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdInfo IONotifyController::getThresholdInfo( uniset::ObjectId sid, uniset::ThresholdId tid )
{
	uniset_rwmutex_rlock lock(trshMutex);

	auto it = askTMap.find(sid);

	if( it == askTMap.end() )
	{
		ostringstream err;
		err << myname << "(getThresholds): Not found sensor (" << sid << ") "
			<< uniset_conf()->oind->getNameById(sid);

		uinfo << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	for( const auto& it2 : it->second.list )
	{
		/*! \warning На самом деле список разрешает иметь много порогов с одинаковым ID, для разных "заказчиков".
		    Но здесь мы возвращаем первый встретившийся..
		*/
		if( it2.id == tid )
			return IONotifyController_i::ThresholdInfo( it2 );
	}

	ostringstream err;
	err << myname << "(getThresholds): Not found for sensor (" << sid << ") "
		<< uniset_conf()->oind->getNameById(sid) << " ThresholdID='" << tid << "'";

	uinfo << err.str() << endl;
	throw IOController_i::NameNotFound(err.str().c_str());
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdList* IONotifyController::getThresholds( uniset::ObjectId sid )
{
	uniset_rwmutex_rlock lock(trshMutex);

	auto it = askTMap.find(sid);

	if( it == askTMap.end() )
	{
		ostringstream err;
		err << myname << "(getThresholds): Not found sensor (" << sid << ") "
			<< uniset_conf()->oind->getNameById(sid);

		uinfo << err.str() << endl;
		throw IOController_i::NameNotFound(err.str().c_str());
	}

	IONotifyController_i::ThresholdList* res = new IONotifyController_i::ThresholdList();

	try
	{
		res->si     = it->second.si;
		res->value  = IOController::localGetValue(it->second.usi);
		res->type   = it->second.type;
	}
	catch( const uniset::Exception& ex )
	{
		uwarn << myname << "(getThresholds): для датчика "
			  << uniset_conf()->oind->getNameById(it->second.si.id)
			  << " " << ex << endl;
	}

	/*
		catch( const IOController_i::NameNotFound& ex )
		{
			uwarn << myname << "(getThresholds): IOController_i::NameNotFound.. for sid"
				  << uniset_conf()->oind->getNameById(it->second.si.id)
				  << endl;
		}
	*/
	res->tlist.length( it->second.list.size() );

	size_t k = 0;

	for( const auto& it2 : it->second.list )
	{
		res->tlist[k].id       = it2.id;
		res->tlist[k].hilimit  = it2.hilimit;
		res->tlist[k].lowlimit = it2.lowlimit;
		res->tlist[k].state    = it2.state;
		res->tlist[k].tv_sec   = it2.tv_sec;
		res->tlist[k].tv_nsec  = it2.tv_nsec;
		k++;
	}

	return res;
}
// --------------------------------------------------------------------------------------------------------------
IONotifyController_i::ThresholdsListSeq* IONotifyController::getThresholdsList()
{
	IONotifyController_i::ThresholdsListSeq* res = new IONotifyController_i::ThresholdsListSeq();

	res->length( askTMap.size() );

	uniset_rwmutex_rlock lock(trshMutex);

	if( !askTMap.empty() )
	{
		size_t i = 0;

		for( auto && it : askTMap )
		{
			try
			{
				(*res)[i].si    = it.second.si;
				(*res)[i].value = IOController::localGetValue(it.second.usi);
				(*res)[i].type  = it.second.type;
			}
			catch( const std::exception& ex )
			{
				uwarn << myname << "(getThresholdsList): for sid="
					  << uniset_conf()->oind->getNameById(it.second.si.id)
					  << " " << ex.what() << endl;
				continue;
			}
			catch( const IOController_i::NameNotFound& ex )
			{
				uwarn << myname << "(getThresholdsList): IOController_i::NameNotFound.. for sid="
					  << uniset_conf()->oind->getNameById(it.second.si.id)
					  << endl;

				continue;
			}

			(*res)[i].tlist.length( it.second.list.size() );

			size_t k = 0;

			for( const auto& it2 : it.second.list )
			{
				(*res)[i].tlist[k].id       = it2.id;
				(*res)[i].tlist[k].hilimit  = it2.hilimit;
				(*res)[i].tlist[k].lowlimit = it2.lowlimit;
				(*res)[i].tlist[k].state    = it2.state;
				(*res)[i].tlist[k].tv_sec   = it2.tv_sec;
				(*res)[i].tlist[k].tv_nsec  = it2.tv_nsec;
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
	SensorMessage sm( std::move(usi->makeSensorMessage()) );

	try
	{
		if( !usi->dbignore )
			logging(sm);
	}
	catch(...) {}

	{
		// lock
		uniset_rwmutex_rlock lock(askIOMutex);

		//! \warning Оптимизация использует userdata! Это опасно, если кто-то ещё захочет его использовать!
		if( usi->userdata[udataConsumerList] != nullptr )
		{
			ConsumerListInfo& lst = *(static_cast<ConsumerListInfo*>(usi->userdata[udataConsumerList]));
			send(lst, sm);
		}
	} // unlock
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
nlohmann::json IONotifyController::httpHelp(const Poco::URI::QueryParameters& p)
{
	nlohmann::json jdata = IOController::httpHelp(p);
	jdata[myname]["help"]["consumers"]["desc"] = "get consumers list";
	jdata[myname]["help"]["consumers"]["params"] = {"sensor1,sensor2,sensor3","get consumers for sensors"};
	jdata[myname]["help"]["lost"]["desc"] = "get lost consumers list";
	return std::move(jdata);
}
// -----------------------------------------------------------------------------
nlohmann::json IONotifyController::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
	if( req == "consumers" )
		return request_consumers(req,p);

	if( req == "lost" )
		return request_lost(req,p);

	return IOController::httpRequest(req,p);
}
// -----------------------------------------------------------------------------
nlohmann::json IONotifyController::request_consumers( const string& req, const Poco::URI::QueryParameters& p )
{
	//! \todo Не реализовано
	nlohmann::json json;

	auto& jdata = json[myname]["consumers"];

	auto oind = uniset_conf()->oind;

	std::list<ParamSInfo> slist;
	if( p.size() > 0 )
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
		auto& jnotfound = json[myname]["notfound"];

		for( const auto& s: slist )
		{
			auto a = askIOList.find(s.si.id);
			if( a == askIOList.end() )
			{
				jnotfound.push_back(std::to_string(s.si.id));
				continue;
			}

			// Включаем в ответ все, даже если список заказчиков пустой
			jdata.push_back( getConsumers(a->first,a->second,false) );
		}
	}
	else // Проход по всему списку
	{
		for( auto&& a : askIOList )
		{
			// добавляем только датчики с непустым списком заказчиков
			auto jret = getConsumers(a.first,a.second,true);
			if( !jret.empty() )
				jdata.push_back( std::move(jret) );
		}
	}

	return std::move(json);
}
// -----------------------------------------------------------------------------
nlohmann::json IONotifyController::getConsumers( ObjectId sid, ConsumerListInfo& ci, bool noEmpty )
{
	nlohmann::json jret;
	auto oind = uniset_conf()->oind;

	uniset_rwmutex_rlock lock(ci.mut);

	if( ci.clst.empty() && noEmpty )
		return std::move(jret);

	string strID( std::to_string(sid) );
	auto& jsens = jret[strID];

	jsens["id"] = strID;
	jsens["sensor_name"] = ORepHelpers::getShortName(oind->getMapName(sid));
	auto& jcons = jsens["consumers"];

	for( const auto& c : ci.clst )
	{
		string cid( std::to_string(c.id) );
		auto& jconsinfo = jcons[cid];
		jconsinfo["id"] = c.id;
		jconsinfo["name"] = ORepHelpers::getShortName(oind->getMapName(c.id));
		jconsinfo["lostEvents"] = c.lostEvents;
		jconsinfo["attempt"] = c.attempt;
		jconsinfo["smCount"] = c.smCount;
	}

	return std::move(jret);
}
// -----------------------------------------------------------------------------
nlohmann::json IONotifyController::request_lost( const string& req, const Poco::URI::QueryParameters& p )
{
	//! \todo Не реализовано
	nlohmann::json json;

	auto& jdata = json[myname]["lost consumers"];

	auto oind = uniset_conf()->oind;

	std::lock_guard<std::mutex> lock(lostConsumersMutex);

	for( const auto& c: lostConsumers )
	{
		string cid( std::to_string(c.first) );
		auto& jcons = jdata[cid];

		jcons["id"] = c.first;
		jcons["name"] = ORepHelpers::getShortName(oind->getMapName(c.first));
		jcons["lostCount"] = c.second.count;
		jcons["lost"] = c.second.lost;
	}

	return std::move(json);
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
