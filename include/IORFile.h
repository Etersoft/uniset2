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
#ifndef IORFile_H_
#define IORFile_H_
// --------------------------------------------------------------------------
#include <string>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace uniset
{
    /*! Класс работы с файлами содержащими IOR объекта
        \todo Для оптимизации можно сделать кэширование id:node > filename
    */
    class IORFile
    {
        public:

            static std::string getIOR( const ObjectId id );
            static void setIOR( const ObjectId id, const std::string& sior );
            static void unlinkIOR( const ObjectId id );

            static std::string getFileName( const ObjectId id );
    };
    // -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
