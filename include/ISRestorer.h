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
 * \brief Интерфейс к объекту сохраняющему список заказчиков и сообщений для InfoServer-а
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef ISRestorer_H_
#define ISRestorer_H_
// ------------------------------------------------------------------------------------------
#include <string>
#include "UniXML.h"
#include "InfoServer.h"
#include "Restorer.h"
// ------------------------------------------------------------------------------------------
/*!
	Это абстрактный интерфейс. В чистом виде не используется.
*/ 
class ISRestorer
{
	public:

		ISRestorer();
	    virtual ~ISRestorer();

		virtual void read(InfoServer* is, const std::string fname="")=0;		
		virtual void dump(InfoServer* is, const UniSetTypes::MessageCode code, 
							const InfoServer::ConsumerList& lst)=0;

	protected:

		// добавление списка заказчиков
		void addlist( InfoServer* is, const UniSetTypes::MessageCode code, 
					  InfoServer::ConsumerList& lst );
};
// ------------------------------------------------------------------------------------------
#if 0

/*!
 * \brief Реализация сохранения списка заказчиков в xml.
 * \author Pavel Vainerman
 *
 	Реализует сохранение списка заказчиков в xml-файле.
	\sa ISRestorer
*/ 
class ISRestorer_XML:
	public ISRestorer,
	public Restorer_XML
{
	public:
	
		ISRestorer_XML(const std::string fname, bool createIfNotFound=false );
//		ISRestorer_XML( UniXML& xml );

	    virtual ~ISRestorer_XML();

		virtual void dump(InfoServer* is, const UniSetTypes::MessageCode code, 
							const InfoServer::ConsumerList& lst);

		virtual void read(InfoServer* is, const std::string fname="");
		virtual void read(InfoServer* is, UniXML& xml );


		void setFileName(const std::string& file);
		inline std::string getFileName(){ return fname; }

		ISRestorer_XML();

	protected:
		virtual void read_list(UniXML& xml, xmlNode* node, InfoServer* is);
	
};
// ------------------------------------------------------------------------------------------
#endif
/*!
 * \brief Реализация сохранения списка заказчиков в xml(работа с файлом проекта)
 * \author Pavel Vainerman
 *
 	Реализует сохранение списка заказчиков в xml-файле (версия для работы с файлом проекта).
	\sa ISRestorer
*/ 
class ISRestorer_XML:
	public ISRestorer,
	public Restorer_XML
{
	public:
		ISRestorer_XML( const std::string fname );
//		ISRestorer_XML( UniXML& xml );
		virtual ~ISRestorer_XML();

		virtual void dump(InfoServer* is, const UniSetTypes::MessageCode code, 
							const InfoServer::ConsumerList& lst);

		virtual void read(InfoServer* is, const std::string fname="");
		virtual void read(InfoServer* is, UniXML& xml );


		void setFileName(const std::string& file);
		inline std::string getFileName(){ return fname; }

	protected:
		void read_list(UniXML& xml, xmlNode* node, InfoServer* is);
		void create_new_file(const std::string& fname);
		xmlNode* bind_node(UniXML& xml, xmlNode* root, const std::string& nodename, const std::string nm="");
		xmlNode* rebind_node(UniXML& xml, xmlNode* root, const std::string& nodename, const std::string nm="");		
		void init( std::string fname );

		std::string fname;
		UniXML uxml;
};

// ------------------------------------------------------------------------------------------
#endif
