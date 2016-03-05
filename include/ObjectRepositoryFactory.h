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
 * \brief Интерфейсный класс для создания структуры репозитария объектов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef ObjectRepositoryFactory_H_
#define ObjectRepositoryFactory_H_
// --------------------------------------------------------------------------
#include <omniORB4/CORBA.h>
#include <omniORB4/Naming.hh>
#include "Exceptions.h"
#include "ObjectRepository.h"

// -----------------------------------------------------------------------------------------
//namespase ORepositoryFacotry
//{

/*!\class ObjectRepositoryFactory */
class ObjectRepositoryFactory: private ObjectRepository
{
	public:

		ObjectRepositoryFactory( const std::shared_ptr<UniSetTypes::Configuration>& conf );
		~ObjectRepositoryFactory();

		//! Создание секции
		bool createSection(const std::string& name, const std::string& in_section)throw(UniSetTypes::ORepFailed, UniSetTypes::InvalidObjectName);
		/*! Создание секции по полному имени */
		bool createSectionF(const std::string& fullName)throw(UniSetTypes::ORepFailed, UniSetTypes::InvalidObjectName);

		//! Функция создания секции в корневом 'каталоге'
		bool createRootSection(const std::string& name);


		//! Функция удаления секции
		bool removeSection(const std::string& fullName, bool recursive = false);

		//! Функция переименования секции
		bool renameSection(const std::string& newName, const std::string& fullName);
		/**
		    @addtogroup ORepServiceGroup
		        @{
		*/

		/*! Функция выводящая на экран список всех объектов расположенных в данной секции */
		void printSection(const std::string& fullName);
		//            void printSection(CosNaming::NamingContext_ptr ctx);
		// @}
		// end of add to ORepServiceGroup

	protected:

	private:
		/*! Создание нового контекста(секции) */
		bool createContext( const std::string& cname, CosNaming::NamingContext_ptr ctx);
};
//};

#endif
