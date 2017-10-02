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
#include <memory>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>
// --------------------------------------------------------------------------
namespace uniset
{
	typedef std::vector< std::pair<const std::string, const std::string> > UniXMLPropList;

	class UniXML_iterator:
		public std::iterator<std::bidirectional_iterator_tag, xmlNode, ptrdiff_t, xmlNode*, xmlNode&>
	{
		public:
			UniXML_iterator(xmlNode* node) noexcept:
				curNode(node)
			{}
			UniXML_iterator() noexcept: curNode(0) {}

			std::string getProp2( const std::string& name, const std::string& defval = "" ) const noexcept;
			std::string getProp( const std::string& name ) const noexcept;
			int getIntProp( const std::string& name ) const noexcept;
			/// if value if not positive ( <= 0 ), returns def
			int getPIntProp( const std::string& name, int def ) const noexcept;
			void setProp( const std::string& name, const std::string& text ) noexcept;

			bool findName( const std::string& node, const std::string& searchname, bool deepfind = true ) noexcept;
			bool find( const std::string& searchnode, bool deepfind = true) noexcept;
			xmlNode* findX( xmlNode* root, const std::string& searchnode, bool deepfind = true ) noexcept;

			/*! Перейти к следующему узлу. Возвращает false, если некуда перейти */
			bool goNext() noexcept;

			/*! Перейти насквозь к следующему узлу. Возвращает false, если некуда перейти */
			bool goThrowNext() noexcept;

			/*! Перейти к предыдущему узлу */
			bool goPrev() noexcept;

			bool canPrev() const noexcept;
			bool canNext() const noexcept;

			// Перейти к следующему узлу
			UniXML_iterator& operator+(int) noexcept;
			UniXML_iterator operator++(int) noexcept;
			UniXML_iterator& operator+=(int) noexcept;
			UniXML_iterator& operator++() noexcept;

			// Перейти к предыдущему узлу
			UniXML_iterator& operator-(int) noexcept;
			UniXML_iterator operator--(int) noexcept;
			UniXML_iterator& operator--() noexcept;
			UniXML_iterator& operator-=(int) noexcept;

			/*! Перейти на один уровень выше
			    \note Если перейти не удалось, итератор остаётся указывать на прежний узел
			*/
			bool goParent() noexcept;

			/*! Перейти на один уровень ниже
			    \note Если перейти не удалось, итератор остаётся указывать на прежний узел
			*/
			bool goChildren() noexcept;

			// Получить текущий узел
			xmlNode* getCurrent() noexcept;

			// Получить название текущего узла
			const std::string getName() const noexcept;
			const std::string getContent() const noexcept;

			operator xmlNode* () const noexcept;

			void goBegin() noexcept;
			void goEnd() noexcept;

			UniXMLPropList getPropList() const;

		private:

			xmlNode* curNode;
	};
	// --------------------------------------------------------------------------
	class UniXML
	{
		public:

			typedef UniXML_iterator iterator;
			typedef UniXMLPropList PropList;

			UniXML( const std::string& filename );
			UniXML();
			~UniXML();

			xmlNode* getFirstNode() noexcept;
			xmlNode* getFirstNode() const noexcept;

			/*! возвращает итератор на самый первый узел документа */
			iterator begin() noexcept;
			iterator end() noexcept;

			// Загружает указанный файл
			void open( const std::string& filename );
			bool isOpen() const noexcept;

			void close();

			std::string getFileName() const noexcept;

			// Создать новый XML-документ
			void newDoc( const std::string& root_node, const std::string& xml_ver = "1.0");

			// Получить свойство name указанного узла node
			static std::string getProp(const xmlNode* node, const std::string& name) noexcept;
			static std::string getProp2(const xmlNode* node, const std::string& name, const std::string& defval = "" ) noexcept;

			static int getIntProp(const xmlNode* node, const std::string& name) noexcept;

			/// if value if not positive ( <= 0 ), returns def
			static int getPIntProp(const xmlNode* node, const std::string& name, int def) noexcept;

			// Установить свойство name указанного узла node
			static void setProp(xmlNode* node, const std::string& name, const std::string& text);

			static UniXMLPropList getPropList( xmlNode* node );

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

			// ??
			//width means number of nodes of the same level as node in 1-st parameter (width number includes first node)
			//depth means number of times we can go to the children, if 0 we can't go only to elements of the same level
			xmlNode* extFindNode( xmlNode* node, int depth, int width, const std::string& searchnode, const std::string& name = "", bool top = true ) const;

			// Функция поиска по текущему уровню (без рекурсии для дочерних узлов)
			// root  указывается исходный, внутри функции осуществляется переход к списку дочерних узлов
			// (другими словами делать goChildren() не надо)
			xmlNode* findNodeLevel1( xmlNode* root, const std::string& nodename, const std::string& nm = "" );


		protected:
			std::string filename;

			struct UniXMLDocDeleter
			{
				void operator()(xmlDoc* doc) const noexcept
				{
					if( doc )
						xmlFreeDoc(doc);
				}
			};

			std::unique_ptr<xmlDoc, UniXMLDocDeleter> doc;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
