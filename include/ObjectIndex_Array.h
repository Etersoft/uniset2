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
#ifndef ObjectIndex_Array_H_
#define ObjectIndex_Array_H_
// --------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <ostream>
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "ObjectIndex.h"
// --------------------------------------------------------------------------
namespace UniSetTypes
{
	/*!
	    \todo Проверить функции этого класса на повторную входимость
	    \bug При обращении к objectsMap[0].textName срабатывает исключение(видимо какое-то из std).
	        Требуется дополнительное изучение.
	*/
	class ObjectIndex_Array:
		public ObjectIndex
	{
		public:
			ObjectIndex_Array(const ObjectInfo* objectInfo);
			virtual ~ObjectIndex_Array();

			virtual const ObjectInfo* getObjectInfo( const ObjectId ) const noexcept override;
			virtual const ObjectInfo* getObjectInfo( const std::string& name ) const noexcept override;
			virtual ObjectId getIdByName( const std::string& name ) const noexcept override;
			virtual std::string getMapName( const ObjectId id ) const noexcept override;
			virtual std::string getTextName( const ObjectId id ) const noexcept override;

			virtual std::ostream& printMap(std::ostream& os) const noexcept override;
			friend std::ostream& operator<<(std::ostream& os, ObjectIndex_Array& oi );

		private:

			size_t numOfObject;
			typedef std::unordered_map<std::string, ObjectId> MapObjectKey;
			MapObjectKey::iterator MapObjectKeyIterator;
			MapObjectKey mok;
			const ObjectInfo* objectInfo;
			size_t maxId;
	};
	// -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
