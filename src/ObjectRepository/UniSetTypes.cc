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
		if (calMin < calMax) {
			if( ret < calMin )
				return calMin;
			if( ret > calMax )
				return calMax;
		} else {
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
		if (calMin < calMax) {
			if( ret < calMin )
				return calMin;
			if( ret > calMax )
				return calMax;
		} else {
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
			if ((ret*10) >= ((calMax + calMin)*5))
				ret = calMax;
			else
				ret = calMin;
		}
		return ret;
	}

	// -------------------------------------------------------------------------
	UniSetTypes::IDList::IDList():
		node(UniSetTypes::conf->getLocalNode())
	{
	
	}
	
	UniSetTypes::IDList::~IDList()
	{
	}

	void UniSetTypes::IDList::add( ObjectId id )
	{
		for( list<ObjectId>::iterator it=lst.begin(); it!=lst.end(); ++it )
		{
			if( (*it) == id )
				return;
		}
		
		lst.push_back(id);
	}
	
	
	void UniSetTypes::IDList::del( ObjectId id )
	{
		for( list<ObjectId>::iterator it=lst.begin(); it!=lst.end(); ++it )
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

	UniSetTypes::ObjectId UniSetTypes::IDList::getFirst()
	{
		if( lst.empty() )
			return UniSetTypes::DefaultObjectId;
			
		return (*lst.begin());
	}

	// за освобождение выделеной памяти
	// отвечает вызывающий!
	IDSeq* UniSetTypes::IDList::getIDSeq()
	{
		IDSeq* seq = new IDSeq();
		seq->length(lst.size());
		int i=0;
		for( list<ObjectId>::iterator it=lst.begin(); it!=lst.end(); ++it,i++ )
			(*seq)[i] = (*it);
		
		return seq;
	}
	// -------------------------------------------------------------------------
	bool UniSetTypes::file_exist( const std::string filename )
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
	UniSetTypes::IDList UniSetTypes::explode( const string str, char sep )
	{
		UniSetTypes::IDList l;

		string::size_type prev = 0;
		string::size_type pos = 0;
		do
		{
			pos = str.find(sep,prev);
			string s(str.substr(prev,pos-prev));
			if( !s.empty() )
			{
				l.add( uni_atoi(s) );
				prev=pos+1;
			}
		}
		while( pos!=string::npos );
	
		return l;
	}
	// -------------------------------------------------------------------------
	std::list<std::string> UniSetTypes::explode_str( const string str, char sep )
	{
		std::list<std::string> l;

		string::size_type prev = 0;
		string::size_type pos = 0;
		do
		{
			pos = str.find(sep,prev);
			string s(str.substr(prev,pos-prev));
			if( !s.empty() )
			{
				l.push_back(s);
				prev=pos+1;
			}
		}
		while( pos!=string::npos );
	
		return l;
	}	
	// ------------------------------------------------------------------------------------------
	bool UniSetTypes::is_digit( const std::string s )
	{
		for( std::string::const_iterator it=s.begin(); it!=s.end(); it++ )
		{
			if( !isdigit(*it) )
				return false;
		}
	
		return true;
	  //return (std::count_if(s.begin(),s.end(),std::isdigit) == s.size()) ? true : false;
	}
	// --------------------------------------------------------------------------------------
	std::list<UniSetTypes::ParamSInfo> UniSetTypes::getSInfoList( string str, Configuration* conf )
	{
		std::list<UniSetTypes::ParamSInfo> res;
		
		std::list<std::string> l = UniSetTypes::explode_str(str,',');
		for( std::list<std::string>::iterator it=l.begin(); it!=l.end(); it++ )
		{
			UniSetTypes::ParamSInfo item;
			
			std::list<std::string> p = UniSetTypes::explode_str((*it),'=');
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
				cerr << "WARNING: parse error for '" << (*it) << "'. IGNORE..." << endl;
				continue;
			}			  
		  	
		  	item.fname = s;
			std::list<std::string> t = UniSetTypes::explode_str(s,'@');
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
			  
				if( is_digit(s_node.c_str()) )
					item.si.node = uni_atoi(s_node);
				else
					item.si.node= conf->getNodeID(s_node);
			}
			else
			{
				cerr << "WARNING: parse error for '" << s << "'. IGNORE..." << endl;
				continue;
			}

			res.push_back(item);
		}
	 
		return res;
	}
	// --------------------------------------------------------------------------------------
	
	UniversalIO::IOType UniSetTypes::getIOType( const std::string stype )
	{
		if ( stype == "DI" || stype == "di" )
			return UniversalIO::DI;
		if( stype == "AI" || stype == "ai" )
			return UniversalIO::AI;
		if ( stype == "DO" || stype == "do" )
			return UniversalIO::DO;
		if ( stype == "AO" || stype == "ao" )
			return UniversalIO::AO;

		return 	UniversalIO::UnknownIOType;
	}
	// ------------------------------------------------------------------------------------------
	std::ostream& UniSetTypes::operator<<( std::ostream& os, const UniversalIO::IOType t )
	{
		if( t == UniversalIO::AnalogInput )
			return os << "AI";

		if( t == UniversalIO::DigitalInput )
			return os << "DI";

		if( t == UniversalIO::AnalogOutput )
			return os << "AO";

		if( t == UniversalIO::DigitalOutput )
			return os << "DO";

		return os << "UnknownIOType";
	}
	// ------------------------------------------------------------------------------------------
	std::ostream& UniSetTypes::operator<<( std::ostream& os, const IOController_i::CalibrateInfo c )
	{
		return os << " rmin=" << c.minRaw << " rmax=" << c.maxRaw
					<< " cmin=" << c.minCal << " cmax=" << c.maxCal
					<< " precision=" << c.precision
					<< " sensibility=" << c.sensibility;
	}
	// ------------------------------------------------------------------------------------------
	bool UniSetTypes::check_filter( UniXML_iterator& it, const std::string f_prop, const std::string f_val )
	{
		if( f_prop.empty() )
			return true;

		// просто проверка на не пустой field
		if( f_val.empty() && it.getProp(f_prop).empty() )
			return false;

		// просто проверка что field = value
		if( !f_val.empty() && it.getProp(f_prop)!=f_val )
			return false;

		return true;
	}
