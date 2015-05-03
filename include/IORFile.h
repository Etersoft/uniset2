/* File: This file is part of the UniSet project
 * Copyright (C) 2002 Vitaly Lipatov, Pavel Vainerman
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

			static std::string getIOR( const ObjectId id );
			static void setIOR( const ObjectId id, const std::string& sior );
			static void unlinkIOR( const ObjectId id );

			static std::string getFileName( const ObjectId id );

		protected:

		private:
	};
	// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
