// -----------------------------------------------------------------------------
#ifndef _MBTCPPersistentSlave_H_
#define _MBTCPPersistentSlave_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include "MBSlave.h"
#include "modbus/ModbusTCPServer.h"
// -----------------------------------------------------------------------------
/*!
        <MBTCPPersistentSlave ....sesscount="">
            <clients>
                <item ip="" respond="" invert="1" askcount=""/>
                <item ip="" respond="" invert="1" askcount=""/>
                <item ip="" respond="" invert="1" askcount=""/>
            </clients>
        </MBTCPPersistentSlave>
*/
// -----------------------------------------------------------------------------
/*! Реализация многоптоточного slave-интерфейса */
class MBTCPPersistentSlave:
	public MBSlave
{
	public:
		MBTCPPersistentSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "mbs" );
		virtual ~MBTCPPersistentSlave();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<MBTCPPersistentSlave> init_mbslave( int argc, const char* const* argv,
				UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "mbs" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		UniSetTypes::SimpleInfo* getInfo( CORBA::Long userparam ) override;

	protected:
		virtual void execute_tcp() override;
		virtual void initIterators() override;
		virtual bool deactivateObject() override;
		virtual void sigterm( int signo ) override;

		timeout_t sessTimeout;  /*!< таймаут на сессию */
		timeout_t waitTimeout;
		ModbusTCPServer::Sessions sess; /*!< список открытых сессий */
		unsigned int sessMaxNum;
		PassiveTimer ptUpdateInfo;

		struct ClientInfo
		{
			ClientInfo(): iaddr(""), respond_s(UniSetTypes::DefaultObjectId), invert(false),
				askCount(0), askcount_s(UniSetTypes::DefaultObjectId)
			{
				ptTimeout.setTiming(0);
			}

			std::string iaddr;

			UniSetTypes::ObjectId respond_s = { UniSetTypes::DefaultObjectId };
			IOController::IOStateList::iterator respond_it;
			bool invert = { false };
			PassiveTimer ptTimeout;
			timeout_t tout = { 2000 };

			long askCount;
			UniSetTypes::ObjectId askcount_s = { UniSetTypes::DefaultObjectId };
			IOController::IOStateList::iterator askcount_it;

			inline void initIterators( const std::shared_ptr<SMInterface>& shm )
			{
				shm->initIterator( respond_it );
				shm->initIterator( askcount_it );
			}

			const std::string getShortInfo() const;
		};

		typedef std::unordered_map<std::string, ClientInfo> ClientsMap;
		ClientsMap cmap;


		UniSetTypes::ObjectId sesscount_id;
		IOController::IOStateList::iterator sesscount_it;
};
// -----------------------------------------------------------------------------
#endif // _MBTCPPersistentSlave_H_
// -----------------------------------------------------------------------------
