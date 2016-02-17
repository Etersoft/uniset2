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
// ----------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// -----------------------------------------------------------------------------
#include <cmath>
#include <iomanip>
#include <fstream>
#include "UniSetTypes.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;

// -----------------------------------------------------------------------------
float UniSetTypes::fcalibrate( float raw, float rawMin, float rawMax,
							   float calMin, float calMax, bool limit )
{
	if( rawMax == rawMin ) return 0; // деление на 0!!!

	float ret = (raw - rawMin) * (calMax - calMin) / ( rawMax - rawMin ) + calMin;

	if( !limit )
		return ret;

	// Переворачиваем calMin и calMax для проверки, если calMin > calMax
	if (calMin < calMax)
	{
		if( ret < calMin )
			return calMin;

		if( ret > calMax )
			return calMax;
	}
	else
	{
		if( ret > calMin )
			return calMin;

		if( ret < calMax )
			return calMax;
	}

	return ret;
}
// -------------------------------------------------------------------------
// Пересчитываем из исходных пределов в заданные
long UniSetTypes::lcalibrate(long raw, long rawMin, long rawMax, long calMin, long calMax, bool limit )
{
	if( rawMax == rawMin ) return 0; // деление на 0!!!

	long ret = lroundf( (float)(raw - rawMin) * (float)(calMax - calMin) /
						(float)( rawMax - rawMin ) + calMin );

	if( !limit )
		return ret;

	return setinregion(ret, calMin, calMax);
}

// -------------------------------------------------------------------------
// Приводим указанное значение в заданные пределы
long UniSetTypes::setinregion(long ret, long calMin, long calMax)
{
	// Переворачиваем calMin и calMax для проверки, если calMin > calMax
	if (calMin < calMax)
	{
		if( ret < calMin )
			return calMin;

		if( ret > calMax )
			return calMax;
	}
	else
	{
		if( ret > calMin )
			return calMin;

		if( ret < calMax )
			return calMax;
	}

	return ret;
}

// Выводим указанное значение из заданных пределов (на границы)
long UniSetTypes::setoutregion(long ret, long calMin, long calMax)
{
	if ((ret > calMin) && (ret < calMax))
	{
		if ((ret * 10) >= ((calMax + calMin) * 5))
			ret = calMax;
		else
			ret = calMin;
	}

	return ret;
}

// -------------------------------------------------------------------------
UniSetTypes::IDList::IDList( std::vector<std::string>&& svec ):
	UniSetTypes::IDList::IDList()
{
	for( const auto& s : svec )
		add( uni_atoi(s) );
}
// ------------------------------------------------------------------
UniSetTypes::IDList::IDList( std::vector<std::string>& svec ):
	UniSetTypes::IDList::IDList()
{
	auto conf = uniset_conf();

	for( const auto& s : svec )
	{
		ObjectId id;

		if( is_digit(s) )
			id = uni_atoi(s);
		else
			id = conf->getSensorID(s);

		add(id);
	}
}
// -------------------------------------------------------------------------
UniSetTypes::IDList::IDList():
	node( (UniSetTypes::uniset_conf() ? UniSetTypes::uniset_conf()->getLocalNode() : DefaultObjectId) )
{

}

UniSetTypes::IDList::~IDList()
{
}

void UniSetTypes::IDList::add( ObjectId id )
{
	for( auto it = lst.begin(); it != lst.end(); ++it )
	{
		if( (*it) == id )
			return;
	}

	lst.push_back(id);
}

void UniSetTypes::IDList::del( ObjectId id )
{
	for( auto it = lst.begin(); it != lst.end(); ++it )
	{
		if( (*it) == id )
		{
			lst.erase(it);
			return;
		}
	}
}

std::list<UniSetTypes::ObjectId> UniSetTypes::IDList::getList()
{
	return lst;
}

UniSetTypes::ObjectId UniSetTypes::IDList::getFirst() const
{
	if( lst.empty() )
		return UniSetTypes::DefaultObjectId;

	return (*lst.begin());
}

