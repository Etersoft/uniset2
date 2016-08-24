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
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef VMonitor_H_
#define VMonitor_H_
// --------------------------------------------------------------------------
#include <string>
#include <list>
#include <ostream>
#include <unordered_map>
#include <Poco/Types.h>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
#ifndef VMON_DEF_FUNC
#define VMON_DEF_FUNC(T) \
	void add( const std::string& name, const T& v );\
	static const std::string pretty_str( const std::string& name, const T* v, int width = NameWidth ); \
	static const std::string pretty_str( const std::string& name, const T& v, int width = NameWidth )
#endif
#ifndef VMON_DEF_FUNC2
#define VMON_DEF_FUNC2(T) \
	void add( const std::string& name, const T& v );\
	void add( const std::string& name, const unsigned T& v );\
	static const std::string pretty_str( const std::string& name, const T* v, int width = NameWidth );\
	static const std::string pretty_str( const std::string& name, const unsigned T* v, int width = NameWidth ); \
	static const std::string pretty_str( const std::string& name, const T& v, int width = NameWidth );\
	static const std::string pretty_str( const std::string& name, const unsigned T& v, int width = NameWidth )
#endif

#ifndef VMON_DEF_MAP
#define VMON_DEF_MAP(T) std::unordered_map<const T*,const std::string> m_##T
#endif

#ifndef VMON_DEF_MAP2
#define VMON_DEF_MAP2(T) \
	std::unordered_map<const T*,const std::string> m_##T; \
	std::unordered_map<const unsigned T*,const std::string> m_unsigned_##T
#endif

#ifndef VMON_DEF_MAP3
#define VMON_DEF_MAP3(T,M) std::unordered_map<const T*,const std::string> m_##M
#endif

// --------------------------------------------------------------------------
/* EXAMPLE HELPER MACROS

#ifndef vmonit
#define vmonit( var ) add( #var, var )
#endif

*/
// --------------------------------------------------------------------------
/*! Вспомогательный класс для реализации "мониторинга" состояния переменных стандартных(!) типов.
 * Необходимые переменные добавляются при помощи функции add() (специально перегруженной под разные типы).
 * Для удобства использования должен быть определён макрос примерно следующего вида
 \code
  #define vmonit( var ) vmon.add( #var, var )
 \endcode
 * При условии, что в классе создан объект VMonitor с именем vmon.
 \code
  class MyClass
  {
	 public:

		MyClass()
		{
		   // сделать один раз для нужных переменных
		   vmonit(myvar1);
		   vmonit(myvar2)
		   vmonit(myvar3);
		}

		...

		void printState()
		{
		  cout << vmon.get_pretty_str() << endl;
		  // или
		  cout << vmon.str() << endl;
		  // или
		  cout << vmon << endl;
		}

	 private:
		int myvar1;
		bool myvar2;
		long myvar3;
		...
		VMonitor vmon;
  }
 \endcode
 *
 *
 * \todo Нужно добавить поддержку "пользовательских типов" (возможно нужно использовать variadic templates)
*/
class VMonitor
{
	public:
		VMonitor() {}

		friend std::ostream& operator<<(std::ostream& os, VMonitor& m );

		static const int NameWidth = { 30 };
		static const int ColCount = { 2 };

		/*! вывести все элементы в "простом формате" (строки "varname = value") */
		std::string str();

		/*! вывести все элементы "с форматированием" (отсортированные по алфавиту)
		 * \param namewidth - ширина резервируемая под "имя"
		 * \param colnum - количество столбцов вывода
		 */
		std::string pretty_str( int namewidth = NameWidth, int colnum = ColCount );

		// функции добавления..
		VMON_DEF_FUNC2(int);
		VMON_DEF_FUNC2(long);
		VMON_DEF_FUNC2(short);
		VMON_DEF_FUNC2(char);
		VMON_DEF_FUNC(bool);
		VMON_DEF_FUNC(float);
		VMON_DEF_FUNC(double);
		VMON_DEF_FUNC(Poco::Int64); // <--- for timeout_t
		//		VMON_DEF_FUNC(UniSetTypes::ObjectId); // <--- long

		void add( const std::string& name, const std::string& v );

		static const std::string pretty_str( const std::string& name, const std::string* v, int width = NameWidth );
		static const std::string pretty_str( const std::string& name, const std::string& v, int width = NameWidth );

		std::list<std::pair<std::string, std::string>> getList();

	protected:

	private:

		VMON_DEF_MAP2(int);
		VMON_DEF_MAP2(long);
		VMON_DEF_MAP2(short);
		VMON_DEF_MAP2(char);
		VMON_DEF_MAP(bool);
		VMON_DEF_MAP(float);
		VMON_DEF_MAP(double);
		std::unordered_map<const Poco::Int64*, const std::string> m_Int64;
		VMON_DEF_MAP3(std::string, string);
};
// --------------------------------------------------------------------------
#endif
