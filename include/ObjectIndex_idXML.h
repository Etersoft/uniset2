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
namespace uniset
{
	/*! реализация интерфейса ObjectIndex на основе xml-файла, в котором прописаны id объектов  */
	class ObjectIndex_idXML:
		public uniset::ObjectIndex
	{
		public:
			ObjectIndex_idXML( const std::string& xmlfile );
			ObjectIndex_idXML( const std::shared_ptr<UniXML>& xml );
			virtual ~ObjectIndex_idXML();

			virtual const uniset::ObjectInfo* getObjectInfo( const uniset::ObjectId ) const noexcept override;
			virtual const uniset::ObjectInfo* getObjectInfo( const std::string& name ) const noexcept override;
			virtual uniset::ObjectId getIdByName( const std::string& name ) const noexcept override;
			virtual std::string getMapName( const uniset::ObjectId id ) const noexcept override;
			virtual std::string getTextName( const uniset::ObjectId id ) const noexcept override;

			virtual std::ostream& printMap( std::ostream& os ) const noexcept override;
			friend std::ostream& operator<<(std::ostream& os, ObjectIndex_idXML& oi );

		protected:
			void build( const std::shared_ptr<UniXML>& xml );
			void read_section( const std::shared_ptr<UniXML>& xml, const std::string& sec );
			void read_nodes( const std::shared_ptr<UniXML>& xml, const std::string& sec );

		private:
			typedef std::unordered_map<uniset::ObjectId, uniset::ObjectInfo> MapObjects;
			MapObjects omap;

			typedef std::unordered_map<std::string, uniset::ObjectId> MapObjectKey;
			MapObjectKey mok; // для обратного писка
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -----------------------------------------------------------------------------------------
#endif
