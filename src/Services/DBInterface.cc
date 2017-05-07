#include "DBInterface.h"
// ------------------------------------------------------------------------------------------
namespace uniset
{
	// ------------------------------------------------------------------------------------------
	bool DBNetInterface::connect( const std::string& param )
	{
		std::string host = "";
		std::string user = "";
		std::string pswd = "";
		std::string dbname = "";
		uint port = 0;

		for(;;)
		{
			std::string::size_type pos = param.find_first_of("@");
			user = param.substr(0, pos);

			if( pos == std::string::npos )
				break;

			std::string::size_type prev = pos + 1;
			pos = param.find_first_of(":", prev);
			host = param.substr(prev, pos - prev);

			if( pos == std::string::npos )
				break;

			prev = pos + 1;
			pos = param.find_first_of(":", prev);
			pswd = param.substr(prev, pos - prev);

			if( pos == std::string::npos )
				break;

			prev = pos + 1;
			pos = param.find_first_of(":", prev);
			dbname = param.substr(prev, pos - prev);

			if( pos == std::string::npos )
				break;

			prev = pos + 1;
			pos = param.find_first_of(":", prev);
			port = (uint)std::atoi( param.substr(prev, pos - prev).c_str() );
			break;
		}

		return nconnect( host, user, pswd, dbname, port );
	}
	//--------------------------------------------------------------------------------------------
	DBResult::ROW& DBResult::row()
	{
		return row_;
	}
	// ----------------------------------------------------------------------------
	DBResult::iterator DBResult::begin()
	{
		return row_.begin();
	}
	// ----------------------------------------------------------------------------
	DBResult::iterator DBResult::end()
	{
		return row_.end();
	}
	// ----------------------------------------------------------------------------
	DBResult::operator bool() const
	{
		return !row_.empty();
	}
	// ----------------------------------------------------------------------------
	size_t DBResult::size() const
	{
		return row_.size();
	}
	// ----------------------------------------------------------------------------
	bool DBResult::empty() const
	{
		return row_.empty();
	}
	// ----------------------------------------------------------------------------
	int DBResult::as_int( const DBResult::iterator& it, int col )
	{
		return uniset::uni_atoi( (*it)[col] );
	}
	// ----------------------------------------------------------------------------
	double DBResult::as_double( const DBResult::iterator& it, int col )
	{
		return atof( ((*it)[col]).c_str() );
	}
	// ----------------------------------------------------------------------------
	std::string DBResult::as_string( const DBResult::iterator& it, int col )
	{
		return ((*it)[col]);
	}

	int DBResult::as_int( const DBResult::iterator& it, const std::string& cname )
	{
		auto i = colname.find(cname);
		if( i == colname.end() )
			throw std::runtime_error("(DBInterface): Unknown field ='" + cname + "'");

		return as_int(it,i->second);
	}

	double DBResult::as_double(const DBResult::iterator& it, const std::string& cname)
	{
		auto i = colname.find(cname);
		if( i == colname.end() )
			throw std::runtime_error("(DBInterface): Unknown field ='" + cname + "'");

		return as_double(it, i->second);
	}

	std::string DBResult::as_string(const DBResult::iterator& it, const std::string& cname )
	{
		auto i = colname.find(cname);
		if( i == colname.end() )
			throw std::runtime_error("(DBInterface): Unknown field ='" + cname + "'");

		return as_string(it, i->second);
	}
	// ----------------------------------------------------------------------------
	size_t DBResult::num_cols( const DBResult::iterator& it )
	{
		return it->size();
	}
	// ----------------------------------------------------------------------------
	void DBResult::setColName( int index, const std::string& name )
	{
		colname[name] = index;
	}
	// ----------------------------------------------------------------------------
	std::string DBResult::getColName( int index )
	{
		for( auto&& c: colname )
		{
			if( c.second == index )
				return c.first;
		}

		return "";
	}
	// ----------------------------------------------------------------------------
	int DBResult::as_int( const DBResult::COL::iterator& it )
	{
		return uniset::uni_atoi( (*it) );
	}
	// ----------------------------------------------------------------------------
	double DBResult::as_double(const  DBResult::COL::iterator& it )
	{
		return atof( (*it).c_str() );
	}
	// ----------------------------------------------------------------------------
	std::string DBResult::as_string( const DBResult::COL::iterator& it )
	{
		return (*it);
	}
	// ----------------------------------------------------------------------------
#if 0
	DBResult::COL DBResult::get_col( DBResult::iterator& it )
	{
		return (*it);
	}
#endif
	// ----------------------------------------------------------------------------
} // end of namespace uniset
