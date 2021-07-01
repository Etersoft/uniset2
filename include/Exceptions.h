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
 *  \brief Иерархия генерируемых библиотекой исключений
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef Exceptions_h_
#define Exceptions_h_
// ---------------------------------------------------------------------------
#include <ostream>
#include <iostream>
#include <string>
#include <exception>
// ---------------------------------------------------------------------

namespace uniset
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

            Exception(const std::string& txt) noexcept: text(txt) {}
            Exception() noexcept: text("Exception") {}
            virtual ~Exception() noexcept(true) {}

            friend std::ostream& operator<<(std::ostream& os, const Exception& ex )
            {
                os << ex.text;
                return os;
            }

            virtual const char* what() const noexcept override
            {
                return text.c_str();
            }

        protected:
            const std::string text;
    };

    class OutOfRange: public Exception
    {
        public:
            OutOfRange() noexcept: Exception("OutOfRange") {}
            OutOfRange(const std::string& err) noexcept: Exception(err) {}
    };

    /*! Исключение, вырабатываемое функциями репозитория объектов.
        Например неверное имя секции при регистрации в репозитории объектов.
    */
    class ORepFailed: public Exception
    {
        public:
            ORepFailed() noexcept: Exception("ORepFailed") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            ORepFailed(const std::string& err) noexcept: Exception(err) {}
    };


    /*! Системные ошибки */
    class SystemError: public Exception
    {
        public:
            SystemError() noexcept: Exception("SystemError") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            SystemError(const std::string& err) noexcept: Exception(err) {}
    };

    /*! Ошибка соединения */
    class CommFailed: public Exception
    {
        public:
            CommFailed() noexcept: Exception("CommFailed") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            CommFailed(const std::string& err) noexcept: Exception(err) {}
    };


    /*!
        Исключение, вырабатываемое функциями, использующими удаленный вызов,
        при невозможности вызвать удалённый метод за заданное время.
    */
    class TimeOut: public CommFailed
    {
        public:
            TimeOut() noexcept: CommFailed("TimeOut") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            TimeOut(const std::string& err) noexcept: CommFailed(err) {}

    };

    /*! Исключение вырабатываемое при ошибке разыменования объекта репозитория */
    class ResolveNameError: public ORepFailed
    {
        public:
            ResolveNameError() noexcept: ORepFailed("ResolveNameError") {}
            ResolveNameError(const std::string& err) noexcept: ORepFailed(err) {}
    };


    class NSResolveError: public ORepFailed
    {
        public:
            NSResolveError() noexcept: ORepFailed("NSResolveError") {}
            NSResolveError(const std::string& err) noexcept: ORepFailed(err) {}
    };


    /*!
        Исключение, вырабатываемое функциями репозитория объектов.
        Попытка зарегистрировать объект под уже существующим именем
    */
    class ObjectNameAlready: public ResolveNameError
    {
        public:
            ObjectNameAlready() noexcept: ResolveNameError("ObjectNameAlready") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            ObjectNameAlready(const std::string& err) noexcept: ResolveNameError(err) {}
    };

    /*!
        Исключение, вырабатываемое в случае указания неправильных аргументов при работе
        с объектами(функциями) ввода/вывода
    */
    class IOBadParam: public Exception
    {
        public:
            IOBadParam() noexcept: Exception("IOBadParam") {}

            /*! Конструктор, позволяющий вывести в сообщении об ошибке дополнительную информацию err */
            IOBadParam(const std::string& err) noexcept: Exception(err) {}
    };

    /*!
        Исключение, вырабатываемое в случае присутствия в имени недопустимых символов.
        См. uniset::BadSymbols[]
    */
    class InvalidObjectName: public ResolveNameError
    {
        public:
            InvalidObjectName() noexcept: ResolveNameError("InvalidObjectName") {}
            InvalidObjectName(const std::string& err) noexcept: ResolveNameError(err) {}
    };

    class NameNotFound: public ResolveNameError
    {
        public:
            NameNotFound() noexcept: ResolveNameError("NameNotFound") {}
            NameNotFound(const std::string& err) noexcept: ResolveNameError(err) {}
    };

    //@}
    // end of UniSetException group
    // ---------------------------------------------------------------------
}   // end of uniset namespace
// ---------------------------------------------------------------------
#endif // Exception_h_
// ---------------------------------------------------------------------