// за освобождение выделеной памяти
// отвечает вызывающий!
IDSeq* UniSetTypes::IDList::getIDSeq() const
{
	IDSeq* seq = new IDSeq();
	seq->length(lst.size());
	int i = 0;

	for( auto it = lst.begin(); it != lst.end(); ++it, i++ )
		(*seq)[i] = (*it);

	return seq;
}
// -------------------------------------------------------------------------
bool UniSetTypes::file_exist( const std::string& filename )
{
	std::ifstream file;
#ifdef HAVE_IOS_NOCREATE
	file.open( filename.c_str(), std::ios::in | std::ios::nocreate );
#else
	file.open( filename.c_str(), std::ios::in );
#endif
	bool result = false;

	if( file )
		result = true;

	file.close();
	return result;
}
// -------------------------------------------------------------------------
UniSetTypes::IDList UniSetTypes::explode( const string& str, char sep )
{
	UniSetTypes::IDList l( explode_str(str, sep) );
	return std::move(l);
}
// -------------------------------------------------------------------------
std::vector<std::string> UniSetTypes::explode_str( const string& str, char sep )
{
	std::vector<std::string> v;

	string::size_type prev = 0;
	string::size_type pos = 0;
	string::size_type sz = str.size();

	do
	{
		if( prev >= sz )
			break;

		pos = str.find(sep, prev);

		if( pos == string::npos )
		{
			string s(str.substr(prev, sz - prev));

			if( !s.empty() )
				v.emplace_back(s);

			break;
		}

		if( pos == 0 )
		{
			prev = 1;
			continue;
		}

		string s(str.substr(prev, pos - prev));

		if( !s.empty() )
		{
			v.emplace_back(s);
			prev = pos + 1;
		}
	}
	while( pos != string::npos );

	return std::move(v);
}
// ------------------------------------------------------------------------------------------
bool UniSetTypes::is_digit( const std::string& s )
{
	for( auto c : s )
	{
		if( !isdigit(c) )
			return false;
	}

	return true;
	//return (std::count_if(s.begin(),s.end(),std::isdigit) == s.size()) ? true : false;
}
// --------------------------------------------------------------------------------------
std::list<UniSetTypes::ParamSInfo> UniSetTypes::getSInfoList( const string& str, std::shared_ptr<Configuration> conf )
{
	if( conf == nullptr )
		conf = uniset_conf();

	std::list<UniSetTypes::ParamSInfo> res;

	auto lst = UniSetTypes::explode_str(str, ',');

	for( const auto& it : lst )
	{
		UniSetTypes::ParamSInfo item;

		auto p = UniSetTypes::explode_str(it, '=');
		std::string s = "";

		if( p.size() == 1 )
		{
			s = *(p.begin());
			item.val = 0;
		}
		else if( p.size() == 2 )
		{
			s = *(p.begin());
			item.val = uni_atoi(*(++p.begin()));
		}
		else
		{
			cerr << "WARNING: parse error for '" << it << "'. IGNORE..." << endl;
			continue;
		}

		item.fname = s;
		auto t = UniSetTypes::explode_str(s, '@');

		if( t.size() == 1 )
		{
			std::string s_id = *(t.begin());

			if( is_digit(s_id) )
				item.si.id = uni_atoi(s_id);
			else
				item.si.id = conf->getSensorID(s_id);

			item.si.node = DefaultObjectId;
		}
		else if( t.size() == 2 )
		{
			std::string s_id = *(t.begin());
			std::string s_node = *(++t.begin());

			if( is_digit(s_id) )
				item.si.id = uni_atoi(s_id);
			else
				item.si.id = conf->getSensorID(s_id);

			if( is_digit(s_node) )
				item.si.node = uni_atoi(s_node);
			else
				item.si.node = conf->getNodeID(s_node);
		}
		else
		{
			cerr << "WARNING: parse error for '" << s << "'. IGNORE..." << endl;
			continue;
		}

		res.push_back(item);
	}

	return std::move(res);
}
// --------------------------------------------------------------------------------------
std::list<UniSetTypes::ConsumerInfo> UniSetTypes::getObjectsList( const string& str, std::shared_ptr<Configuration> conf )
{
	if( conf == nullptr )
		conf = uniset_conf();

	std::list<UniSetTypes::ConsumerInfo> res;

	auto lst = UniSetTypes::explode_str(str, ',');

	for( const auto& it : lst )
	{
		UniSetTypes::ConsumerInfo item;

		auto t = UniSetTypes::explode_str(it, '@');

		if( t.size() == 1 )
		{
			std::string s_id(*(t.begin()));

			if( is_digit(s_id) )
				item.id = uni_atoi(s_id);
			else
			{
				item.id = conf->getObjectID(s_id);

				if( item.id == DefaultObjectId )
					item.id = conf->getControllerID(s_id);
			}

			item.node = DefaultObjectId;
		}
		else if( t.size() == 2 )
		{
			std::string s_id = *(t.begin());
			std::string s_node = *(++t.begin());

			if( is_digit(s_id) )
				item.id = uni_atoi(s_id);
			else
			{
				item.id = conf->getObjectID(s_id);

				if( item.id == DefaultObjectId )
					item.id = conf->getControllerID(s_id);
			}

			if( is_digit(s_node) )
				item.node = uni_atoi(s_node);
			else
				item.node = conf->getNodeID(s_node);
		}
		else
		{
			cerr << "WARNING: parse error for '" << it << "'. IGNORE..." << endl;
			continue;
		}

		res.push_back(item);
	}

	return std::move(res);
}
// --------------------------------------------------------------------------------------
UniversalIO::IOType UniSetTypes::getIOType( const std::string& stype )
{
	if ( stype == "DI" || stype == "di" )
		return UniversalIO::DI;

	if( stype == "AI" || stype == "ai" )
		return UniversalIO::AI;

	if ( stype == "DO" || stype == "do" )
		return UniversalIO::DO;

	if ( stype == "AO" || stype == "ao" )
		return UniversalIO::AO;

	return     UniversalIO::UnknownIOType;
}
// ------------------------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<( std::ostream& os, const UniversalIO::IOType t )
{
	if( t == UniversalIO::AI )
		return os << "AI";

	if( t == UniversalIO::DI )
		return os << "DI";

	if( t == UniversalIO::AO )
		return os << "AO";

	if( t == UniversalIO::DO )
		return os << "DO";

	return os << "UnknownIOType";
}
// ------------------------------------------------------------------------------------------
bool UniSetTypes::check_filter( UniXML::iterator& it, const std::string& f_prop, const std::string& f_val )
{
	if( f_prop.empty() )
		return true;

	// просто проверка на не пустой field
	if( f_val.empty() && it.getProp(f_prop).empty() )
		return false;

	// просто проверка что field = value
	if( !f_val.empty() && it.getProp(f_prop) != f_val )
		return false;

	return true;
}
// ------------------------------------------------------------------------------------------
string UniSetTypes::timeToString(time_t tm, const std::string& brk )
{
	struct tm* tms = localtime(&tm);
	ostringstream time;
	time << std::setw(2) << std::setfill('0') << tms->tm_hour << brk;
	time << std::setw(2) << std::setfill('0') << tms->tm_min << brk;
	time << std::setw(2) << std::setfill('0') << tms->tm_sec;
	return time.str();
}

