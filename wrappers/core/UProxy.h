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
// -------------------------------------------------------------------------
#ifndef UProxy_H_
#define UProxy_H_
// --------------------------------------------------------------------------
#include <memory>
#include "Configuration.h"
#include "UExceptions.h"
#include "UTypes.h"
// --------------------------------------------------------------------------
class UProxy_impl; // PIMPL
// --------------------------------------------------------------------------
/*! Интерфейс для взаимодействия с SM (с заказом датчиков).
 * Но при этом, обработка сообщений ложится на вызывающего.
 */
class UProxy
{
	public:
		UProxy( const std::string& name ) throw(UException);
		UProxy( long id ) throw(UException);
		~UProxy();

		//!  Заказать датчик
		void askSensor( long id ) throw(UException);

		long getValue( long id ) throw(UException);

		/*! изменить состояние датчика */
		void setValue( long id, long val ) throw(UException);

		// ожидание события "SensorMessage"
		// timeout_msec <= 0 - ждать вечно
		UTypes::ShortIOInfo waitMessage( unsigned long timeout_msec ) throw(UException);

		// -----------------------------------
		// релизации без кидания исключений
		// -----------------------------------

		UTypes::ResultIO safeWaitMessage( unsigned long timeout_msec ) noexcept;

		UTypes::ResultBool safeSetValue( long id, long val ) noexcept;

		UTypes::ResultValue safeGetValue( long id ) noexcept;

		UTypes::ResultBool safeAskSensor( long id ) noexcept;

		bool isExist( long id ) noexcept;


	protected:
		void init( long id ) throw( UException );

	private:
		UProxy()throw(UException);

		std::shared_ptr<UProxy_impl> uobj;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
