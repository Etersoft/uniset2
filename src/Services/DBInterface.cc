#include "DBInterface.h"
// ------------------------------------------------------------------------------------------
bool DBNetInterface::connect( const std::string& param )
{
	std::string host;
	std::string user;
	std::string pswd;
	std::string dbname;
	std::string::size_type pos = param.find_first_of("@");
	std::string::size_type prev = 0;
	if( pos != std::string::npos )
		user = param.substr(prev, pos);
	prev = pos + 1;
	pos = param.find_first_of(":", prev);
	if( pos != std::string::npos )
		host = param.substr(prev, pos);
	prev = pos + 1;
	pos = param.find_first_of(":", prev);
	if( pos != std::string::npos )
		pswd = param.substr(prev, pos);
	prev = pos + 1;
	pos = param.find_first_of(":", prev);
	if( pos != std::string::npos )
		dbname = param.substr(prev, pos);
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