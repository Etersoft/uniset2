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
 * \brief Интерфейсный класс для работы с репозитарием объектов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef ObjectRepository_H_
#define ObjectRepository_H_
// --------------------------------------------------------------------------
#include <memory>
#include <omniORB4/CORBA.h>
#include <omniORB4/Naming.hh>
#include <string>
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------------------
namespace uniset
{
	//namespase ORepository
	//{

	/*! \class ObjectRepository
	 * \par
	 * ... а здесь идет кратенькое описание... (коротенько минут на 40!...)
	 *
	 *    \note Репозиторий работает только, с локальным репозиторием
	 * \todo получение списка начиная с элемента номер N.
	*/
	class ObjectRepository
	{
		public:

			ObjectRepository( const std::shared_ptr<uniset::Configuration>& conf );
			~ObjectRepository();

			/**
			    @defgroup ORepGroup Группа функций регистрации в репозитории объектов
			 @{     */
			//! Функция регистрации объекта по имени с указанием секции
			void registration(const std::string& name, const uniset::ObjectPtr oRef, const std::string& section, bool force = false) const
			throw(uniset::ORepFailed, uniset::ObjectNameAlready, uniset::InvalidObjectName, uniset::NameNotFound);

			//! Функция регистрации объекта по полному имени.
			void registration(const std::string& fullName, const uniset::ObjectPtr oRef, bool force = false) const
			throw(uniset::ORepFailed, uniset::ObjectNameAlready, uniset::InvalidObjectName, uniset::NameNotFound);

			//! Удаление записи об объекте name в секции section
			void unregistration(const std::string& name, const std::string& section) const throw(uniset::ORepFailed, uniset::NameNotFound);
			//! Удаление записи об объекте по полному имени
			void unregistration(const std::string& fullName) const throw(uniset::ORepFailed, uniset::NameNotFound);
			// @}
			// end of ORepGroup

			/*! Получение ссылки по заданному полному имени (разыменовывание) */
			uniset::ObjectPtr resolve(const std::string& name, const std::string& NSName = "NameService")const throw(uniset::ORepFailed, uniset::NameNotFound);

			/*!  Проверка существования и доступности объекта */
			bool isExist( const uniset::ObjectPtr& oref ) const;
			/*!  Проверка существования и доступности объекта */
			bool isExist( const std::string& fullName ) const;

			/**
			 @defgroup ORepServiceGroup Группа сервисных функций Репозитория объектов
			 @{
			*/

			/*! Тип объекта  */
			enum ObjectType
			{
				ObjectRef,  /*!< ссылка на объект  */
				Section     /*!< подсекция     */
			};

			//! Получение списка how_many объектов из секции section.
			bool list(const std::string& section, uniset::ListObjectName* ls, size_t how_many = 300)throw(uniset::ORepFailed);

			//! Получние списка how_many подсекций из секции in_section.
			bool listSections(const std::string& in_section, uniset::ListObjectName* ls, size_t how_many = 300)throw(uniset::ORepFailed);

			// -------------------------------------------------------------------
			//! Создание секции
			bool createSection( const std::string& name, const std::string& in_section )throw(uniset::ORepFailed, uniset::InvalidObjectName);

			/*! Создание секции по полному имени */
			bool createSectionF(const std::string& fullName)throw(uniset::ORepFailed, uniset::InvalidObjectName);

			//! Функция создания секции в корневом 'каталоге'
			bool createRootSection(const std::string& name);

			//! Функция удаления секции
			bool removeSection(const std::string& fullName, bool recursive = false);

			//! Функция переименования секции
			bool renameSection(const std::string& newName, const std::string& fullName);

			/*! Функция выводящая на экран список всех объектов расположенных в данной секции */
			void printSection(const std::string& fullName);

			// @}
			// end of add to ORepServiceGroup

		protected:

			ObjectRepository();
			mutable std::string nsName;
			std::shared_ptr<uniset::Configuration> uconf;

			bool list(const std::string& section, uniset::ListObjectName* ls, unsigned int how_many, ObjectType type);

			/*! Создание нового контекста(секции) */
			bool createContext( const std::string& cname, CosNaming::NamingContext_ptr ctx);

		private:
			bool init() const;
			mutable CosNaming::NamingContext_var localctx;
	};
	// -------------------------------------------------------------------------
} // end of uniset namespace
//};
// -----------------------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------------------