string UniSetTypes::dateToString(time_t tm, const std::string& brk )
{
	struct tm* tms = localtime(&tm);
	ostringstream date;
	date << std::setw(4) << std::setfill('0') << tms->tm_year + 1900 << brk;
	date << std::setw(2) << std::setfill('0') << tms->tm_mon + 1 << brk;
	date << std::setw(2) << std::setfill('0') << tms->tm_mday;
	return date.str();
}

//--------------------------------------------------------------------------------------------
int UniSetTypes::uni_atoi( const char* str )
{
	// if str is NULL or sscanf failed, we return 0
	if( str == nullptr )
		return 0;

	// приходиться самостоятельно проверять на наличие префикса "0x"
	// чтобы применить соответствующую функцию.
	// причём для чисел применяется atoll,
	// чтобы корректно обрабатывать большие числа типа std::numeric_limits<unsigned int>::max()
	// \warning есть сомнения, что на 64bit-тах это будет корректно работать..

	unsigned int n = 0;

	if( strlen(str) > 2 )
	{
		if( str[0] == '0' && str[1] == 'x' )
		{
			std::sscanf(str, "%x", &n);
			return n;
		}
	}

	n = std::atoll(str); // универсальнее получать unsigned..чтобы не потерять "большие числа"..
	return n; // а возвращаем int..
}
//--------------------------------------------------------------------------------------------
char* UniSetTypes::uni_strdup( const string& src )
{
	const char* s = src.c_str();
	size_t len = strlen(s);
	char* d = new char[len + 1];
	memcpy(d, s, len + 1);
	return d;
}
// -------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<( std::ostream& os, const IOController_i::CalibrateInfo& c )
{
	os << "[ rmin=" << c.minRaw
	   << " rmax=" << c.maxRaw
	   << " cmin=" << c.minCal
	   << " cmax=" << c.maxCal
	   << " prec=" << c.precision
	   << " ]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<( std::ostream& os, const IONotifyController_i::ThresholdInfo& ti )
{
	os << "[ id=" << ti.id
	   << " hilim=" << ti.hilimit
	   << " lowlim=" << ti.lowlimit
	   << " state=" << ti.state
	   << " tv_sec=" << ti.tv_sec
	   << " tv_usec=" << ti.tv_usec
	   << " invert=" << ti.invert
	   << " ]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<( std::ostream& os, const IOController_i::ShortIOInfo& s )
{
	os << setw(10) << dateToString(s.tv_sec)
	   << " " << setw(8) << timeToString(s.tv_sec) << "." << s.tv_usec
	   << " [ value=" << s.value << " supplier=" << s.supplier << " ]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& UniSetTypes::operator<<( std::ostream& os, const IONotifyController_i::ThresholdState& s )
{
	if( s == IONotifyController_i::LowThreshold )
		return os << "low";

	if( s == IONotifyController_i::HiThreshold )
		return os << "hi";

	if( s == IONotifyController_i::NormalThreshold )
		return os << "norm";

	return os << "Unknown";
}
// -------------------------------------------------------------------------
