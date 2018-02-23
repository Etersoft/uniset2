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
#ifndef UExceptions_H_
#define UExceptions_H_
// --------------------------------------------------------------------------
#include <string>
// --------------------------------------------------------------------------
struct UException
{
	UException(): err("UException") {}
	explicit UException( const std::string& e ): err(e) {}
	explicit UException( const char* e ): err( std::string(e)) {}
	~UException() {}

	const std::string getError()
	{
		return err;
	}

	std::string err;
};
//---------------------------------------------------------------------------
struct UTimeOut:
	public UException
{
	UTimeOut(): UException("UTimeOut") {}
	explicit UTimeOut( const std::string& e ): UException(e) {}
	~UTimeOut() {}
};
//---------------------------------------------------------------------------
struct USysError:
	public UException
{
	USysError(): UException("USysError") {}
	explicit USysError( const std::string& e ): UException(e) {}
	~USysError() {}
};
//---------------------------------------------------------------------------
struct UValidateError:
	public UException
{
	UValidateError(): UException("UValidateError") {}
	explicit UValidateError( const std::string& e ): UException(e) {}
	~UValidateError() {}
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
