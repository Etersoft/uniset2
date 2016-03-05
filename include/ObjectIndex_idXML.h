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
#ifndef ObjectIndex_idXML_H_
#define ObjectIndex_idXML_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include <string>
#include "ObjectIndex.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
class ObjectIndex_idXML:
	public UniSetTypes::ObjectIndex
{
	public:
		ObjectIndex_idXML( const std::string& xmlfile );
		ObjectIndex_idXML( const std::shared_ptr<UniXML>& xml );
		virtual ~ObjectIndex_idXML();

		virtual const UniSetTypes::ObjectInfo* getObjectInfo( const UniSetTypes::ObjectId ) override;
		virtual const UniSetTypes::ObjectInfo* getObjectInfo( const std::string& name ) override;
		virtual UniSetTypes::ObjectId getIdByName( const std::string& name ) override;
		virtual std::string getMapName( const UniSetTypes::ObjectId id ) override;
		virtual std::string getTextName( const UniSetTypes::ObjectId id ) override;

		virtual std::ostream& printMap( std::ostream& os ) override;
		friend std::ostream& operator<<(std::ostream& os, ObjectIndex_idXML& oi );

	protected:
		void build( const std::shared_ptr<UniXML>& xml );
		void read_section( const std::shared_ptr<UniXML>& xml, const std::string& sec );
		void read_nodes( const std::shared_ptr<UniXML>& xml, const std::string& sec );

	private:
		typedef std::unordered_map<UniSetTypes::ObjectId, UniSetTypes::ObjectInfo> MapObjects;
		MapObjects omap;

		typedef std::unordered_map<std::string, UniSetTypes::ObjectId> MapObjectKey;
		MapObjectKey mok; // для обратного писка
};
// -----------------------------------------------------------------------------------------
#endif
