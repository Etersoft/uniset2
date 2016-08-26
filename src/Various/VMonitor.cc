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
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "VMonitor.h"
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD_N(T,m) void VMonitor::add( const std::string& name, const T& v ) \
	{\
		m.emplace(&v,name); \
	} \
	\
	const std::string VMonitor::pretty_str( const std::string& name, const T* v, int nwidth ) \
	{ \
		std::ostringstream s; \
		s << std::right << std::setw(nwidth) << name << std::left << " = " << std::right << std::setw(10)  << *(v); \
		return std::move(s.str()); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T& v, int nwidth ) \
	{ \
		return pretty_str(name,&v,nwidth); \
	}
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD(T) void VMonitor::add( const std::string& name, const T& v ) \
	{\
		m_##T.emplace(&v,name); \
	} \
	\
	const std::string VMonitor::pretty_str( const std::string& name, const T* v, int nwidth ) \
	{ \
		std::ostringstream s; \
		s << std::right << std::setw(nwidth) << name << std::left << " = " << std::right << std::setw(10)  << *(v); \
		return std::move(s.str()); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T& v, int nwidth ) \
	{ \
		return pretty_str(name,&v,nwidth); \
	}
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD2(T) \
	void VMonitor::add( const std::string& name, const T& v ) \
	{\
		m_##T.emplace(&v,name); \
	} \
	void VMonitor::add( const std::string& name, const unsigned T& v ) \
	{\
		m_unsigned_##T.emplace(&v,name); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T* v, int nwidth ) \
	{ \
		std::ostringstream s; \
		s << std::right << std::setw(nwidth) << name << std::left << " = " << std::right << std::setw(10)  << *(v); \
		return std::move(s.str()); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const unsigned T* v, int nwidth ) \
	{ \
		std::ostringstream s; \
		s << std::right << std::setw(nwidth) << name << std::left << " = " << std::right << std::setw(10)  << *(v); \
		return std::move(s.str()); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T& v, int nwidth ) \
	{ \
		return pretty_str(name,&v,nwidth); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const unsigned T& v, int nwidth ) \
	{ \
		return pretty_str(name,&v,nwidth); \
	}
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD3(T,M) void VMonitor::add( const std::string& name, const T& v ) \
	{\
		m_##M.emplace(&v,name); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T* v, int nwidth ) \
	{ \
		std::ostringstream s; \
		s << std::right << std::setw(nwidth) << name << std::left << " = " << std::right << std::setw(10)  << *(v); \
		return std::move(s.str()); \
	} \
	const std::string VMonitor::pretty_str( const std::string& name, const T& v, int nwidth ) \
	{ \
		return pretty_str(name,&v,nwidth); \
	}
// --------------------------------------------------------------------------
#define VMON_MAKE_PAIR(vlist, T) \
	{\
		for( const auto& e: m_##T ) \
			vlist.emplace_back( e.second, std::to_string(*(e.first)) );\
	}
// --------------------------------------------------------------------------
#define VMON_MAKE_PAIR_S(vlist, T) \
	{\
		for( const auto& e: m_##T ) \
			vlist.emplace_back( e.second,*e.first );\
	}
// --------------------------------------------------------------------------
#define VMON_MAKE_PAIR2(vlist, T) \
	{\
		std::ostringstream s;\
		for( const auto& e: m_##T ) \
			vlist.emplace_back( e.second, std::to_string(*(e.first)) );\
		\
		for( const auto& e: m_unsigned_##T ) \
			vlist.emplace_back( e.second, std::to_string(*(e.first)) );\
	}
// --------------------------------------------------------------------------
#define VMON_MAKE_PAIR_CHAR(vlist) \
	{\
		std::ostringstream s;\
		for( const auto& e: m_char ) \
			vlist.emplace_back( e.second,std::to_string((int)(*(e.first))) );\
		\
		for( const auto& e: m_unsigned_char ) \
			vlist.emplace_back( e.second,std::to_string((int)(*(e.first))) );\
	}
// --------------------------------------------------------------------------
VMON_IMPL_ADD2(int)
VMON_IMPL_ADD2(long)
VMON_IMPL_ADD2(short)
VMON_IMPL_ADD2(char)
VMON_IMPL_ADD(bool)
VMON_IMPL_ADD(float)
VMON_IMPL_ADD(double)
VMON_IMPL_ADD3(std::string, string)

#ifndef	POCO_LONG_IS_64_BIT
VMON_IMPL_ADD_N(Poco::Int64, m_Int64)
#endif

//VMON_IMPL_ADD3(UniSetTypes::ObjectId,ObjectId)
// --------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, VMonitor& m )
{
	auto vlist = m.getList();
	// сортируем по алфавиту
	vlist.sort( []( const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b )
	{
		return a.first < b.first;
	});

	for( const auto& e : vlist )
		os << e.first << " = " << e.second;

	return os;
}
// --------------------------------------------------------------------------
std::string VMonitor::str()
{
	std::ostringstream s;
	s << (*this);
	return std::move(s.str());
}
// --------------------------------------------------------------------------
std::list<std::pair<std::string, std::string> > VMonitor::getList()
{
	std::list<std::pair<std::string, std::string>> vlist;
	VMON_MAKE_PAIR2(vlist, int);
	VMON_MAKE_PAIR2(vlist, long);
	VMON_MAKE_PAIR2(vlist, short);
	VMON_MAKE_PAIR_CHAR(vlist);
	VMON_MAKE_PAIR(vlist, bool);
	VMON_MAKE_PAIR(vlist, float);
	VMON_MAKE_PAIR(vlist, double);
	VMON_MAKE_PAIR_S(vlist, string);
	return std::move(vlist);
}
// --------------------------------------------------------------------------
std::string VMonitor::pretty_str( int namewidth, int colnum )
{
	auto vlist = getList();
	std::ostringstream os;

	// сортируем по алфавиту
	vlist.sort( []( const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b )
	{
		return a.first < b.first;
	});

	int n = 0;

	for( const auto& e : vlist )
	{
		os << std::right << std::setw(namewidth) << e.first << std::left << " = " << std::right << std::setw(10) << e.second;

		if( (n++) % colnum )
			os << std::endl;
	}

	return std::move(os.str());
}
// --------------------------------------------------------------------------
