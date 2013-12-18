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
		ObjectIndex(){};
		virtual ~ObjectIndex(){};

		static const std::string sepName;
		static const std::string sepNode;

		// info
		virtual const ObjectInfo* getObjectInfo( const ObjectId )=0;
        virtual const ObjectInfo* getObjectInfo( const std::string& name )=0;

		// создание полного имени в репозитории по паре имя:узел
        static std::string mkRepName( const std::string& repname, const std::string& nodename );
		// создание имени узла по реальному и виртуальному
        static std::string mkFullNodeName( const std::string& realnode, const std::string& virtnode );
		// получить имя для регистрации в репозитории (т.е. без секции)
        static std::string getBaseName( const std::string& fname );

		// object id
		virtual ObjectId getIdByName(const std::string& name)=0;
		virtual std::string getNameById( const ObjectId id );
		virtual std::string getNameById( const ObjectId id, const ObjectId node );

		// node
		virtual std::string getFullNodeName(const std::string& fullname);
		virtual std::string getVirtualNodeName(const std::string& fullNodeName);
		virtual std::string getRealNodeName(const std::string& fullNodeName);
		virtual std::string getRealNodeName( const ObjectId id );
		
		virtual std::string getName( const std::string& fullname );

		// src name
		virtual std::string getMapName(const ObjectId id)=0;
		virtual std::string getTextName(const ObjectId id)=0;

		// 
		virtual std::ostream& printMap(std::ostream& os)=0;

		// ext
		inline ObjectId getIdByFullName(const std::string& fname)
		{
			return getIdByName(getName(fname));
		}
		
		inline ObjectId getNodeId( const std::string& fullname )
		{
			return getIdByName( getFullNodeName(fullname) );
		}

		inline std::string getFullNodeName( const ObjectId nodeid )
		{
			return getMapName(nodeid);	
		}

		void initLocalNode( ObjectId nodeid );
	
	
	protected:
		std::string nmLocalNode;	// для оптимизации

	private:
	
};
	

// -----------------------------------------------------------------------------------------
}	// end of namespace
// -----------------------------------------------------------------------------------------
#endif
