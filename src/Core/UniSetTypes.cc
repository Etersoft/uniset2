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
// ----------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// -----------------------------------------------------------------------------
#include <cmath>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "UniSetTypes.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;

// -----------------------------------------------------------------------------
float uniset::fcalibrate( float raw, float rawMin, float rawMax,
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
// -----------------------------------------------------------------------------
double uniset::dcalibrate( double raw, double rawMin, double rawMax,
						   double calMin, double calMax, bool limit )
{
	if( rawMax == rawMin ) return 0; // деление на 0!!!

	double ret = (raw - rawMin) * (calMax - calMin) / ( rawMax - rawMin ) + calMin;

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
long uniset::lcalibrate(long raw, long rawMin, long rawMax, long calMin, long calMax, bool limit )
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
long uniset::setinregion(long ret, long calMin, long calMax)
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
long uniset::setoutregion(long ret, long calMin, long calMax)
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
uniset::IDList::IDList( const std::vector<string>& svec ): // -V730
	uniset::IDList::IDList()
{
	auto conf = uniset_conf();

	for( const auto& s : svec )
	{
		ObjectId id;

		if( is_digit(s) )
			id = uni_atoi(s);
		else
			id = conf->getSensorID(s);

		if( id != DefaultObjectId )
			add(id);
	}
}
// -------------------------------------------------------------------------
uniset::IDList::IDList():
	node( (uniset::uniset_conf() ? uniset::uniset_conf()->getLocalNode() : DefaultObjectId) )
{

}

uniset::IDList::~IDList()
{
}

void uniset::IDList::add( ObjectId id )
{
	for( auto it = lst.begin(); it != lst.end(); ++it )
	{
		if( (*it) == id )
			return;
	}

	lst.push_back(id);
}

void uniset::IDList::del( ObjectId id )
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

std::list<uniset::ObjectId> uniset::IDList::getList() const noexcept
{
	return lst;
}

uniset::ObjectId uniset::IDList::getFirst() const noexcept
{
	if( lst.empty() )
		return uniset::DefaultObjectId;

	return (*lst.begin());
}

// за освобождение выделенной памяти
// отвечает вызывающий!
IDSeq* uniset::IDList::getIDSeq() const
{
	IDSeq* seq = new IDSeq();
	seq->length(lst.size());
	int i = 0;

	for( auto it = lst.begin(); it != lst.end(); ++it, i++ )
		(*seq)[i] = (*it);

	return seq;
}
// -------------------------------------------------------------------------
bool uniset::file_exist( const std::string& filename )
{
	std::ifstream file;
#ifdef HAVE_IOS_NOCREATE
	file.open( filename.c_str(), std::ios::in | std::ios::nocreate );
#else
	file.open( filename.c_str(), std::ios::in );
#endif
	bool result = false;

	if( file.is_open() )
		result = true;

	file.close();
	return result;
}
// -------------------------------------------------------------------------
uniset::IDList uniset::explode( const std::string& str, char sep )
{
	uniset::IDList l( explode_str(str, sep) );
	return l;
}
// -------------------------------------------------------------------------
std::vector<std::string> uniset::explode_str( const std::string& str, char sep )
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
				v.emplace_back( std::move(s) );

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
			v.emplace_back(std::move(s));
			prev = pos + 1;
		}
	}
	while( pos != string::npos );

	return v;
}
// ------------------------------------------------------------------------------------------
bool uniset::is_digit( const std::string& s ) noexcept
{
	if( s.empty() )
		return false;

	for( const auto& c : s )
	{
		if( !isdigit(c) )
			return false;
	}

	return true;
	//return (std::count_if(s.begin(),s.end(),std::isdigit) == s.size()) ? true : false;
}
// --------------------------------------------------------------------------------------
std::list<uniset::ParamSInfo> uniset::getSInfoList( const string& str, std::shared_ptr<Configuration> conf )
{
	std::list<uniset::ParamSInfo> res;

	auto lst = uniset::explode_str(str, ',');

	for( const auto& it : lst )
	{
		uniset::ParamSInfo item;

		auto p = uniset::explode_str(it, '=');
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
		auto t = uniset::explode_str(s, '@');

		if( t.size() == 1 )
		{
			std::string s_id = *(t.begin());

			if( is_digit(s_id) || !conf )
				item.si.id = uni_atoi(s_id);
			else
				item.si.id = conf->getSensorID(s_id);

			item.si.node = DefaultObjectId;
		}
		else if( t.size() == 2 )
		{
			std::string s_id = *(t.begin());
			std::string s_node = *(++t.begin());

			if( is_digit(s_id) || !conf )
				item.si.id = uni_atoi(s_id);
			else
				item.si.id = conf->getSensorID(s_id);

			if( is_digit(s_node) || !conf )
				item.si.node = uni_atoi(s_node);
			else
				item.si.node = conf->getNodeID(s_node);
		}
		else
		{
			cerr << "WARNING: parse error for '" << s << "'. IGNORE..." << endl;
			continue;
		}

		res.emplace_back( std::move(item) );
	}

	return res;
}
// --------------------------------------------------------------------------------------
std::list<uniset::ConsumerInfo> uniset::getObjectsList( const string& str, std::shared_ptr<Configuration> conf )
{
	if( conf == nullptr )
		conf = uniset_conf();

	std::list<uniset::ConsumerInfo> res;

	auto lst = uniset::explode_str(str, ',');

	for( const auto& it : lst )
	{
		uniset::ConsumerInfo item;

		auto t = uniset::explode_str(it, '@');

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

				if( item.id == DefaultObjectId )
					item.id = conf->getServiceID(s_id);
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

				if( item.id == DefaultObjectId )
					item.id = conf->getServiceID(s_id);
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

		res.emplace_back( std::move(item) );
	}

	return res;
}
// --------------------------------------------------------------------------------------
UniversalIO::IOType uniset::getIOType( const std::string& stype ) noexcept
{
	if ( stype == "DI" || stype == "di" )
		return UniversalIO::DI;

	if( stype == "AI" || stype == "ai" )
		return UniversalIO::AI;

	if ( stype == "DO" || stype == "do" )
		return UniversalIO::DO;

	if ( stype == "AO" || stype == "ao" )
		return UniversalIO::AO;

	return UniversalIO::UnknownIOType;
}
// ------------------------------------------------------------------------------------------

