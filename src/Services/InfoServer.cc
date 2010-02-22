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
 *  \brief файл реализации Info-сервера
 *  \author Pavel Vainerman
 *  \date   $Date: 2007/01/17 23:33:41 $
 *  \version $Id: InfoServer.cc,v 1.14 2007/01/17 23:33:41 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#include <string>
#include <sstream>
#include "Configuration.h"
#include "InfoServer.h"
#include "ISRestorer.h"
#include "UniXML.h"
// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------

InfoServer::InfoServer( ObjectId id, ISRestorer* d ):
	UniSetObject(id),
	restorer(d),
	dbrepeat(true)
{
	if( id == DefaultObjectId )
	{
		id = conf->getInfoServer();
		if( id == DefaultObjectId )
		{
			ostringstream msg;
			msg << "(InfoServer): Запуск невозможен! НЕ ЗАДАН ObjectId !!!!!\n";
//			unideb[Debug::CRIT] << msg.str() << endl;
			throw Exception(msg.str());
		}
		setID(id);
	}
	
	routeList.clear();

	UniXML* xml = conf->getConfXML();
	if( xml )
	{
		xmlNode* root = xml->findNode(xml->getFirstNode(),"LocalInfoServer");
		if( root )
		{
			dbrepeat = xml->getIntProp(root,"dbrepeat");
			if( !dbrepeat )
				unideb[Debug::INFO] << myname << "(init): dbrepeat="<< dbrepeat << endl;
			
			xmlNode* node(xml->findNode(root,"RouteList"));
			if(!node) 
				unideb[Debug::WARN] << myname << ": старый формат конф-файла. Нет раздела RouteList" << endl;
			else
			{
				UniXML_iterator it(node);
				if( it.goChildren() )
				{
					for( ;it.getCurrent(); it.goNext() )
					{
						string cname(xml->getProp(it,"name"));
						ConsumerInfo ci;
						ci.id =	 conf->oind->getIdByName(cname);
						if( ci.id == UniSetTypes::DefaultObjectId )
						{
							unideb[Debug::CRIT] << myname << ": НЕ НАЙДЕН ИДЕНТИФИКАТОР объекта -->" << cname << endl;
							continue;
						}
					
						ci.node = conf->getLocalNode();
						string cnodename(xml->getProp(it,"node"));
						if( !cnodename.empty() )
							ci.node = conf->oind->getIdByName(cnodename);
						if( ci.node == UniSetTypes::DefaultObjectId )
						{
							unideb[Debug::CRIT] << myname << ": НЕ НАЙДЕН ИДЕНТИФИКАТОР узла -->" << cnodename << endl;
							continue;
						}
						routeList.push_back(ci);
					}
				}	
			}
		}
	}
}

InfoServer::~InfoServer()
{
}

// ------------------------------------------------------------------------------------------
void InfoServer::preprocessing(TransportMessage& tmsg, bool broadcast)
{
//	unideb[Debug::INFO] << myname << "... preprocessing... "<< endl;

	// Пересылаем на другие узлы
	if( broadcast )
	{
		for ( UniSetTypes::ListOfNode::const_iterator it = conf->listNodesBegin();
			it != conf->listNodesEnd(); ++it )
		{
			if( it->infserver!=UniSetTypes::DefaultObjectId && it->connected && it->id != conf->getLocalNode() )
			{
				try
				{	
					ui.send(it->infserver, tmsg, it->id);
				}
				catch(...)
				{
					unideb[Debug::CRIT] << myname << "(preprocessing): не смог послать сообщение узлу "<< conf->oind->getMapName(it->id)<< endl;
				}
			}
		}
	}

	

//	unideb[Debug::INFO] << myname << " пишем в базу... "<< endl;
	// Сохраняем в базу
	try
	{
		if( dbrepeat )
			ui.send(conf->getDBServer(), tmsg);
	}
	catch(...)
	{
		unideb[Debug::CRIT] << myname << "(preprocessing): не смог послать сообщение DBServer-у" << endl;
	}

	
	// Пересылаем по routeList-у
	for( list<UniSetTypes::ConsumerInfo>::const_iterator it=routeList.begin(); it!=routeList.end(); ++it )
	{
		try
		{	
			ui.send(it->id, tmsg, it->node);
		}
		catch(...)
		{
			unideb[Debug::CRIT] << myname << "(preprocessing):"
				<< " не смог послать сообщение объекту "
				<< conf->oind->getNameById(it->id,it->node)<< endl;
		}
	}
}

