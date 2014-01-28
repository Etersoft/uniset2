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
/*!
 * \namespace ORepHelpers
 * В этом пространстве имен заключены вспомогательные функции используемые функциями ObjectRepository
*/
namespace ORepHelpers
{
    //! Получение ссылки на корень репозитория 
    CosNaming::NamingContext_ptr getRootNamingContext( const CORBA::ORB_ptr orb, 
                                                        const std::string& nsName, int timeOutSec=2);

    //! Получение контекста по заданному имени 
    CosNaming::NamingContext_ptr getContext(const std::string& cname, int argc,
                        const char* const* argv, const std::string& nsName)
                        throw(UniSetTypes::ORepFailed);

    CosNaming::NamingContext_ptr getContext(const CORBA::ORB_ptr orb, const std::string& cname,
                                            const std::string& nsName)
                                                            throw(UniSetTypes::ORepFailed);

    //! Функция отделяющая имя секции от полного имени 
    const std::string getSectionName(const std::string& fullName, const std::string& brk="/");

    //! Функция выделения имени из полного имени 
    const std::string getShortName(const std::string& fullName, const std::string& brk="/");


    //! Проверка на наличие недопустимых символов
    char checkBadSymbols(const std::string& str);
    
    /*! Получение строки запрещенных символов в виде '.', '/', и т.д. */
    std::string BadSymbolsToStr();

}
// -----------------------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------------------

