#include "DBInterface.h"
// ------------------------------------------------------------------------------------------
bool DBNetInterface::connect( const std::string& param )
{
	std::string host = "";
	std::string user = "";
	std::string pswd = "";
	std::string dbname = "";
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
		break;
	}

	return nconnect( host, user, pswd, dbname );
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
DBResult::operator bool()
{
	return !row_.empty();
}
// ----------------------------------------------------------------------------
size_t DBResult::size()
{
	return row_.size();
}
// ----------------------------------------------------------------------------
bool DBResult::empty()
{
	return row_.empty();
}
// ----------------------------------------------------------------------------
int DBResult::as_int( const DBResult::iterator& it, int col )
{
	return UniSetTypes::uni_atoi( (*it)[col] );
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
// ----------------------------------------------------------------------------
int DBResult::num_cols( const DBResult::iterator& it )
{
	return it->size();
}
// ----------------------------------------------------------------------------
int DBResult::as_int( const DBResult::COL::iterator& it )
{
	return UniSetTypes::uni_atoi( (*it) );
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