// ------------------------------------------------------------------------------------------
void InfoServer::preprocessingConfirm(UniSetTypes::ConfirmMessage& am, bool broadcast)
{
//	unideb[Debug::INFO] << myname << "... preprocessing... "<< endl;

	TransportMessage tmsg(am.transport_msg());
	// Пересылаем на другие узлы
	if( broadcast )
	{
		for ( UniSetTypes::ListOfNode::const_iterator it = conf->listNodesBegin();
			it != conf->listNodesEnd(); ++it )
		{
			if( it->infserver!=UniSetTypes::DefaultObjectId && it->connected && it->id != conf->getLocalNode() )
			{
				try
				{	
					ui.send(it->infserver, tmsg, it->id);
				}
				catch(...)
				{
					unideb[Debug::CRIT] << myname << "(preprocessing):"
						<< " не смог послать сообщение узлу "
						<< conf->oind->getMapName(it->id)<< endl;
				}
			}
		}
	}

	try
	{
		if( dbrepeat )
			ui.send(conf->getDBServer(), tmsg);
	}
	catch(...)
	{
		unideb[Debug::CRIT] << myname << "(preprocessing): не смог послать сообщение DBServer-у" << endl;
	}

	// Пересылаем по routeList-у
	for( list<UniSetTypes::ConsumerInfo>::const_iterator it=routeList.begin(); it!=routeList.end(); ++it )
	{
		try
		{	
			ui.send(it->id, tmsg, it->node);
		}
		catch(...)
		{
			unideb[Debug::CRIT] << myname << "(preprocessing):"
					<< " не смог послать сообщение объекту "
					<< conf->oind->getNameById(it->id,it->node) << endl;
		}
	}
}

// ------------------------------------------------------------------------------------------

