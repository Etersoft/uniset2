#include <iomanip>
#include "Exceptions.h"
#include "LostTestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// -----------------------------------------------------------------------------
LostTestProc::LostTestProc( uniset::ObjectId id, xmlNode* confnode ):
	LostPassiveTestProc( id, confnode )
{
	vmonit(ncycle);
	vmonit(waitEmpty);
}
// -----------------------------------------------------------------------------
LostTestProc::~LostTestProc()
{
}
// -----------------------------------------------------------------------------
void LostTestProc::setChildPassiveProc( const std::shared_ptr<LostPassiveTestProc>& lp )
{
	child = lp;
}
// -----------------------------------------------------------------------------
LostTestProc::LostTestProc()
{
	cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
	throw uniset::Exception(myname + "(init): FAILED..");
}
// -----------------------------------------------------------------------------
void LostTestProc::sysCommand( const uniset::SystemMessage* sm )
{
	LostTestProc_SK::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
	{
		askTimer(tmCheck, checkTime);

		// начальная инициализация значений в SM
		for( auto && s : slist )
		{
			try
			{
				ui->setValue(s.first, s.second);
			}
			catch( std::exception& ex )
			{
				mycrit << myname << "(startUp): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
string LostTestProc::getMonitInfo()
{
	ostringstream inf;
	inf << "COUNT SENSORS: " << slist.size() << endl;
	return inf.str();
}
// -----------------------------------------------------------------------------
void LostTestProc::timerInfo( const TimerMessage* tm )
{
	if( tm->id == tmCheck )
	{
		if( countMessages() > 0 )
		{
			mylog1 << myname << ": [WAIT]: count of messages " << countMessages() << endl;
			waitEmpty = true;
			return;
		}

		if( child && !child->emptyQueue() )
		{
			mylog1 << myname << ": [WAIT]: wait empty queue for " << child->getName() << endl;
			waitEmpty = true;
			return;
		}

		waitEmpty = false;

		myinfo << myname << ": [OK]: empty queue. CHECK VALUE... " << endl;

		for( auto && s : slist )
		{
			try
			{
				long smValue = ui->getValue(s.first);

				if( smValue != s.second )
				{
					cerr << myname << "(check): ERROR!! sid=" << s.first << " smValue=" << smValue << " != " << s.second << endl;
					uniset::SimpleInfo_var i = getInfo("");
					cerr << i->info << endl;
					std::abort();
				}

				if( child )
				{
					long childValue = child->checkValue(s.first);

					if( smValue != childValue )
					{
						cerr << myname << "(check): ERROR!! sid=" << s.first << " smValue=" << smValue << " != " << childValue
							 << " FOR CHILD: " << child->getName()
							 << endl;

						uniset::SimpleInfo_var i = getInfo("");
						cerr << i->info << endl;

						cerr << "=============== CHILD INFO: ==============" << endl;
						uniset::SimpleInfo_var i2 = child->getInfo("");
						cerr << i2->info << endl;


						//						cerr << "JSON: " << endl;
						//						Poco::URI::QueryParameters p;
						//						auto j = httpGet(p);
						//						cerr << j.dump() << endl;
						//						cerr << "-------------------------" << endl;
						//						auto j2 = child->httpGet(p);
						//						cerr << j2.dump() << endl;

						std::abort();
					}
				}
			}
			catch( std::exception& ex )
			{
				mycrit << myname << "(check): " << ex.what() << endl;
			}
		}

		myinfo << myname << ": [OK]: UPDATE VALUE... " << endl;

		for( auto && s : slist )
		{
			try
			{
				// Выставляем новое значение
				ui->setValue(s.first, s.second + 1);

				// проверяем что сохранилось
				long smValue = ui->getValue(s.first);

				if(ui->getValue(s.first) != (s.second + 1) )
				{
					cerr << myname << "(check): SAVE TO SM ERROR!! sid=" << s.first
						 << " value=" << smValue << " != " << (s.second + 1) << endl;
					uniset::SimpleInfo_var i = getInfo("");
					cerr << i->info << endl;
					std::abort();
				}
			}
			catch( std::exception& ex )
			{
				mycrit << myname << "(check): " << ex.what() << endl;
			}
		}

		myinfo << myname << ": [OK]: UPDATE FINISHED: cycle=" << ++ncycle << endl;
	}
}
// -----------------------------------------------------------------------------
