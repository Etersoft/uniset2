/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 *	\brief Иерархия генерируемых библиотекой исключений
 *  \author Pavel Vainerman <pv>
 *  \date  $Date: 2008/12/14 21:57:51 $
 *  \version $Id: Exceptions.h,v 1.10 2008/12/14 21:57:51 vpashka Exp $
*/
// -------------------------------------------------------------------------- 
#ifndef Exceptions_h_
#define Exceptions_h_
// ---------------------------------------------------------------------------
#include <ostream>
#include <iostream>
#include <exception>
// ---------------------------------------------------------------------

namespace UniSetTypes
{
	
/**
  @defgroup UniSetExceptions Исключения
  @{
*/

// namespase UniSetExceptions

/*!
    Базовый класс для всех исключений в библиотеке UniSet
    \note Все вновь создаваемые исключения обязаны наследоваться от него или его потомков
*/
class Exception:
	public std::exception
{
public:
	void printException() 
	{ 
		std::cerr << "Exception: " << text << std::endl;
	}

	Exception(const std::string& txt):text(txt.c_str()){}
	Exception():text("Exception"){}
	virtual ~Exception() throw(){}

	friend std::ostream& operator<<(std::ostream& os, Exception& ex )
	{
		os << ex.text;
		return os;
	}

	virtual const char* what() { return text.c_str(); }
	
protected:
	const std::string text;
};


class PermissionDenied: public Exception
{
public:
	PermissionDenied():Exception("PermissionDenied"){}
};

class NotEnoughMemory: public Exception
{
public:
	NotEnoughMemory():Exception("NotEnoughMemory"){}
};


class OutOfRange: public Exception
{
public:
	OutOfRange():Exception("OutOfRange"){}
	OutOfRange(const std::string err):Exception(err){}
};


class ErrorHandleResource: public Exception
{
public:
	ErrorHandleResource():Exception("ErrorHandleResource"){}
};

/*!
    Исключение, вырабатываемое при превышении максимально допустимого числа пассивных
    таймеров для системы
*/
class LimitWaitingPTimers: public Exception
{
public:
	LimitWaitingPTimers():Exception("LimitWaitingPassiveTimers"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	LimitWaitingPTimers(const std::string err):Exception(err){}
};


/*!
    Исключение, вырабатываемое функциями репозитория объектов.
    Например неверное имя секции при регистрации в репозитории объектов.
*/
class ORepFailed: public Exception
{
public:
	ORepFailed():Exception("ORepFailed"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	ORepFailed(const std::string err):Exception(err){}
};


/*!
    Системные ошибки
*/
class SystemError: public Exception
{
public:
	SystemError():Exception("SystemError"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	SystemError(const std::string err):Exception(err){}
};

class CRCError: public Exception
{
public:
	CRCError():Exception("CRCError"){}
};


/*!
    Ошибка соединения
*/
class CommFailed: public Exception
{
public:
	CommFailed():Exception("CommFailed"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	CommFailed(const std::string err):Exception(err){}
};


/*!
    Исключение, вырабатываемое функциями, использующими удаленный вызов,
    при невозможности вызвать удалённый метод за заданное время.
*/
class TimeOut: public CommFailed
{
	public:
		TimeOut():CommFailed("TimeOut") {}

	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	TimeOut(const std::string err):CommFailed(err){}

};

/*!
    Исключение вырабатываемое при ошибке разыменования объекта репозитория
*/
class ResolveNameError: public ORepFailed
{
	public:
		ResolveNameError():ORepFailed("ResolveNameError"){}
		ResolveNameError(const std::string err):ORepFailed(err){}
};


class NSResolveError: public ORepFailed
{
	public:
		NSResolveError():ORepFailed("NSResolveError"){}
		NSResolveError(const std::string err):ORepFailed(err){}
};


/*!
    Исключение, вырабатываемое функциями репозитория объектов.
    Попытка зарегистрировать объект под уже существующим именем
*/
class ObjectNameAlready: public ResolveNameError
{
public:
	ObjectNameAlready():ResolveNameError("ObjectNameAlready"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	ObjectNameAlready(const std::string err):ResolveNameError(err){}
};

/*!
    Исключение, вырабатываемое в случае указания неправильных аргументов при работе
    с объектами(функциями) ввода/вывода
*/
class IOBadParam: public Exception
{
	public:
	IOBadParam():Exception("IOBadParam"){}
	
	/*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
	IOBadParam(const std::string err):Exception(err){}
};

/*!
    Исключение, вырабатываемое в случае присутствия в имени недопустимых символов.
	См. UniSetTypes::BadSymbols[]
*/
class InvalidObjectName: public ResolveNameError
{
	public:
		InvalidObjectName():ResolveNameError("InvalidObjectName"){}
		InvalidObjectName(const std::string err):ResolveNameError(err){}
};

/*! Исключение, вырабатываемое в случае если не удалось установить обработчик сигнала */
class NotSetSignal: public Exception
{
	public:
		NotSetSignal():Exception("NotSetSignal"){}
		NotSetSignal(const std::string err):Exception(err){}
};

class NameNotFound: public ResolveNameError
{
public:
	NameNotFound():ResolveNameError("NameNotFound"){}
	NameNotFound(const std::string err):ResolveNameError(err){}
};

//@}
// end of UniSetException group
// ---------------------------------------------------------------------
}	// end of UniSetTypes namespace
// ---------------------------------------------------------------------
#endif // Exception_h_
// ---------------------------------------------------------------------
