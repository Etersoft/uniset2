/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Vitaly Lipatov
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
 *  \author Vitaly Lipatov
 *  \date   $Date: 2006/12/20 13:43:41 $
 *  \version $Id: UniXML.cc,v 1.13 2006/12/20 13:43:41 vpashka Exp $
 *  \par
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

using namespace UniSetTypes;
using namespace std;

/* FIXME:
По хорошему, надо отказаться от этого перекодирования
на ходу, потому что в поиске это наверняка чрезвычайно замедляет.
Возможно стоит использовать в качестве основы libxmlmm.
Перед переделкой нужно написать полный тест на все функции UniXML.
Особенно проверить распознавание кодировки XML-файла
В качестве начальной мере добавлены функции getPropUtf8
*/
const string UniXML::InternalEncoding("koi8-r");
const string UniXML::ExternalEncoding("koi8-r");
const string UniXML::xmlEncoding("utf-8");

// Временная переменная для подсчёта рекурсии
int UniXML::recur=0;

UniXML::UniXML(const string filename):
	doc(0),
	filename(filename)
{
	open(filename);
}

UniXML::UniXML():
	doc(0)
{
}

UniXML::~UniXML()
{
	//unideb << "UniXML destr" << endl;
	close();
}

void UniXML::newDoc(const string& root_node, string xml_ver)
{
	assert(doc==0);	// предыдущий doc не удален из памяти

	xmlKeepBlanksDefault(0);
	xmlNode* rootnode;
	doc = xmlNewDoc((const xmlChar*)xml_ver.c_str());
//	xmlEncodeEntitiesReentrant(doc, (const xmlChar*)ExternalEncoding.c_str());
    rootnode = xmlNewDocNode(doc, NULL, (const xmlChar*)root_node.c_str(), NULL);
	xmlDocSetRootElement(doc, rootnode);
	//assert(doc != NULL);
	if(doc == NULL)
		throw NameNotFound("UniXML(open): не смогли создать doc="+root_node);
	cur = getFirstNode();
}

void UniXML::open(const string _filename)
{
//	if(doc)
//		close();
	assert(doc==0);	// предыдущий doc не удален из памяти

	xmlKeepBlanksDefault(0);
	doc = xmlParseFile(_filename.c_str());
	if(doc == NULL)
		throw NameNotFound("UniXML(open): NotFound file="+_filename);
	cur = getFirstNode();
	filename = _filename;
}

void UniXML::close()
{
	if(doc)
	{
		xmlFreeDoc(doc);
		doc=0;
	}
	
	filename = "";
}

// Преобразование текстовой строки из XML в строку нашего внутреннего представления

string UniXML::xml2local(const string t)
{
	const char* text= t.c_str();
	iconv_t frt;
	frt = iconv_open(InternalEncoding.c_str(), xmlEncoding.c_str());
	if (frt == (iconv_t) -1)
	{
		perror("iconv_open()");
		return "";
	}

	size_t inl=strlen(text);
	size_t outl=inl;

	char* buf = new char[outl+1];
	char* inbuf = new char[inl+1];

	strncpy(inbuf,text,inl);
	char* bufptr = buf;
	char* inbufptr = inbuf;
	unsigned int result = iconv (frt, &inbufptr, &inl, &bufptr, &outl);
    if (result == (size_t) - 1)
	{
		perror ("iconv()");
		delete[] buf;
		delete[] inbuf;
		return "";
	}
	buf[strlen(text)-outl]=0;
	string tmp(buf);
	if (iconv_close(frt) != 0)
		perror ("iconv_close()");

	delete[] buf;
	delete[] inbuf;
	return tmp;
	
}

static char tmpbuf_l2x[500];

