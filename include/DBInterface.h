#ifndef DBInterface_H_
#define DBInterface_H_
// --------------------------------------------------------------------------
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include "UniSetTypes.h"
// --------------------------------------------------------------------------
namespace uniset
{
	class DBResult;

	/*! Абстрактный класс для доступа к БД */
	class DBInterface
	{
		public:

			DBInterface() {}
			virtual ~DBInterface() {}

			// Функция подключения к БД, параметры подключения зависят от типа БД
			virtual bool connect( const std::string& param ) = 0;
			virtual bool close() = 0;
			virtual bool isConnection() const = 0;
			virtual bool ping() const = 0; // проверка доступности БД

			virtual DBResult query( const std::string& q ) = 0;
			virtual void cancel_query(){}; // попытка отменить текущий запрос
			virtual const std::string lastQuery() = 0;
			virtual bool insert( const std::string& q ) = 0;
			virtual double insert_id() = 0;
			virtual const std::string error() = 0;
	};
	// ----------------------------------------------------------------------------------
	class DBNetInterface : public DBInterface
	{
		public:

			DBNetInterface() {}
			virtual ~DBNetInterface() {}

			// Для сетевых БД параметры должны быть в формате user@host:pswd:dbname:port
			virtual bool connect( const std::string& param );
			virtual bool nconnect( const std::string& host, const std::string& user, const std::string& pswd,
								   const std::string& dbname, unsigned int port ) = 0;
	};
	// ----------------------------------------------------------------------------------
	class DBRowIterator;

	class DBResult
	{
		public:

			DBResult() {}
			virtual ~DBResult() {}

			typedef std::vector<std::string> COL;
			typedef std::deque<COL> ROW;
			typedef DBRowIterator iterator;

			ROW& row();
			iterator begin();
			iterator end();
			operator bool() const;
			size_t size() const;
			bool empty() const;

			/*! установить соответствие индекса и имени поля */
			void setColName( int index, const std::string& name );

			/*! throw std::runtime_error if name not found */
			int getColIndex( const std::string& name );

			// slow function
			std::string getColName( int index );

			// ----------------------------------------------------------------------------
			// COL
			static int as_int( const DBResult::COL::iterator& it );
			static double as_double(const  DBResult::COL::iterator& it );
			static std::string as_string( const DBResult::COL::iterator& it );
			static size_t num_cols( const DBResult::iterator& it );
			// ----------------------------------------------------------------------------

		protected:

			ROW row_;

			std::unordered_map<std::string, int> colname;
	};
	// ----------------------------------------------------------------------------------
	class DBRowIterator:
		public std::iterator<std::bidirectional_iterator_tag,
		DBResult::ROW::value_type,
		DBResult::ROW::difference_type,
		DBResult::ROW::pointer,
		DBResult::ROW::reference>
	{

		public:

			std::string as_string( const char* name ) const;
			std::string as_string( const std::string& name ) const;
			int as_int( const std::string& name ) const;
			double as_double( const std::string& name ) const;

			std::string as_string( int col ) const;
			int as_int( int col ) const;
			double as_double( int col ) const;

			size_t num_cols() const;

			typename DBRowIterator::pointer operator->();
			typename DBRowIterator::reference operator*() const;

			DBRowIterator( const DBRowIterator& it );

			bool operator!=(DBRowIterator const& ) const;
			bool operator==(DBRowIterator const& ) const;
			DBRowIterator& operator+(int) noexcept;
			DBRowIterator& operator+=(int) noexcept;
			DBRowIterator& operator++() noexcept;	// ++x
			DBRowIterator operator++(int) noexcept; // x++
			DBRowIterator& operator-(int) noexcept;
			DBRowIterator operator--(int) noexcept; // x--
			DBRowIterator& operator--() noexcept; // --x
			DBRowIterator& operator-=(int) noexcept;

		private:
			friend class DBResult;
			DBRowIterator( DBResult& dbres, const DBResult::ROW::iterator& );

			DBResult& dbres;
			DBResult::ROW::iterator it;
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
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // DBInterface_H_
// --------------------------------------------------------------------------
