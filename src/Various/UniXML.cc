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
 *  \author Vitaly Lipatov
 */
// --------------------------------------------------------------------------
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iconv.h>

#include <iostream>
#include <string>

#include "UniSetTypes.h"
#include "UniXML.h"
#include "Exceptions.h"
#include <libxml/xinclude.h>
// -----------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -----------------------------------------------------------------------------
/* FIXME:
Возможно стоит использовать в качестве основы libxmlmm.
Перед переделкой нужно написать полный тест на все функции UniXML.
Особенно проверить распознавание кодировки XML-файла
----
Либо переходить на Poco::XML - но это несовместимость со всеми проектам, т.к.
не станет xmlNode*.
Либо возможно typedef Poco::XML::Node xmlNode;
*/
// -----------------------------------------------------------------------------
UniXML::UniXML(const string& fname):
	filename(fname)
{
	open(filename);
}

UniXML::UniXML()
{
}
// -----------------------------------------------------------------------------
UniXML::~UniXML()
{
	close();
}
// -----------------------------------------------------------------------------
string UniXML::getFileName() const noexcept
{
	return filename;
}
// -----------------------------------------------------------------------------
struct UniXMLDocDeleter
{
	void operator()(xmlDoc* doc) const noexcept
	{
		try
		{
			if( doc )
				xmlFreeDoc(doc);
		}
		catch(...) {}
	}
};
// -----------------------------------------------------------------------------
void UniXML::newDoc(const string& root_node, const string& xml_ver)
{
	assert(doc == nullptr);  // предыдущий doc не удален из памяти

	xmlKeepBlanksDefault(0);
	xmlDoc* d = xmlNewDoc((const xmlChar*)xml_ver.c_str());
	if( d == NULL )
		throw NameNotFound("UniXML(open): не смогли создать doc=" + root_node);

	doc = std::shared_ptr<xmlDoc>(d, UniXMLDocDeleter());

	//    xmlEncodeEntitiesReentrant(doc, (const xmlChar*)ExternalEncoding.c_str());
	xmlNode* rootnode = xmlNewDocNode(d, NULL, (const xmlChar*)root_node.c_str(), NULL);
	xmlDocSetRootElement(d, rootnode);
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::getFirstNode() noexcept
{
	if( !doc )
		return nullptr;

	return xmlDocGetRootElement(doc.get());
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::getFirstNode() const noexcept
{
	if( !doc )
		return nullptr;

	return xmlDocGetRootElement(doc.get());
}
// -----------------------------------------------------------------------------
UniXML::iterator UniXML::begin() noexcept
{
	return iterator(getFirstNode());
}
// -----------------------------------------------------------------------------
UniXML::iterator UniXML::end() noexcept
{
	return  iterator(NULL);
}
// -----------------------------------------------------------------------------
void UniXML::open( const string& _filename )
{
	assert( doc == nullptr );  // предыдущий doc не удален из памяти

	xmlKeepBlanksDefault(0);
	// Can read files in any encoding, recode to UTF-8 internally
	xmlDoc* d = xmlParseFile(_filename.c_str());

	if( d == NULL )
		throw NameNotFound("UniXML(open): NotFound file=" + _filename);

	doc = std::shared_ptr<xmlDoc>(d,UniXMLDocDeleter());

	// Support for XInclude (see eterbug #6304)
	// main tag must to have follow property: xmlns:xi="http://www.w3.org/2001/XInclude"
	//For include: <xi:include href="test2.xml"/>
	xmlXIncludeProcess(doc.get());

	filename = _filename;
}
// -----------------------------------------------------------------------------
void UniXML::close()
{
	doc= nullptr;
	filename = "";
}
// -----------------------------------------------------------------------------
bool UniXML::isOpen() const noexcept
{
	return (doc != nullptr);
}
// -----------------------------------------------------------------------------
string UniXML::getProp2(const xmlNode* node, const string& name, const string& defval) noexcept
{
	// формально при конструировании строки может быть exception
	try
	{
		string s(getProp(node, name));
		if( !s.empty() )
			return std::move(s);
	}
	catch(...){}

	return defval;
}
// -----------------------------------------------------------------------------
string UniXML::getProp(const xmlNode* node, const string& name) noexcept
{
	xmlChar* text = ::xmlGetProp((xmlNode*)node, (const xmlChar*)name.c_str());

	if( text == NULL )
	{
		xmlFree(text);
		return "";
	}

	try
	{
		// формально при конструировании строки может быть exception
		const string t( (const char*)text );
		xmlFree( (xmlChar*) text );
		return std::move(t);
	}
	catch(...){}

	xmlFree( (xmlChar*) text );
	return "";
}
// -----------------------------------------------------------------------------
int UniXML::getIntProp(const xmlNode* node, const string& name ) noexcept
{
	return UniSetTypes::uni_atoi(getProp(node, name));
}
// -----------------------------------------------------------------------------
int UniXML::getPIntProp(const xmlNode* node, const string& name, int def ) noexcept
{
	string param( getProp(node, name) );

	if( param.empty() )
		return def;

	return UniSetTypes::uni_atoi(param);
}
// -----------------------------------------------------------------------------
void UniXML::setProp(xmlNode* node, const string& name, const string& text )
{
	::xmlSetProp(node, (const xmlChar*)name.c_str(), (const xmlChar*)text.c_str());
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::createChild(xmlNode* node, const string& title, const string& text)
{
	return ::xmlNewChild(node, NULL, (const xmlChar*)title.c_str(), (const xmlChar*)text.c_str());
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::createNext(xmlNode* node, const string& title, const string& text)
{
	if( node->parent )
		return createChild(node->parent, title, text);

	return 0;
}
// -----------------------------------------------------------------------------
/// Удаление указанного узла со всеми вложенными
void UniXML::removeNode(xmlNode* node)
{
	::xmlUnlinkNode(node);
	::xmlFreeNode(node);
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::copyNode(xmlNode* node, int recursive)
{
	//    return ::xmlCopyNode(node,recursive);

	xmlNode* copynode(::xmlCopyNode(node, recursive));
	/*!    \bug Почему-то портятся русские имена (точнее становятся UTF8)
	        независимо от текущей локали файла
	         спасает только такое вот дополнительное копирование списка свойств
	    \bug Непонятный параметр 'target'
	        - при указании NULL нормально работает
	        - при указании copynode - проблеммы с русским при сохранении
	        - при указании node - SEGFAULT при попытке удалить исходный(node) узел
	    \todo "Нужно тест написать на copyNode"
	*/

	if( copynode )
		copynode->properties = ::xmlCopyPropList(NULL, node->properties);

	if( copynode != 0 && node->parent )
	{
		xmlNode* newnode = ::xmlNewChild(node->parent, NULL, (const xmlChar*)"", (const xmlChar*)"" );

		if( newnode != 0 )
		{
			::xmlReplaceNode(newnode, copynode);
			return copynode;
		}
	}

	return 0;
}
// -----------------------------------------------------------------------------
bool UniXML::save(const string& filename, int level)
{
	string fn(filename);

	if (fn.empty())
		fn = this->filename;

	// Если файл уже существует, переименовываем его в *.xml.bak
	string bakfilename(fn + ".bak");
	(void)rename(fn.c_str(), bakfilename.c_str());
	//    int res = ::xmlSaveFormatFileEnc(fn.c_str(), doc, ExternalEncoding.c_str(), level);
	// Write in UTF-8 without XML encoding in the header */
	int res = ::xmlSaveFormatFile(fn.c_str(), doc.get(), level);
	//    int res = ::xmlSaveFile(fn.c_str(), doc);
	return res > 0;
}
// -----------------------------------------------------------------------------
// Переместить указатель к следующему узлу, обходит по всему дереву
xmlNode* UniXML::nextNode(xmlNode* n)
{
	if (!n)
		return 0;

	if (n->children)
		return n->children;

	if (n->next)
		return n->next;

	for(; n && n->parent && !n->next;)
		n = n->parent;

	if (n && n->next)
		n = n->next;

	if (n && !n->name)
		n = 0;

	return n;
}
// -----------------------------------------------------------------------------
xmlNode* UniXML::findNode( xmlNode* node, const string& searchnode, const string& name ) const
{
	xmlNode* fnode = node;

	while (fnode != NULL)
	{
		if (searchnode == (const char*)fnode->name)
		{
			/* Если name не задано, не сверяем. Иначе ищем, пока не найдём с таким именем */
			if( name.empty() )
				return fnode;

			if( name == getProp(fnode, "name") )
				return fnode;

		}

		xmlNode* nodeFound = findNode(fnode->children, searchnode, name);

		if ( nodeFound != NULL )
			return nodeFound;

		fnode = fnode->next;
	}

	return NULL;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

//width means number of nodes of the same level as node in 1-st parameter (width number includes first node)
//depth means number of times we can go to the children, if 0 we can't go only to elements of the same level
xmlNode* UniXML::extFindNode( xmlNode* node, int depth, int width, const string& searchnode, const string& name, bool top ) const
{
	auto i = 0;
	xmlNode* fnode = node;

	while( fnode != NULL )
	{
		if(top && (i >= width)) return NULL;

		if (searchnode == (const char*)fnode->name)
		{
			if( name == getProp(fnode, "name") )
				return fnode;

			if( name.empty() )
				return node;
		}

		if(depth > 0)
		{
			xmlNode* nodeFound = extFindNode(fnode->children, depth - 1, width, searchnode, name, false);

			if ( nodeFound != NULL )
				return nodeFound;
		}

		i++;
		fnode = fnode->next;
	}

	return NULL;
}
// -----------------------------------------------------------------------------
bool UniXML_iterator::goNext() noexcept
{
	if( !curNode ) // || !curNode->next )
		return false;

	curNode = curNode->next;

	if ( getName() == "text" )
		return goNext();

	if ( getName() == "comment" )
		return goNext();

	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::goThrowNext() noexcept
{
	xmlNode* node = UniXML::nextNode(curNode);

	if (!node)
		return false;

	curNode = node;

	if ( getName() == "text" )
		return goThrowNext();

	if ( getName() == "comment" )
		return goThrowNext();

	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::goPrev() noexcept
{
	if( !curNode ) // || !curNode->prev )
		return false;

	curNode = curNode->prev;

	if ( getName() == "text" )
		return goPrev();

	if ( getName() == "comment" )
		return goPrev();

	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::canPrev() const noexcept
{
	if( !curNode || !curNode->prev )
		return false;

	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::canNext() const noexcept
{
	if (!curNode || !curNode->next )
		return false;

	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::goParent() noexcept
{
	if( !curNode )
		return false;

	if( !curNode->parent )
		return false;

	curNode = curNode->parent;
	return true;
}
// -------------------------------------------------------------------------
bool UniXML_iterator::goChildren() noexcept
{
	if (!curNode || !curNode->children )
		return false;

	xmlNode* tmp = curNode;
	curNode = curNode->children;

	// использовать везде xmlIsBlankNode, если подходит
	if( getName() == "text" )
		return goNext();

	if( getName() == "comment" )
		return goNext();

	if( getName().empty() )
	{
		curNode = tmp;
		return false;
	}

	return true;
}
// -------------------------------------------------------------------------
xmlNode*UniXML_iterator::getCurrent() noexcept
{
	return curNode;
}
// -------------------------------------------------------------------------
const string UniXML_iterator::getName() const noexcept
{
	if( curNode )
	{
		if( !curNode->name )
			return "";

		return (char*) curNode->name;
	}

	return "";
}

// -------------------------------------------------------------------------
string UniXML_iterator::getProp2( const string& name, const string& defval ) const noexcept
{
	return UniXML::getProp2(curNode, name, defval);
}

string UniXML_iterator::getProp( const string& name ) const noexcept
{
	return UniXML::getProp(curNode, name);
}

// -------------------------------------------------------------------------
const string UniXML_iterator::getContent() const noexcept
{
	if (curNode == NULL)
		return "";

	return (const char*)::xmlNodeGetContent(curNode);
}
// -------------------------------------------------------------------------
void UniXML_iterator::goBegin() noexcept
{
	while(canPrev())
	{
		goPrev();
	}
}
// -------------------------------------------------------------------------
void UniXML_iterator::goEnd() noexcept
{
	while(canNext())
	{
		goNext();
	}
}
// -------------------------------------------------------------------------
UniXML_iterator::operator xmlNode*() const noexcept
{
	//ulog.< "current\n";
	return curNode;
}
// -------------------------------------------------------------------------
int UniXML_iterator::getIntProp( const string& name ) const noexcept
{
	return UniSetTypes::uni_atoi(UniXML::getProp(curNode, name));
}

int UniXML_iterator::getPIntProp( const string& name, int def ) const noexcept
{
	string param( getProp(name) );

	if( param.empty() )
		return def;

	return UniSetTypes::uni_atoi(param);
}

// -------------------------------------------------------------------------
void UniXML_iterator::setProp( const string& name, const string& text ) noexcept
{
	UniXML::setProp(curNode, name, text);
}

// -------------------------------------------------------------------------
bool UniXML_iterator::findName( const std::string& nodename, const std::string& searchname, bool deepfind ) noexcept
{
	xmlNode* fnode = curNode;

	while( fnode != NULL )
	{
		fnode = this->findX(fnode, nodename, deepfind);

		if ( searchname == UniXML::getProp(fnode, "name") )
		{
			curNode = fnode;
			return true;
		}

		// ищем дальше
		fnode = fnode->next;
	}

	return false;
}

// -------------------------------------------------------------------------
bool UniXML_iterator::find( const std::string& searchnode, bool deepfind ) noexcept
{
	xmlNode* fnode = findX(curNode, searchnode, deepfind);

	if( fnode != NULL )
	{
		curNode = fnode;
		return true;
	}

	return false;
}
// -------------------------------------------------------------------------
xmlNode* UniXML_iterator::findX( xmlNode* root, const std::string& searchnode, bool deepfind ) noexcept
{
	if( root == NULL )
		return NULL;

	// Функция ищет "в ширину и в глубь" начиная с текущего узла!
	xmlNode* fnode = root;

	while( fnode != NULL )
	{
		if( searchnode == (const char*)fnode->name )
			return fnode;

		// идём "в глубь"
		if( deepfind && fnode->children )
		{
			xmlNode* cnode = findX(fnode->children, searchnode);

			if( cnode != NULL )
				return cnode;
		}

		// идём в ширину
		fnode = fnode->next;
	}

	return NULL;
}
// -------------------------------------------------------------------------
UniXML_iterator& UniXML_iterator::operator++() noexcept
{
	return (*this) + 1;
}
// -------------------------------------------------------------------------
UniXML_iterator& UniXML_iterator::operator++(int) noexcept
{
	return (*this) + 1;
}
// -------------------------------------------------------------------------
UniXML_iterator& UniXML_iterator::operator+=(int s) noexcept
{
	return (*this) + s;
}
// -------------------------------------------------------------------------
UniXML_iterator& UniXML_iterator::operator+(int step) noexcept
{
	int i = 0;

	while( i < step && curNode )
	{
		curNode = curNode->next;

		if( curNode )
		{
			if( getName() == "text" || getName() == "comment" )
				continue;

			i++;
		}
	}

	return *this;
}
// -------------------------------------------------------------------------
UniXML_iterator& UniXML_iterator::operator--(int) noexcept
{
	return (*this) - 1;
}

UniXML_iterator& UniXML_iterator::operator--() noexcept
{
	return (*this) - 1;
}

UniXML_iterator& UniXML_iterator::operator-=(int s) noexcept
{
	return (*this) - s;
}
// -------------------------------------------------------------------------

UniXML_iterator& UniXML_iterator::operator-(int step) noexcept
{
	int i = 0;

	while( i < step && curNode )
	{
		curNode = curNode->prev;

		if( curNode )
		{
			if( getName() == "text" || getName() == "comment" )
				continue;

			i++;
		}
	}

	return (*this);
}
// -------------------------------------------------------------------------------
