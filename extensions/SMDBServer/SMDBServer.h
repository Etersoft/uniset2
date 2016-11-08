#ifndef _SMDBServer_H_
#define _SMDBServer_H_
// -----------------------------------------------------------------------------
#include "DBServer_MySQL.h"
#include "SMInterface.h"
#include "SharedMemory.h"
// -----------------------------------------------------------------------------
/*!
*/
class SMDBServer:
	public DBServer_MySQL
{
	public:
		SMDBServer( uniset::ObjectId objId, uniset::ObjectId shmID, SharedMemory* ic = 0,
					const std::string& prefix = "dbserver" );
		virtual ~SMDBServer();

		/*! глобальная функция для инициализации объекта */
		static SMDBServer* init_smdbserver( int argc, const char* const* argv,
											uniset::ObjectId shmID, SharedMemory* ic = 0,
											const std::string& prefix = "dbserver" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

	protected:
		SMDBServer();

		virtual void initDB(DBInterface* db);
		void waitSMReady();
		void step();

		SMInterface* shm;

	private:
		bool aiignore;

		PassiveTimer ptHeartBeat;
		uniset::ObjectId sidHeartBeat;
		int maxHeartBeat;
		IOController::IOStateList::iterator aitHeartBeat;
		uniset::ObjectId test_id;

		std::string db_locale;
		std::string prefix;
};
// -----------------------------------------------------------------------------
#endif // _SMDBServer_H_
// -----------------------------------------------------------------------------
