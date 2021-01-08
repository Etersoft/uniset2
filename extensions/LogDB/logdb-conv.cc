#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include "SQLiteInterface.h"
// --------------------------------------------------------------------------
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------
typedef std::pair<std::string, std::string> LogInfo;
const size_t maxRead = 5000; // сколько читать прежде чем записать в БД
static std::vector<std::string> qbuf;
// --------------------------------------------------------------------------
void saveToDB( SQLiteInterface* db );
void parseLine( SQLiteInterface* db, const LogInfo& loginfo, const std::string& line );
void parseFile( SQLiteInterface* db, const LogInfo& loginfo );
LogInfo makeLogInfo( const std::string& name );

// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    try
    {
        if( argc < 3 )
        {
            cout << "uniset2-logdb-conv dbfile [logname1:]logfile1.log [logname2:]logfile2.log..." << endl;
            cout << "  logfile    - log file" << endl;
            cout << "  [lognameX] - log name for db. Default: name of logfile" << endl;
            cout << "  :          - separator for logname" << endl;
            cout << endl;
            cout << "log format: " << endl;
            cout << "28/07/2017 10:55:19(  info):  xxx text text text" << endl;
            cout << "28/07/2017 10:55:19(  info):  xxx text text text" << endl;
            cout << "28/07/2017 10:55:19(  info):  xxx text text text" << endl;
            cout << endl;
            return 0;
        }

        std::string dbfile = argv[1];

        std::vector<LogInfo> files;

        for( int i = 2; i < argc; i++ )
            files.emplace_back(makeLogInfo(argv[i]));

        SQLiteInterface db;

        if( !db.connect(dbfile, false) )
        {
            cerr << "(logdb-conv): DB connection error: " << db.error() << endl;
            return 1;
        }


        qbuf.reserve(maxRead);

        for( const auto& f : files )
            parseFile(&db, f);

        db.close();
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(logdb-conv): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(logdb-conv): catch ..." << endl;
    }

    return 1;
}

// --------------------------------------------------------------------------
LogInfo makeLogInfo( const std::string& name )
{
    LogInfo linf;

    auto pos = name.find(':');

    if( pos != string::npos )
    {
        linf.first = name.substr(0, pos);
        linf.second = name.substr(pos + 1, name.size());
    }
    else
    {
        linf.second = name;

        // вырезаем расширение файла
        auto epos = name.rfind('.');

        if( epos == string::npos )
            linf.first = name;
        else
            linf.first = name.substr(0, epos);
    }


    return linf;
}

// --------------------------------------------------------------------------

void parseLine( SQLiteInterface* db, const LogInfo& loginfo, const std::string& line )
{
    // 28/07/2017 10:55:19(  info):  text...text...more text

    static const std::regex re("[::space::]{0,}"
                               "(\\d{2})[/-]{1}(\\d{2})[/-]{1}(\\d{4})" //date
                               " "
                               "(\\d{2}:\\d{2}:\\d{2})"  // time
                               //                              "(.*)$"
                              );

    std::smatch match;

    if( !std::regex_search(line, match, re) )
    {
        cerr << "error: parse line: " << line << endl;
        return;
    }

    if( match.size() < 5 )
    {
        cerr << "error: parse line: " << line << endl;
        return;
    }

    ostringstream q;
    q << "INSERT INTO logs(tms,usec,name,text) VALUES("
      << "CAST( strftime('%s','" << match[3] << "-" << match[2] << "-" << match[1] << " " << match[4] << "') AS INT)"
      << ",0,'"  //  usec
      << loginfo.first << "','" // name
      << line << "');"; // text

    qbuf.push_back(q.str());

    if( qbuf.size() >= maxRead )
        saveToDB(db);
}
// --------------------------------------------------------------------------
void parseFile( SQLiteInterface* db, const LogInfo& loginfo )
{
    ifstream f(loginfo.second);

    if( !f.is_open() )
    {
        cerr << "(logdb-conv): can`t open "
             << loginfo.second << ". error: " << strerror(errno)
             << endl;
        return;
    }

    std::string line;

    while( std::getline(f, line) )
        parseLine(db, loginfo, line);

    f.close();

    saveToDB(db);
}
// --------------------------------------------------------------------------
void saveToDB( SQLiteInterface* db )
{
    db->query("BEGIN;");

    for( const auto& q : qbuf )
    {
        if( !db->insert(q) )
        {
            cerr << "(saveToDB): error: " << db->error()
                 << " lost log: " << q << endl;
        }
    }

    db->query("COMMIT;");

    if( !db->lastQueryOK() )
        cerr << "(saveToDB): error: " << db->error() << endl;

    qbuf.clear();
    qbuf.reserve(maxRead);
}

// --------------------------------------------------------------------------
