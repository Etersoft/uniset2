/* This file is part of the UniSet project
 * Copyright (c) 2002-2005 Free Software Foundation, Inc.
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
 *  \author Pavel Vainerman
 *    \brief базовые типы и вспомогательные функции библиотеки UniSet
*/
// --------------------------------------------------------------------------
#ifndef UniSetTypes_H_
#define UniSetTypes_H_
// --------------------------------------------------------------------------
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <ostream>
#include <unistd.h>

#include <omniORB4/CORBA.h>
#include "UniSetTypes_i.hh"
#include "IOController_i.hh"
#include "Mutex.h"
#include "UniXML.h"
// -----------------------------------------------------------------------------------------
/*! Задержка в миллисекундах */
inline void msleep( unsigned int m )
{
	usleep(m * 1000);
}

/*! Определения базовых типов библиотеки UniSet (вспомогательные типы данных, константы, полезные функции) */
namespace UniSetTypes
{
	class Configuration;
	// ---------------------------------------------------------------
	// Вспомогательные типы данных и константы

	const ObjectId DefaultObjectId = -1;    /*!< Идентификатор объекта по умолчанию */
	const ThresholdId DefaultThresholdId = -1;      /*!< идентификатор порогов по умолчанию */
	const ThresholdId DefaultTimerId = -1;      /*!< идентификатор таймера по умолчанию */

	const ObjectId AdminID = -2; /*!< сервисный идентификатор используемый утилитой admin */

	typedef unsigned long KeyType;    /*!< уникальный ключ объекта */

	/*! генератор уникального положительного ключа
	 *    Уникальность гарантируется только для пары значений
	 *  id и node.
	 * \warning что тут у нас с переполнением..
	 * \warning Уникальность генерируемого ключа еще не проверялась,
	     но нареканий по использованию тоже не было :)
	*  \todo Желательно продумать что-нибудь с использованием хэш.
	*/
	inline static KeyType key( const UniSetTypes::ObjectId id, const UniSetTypes::ObjectId node )
	{
		return KeyType((id * node) + (id + 2 * node));
	}

	inline static KeyType key( const IOController_i::SensorInfo& si )
	{
		return key(si.id, si.node);
	}

	typedef std::list<std::string> ListObjectName;    /*!< Список объектов типа ObjectName */

	typedef ObjectId SysId;
	typedef CORBA::Object_ptr ObjectPtr;    /*!< Ссылка на объект регистрируемый в ObjectRepository */
	typedef CORBA::Object_var ObjectVar;    /*!< Ссылка на объект регистрируемый в ObjectRepository */

	UniversalIO::IOType getIOType( const std::string& s );
	std::ostream& operator<<( std::ostream& os, const UniversalIO::IOType t );
	std::ostream& operator<<( std::ostream& os, const IONotifyController_i::ThresholdInfo& ti );
	std::ostream& operator<<( std::ostream& os, const IOController_i::ShortIOInfo& s );

	/*! Команды для управления лампочками */
	enum LampCommand
	{
		lmpOFF      = 0,    /*!< выключить */
		lmpON       = 1,    /*!< включить */
		lmpBLINK    = 2,    /*!< мигать */
		lmpBLINK2   = 3,    /*!< мигать */
		lmpBLINK3   = 4     /*!< мигать */
	};

	static const long ChannelBreakValue = std::numeric_limits<long>::max();

	class IDList
	{
		public:
			IDList();
			~IDList();

			void add( ObjectId id );
			void del( ObjectId id );

			inline int size() const
			{
				return lst.size();
			}
			inline bool empty() const
			{
				return lst.empty();
			}

			std::list<ObjectId> getList();

			// за освобождение выделеной памяти
			// отвечает вызывающий!
			IDSeq* getIDSeq() const;

			//
			ObjectId getFirst() const;
			ObjectId node;    // узел, на котором находятся датчики

		private:
			std::list<ObjectId> lst;
	};

	/*! Информация об имени объекта */
	struct ObjectInfo
	{
		ObjectInfo():
			id(DefaultObjectId),
			repName(0), textName(0), data(0) {}

		ObjectId id;        /*!< идентификатор */
		char* repName;      /*!< текстовое имя для регистрации в репозитории */
		char* textName;     /*!< текстовое имя */
		void* data;

		inline bool operator < ( const ObjectInfo& o ) const
		{
			return (id < o.id);
		}
	};

	typedef std::list<NodeInfo> ListOfNode;

	/*! Запрещенные для использования в именах объектов символы */
	const char BadSymbols[] = {'.', '/'};

	// ---------------------------------------------------------------
	// Различные преобразования

