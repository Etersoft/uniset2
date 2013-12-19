/* File: This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov
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
#ifndef IORFile_H_
#define IORFile_H_
// --------------------------------------------------------------------------
#include <string>
#include "UniSetTypes.h"
#include "Exceptions.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
/*! Класс работы с файлами содержащими IOR объекта 
    \todo Для оптимизации можно сделать кэширование id:node > filename 
*/
class IORFile
{
    public:
        IORFile();

        std::string getIOR( const ObjectId id, const ObjectId node );
        void setIOR( const ObjectId id, const ObjectId node, const std::string& sior );
        void unlinkIOR( const ObjectId id, const ObjectId node );

    protected:
        std::string genFName( const ObjectId id, const ObjectId node );

    private:
};
// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
