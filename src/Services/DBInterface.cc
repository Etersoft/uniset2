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
		return DBRowIterator(*this, row_.begin());
	}
	// ----------------------------------------------------------------------------
	DBResult::iterator DBResult::end()
	{
		return DBRowIterator(*this, row_.end());
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
		return it.as_int(col);
	}
	// ----------------------------------------------------------------------------
	double DBResult::as_double( const DBResult::iterator& it, int col )
	{
		return it.as_double(col);
	}
	// ----------------------------------------------------------------------------
	std::string DBResult::as_string( const DBResult::iterator& it, int col )
	{
		return it.as_string(col);
	}

	int DBResult::as_int( const DBResult::iterator& it, const std::string& cname )
	{
		return it.as_int(cname);
	}

	double DBResult::as_double(const DBResult::iterator& it, const std::string& cname)
	{
		return it.as_double(cname);
	}

	std::string DBResult::as_string( const DBResult::iterator& it, const std::string& cname )
	{
		return it.as_string(cname);
	}
	// ----------------------------------------------------------------------------
	size_t DBResult::num_cols( const DBResult::iterator& it )
	{
		return it.num_cols();
	}
	// ----------------------------------------------------------------------------
	void DBResult::setColName( int index, const std::string& name )
	{
		colname[name] = index;
	}

	int DBResult::getColIndex( const std::string& name )
	{
		auto i = colname.find(name);

		if( i == colname.end() )
			throw std::runtime_error("(DBInterface): Unknown field ='" + name + "'");

		return i->second;
	}
	// ----------------------------------------------------------------------------
	std::string DBResult::getColName( int index )
	{
		for( auto && c : colname )
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
	DBRowIterator::DBRowIterator( DBResult& _dbres, const DBResult::ROW::iterator& _it ):
		dbres(_dbres), it(_it)
	{

	}

	DBRowIterator::DBRowIterator( const DBRowIterator& i ):
		dbres(i.dbres), it(i.it)
	{

	}

	bool DBRowIterator::operator!=( const DBRowIterator& i ) const
	{
		return (it != i.it);
	}

	bool DBRowIterator::operator==( const DBRowIterator& i) const
	{
		return (it == i.it);
	}


	DBRowIterator& DBRowIterator::operator+(int i) noexcept
	{
		it += i;
		return (*this);
	}

	DBRowIterator& DBRowIterator::operator-(int i) noexcept
	{
		it -= i;
		return (*this);
	}

	DBRowIterator DBRowIterator::operator--(int) noexcept
	{
		DBRowIterator  tmp(*this);
		it--;
		return tmp;
	}

	DBRowIterator& DBRowIterator::operator--() noexcept
	{
		it--;
		return (*this);
	}

	DBRowIterator& DBRowIterator::operator-=(int i) noexcept
	{
		return (*this) - i;
	}

	DBRowIterator& DBRowIterator::operator+=(int i) noexcept
	{
		return (*this) + i;
	}

	DBRowIterator& DBRowIterator::operator++() noexcept
	{
		++it;
		return (*this);
	}

	DBRowIterator DBRowIterator::operator++(int) noexcept
	{
		DBRowIterator tmp(*this);
		it++;
		return tmp;
	}

	// ----------------------------------------------------------------------------
	std::string DBRowIterator::as_string( const char* name ) const
	{
		return as_string( dbres.getColIndex(name) );
	}

	std::string DBRowIterator::as_string( const std::string& name ) const
	{
		return as_string( dbres.getColIndex(name) );
	}

	int DBRowIterator::as_int(const std::string& name ) const
	{
		return as_int(dbres.getColIndex(name));
	}

	double DBRowIterator::as_double( const std::string& name ) const
	{
		return as_double(dbres.getColIndex(name));
	}

	std::string DBRowIterator::as_string( int col ) const
	{
		return ((*it)[col]);
	}

	int DBRowIterator::as_int( int col ) const
	{
		return uniset::uni_atoi( (*it)[col] );
	}

	double DBRowIterator::as_double( int col ) const
	{
		return atof( ((*it)[col]).c_str() );
	}

	size_t DBRowIterator::num_cols() const
	{
		return it->size();
	}

	DBRowIterator::pointer DBRowIterator::operator->()
	{
		return it.operator->();
	}

	DBRowIterator::reference DBRowIterator::operator*() const
	{
		return (*it);
	}
	// ----------------------------------------------------------------------------
} // end of namespace uniset