// Преобразование текстовой строки из нашего внутреннего представления в строку для XML
// Возвращает указатель на временный буфер, который один на все вызовы функции.
const xmlChar* UniXML::local2xml(string text)
{

	iconv_t frt;
	frt = iconv_open(xmlEncoding.c_str(), InternalEncoding.c_str() );
	if (frt == (iconv_t) -1)
	{
		perror("iconv_open()");
		return (xmlChar*)"";
	}
	size_t inl=text.size(), outl, soutl;
	outl=inl*4;
	if (outl > 100)
		outl = 100;
	soutl=outl;

	char* inbuf = new char[inl+1];

	strncpy(inbuf,text.c_str(),inl+1);
	char* bufptr = tmpbuf_l2x;
	char* inbufptr = inbuf;

	unsigned int result = iconv (frt, &inbufptr, &inl, &bufptr, &outl);

    if (result == (size_t) - 1)
	{
		perror ("iconv()");
		delete[] inbuf;
		return (xmlChar*)"";
	}
	tmpbuf_l2x[soutl-outl]=0;

	if (iconv_close(frt) != 0)
		perror ("iconv_close()");
//	cout << "L2XML" << tmpbuf_l2x << endl;

	delete[] inbuf;
	return (xmlChar*)tmpbuf_l2x;
}

string UniXML::local2utf8(const string text)
{
	return string((const char*)local2xml(text));
}


string UniXML::getProp(const xmlNode* node, const string name)
{
	return xml2local(getPropUtf8(node, name));
}

string UniXML::getPropUtf8(const xmlNode* node, const string name)
{
	const char * text = (const char*)::xmlGetProp((xmlNode*)node, (const xmlChar*)name.c_str());
	if (text == NULL)
		return "";
	return text;
}

int UniXML::getIntProp(const xmlNode* node, const string name )
{
	return UniSetTypes::uni_atoi(getPropUtf8(node, name));
}

int UniXML::getPIntProp(const xmlNode* node, const string name, int def )
{
	int i = getIntProp(node, name);
	if (i <= 0)
		return def;
	return i;
}

void UniXML::setProp(xmlNode* node, string name, string text)
{
	xmlSetProp(node, (const xmlChar*)name.c_str(), local2xml(text));
}

xmlNode* UniXML::createChild(xmlNode* node, string title, string text)
{
	return ::xmlNewChild(node, NULL, (const xmlChar*)title.c_str(), local2xml(text.c_str()));
//	return ::xmlNewChild(node, NULL, (const xmlChar*)title.c_str(), (const xmlChar*)text.c_str());
}

xmlNode* UniXML::createNext(xmlNode* node, const string title, const string text)
{
//	cerr << "createNext is not yet realized" << endl;
	if( node->parent )
		return createChild(node->parent, title,text);
	return 0;
}

/// Удаление указанного узла со всеми вложенными
void UniXML::removeNode(xmlNode* node)
{
	::xmlUnlinkNode(node);
	::xmlFreeNode(node);
}

xmlNode* UniXML::copyNode(xmlNode* node, int recursive)
{
//	return ::xmlCopyNode(node,recursive);

	xmlNode* copynode(::xmlCopyNode(node,recursive));
/*!	\bug Почему-то портятся русские имена (точнее становятся UTF8)
		независимо от текущей локали файла
 		спасает только такое вот дополнительное копирование списка свойств
	\bug Непонятный параметр 'target' 
		- при указании NULL нормально работает
		- при указании copynode - проблеммы с русским при сохранении
		- при указании node - SEGFAULT при попытке удалить исходный(node) узел
*/
	copynode->properties = ::xmlCopyPropList(NULL,node->properties);
	if( copynode != 0 && node->parent )
	{
		xmlNode* newnode = ::xmlNewChild(node->parent, NULL, (const xmlChar*)"", (const xmlChar*)"" );
		if( newnode != 0 )
		{
			::xmlReplaceNode(newnode,copynode);
			return copynode;
		}
	}
	return 0;
}


bool UniXML::save(const string filename, int level)
{
	string fn(filename);
	if (fn.empty())
		fn = this->filename;
	// Если файл уже существует, переименовываем его в *.xml.bak
	string bakfilename(fn+".bak");
	rename(fn.c_str(),bakfilename.c_str());
//	xmlEncodeEntitiesReentrant(doc, (const xmlChar*)ExternalEncoding.c_str());
	int res = ::xmlSaveFormatFileEnc(fn.c_str(), doc, ExternalEncoding.c_str(), level);
//	int res = ::xmlSaveFormatFile(fn.c_str(), doc, 2);
//	int res = ::xmlSaveFile(fn.c_str(), doc);
	return res > 0;
}