void InfoServer::processingMessage( UniSetTypes::VoidMessage *msg )
{
	switch( msg->type )
	{
		case Message::Info:
		{
			InfoMessage im(msg);
			unideb[Debug::INFO] << myname << " InfoMessage code= "<< im.infocode << endl;
			try
			{
				// если это не пересланное сообщение 
				// то обрабатываем его
				if( !im.route )
				{
					im.route = true;	// выставляем признак пересылки
					TransportMessage tm(im.transport_msg());
					preprocessing(tm, im.broadcast);
				}
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(info preprocessing):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(info preprocessing): catch ..." << endl;
			}

			// посылаем всем зазкачикам уведомление
			try
			{
				event(im.infocode, im, false);
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(info event):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(info event): catch ..." << endl;
			}

			try
			{
				processing(im);
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(info processing):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(info processing) catch ..." << endl;
			}
			break;
		}

		case Message::Alarm:
		{
			AlarmMessage am(msg);
	
			unideb[Debug::INFO] << myname 
				<< " AlarmMessage code= "<< am.alarmcode
				<< " cause="<< am.causecode << endl;

			try
			{
				// если это не пересланное сообщение 
				// то обрабатываем его
				if( !am.route )
				{
					am.route = true;	// выставляем признак пересылки
					TransportMessage tm(am.transport_msg());
					preprocessing(tm, am.broadcast);
				}
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(alarm preprocessing):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(alarm preprocessing): catch ..." << endl;
			}

			try
			{
				// посылаем всем зазкачикам уведомление
				event(am.alarmcode, am, false);
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(alarm event):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(alarm event): catch ..." << endl;
			}
			
			try
			{
				processing(am);
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(alarm processing):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(alarm processing): catch ..." << endl;
			}

			break;
		}

		case Message::Confirm:
		{
			UniSetTypes::ConfirmMessage cm(msg);
	
			unideb[Debug::INFO] << myname << " ConfirmMessage на сообщение code= "<< cm.code << endl;
			try
			{
				// если это не пересланное сообщение 
				// то обрабатываем его
				if( !cm.route )
				{
					cm.route = true;
					preprocessingConfirm(cm, cm.broadcast);
				}
			}
			catch(Exception& ex )
			{
				unideb[Debug::CRIT] << myname << "(alarm processing):" << ex << endl;
			}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(alarm processing): catch ..." << endl;
			}

		
			// посылаем всем зазкачикам уведомление
			event(cm.code, cm, true);
			
			try
			{
				processing(cm);
			}
			catch(...){}

			break;
		}

		case Message::SysCommand:
			unideb[Debug::INFO] << myname << " system command... "<< endl;
			break;

		default:
			unideb[Debug::CRIT] << myname << ": НЕИЗВЕСТНОЕ СООБЩЕНИЕ"<< endl;
			break;
	}
	
}
// ------------------------------------------------------------------------------------------
void InfoServer::ackMessage(UniSetTypes::MessageCode mid, const UniSetTypes::ConsumerInfo& ci, 
							UniversalIO::UIOCommand cmd, CORBA::Boolean acknotify)
{
	unideb[Debug::INFO] << myname << "(askMessage): поступил заказ от "
		<< conf->oind->getNameById(ci.id, ci.node)
		<< " на сообщение " << mid << endl;

	// Проверка на существование
	if(!conf->mi->isExist(mid) )
	{
		unideb[Debug::CRIT] << myname << "(askMessage): сообщения с кодом "
				<< mid << " НЕТ в MessagesMap" << endl;

//		InfoServer_i::MsgNotFound nf;
//		nf.bad_code = mid;
//		throw nf;
	}

	{	// lock
 		uniset_mutex_lock lock(askMutex, 200);
		// а раз есть заносим(исключаем) заказчика 
		ask( askList, mid, ci, cmd, acknotify);		
	}	// unlock
}
// ------------------------------------------------------------------------------------------
void InfoServer::ackMessageRange(UniSetTypes::MessageCode from, UniSetTypes::MessageCode to, 
						const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd, 
						CORBA::Boolean acknotify)
{
	// Проверка корректности диапазона
	if( from>to )
		throw InfoServer_i::MsgBadRange();


	for( UniSetTypes::MessageCode c=from; c<=to; c++ )
		ackMessage(c,ci,cmd, acknotify);
}
// ------------------------------------------------------------------------------------------
/*!
 *	\param lst - указатель на список в который необходимо внести потребителя
 *	\param name - имя вносимого потребителя
 *	\note Добавление произойдет только если такого потребителя не существует в списке
*/
bool InfoServer::addConsumer(ConsumerList& lst, const ConsumerInfo& ci, CORBA::Boolean acknotify )
{
	for( ConsumerList::const_iterator li=lst.begin();li!=lst.end(); ++li )
	{
		if( li->id == ci.id && li->node == ci.node )
			return false;
	}
	
	ConsumerInfoExt cinf(ci);
	cinf.ask = acknotify;
	// получаем ссылку
	try
	{
		UniSetTypes::ObjectVar op = ui.resolve(ci.id,ci.node);
		cinf.ref = UniSetObject_i::_narrow(op);
	}
	catch(...){}
	
	lst.push_front(cinf);
	return true;
}

// ------------------------------------------------------------------------------------------
/*!
 *	\param lst - указатель на список из которго происходит удаление потребителя
 *	\param name - имя удаляемого потребителя
*/
bool InfoServer::removeConsumer(ConsumerList& lst, const ConsumerInfo& cons, CORBA::Boolean acknotify )
{
	for( ConsumerList::iterator li=lst.begin();li!=lst.end();++li)
	{
		if( li->id == cons.id && li->node == cons.node  )		
		{
			lst.erase(li);
			return true;
		}
	}
	
	return false;
}

