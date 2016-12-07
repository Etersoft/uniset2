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
 * \brief Интерфейс к объекту сохраняющему список заказчиков в файл и умеющему его потом прочитать
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef Restorer_H_
#define Restorer_H_
// --------------------------------------------------------------------------
#include <memory>
#include <sigc++/sigc++.h>
#include <string>
#include "UniXML.h"
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace uniset
{
/*!
    Это абстрактный интерфейс.
    Содержит общие для всех xxx_XML интерфейсов функции.
    Расчитан на работу с файлом формата файла проекта.
*/
class Restorer_XML
{
	public:

		Restorer_XML();
		virtual ~Restorer_XML();

		/*! слот для подключения функции чтения датчика из xml-файла.
		    \param uxml    - интерфейс для работы с xml-файлом
		    \param it     - итератор (указатель) на текущий считываемый xml-узел (item)
		    \param sec    - итератор (указатель) на корневой узел секции (SubscriberList)
		    \return TRUE - если чтение параметров прошло успешно, FALSE - если нет
		*/
		typedef sigc::slot<bool, const std::shared_ptr<UniXML>&, UniXML::iterator&, xmlNode*> ReaderSlot;

		/*! установить функцию для callback-вызова при чтении списка сообщений
		    For example:
		        setReadItem( sigc::mem_fun(this,&MyClass::myReadItem) );

		    bool myReadItem::myfunc(UniXML& xml,
		                                UniXML::iterator& it, xmlNode* sec)

		    uxml    - интерфейс для работы с xml-файлом
		    it     - интератор(указатель) на текущий считываемый xml-узел (item)
		    sec    - указатель на корневой узел секции (SubscriberList)
		*/
		void setReadItem( ReaderSlot sl );


		/*! установить функцию для callback-вызова при чтении списка заказчиков
		    For example:
		        setReadItem( sigc::mem_fun(this,&MyClass::myReadItem) );

		    bool myReadItem::myfunc(UniXML& xml,
		                                UniXML::iterator& it, xmlNode* sec)

		    uxml    - интерфейс для работы с xml-файлом
		    it     - интератор(указатель) на текущий считываемый xml-узел (<consumer>)
		    sec    - указатель на текущий узел сообщения (<item>)
		*/
		void setReadConsumerItem( ReaderSlot sl );


		/*! Установить фильтр на чтение списка датчиков
		    \note Функцию необходимо вызывать до вызова read(...)
		 */
		void setItemFilter( const std::string& filterField, const std::string& filterValue = "" );

		/*! Установить фильтр на чтение списка заказчиков (по каждому датчику)
		    \note Функцию необходимо вызывать до вызова read(...)
		 */
		void setConsumerFilter( const std::string& filterField, const std::string& filterValue = "" );


		/*! универсальная функция получения информации о заказчике (id и node)
		    по новому формату файла (<consumer name="xxxx" type="objects" />)
		    \return true - если идентификаторы определены
		*/
		bool getConsumerInfo( UniXML::iterator& it,
							  uniset::ObjectId& cid, uniset::ObjectId& cnode );

		/*! Функция поиска по текущему уровню (без рекурсии для дочерних узлов) */
		static xmlNode* find_node( const std::shared_ptr<UniXML>& xml, xmlNode* root, const std::string& nodename, const std::string& nm = "" );

	protected:

		virtual bool check_list_item( UniXML::iterator& it );
		virtual bool check_consumer_item( UniXML::iterator& it );

		ReaderSlot rslot;
		ReaderSlot cslot;

		std::string i_filterField = { "" };
		std::string i_filterValue = { "" };
		std::string c_filterField = { "" };
		std::string c_filterValue = { "" };
};
// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