	//! Преобразование строки в число (воспринимает префикс 0, как 8-ное, префикс 0x, как 16-ное, минус для отриц. чисел)
	int uni_atoi( const char* str );
	inline int uni_atoi( const std::string& str )
	{
		return uni_atoi(str.c_str());
	}

	char* uni_strdup( const std::string& src );

	std::string timeToString(time_t tm = time(0), const std::string& brk = ":"); /*!< Преобразование времени в строку HH:MM:SS */
	std::string dateToString(time_t tm = time(0), const std::string& brk = "/"); /*!< Преобразование даты в строку DD/MM/YYYY */

	/*! Разбивка строки по указанному символу */
	IDList explode( const std::string& str, char sep = ',' );
	std::vector<std::string> explode_str( const std::string& str, char sep = ',' );

	struct ParamSInfo
	{
		IOController_i::SensorInfo si;
		long val;
		std::string fname; // fullname id@node or id
	};

	/*! Функция разбора строки вида: id1@node1=val1,id2@node2=val2,...
	   Если '=' не указано, возвращается val=0
	  Если @node не указано, возвращается node=DefaultObjectId */
	std::list<ParamSInfo> getSInfoList( const std::string& s, std::shared_ptr<UniSetTypes::Configuration> conf = nullptr );

	/*! проверка является текст в строке - числом..*/
	bool is_digit( const std::string& s );

	// ---------------------------------------------------------------
	// Работа с командной строкой

	/*! Получение параметра командной строки
	    \param name - название параметра
	    \param defval - значение, которое будет возвращено, если параметр не найден
	*/
	inline std::string getArgParam( const std::string& name,
									int _argc, const char* const* _argv,
									const std::string& defval = "" )
	{
		for( int i = 1; i < (_argc - 1) ; i++ )
		{
			if( name == _argv[i] )
				return _argv[i + 1];
		}

		return defval;
	}

	inline int getArgInt( const std::string& name,
						  int _argc, const char* const* _argv,
						  const std::string defval = "" )
	{
		return uni_atoi(getArgParam(name, _argc, _argv, defval));
	}

	/*! Проверка наличия параметра в командной строке
	    \param name - название параметра
	    \param _argc - argc
	    \param _argv - argv
	    \return Возвращает -1, если параметр не найден.
	        Или позицию параметра, если найден.
	*/
	inline int findArgParam( const std::string& name, int _argc, const char* const* _argv )
	{
		for( int i = 1; i < _argc; i++ )
		{
			if( name == _argv[i] )
				return i;
		}

		return -1;
	}

	// ---------------------------------------------------------------
	// Калибровка

	std::ostream& operator<<( std::ostream& os, const IOController_i::CalibrateInfo& c );

	// Функции калибровки значений
	// raw      - преобразуемое значение
	// rawMin   - минимальная граница исходного диапазона
	// rawMax   - максимальная граница исходного диапазона
	// calMin   - минимальная граница калиброванного диапазона
	// calMin   - минимальная граница калиброванного диапазона
	// limit    - обрезать итоговое значение по границам
	float fcalibrate(float raw, float rawMin, float rawMax, float calMin, float calMax, bool limit = true );
	long lcalibrate(long raw, long rawMin, long rawMax, long calMin, long calMax, bool limit = true );

	// установка значения в нужный диапазон
	long setinregion(long raw, long rawMin, long rawMax);
	// установка значения вне диапазона
	long setoutregion(long raw, long rawMin, long rawMax);

	// ---------------------------------------------------------------
	// Всякие helper-ы

	bool file_exist( const std::string& filename );

	// Проверка xml-узла на соответствие <...f_prop="f_val">,
	// если не задано f_val, то проверяется, что просто f_prop!=""
	bool check_filter( UniXML::iterator& it, const std::string& f_prop, const std::string& f_val = "" );

	/*! алгоритм копирования элементов последовательности удовлетворяющих условию */
	template<typename InputIterator,
			 typename OutputIterator,
			 typename Predicate>
	OutputIterator copy_if(InputIterator begin,
						   InputIterator end,
						   OutputIterator destBegin,
						   Predicate p)
	{
		while( begin != end)
		{
			if( p(*begin) ) &destBegin++ = *begin;

			++begin;
		}

		return destBegin;
	}
	// -----------------------------------------------------------------------------
}

// Варварский запрет на использование atoi вместо uni_atoi..
// #define atoi atoi##_Do_not_use_atoi_function_directly_Use_getIntProp90,_getArgInt_or_uni_atoi

// -----------------------------------------------------------------------------------------
#endif
