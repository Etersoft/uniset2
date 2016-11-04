#include <iomanip>
#include "Exceptions.h"
#include "LostTestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
LostTestProc::LostTestProc( UniSetTypes::ObjectId id, xmlNode* confnode ):
	LostTestProc_SK( id, confnode )
{
	auto conf = uniset_conf();

	UniXML::iterator cit(confnode);
	string f_field = conf->getArgParam("--" + argprefix + "filter-field", cit.getProp("filterField"));
	string f_value = conf->getArgParam("--" + argprefix + "filter-value", cit.getProp("filterValue"));

	xmlNode* snode = conf->getXMLSensorsSection();

	if( !snode )
		throw SystemError(myname + "(init): Not found <sensors> section?!");

	UniXML::iterator it(snode);

	if( !it.goChildren() )
		throw SystemError(myname + "(init): Section <sensors> empty?!");

	for( ; it; it++ )
	{
		if( !UniSetTypes::check_filter(it, f_field, f_value) )
			continue;

		if( it.getProp("iotype") != "AI" )
			continue;

		slist.emplace(it.getIntProp("id"),0);
	}

	setMaxSizeOfMessageQueue(slist.size()+500);
	vmonit(ncycle);
	vmonit(waitEmpty);
}
// -----------------------------------------------------------------------------
LostTestProc::~LostTestProc()
{
}
// -----------------------------------------------------------------------------
LostTestProc::LostTestProc()
{
	cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
	throw UniSetTypes::Exception(myname+"(init): FAILED..");
}
// -----------------------------------------------------------------------------
void LostTestProc::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	LostTestProc_SK::sysCommand(sm);

	if( sm->command == SystemMessage::StartUp || sm->command == SystemMessage::WatchDog )
	{
		askTimer(tmCheck, checkTime);

		// начальная инициализация значений в SM
		for( auto&& s: slist )
		{
			try
			{
				ui->setValue(s.first,s.second);
			}
			catch( std::exception& ex )
			{
				mycrit << myname << "(startUp): " << ex.what() << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void LostTestProc::askSensors(UniversalIO::UIOCommand cmd)
{
	for( const auto& s: slist )
		askSensor(s.first,cmd);
}
// -----------------------------------------------------------------------------
void LostTestProc::sensorInfo(const SensorMessage* sm)
{
	auto s = slist.find(sm->id);
	if( s == slist.end() )
	{
		mycrit << myname << "(sensorInfo): ERROR: message from UNKNOWN SENSOR sm->id=" << sm->id << endl;
		UniSetTypes::SimpleInfo_var i = getInfo();
		mycrit << i->info << endl;
		std::abort();
	}

	s->second = sm->value;
}
// -----------------------------------------------------------------------------
string LostTestProc::getMonitInfo()
{
	ostringstream inf;
	inf << "COUNT SENSORS: " << slist.size() << endl;
	return std::move(inf.str());
}
// -----------------------------------------------------------------------------
void LostTestProc::timerInfo( const TimerMessage* tm )
{
	if( tm->id == tmCheck )
	{
		if(  countMessages() > 0 )
		{
			mylog1 << myname << ": [WAIT]: count of messages " << countMessages() << endl;
			waitEmpty = true;
			return;
		}

		waitEmpty = false;

		myinfo << myname << ": [OK]: empty queue. UPDATE VALUE... " << endl;
		for( auto&& s: slist )
		{
			try
			{
				long smValue = ui->getValue(s.first);
				if( smValue != s.second )
				{
					cerr << myname << "(check): ERROR!! smValue=" << smValue << " != " << s.second << endl;
					UniSetTypes::SimpleInfo_var i = getInfo();
					cerr << i->info << endl;
					std::abort();
				}

				//			cerr << "sm_value=" << smValue << " save " << (in_sensor_s+1) << endl;

				// Выставляем новое значение
				ui->setValue(s.first, s.second+1);

				// проверяем что сохранилось
				smValue = ui->getValue(s.first);

				if(ui->getValue(s.first) != (s.second+1) )
				{
					cerr << myname << "(check): SAVE TO SM ERROR!! smValue=" << smValue  << endl;
					UniSetTypes::SimpleInfo_var i = getInfo();
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