std::string uniset::iotype2str( const UniversalIO::IOType& t ) noexcept
{
	if( t == UniversalIO::AI )
		return "AI";

	if( t == UniversalIO::DI )
		return "DI";

	if( t == UniversalIO::AO )
		return "AO";

	if( t == UniversalIO::DO )
		return "DO";

	return "UnknownIOType";
}
// ------------------------------------------------------------------------------------------
std::ostream& uniset::operator<<( std::ostream& os, const UniversalIO::IOType t )
{
	return os << iotype2str(t);
}
// ------------------------------------------------------------------------------------------
bool uniset::check_filter( UniXML::iterator& it, const std::string& f_prop, const std::string& f_val ) noexcept
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
string uniset::timeToString(time_t tm, const std::string& brk ) noexcept
{
	std::tm tms;
	gmtime_r(&tm, &tms);
	ostringstream time;
	time << std::setw(2) << std::setfill('0') << tms.tm_hour << brk;
	time << std::setw(2) << std::setfill('0') << tms.tm_min << brk;
	time << std::setw(2) << std::setfill('0') << tms.tm_sec;
	return time.str();
}

string uniset::dateToString(time_t tm, const std::string& brk ) noexcept
{
	std::tm tms;
	gmtime_r(&tm, &tms);
	ostringstream date;
	date << std::setw(4) << std::setfill('0') << tms.tm_year + 1900 << brk;
	date << std::setw(2) << std::setfill('0') << tms.tm_mon + 1 << brk;
	date << std::setw(2) << std::setfill('0') << tms.tm_mday;
	return date.str();
}

