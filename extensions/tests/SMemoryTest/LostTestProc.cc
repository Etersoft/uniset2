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
	}
}
// -----------------------------------------------------------------------------
void LostTestProc::timerInfo( const TimerMessage* tm )
{
	if( tm->id == tmCheck )
	{
		try
		{
			long smValue = ui->getValue(sensor_s);
			if( smValue != in_sensor_s )
			{
				mycrit << myname << "(check): ERROR!! smValue=" << smValue << " != " << strval(sensor_s) << endl;
				//std::terminate();
				UniSetTypes::SimpleInfo_var i = getInfo();
				mycrit << i->info << endl;
				std::abort();
			}

//			cerr << "sm_value=" << smValue << " save " << (in_sensor_s+1) << endl;

			// Выставляем новое значение
			ui->setValue(sensor_s, in_sensor_s+1);

			// проверяем что сохранилось
			smValue = ui->getValue(sensor_s);

			if(ui->getValue(sensor_s) != (in_sensor_s+1) )
			{
				mycrit << myname << "(check): SAVE TO SM ERROR!! smValue=" << smValue << strval(sensor_s) << endl;
				std::abort();
			}
		}
		catch( std::exception& ex )
		{
			mycrit << myname << "(check): " << ex.what() << endl;
		}
	}
}
// -----------------------------------------------------------------------------
