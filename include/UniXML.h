/* This file is part of the UniSet project
 * Copyright (c) 2002-2010 Free Software Foundation, Inc.
 * Copyright (c) 2002, 2009, 2010 Vitaly Lipatov
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
 *  \author Vitaly Lipatov, PavelVainerman
 *  \par

 *    \bug НЕ РАБОТАЕТ функция findNode. (не ищет по полю name, если задать)
 */
// --------------------------------------------------------------------------

// Класс для работы с данными в XML, выборки и перекодирования

#ifndef UniXML_H_
#define UniXML_H_

#include <assert.h>
#include <string>
#include <cstddef>

#include <libxml/parser.h>
#include <libxml/tree.h>
// --------------------------------------------------------------------------
class UniXML_iterator:
	public std::iterator<std::bidirectional_iterator_tag, xmlNode, ptrdiff_t, xmlNode*, xmlNode&>
{
	public:
		UniXML_iterator(xmlNode* node) :
			curNode(node)
		{}
		UniXML_iterator() {}

		std::string getProp2( const std::string& name, const std::string& defval = "" );
		std::string getProp( const std::string& name );
		std::string getPropUtf8( const std::string& name );
		int getIntProp( const std::string& name );
		/// if value if not positive ( <= 0 ), returns def
		int getPIntProp( const std::string& name, int def );
		void setProp( const std::string& name, const std::string& text );

		bool findName( const std::string& node, const std::string& searchname, bool deepfind = true );
		bool find( const std::string& searchnode, bool deepfind = true);
		xmlNode* findX( xmlNode* root, const std::string& searchnode, bool deepfind = true );

		/*! Перейти к следующему узлу. Возвращает false, если некуда перейти */
		bool goNext();

		/*! Перейти насквозь к следующему узлу. Возвращает false, если некуда перейти */
		bool goThrowNext();

		/*! Перейти к предыдущему узлу */
		bool goPrev();

		bool canPrev();
		bool canNext();
#if 0
		friend inline bool operator==(const UniXML_iterator& lhs, const UniXML_iterator& rhs)
		{
			return ( lhs.curNode != 0 && rhs.curNode != 0 && lhs.curNode == rhs.curNode );
		}

		friend inline bool operator!=(const UniXML_iterator& lhs, const UniXML_iterator& rhs)
		{
			return !(lhs == rhs);
		}

		inline bool operator==(int a)
		{
			if( a == 0 )
				return (curNode == NULL);

			return false;
		}
#endif
		// Перейти к следующему узлу
		UniXML_iterator operator+(int);
		UniXML_iterator operator++(int);
		UniXML_iterator operator+=(int);
		UniXML_iterator operator++();

		// Перейти к предыдущему узлу
		UniXML_iterator operator-(int);
		UniXML_iterator operator--(int);
		UniXML_iterator operator--();
		UniXML_iterator operator-=(int);

		/*! Перейти на один уровень выше
		    \note Если перейти не удалось, итератор остаётся указывать на прежний узел
		*/
		bool goParent();

		/*! Перейти на один уровень ниже
		    \note Если перейти не удалось, итератор остаётся указывать на прежний узел
		*/
		bool goChildren();

		// Получить текущий узел
		xmlNode* getCurrent()
		{
			return curNode;
		}

		// Получить название текущего узла
		inline const std::string getName() const
		{
			if( curNode )
			{
				if( !curNode->name )
					return "";

				return (char*) curNode->name;
			}

			return "";
		}

		const std::string getContent() const;

		inline operator xmlNode* () const
		{
			//ulog.< "current\n";
			return curNode;
		}

		inline void goBegin()
		{
			while(canPrev())
			{
				goPrev();
			}
		}

		inline void goEnd()
		{
			while(canNext())
			{
				goNext();
			}
		}

	protected:
		xmlNode* curNode;
};
// --------------------------------------------------------------------------
class UniXML
{
	public:

		typedef UniXML_iterator iterator;

		inline xmlNode* getFirstNode()
		{
			return xmlDocGetRootElement(doc);
		}

		inline xmlNode* getFirstNode() const
		{
			return xmlDocGetRootElement(doc);
		}


		/*! возвращает итератор на самый первый узел документа */
		inline iterator begin()
		{
			return iterator(getFirstNode());
		}

		inline iterator end()
		{
			return  iterator(NULL);
		}

		// Загружает указанный файл
		void open(const std::string& filename);

		void close();
		inline bool isOpen()
		{
			return doc != 0;
		}
		UniXML( const std::string& filename );

		UniXML();

		~UniXML();

		xmlNode* cur;
		xmlDoc* doc;
		inline std::string getFileName()
		{
			return filename;
		}

		// Создать новый XML-документ
		void newDoc(const std::string& root_node, std::string xml_ver = "1.0");

		// Получить свойство name указанного узла node
		static std::string getProp(const xmlNode* node, const std::string& name);
		static std::string getProp2(const xmlNode* node, const std::string& name, const std::string& defval = "" );

		static std::string getPropUtf8(const xmlNode* node, const std::string& name);
		static int getIntProp(const xmlNode* node, const std::string& name);
		/// if value if not positive ( <= 0 ), returns def
		static int getPIntProp(const xmlNode* node, const std::string& name, int def);

		// Установить свойство name указанного узла node
		static void setProp(xmlNode* node, const std::string& name, const std::string& text);

		// Добавить новый дочерний узел
		static xmlNode* createChild(xmlNode* node, const std::string& title, const std::string& text);

		// Добавить следующий узел
		static xmlNode* createNext(xmlNode* node, const std::string& title, const std::string& text);

		// Удалить указанный узел и все вложенные узлы
		static void removeNode(xmlNode* node);

		// копировать указанный узел и все вложенные узлы
		static xmlNode* copyNode(xmlNode* node, int recursive = 1);

		// Сохранить в файл, если параметр не указан, сохраняет в тот файл
		// который был загружен последним.
		bool save(const std::string& filename = "", int level = 2);

		// Переместить указатель к следующему узлу
		static xmlNode* nextNode(xmlNode* node);

		// После проверки исправить рекурсивный алгоритм на обычный,
		// используя ->parent
		xmlNode* findNode( xmlNode* node, const std::string& searchnode, const std::string& name = "") const;
		xmlNode* findNodeUtf8( xmlNode* node, const std::string& searchnode, const std::string& name = "") const;

		// ??
		//width means number of nodes of the same level as node in 1-st parameter (width number includes first node)
		//depth means number of times we can go to the children, if 0 we can't go only to elements of the same level
		xmlNode* extFindNode( xmlNode* node, int depth, int width, const std::string& searchnode, const std::string& name = "", bool top = true ) const;
		xmlNode* extFindNodeUtf8( xmlNode* node, int depth, int width, const std::string& searchnode, const std::string& name = "", bool top = true ) const;


	protected:
		std::string filename;

		// Преобразование текстовой строки из XML в строку нашего внутреннего представления
		static std::string xml2local( const std::string& text );

		// Преобразование текстовой строки из нашего внутреннего представления в строку для XML
		// Возвращает указатель на временный буфер, который один на все вызовы функции.
		static const xmlChar* local2xml( const std::string& text );
		static std::string local2utf8( const std::string& text );

		static int recur;
};
// --------------------------------------------------------------------------
#endif