//--------------------------------------------------------------------------------------------
int uniset::uni_atoi( const char* str ) noexcept
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

	n = std::atoll(str); // универсальнее получать unsigned, чтобы не потерять "большие числа"..
	return n; // а возвращаем int..
}
//--------------------------------------------------------------------------------------------
char* uniset::uni_strdup( const string& src )
{
	size_t len = src.size();
	char* d = new char[len + 1];
	memcpy(d, src.data(), len);
	d[len] = '\0';
	return d;
}
// -------------------------------------------------------------------------
std::ostream& uniset::operator<<( std::ostream& os, const IOController_i::CalibrateInfo& c )
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
std::ostream& uniset::operator<<( std::ostream& os, const IONotifyController_i::ThresholdInfo& ti )
{
	os << "[ id=" << ti.id
	   << " hilim=" << ti.hilimit
	   << " lowlim=" << ti.lowlimit
	   << " state=" << ti.state
	   << " tv_sec=" << ti.tv_sec
	   << " tv_nsec=" << ti.tv_nsec
	   << " invert=" << ti.invert
	   << " ]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& uniset::operator<<( std::ostream& os, const IOController_i::ShortIOInfo& s )
{
	os << setw(10) << dateToString(s.tv_sec)
	   << " " << setw(8) << timeToString(s.tv_sec) << "." << s.tv_nsec
	   << " [ value=" << s.value << " supplier=" << s.supplier << " ]";

	return os;
}
// -------------------------------------------------------------------------
std::ostream& uniset::operator<<( std::ostream& os, const IONotifyController_i::ThresholdState& s )
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
std::string uniset::replace_all( const std::string& src, const std::string& from, const std::string& to )
{
	string res(src);

	if( from.empty() )
		return res;

	size_t pos = res.find(from, 0);

	while( pos != std::string::npos )
	{
		res.replace(pos, from.length(), to);
		pos += to.length();
		pos = res.find(from, pos);
	}

	return res;
}
// -------------------------------------------------------------------------
timeval uniset::to_timeval( const chrono::system_clock::duration& d )
{
	struct timeval tv;

	if( d.count() == 0 )
		tv.tv_sec = tv.tv_usec = 0;
	else
	{
		std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);
		tv.tv_sec  = sec.count();
		tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(d - sec).count();
	}

	return tv;
}
// -------------------------------------------------------------------------
timespec uniset::to_timespec( const chrono::system_clock::duration& d )
{
	struct timespec ts;

	if( d.count() == 0 )
		ts.tv_sec = ts.tv_nsec = 0;
	else
	{
		std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);
		ts.tv_sec  = sec.count();
		ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec).count();
	}

	return ts;
}
// -------------------------------------------------------------------------
timespec uniset::now_to_timespec()
{
	auto d = std::chrono::system_clock::now().time_since_epoch();
	return to_timespec(d);
}
// -------------------------------------------------------------------------
uniset::Timespec_var uniset::now_to_uniset_timespec()
{
	auto d = std::chrono::system_clock::now().time_since_epoch();
	return to_uniset_timespec(d);
}
// -------------------------------------------------------------------------
uniset::Timespec_var uniset::to_uniset_timespec( const chrono::system_clock::duration& d )
{
	uniset::Timespec_var ts;

	if( d.count() == 0 )
		ts->sec = ts->nsec = 0;
	else
	{
		std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);
		ts->sec  = sec.count();
		ts->nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec).count();
	}

	return ts;
}
// -------------------------------------------------------------------------
char uniset::checkBadSymbols( const string& str )
{
	for ( const auto& c : str )
	{
		for( size_t k = 0; k < sizeof(BadSymbols); k++ )
		{
			if ( c == BadSymbols[k] )
				return (char)BadSymbols[k];
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------------------------------------------
string uniset::BadSymbolsToStr()
{
	string bad = "";

	for( size_t i = 0; i < sizeof(BadSymbols); i++ )
	{
		bad += "'" +
			   bad += BadSymbols[i];
		bad += "', ";
	}

	return bad;
}
// ---------------------------------------------------------------------------------------------------------------
uniset::KeyType uniset::key( const uniset::ObjectId id, const uniset::ObjectId node )
{
	//! \warning что тут у нас с переполнением..
	return KeyType( (id * node) + (id + 2 * node) );
}
// ---------------------------------------------------------------------------------------------------------------
uniset::KeyType uniset::key( const IOController_i::SensorInfo& si )
{
	return key(si.id, si.node);
}
// ---------------------------------------------------------------------------------------------------------------