// Переместить указатель к следующему узлу, обходит по всему дереву
xmlNode* UniXML::nextNode(xmlNode* n)
{
	if (!n)
		return 0;
	if (n->children)
		return n->children;
	if (n->next)
		return n->next;

	for(;n->parent && !n->next && n;)
		n = n->parent;
	if (n->next)
		n = n->next;
	if (!n->name)
		n = 0;
	return n;
}

xmlNode* UniXML::findNodeUtf8(xmlNode* node, const string searchnode, const string name ) const
{
	while (node != NULL)
	{
		if (searchnode == (const char*)node->name)
		{
			/* Если name не задано, не сверяем. Иначе ищем, пока не найдём с таким именем */
			if( name.empty() )
				return node;
			if( name == getPropUtf8(node, "name") )
				return node;

		}
		xmlNode * nodeFound = findNodeUtf8(node->children, searchnode, name);
		if ( nodeFound != NULL )
			return nodeFound;

		node = node->next;
	}
	return NULL;
}

xmlNode* UniXML::findNode(xmlNode* node, const string searchnode, const string name ) const
{
	return findNodeUtf8(node, local2utf8(searchnode), local2utf8(name));
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

//width means number of nodes of the same level as node in 1-st parameter (width number includes first node)
//depth means number of times we can go to the children, if 0 we can't go only to elements of the same level
xmlNode* UniXML::extFindNodeUtf8(xmlNode* node, int depth, int width, const string searchnode, const string name, bool top )
{
	int i=0;
	while (node != NULL)
	{
		if(top&&(i>=width)) return NULL;
		if (searchnode == (const char*)node->name)
		{
			if( name == getPropUtf8(node, "name") )
				return node;

			if( name.empty() )
				return node;
		}
		if(depth > 0)
		{
			xmlNode* nodeFound = extFindNodeUtf8(node->children, depth-1,width, searchnode, name, false);
			if ( nodeFound != NULL )
				return nodeFound;
		}
		i++;
		node = node->next;
	}
	return NULL;
}

xmlNode* UniXML::extFindNode(xmlNode* node, int depth, int width, const string searchnode, const string name, bool top )
{
	return extFindNodeUtf8(node, depth, width, local2utf8(searchnode), local2utf8(name), top );
}


bool UniXML_iterator::goNext()
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
bool UniXML_iterator::goThrowNext()
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
bool UniXML_iterator::goPrev()
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
bool UniXML_iterator::canPrev()
{
	if( !curNode || !curNode->prev )
		return false;
	return true;
}
// -------------------------------------------------------------------------		
bool UniXML_iterator::canNext()
{
	if (!curNode || !curNode->next )
		return false;
	return true;
}
// -------------------------------------------------------------------------		
bool UniXML_iterator::goParent()
{
	if( !curNode )
		return false;
	
	if( !curNode->parent )
		return false;
		
	curNode = curNode->parent;
	return true;
}
// -------------------------------------------------------------------------		
bool UniXML_iterator::goChildren()
{
	if (!curNode || !curNode->children )
		return false;

	xmlNode* tmp(curNode);
	curNode = curNode->children;
	// использовать везде xmlIsBlankNode, если подходит
	if ( getName() == "text" )
		return goNext();
	if ( getName() == "comment" )
		return goNext();
	if ( getName().empty() )
	{
		curNode = tmp;
		return false;
	}
	return true;
}

// -------------------------------------------------------------------------		
string UniXML_iterator::getProp( const string name ) const
{
	return UniXML::getProp(curNode, name);
}

// -------------------------------------------------------------------------		
const string UniXML_iterator::getContent() const
{
	if (curNode == NULL)
		return "";
	return (const char*)::xmlNodeGetContent(curNode);
}

// -------------------------------------------------------------------------
string UniXML_iterator::getPropUtf8( const string name ) const
{
	return UniXML::getPropUtf8(curNode, name);
}

// -------------------------------------------------------------------------
int UniXML_iterator::getIntProp( const string name ) const
{
	return UniSetTypes::uni_atoi((char*)::xmlGetProp(curNode, (const xmlChar*)name.c_str()));
}

int UniXML_iterator::getPIntProp( const string name, int def ) const
{
	int i = getIntProp(name);
	if (i <= 0)
		return def;
	return i;
}

// -------------------------------------------------------------------------		
void UniXML_iterator::setProp( const string name, const string text )
{
	UniXML::setProp(curNode, name, text);
}
// -------------------------------------------------------------------------		
