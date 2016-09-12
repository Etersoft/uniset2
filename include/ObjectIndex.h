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
 * \author Vitaly Lipatov
 */
// --------------------------------------------------------------------------
#ifndef ObjcetIndex_H_
#define ObjcetIndex_H_
// --------------------------------------------------------------------------
#include <ostream>
#include <string>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
	class ObjectIndex
	{
		public:
			ObjectIndex() {};
			virtual ~ObjectIndex() {};

			// info
			// \return nullptr if not found
			virtual const ObjectInfo* getObjectInfo( const UniSetTypes::ObjectId ) const noexcept = 0;
			virtual const ObjectInfo* getObjectInfo( const std::string& name ) const noexcept = 0;

			static std::string getBaseName( const std::string& fname ) noexcept;

			// object id

			//! \return UniSetTypes::DefaultObjectId if not found
			virtual ObjectId getIdByName(const std::string& name) const noexcept = 0;

			//! \return "" if not found
			virtual std::string getNameById( const UniSetTypes::ObjectId id ) const noexcept;

			// node
			//! \return "" if not found
			inline std::string getNodeName( const UniSetTypes::ObjectId id ) const noexcept
			{
				return getNameById(id);
			}

			//! \return UniSetTypes::DefaultObjectId if not found
			inline ObjectId getNodeId( const std::string& name ) const noexcept
			{
				return getIdByName(name);
			}

			// src name
			//! \return "" if not found
			virtual std::string getMapName( const UniSetTypes::ObjectId id ) const noexcept = 0;
			//! \return "" if not found
			virtual std::string getTextName( const UniSetTypes::ObjectId id ) const noexcept = 0;

			//
			virtual std::ostream& printMap(std::ostream& os) const noexcept = 0;

			void initLocalNode( const UniSetTypes::ObjectId nodeid ) noexcept;

		protected:
			std::string nmLocalNode = {""};  // поле для оптимизации получения LocalNode

		private:

	};


	// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
