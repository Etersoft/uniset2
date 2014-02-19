// -----------------------------------------------------------------------------
#ifndef _MBTCPMultiSlave_H_
#define _MBTCPMultiSlave_H_
// -----------------------------------------------------------------------------
#include <map>
#include "MBSlave.h"
#include "modbus/ModbusTCPServer.h"
// -----------------------------------------------------------------------------
/*!
        <MBTCPMultiSlave ....sesscount="">
            <clients>
                <item ip="" respond="" invert="1" askcount=""/>
                <item ip="" respond="" invert="1" askcount=""/>
                <item ip="" respond="" invert="1" askcount=""/>
            </clients>
        </MBTCPMultiSlave>
*/
// -----------------------------------------------------------------------------
/*! Реализация многоптоточного slave-интерфейса */
class MBTCPMultiSlave:
    public MBSlave
{
    public:
        MBTCPMultiSlave( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID, SharedMemory* ic=0, const std::string& prefix="mbs" );
        virtual ~MBTCPMultiSlave();

        /*! глобальная функция для инициализации объекта */
        static MBTCPMultiSlave* init_mbslave( int argc, const char* const* argv,
                                            UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
                                            const std::string& prefix="mbs" );

        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char* const* argv );

    protected:
        virtual void execute_tcp() override;
        virtual void initIterators() override;

        timeout_t sessTimeout;  /*!< таймаут на сессию */
        timeout_t waitTimeout;
        ModbusTCPServer::Sessions sess; /*!< список открытых сессий */
        unsigned int sessMaxNum;

        struct ClientInfo
        {
            ClientInfo():iaddr(""),respond_s(UniSetTypes::DefaultObjectId),invert(false),
              askCount(0),askcount_s(UniSetTypes::DefaultObjectId){ ptTimeout.setTiming(0); }

            std::string iaddr;

            UniSetTypes::ObjectId respond_s;
            IOController::IOStateList::iterator respond_it;
            bool invert;
            PassiveTimer ptTimeout;
            timeout_t tout;

            long askCount;
            UniSetTypes::ObjectId askcount_s;
            IOController::IOStateList::iterator askcount_it;

            inline void initIterators( SMInterface* shm )
            {
                shm->initIterator( respond_it );
                shm->initIterator( askcount_it );
            }
        };

        typedef std::map<const std::string,ClientInfo> ClientsMap;
        ClientsMap cmap;


        UniSetTypes::ObjectId sesscount_id;
        IOController::IOStateList::iterator sesscount_it;
};
// -----------------------------------------------------------------------------
#endif // _MBTCPMultiSlave_H_
// -----------------------------------------------------------------------------
