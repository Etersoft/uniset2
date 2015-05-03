/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
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

		ObjectRepository( const std::shared_ptr<UniSetTypes::Configuration>& conf );
		~ObjectRepository();

		/**
		    @defgroup ORepGroup Группа функций регистрации в репозитории объектов
		 @{     */
		//! Функция регистрации объекта по имени с указанием секции
		void registration(const std::string& name, const UniSetTypes::ObjectPtr oRef, const std::string& section, bool force = false) const
		throw(UniSetTypes::ORepFailed, UniSetTypes::ObjectNameAlready, UniSetTypes::InvalidObjectName, UniSetTypes::NameNotFound);

		//! Функция регистрации объекта по полному имени.
		void registration(const std::string& fullName, const UniSetTypes::ObjectPtr oRef, bool force = false) const
		throw(UniSetTypes::ORepFailed, UniSetTypes::ObjectNameAlready, UniSetTypes::InvalidObjectName, UniSetTypes::NameNotFound);

		//! Удаление записи об объекте name в секции section
		void unregistration(const std::string& name, const std::string& section) const throw(UniSetTypes::ORepFailed, UniSetTypes::NameNotFound);
		//! Удаление записи об объекте по полному имени
		void unregistration(const std::string& fullName) const throw(UniSetTypes::ORepFailed, UniSetTypes::NameNotFound);
		// @}
		// end of ORepGroup

		/*! Получение ссылки по заданному полному имени (разыменовывание) */
		UniSetTypes::ObjectPtr resolve(const std::string& name, const std::string& NSName = "NameService")const throw(UniSetTypes::ORepFailed, UniSetTypes::NameNotFound);

		/*!  Проверка существования и доступности объекта */
		bool isExist( const UniSetTypes::ObjectPtr& oref ) const;
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
		bool list(const std::string& section, UniSetTypes::ListObjectName* ls, unsigned int how_many = 300)throw(UniSetTypes::ORepFailed);

		//! Получние списка how_many подсекций из секции in_section.
		bool listSections(const std::string& in_section, UniSetTypes::ListObjectName* ls, unsigned int how_many = 300)throw(UniSetTypes::ORepFailed);

		// @}
		// end of ORepServiceGroup

	protected:

		ObjectRepository();
		mutable std::string nsName;
		std::shared_ptr<UniSetTypes::Configuration> uconf;

		bool list(const std::string& section, UniSetTypes::ListObjectName* ls, unsigned int how_many, ObjectType type);

	private:
		bool init() const;
		mutable CosNaming::NamingContext_var localctx;
};

//};
// -----------------------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------------------
