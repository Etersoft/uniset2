// --------------------------------------------------------------------------
#include <sstream>
#include <cstdio>
#include <UniSetTypes.h>
#include "PostgreSQLInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------

PostgreSQLInterface::PostgreSQLInterface():
db(0),
result(0),
lastQ(""),
lastE(""),
queryok(false),
connected(false),
last_inserted_id(0)
{
}

PostgreSQLInterface::~PostgreSQLInterface()
{
    close();
}

// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::ping()
{
    return db && ( PQstatus(db) == CONNECTION_OK );
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::connect( const string& host, const string& user, const string& pswd, const string& dbname)
{
    if(connected == true)
        return true;

    std::string conninfo ="dbname="+dbname+" host="+host+" user="+user+" password="+pswd;
    db = PQconnectdb(conninfo.c_str());
    if (PQstatus(db) == CONNECTION_BAD) {
        connected = false;
        return false;
    }

    connected = true;
    return true;

}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::close()
{
    if( db )
    {
        freeResult();
        PQfinish(db);
        db = 0;
    }

    return true;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::insert( const string& q )
{
    if( !db )
        return false;

    result = PQexec(db, q.c_str());
    ExecStatusType status = PQresultStatus(result);

    if( !checkResult(status) ){
        freeResult();
        queryok = false;
        return false;
    }

    queryok = true;
    freeResult();
    return true;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::insertAndSaveRowid( const string& q )
{
    if( !db )
        return false;

    std::string qplus=q+" RETURNING id";
    result = PQexec(db, qplus.c_str());
    ExecStatusType status = PQresultStatus(result);

    if( !checkResult(status) ){
        queryok = false;
        freeResult();
        return false;
    }

    save_inserted_id(result);

    queryok = true;
    freeResult();
    return true;
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::checkResult( ExecStatusType rc )
{
    if( rc == PGRES_BAD_RESPONSE  || rc == PGRES_NONFATAL_ERROR || rc == PGRES_FATAL_ERROR )
        return false;
/*
NORMAL
    PGRES-COMMAND-OK -- Successful completion of a command returning no data
    PGRES-TUPLES-OK -- The query successfully executed
    PGRES-COPY-OUT -- Copy Out (from server) data transfer started
    PGRES-COPY-IN -- Copy In (to server) data transfer started

    PGRES-EMPTY-QUERY -- The string sent to the backend was empty
ERRORS:

    PGRES-BAD-RESPONSE -- The server's response was not understood
    PGRES-NONFATAL-ERROR
    PGRES-FATAL-ERROR
*/
    return true;
}
// -----------------------------------------------------------------------------------------
PostgreSQLResult PostgreSQLInterface::query( const string& q )
{
    if( !db )
        return PostgreSQLResult();

    result = PQexec(db, q.c_str());
    ExecStatusType status = PQresultStatus(result);

    if( !checkResult(status) )
    {
        queryok = false;
        return PostgreSQLResult();
    }

    lastQ = q;
    queryok = true;
    return PostgreSQLResult(result);
}
// -----------------------------------------------------------------------------------------
string PostgreSQLInterface::error()
{
    if( db )
        lastE = PQerrorMessage(db);

    return lastE;
}
// -----------------------------------------------------------------------------------------
const string PostgreSQLInterface::lastQuery()
{
    return lastQ;
}
// -----------------------------------------------------------------------------------------
double PostgreSQLInterface::insert_id()
{
    return last_inserted_id;
}
// -----------------------------------------------------------------------------------------
void PostgreSQLInterface::save_inserted_id( PGresult* res )
{
    if( res && PQntuples(res) == 1 && PQnfields(res) == 1 )
        last_inserted_id = atoll(PQgetvalue(res, 0, 0));
}
// -----------------------------------------------------------------------------------------
bool PostgreSQLInterface::isConnection()
{
	return (db && PQstatus(db) == CONNECTION_OK);
//    return connected;
}
// -----------------------------------------------------------------------------------------
void PostgreSQLInterface::freeResult()
{
    if( !db || !result )
        return;

    PQclear(result);
    result = 0;
}
// -----------------------------------------------------------------------------------------
int num_cols( PostgreSQLResult::iterator& it )
{
    return it->size();
}
// -----------------------------------------------------------------------------------------
int as_int( PostgreSQLResult::iterator& it, int col )
{
//    if( col<0 || col >it->size() )
//        return 0;
    return uni_atoi( (*it)[col] );
}
// -----------------------------------------------------------------------------------------
double as_double( PostgreSQLResult::iterator& it, int col )
{
    return atof( ((*it)[col]).c_str() );
}
// -----------------------------------------------------------------------------------------
string as_string( PostgreSQLResult::iterator& it, int col )
{
    return ((*it)[col]);
}
// -----------------------------------------------------------------------------------------
int as_int( PostgreSQLResult::COL::iterator& it )
{
    return uni_atoi( (*it) );
}
// -----------------------------------------------------------------------------------------
double as_double( PostgreSQLResult::COL::iterator& it )
{
    return atof( (*it).c_str() );
}
// -----------------------------------------------------------------------------------------
std::string as_string( PostgreSQLResult::COL::iterator& it )
{
    return (*it);
}
// -----------------------------------------------------------------------------------------
#if 0
PostgreSQLResult::COL get_col( PostgreSQLResult::ROW::iterator& it )
{
    return (*it);
}
#endif
// -----------------------------------------------------------------------------------------
PostgreSQLResult::~PostgreSQLResult()
{

}
// -----------------------------------------------------------------------------------------
PostgreSQLResult::PostgreSQLResult( PGresult* res, bool clearRES )
{
    int rec_count = PQntuples(res);
    int rec_fields = PQnfields(res);

    for (int nrow=0; nrow<rec_count; nrow++) {
        COL c;
        for (int ncol=0; ncol<rec_fields; ncol++) {
            char* p = (char*)PQgetvalue(res, nrow, ncol);
            if( p )
                c.push_back(p);
            else
                c.push_back("");

        }
        row.push_back(c);
    }

    if( clearRES )
        PQclear(res);
}
// -----------------------------------------------------------------------------------------
