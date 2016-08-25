#ifndef DBInterface_H_
#define DBInterface_H_
// --------------------------------------------------------------------------
#include <string>
#include <deque>
#include <vector>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
/*! Абстрактный класс для доступа к БД
*/
class DBResult;

class DBInterface
{
	public:

		DBInterface() {};
		virtual ~DBInterface() {};

		// Функция подключения к БД, параметры подключения зависят от типа БД
		virtual bool connect( const std::string& param ) = 0;
		virtual bool close() = 0;
		virtual bool isConnection() const = 0;
		virtual bool ping() const = 0; // проверка доступности БД

		virtual DBResult query( const std::string& q ) = 0;
		virtual const std::string lastQuery() = 0;
		virtual bool insert( const std::string& q ) = 0;
		virtual double insert_id() = 0;
		virtual const std::string error() = 0;
};
// ----------------------------------------------------------------------------------
class DBNetInterface : public DBInterface
{
	public:

		DBNetInterface() {};
		virtual ~DBNetInterface() {};

		// Для сетевых БД параметры должны быть в формате user@host:pswd:dbname:port
		virtual bool connect( const std::string& param );
		virtual bool nconnect( const std::string& host, const std::string& user, const std::string& pswd,
							   const std::string& dbname, unsigned int port ) = 0;
};
// ----------------------------------------------------------------------------------
class DBResult
{
	public:

		DBResult() {}
		virtual ~DBResult() {};

		typedef std::vector<std::string> COL;
		typedef std::deque<COL> ROW;
		typedef ROW::iterator iterator;

		ROW& row();
		iterator begin();
		iterator end();
		operator bool() const;
		size_t size() const;
		bool empty() const;

		// ----------------------------------------------------------------------------
		// ROW
		static int as_int( const DBResult::iterator& it, int col );
		static double as_double( const DBResult::iterator& it, int col );
		static std::string as_string( const DBResult::iterator& it, int col );
		// ----------------------------------------------------------------------------
		// COL
		static int as_int( const DBResult::COL::iterator& it );
		static double as_double(const  DBResult::COL::iterator& it );
		static std::string as_string( const DBResult::COL::iterator& it );
		static int num_cols( const DBResult::iterator& it );
		// ----------------------------------------------------------------------------

	protected:

		ROW row_;
};
// ----------------------------------------------------------------------------------
struct DBInterfaceDeleter
{
	void operator()(DBInterface* p) const
	{
		try
		{
			delete p;
		}
		catch(...) {}
	}
};
// ----------------------------------------------------------------------------------
// the types of the class factories
typedef std::shared_ptr<DBInterface> create_dbinterface_t();
// --------------------------------------------------------------------------
#endif // DBInterface_H_
// --------------------------------------------------------------------------