// ------------------------------------------------------------------------------------------
void InfoServer::ask(AskMap& askLst, UniSetTypes::MessageCode key, 
					const UniSetTypes::ConsumerInfo& cons, 
					UniversalIO::UIOCommand cmd,
					CORBA::Boolean acknotify)
{
	// поиск датчика в списке 
	AskMap::iterator askIterator = askLst.find(key);

  	switch( cmd )
	{
		case UniversalIO::UIONotify: // заказ
		{
   			if( askIterator==askLst.end() ) 
			{
				ConsumerList lst; // создаем новый список
				addConsumer(lst,cons, acknotify);	  
				askLst.insert(AskMap::value_type(key,lst));	// более оптимальный способ(при условии вставки первый раз)
				try
				{
					dumpOrdersList(key,lst);
				}
				catch(Exception& ex)
				{
					unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
				}
				catch(...)
				{
			    	unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
				}
		    }
			else
			{
				if( addConsumer(askIterator->second,cons, acknotify) )
				{  
					try
					{
						dumpOrdersList(key,askIterator->second);
					}
					catch(...)
					{	
				    	unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
					}
				}
		    }
			break;
		}

		case UniversalIO::UIODontNotify: 	// отказ
		{
			if( askIterator!=askLst.end() )	// существует
			{
				if( removeConsumer(askIterator->second, cons, acknotify) )
				{
					if( askIterator->second.empty() )
						askLst.erase(askIterator);	
					else
					{
						try
						{
							dumpOrdersList(key,askIterator->second);
						}
						catch(Exception& ex)
						{
							unideb[Debug::WARN] << myname << " не смогли сделать dump: " << ex << endl;
						}
						catch(...)
						{	
				    		unideb[Debug::WARN] << myname << " не смогли сделать dump" << endl;
						}
					}
				}
			}
			break;
		}
	
		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void InfoServer::dumpOrdersList(UniSetTypes::MessageCode mid, const ConsumerList& lst)
{
	try
	{
		if(restorer)
			restorer->dump(this,mid,lst);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname << "(dumpOrdersList): " << ex << endl;
	}
			
}
// ------------------------------------------------------------------------------------------
bool InfoServer::activateObject()
{
	UniSetObject::activateObject();
	readDump();
	return true;
}
// ------------------------------------------------------------------------------------------
void InfoServer::readDump()
{
	try
	{
		if( restorer )
			restorer->read(this);
	}
	catch(Exception& ex)
	{ 
		unideb[Debug::WARN] << myname << "(readDump): " << ex << endl;
	}
}
// ------------------------------------------------------------------------------------------
/*!
	\warning В случае зависания в функции push, будет остановлена рассылка другим объектам.
*/
template<class TMessage>
void InfoServer::send(ConsumerList& lst, TMessage& msg, CORBA::Boolean askn)
{
	for( ConsumerList::iterator li=lst.begin();li!=lst.end();++li )
	{
		// пропускаем всех кто не хочет знать о подтверждении
		if( askn && !li->ask )
			continue;
		
		for(int i=0; i<2; i++ )	// на каждый объект по две поптыки
		{
		    try
		    {
				if( CORBA::is_nil(li->ref) )
				{	
					CORBA::Object_var op = ui.resolve(li->id, li->node);			
					li->ref = UniSetObject_i::_narrow(op);
				}

				msg.consumer = li->id;
				li->ref->push( msg.transport_msg() );
//				unideb[Debug::INFO] << myname << "(send): посылаем "<< conf->oind->getMapName( li->node ) << "/" << ui.getNameById( li->id ) << " notify" << endl;
				break;					
		    }
			catch(Exception& ex)
			{
			   	unideb[Debug::CRIT] << myname << "(send): " << ex << endl;
			}
		    catch( CORBA::SystemException& ex )
		    {
		    	unideb[Debug::CRIT] << myname << "(send): " 
						<< conf->oind->getNameById( li->id )
						<< " недоступен!! " << ex.NP_minorString() << endl;
	    	}
			catch(...)
			{
				unideb[Debug::CRIT] << myname << "(send): "
						<< conf->oind->getNameById( li->id ) 
						<< " недоступен!!(...)" << endl;
			}	
			
//			li->ref = 0;
			li->ref=UniSetObject_i::_nil();
		}
	}
}
// ------------------------------------------------------------------------------------------
template <class TMessage>
void InfoServer::event(UniSetTypes::MessageCode key, TMessage& msg, CORBA::Boolean askn)
{
	{	// lock
		uniset_mutex_lock lock(askMutex, 1000);	

		// поcылка сообщения об изменении всем потребителям
		AskMap::iterator it = askList.find(key);
		if( it!=askList.end() )
			send(it->second, msg,askn);
	}	// unlock 
}
// ------------------------------------------------------------------------------------------
