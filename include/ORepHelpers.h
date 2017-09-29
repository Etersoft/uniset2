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
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef ORepHelpers_H_
#define ORepHelpers_H_
// --------------------------------------------------------------------------
#include <omniORB4/CORBA.h>
#include <omniORB4/Naming.hh>
#include <string>
#include "Exceptions.h"
// -----------------------------------------------------------------------------------------
namespace uniset
{
	/*!
	 * \namespace ORepHelpers
	 * В этом пространстве имен заключены вспомогательные функции используемые функциями ObjectRepository
	*/
	namespace ORepHelpers
	{
		//! Получение ссылки на корень репозитория
		CosNaming::NamingContext_ptr getRootNamingContext( const CORBA::ORB_ptr orb, const std::string& nsName );

		/*!
		 * Получение контекста по заданному имени
		 * \param cname - полное имя контекста ссылку на который, возвратит функция.
		 * \param argc  - argc
		 * \param argc  - argv
		 * \param nsName  - параметры инициализации ORB
		 *
		 * throw uniset::ORepFailed
		*/
		CosNaming::NamingContext_ptr getContext(const std::string& cname, int argc,
												const char* const* argv, const std::string& nsName);

		// throw uniset::ORepFailed
		CosNaming::NamingContext_ptr getContext(const CORBA::ORB_ptr orb, const std::string& cname,
												const std::string& nsName);

		//! Функция отделяющая имя секции от полного имени
		const std::string getSectionName(const std::string& fullName, const std::string& brk = "/");

		//! Функция выделения имени из полного имени
		const std::string getShortName(const std::string& fullName, const std::string& brk = "/");
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -----------------------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------------------
