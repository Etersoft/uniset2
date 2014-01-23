// -----------------------------------------------------------------------------
#ifndef UniExchange_H_
#define UniExchange_H_
// -----------------------------------------------------------------------------
#include <list>
#include "UniXML.h"
#include "IOController.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "PassiveTimer.h"
// -----------------------------------------------------------------------------
/*!
        \page pageUniExchange Обмен между узлами на основе TCP.
    
    \par Обмен построен на основе функций IOControl-ера получения списка дискретных
    и аналоговых датчиков. Работает через удалённые CORBA-вызовы.
    
    \par Процесс считывает из конфигурационного файла список узлов которые необходимо
    опрашивать (точнее список IOControl-еров), запускается поток обмена, в котором
    эти узлы ПОСЛЕДОВАТЕЛЬНО опрашиваются..
    
    \par Пример записи в конфигурационном файле для опроса пяти узлов...
    \code
        <UniExchange name="UniExchange">
            <item name="UniExchange2" node="Node2"/>
            <item id="3001" node_id="Node2"/>
            <item name="UniExchange3" node="Node3"/>
            <item name="UniExchange4" node="Node4"/>
            <item name="UniExchange5" node="Node5"/>
        </UniExchange>
    \endcode
    Запись можно делать по "id" или по "name"
*/
// -----------------------------------------------------------------------------
class UniExchange:
    public IOController
{
    public:
        UniExchange( UniSetTypes::ObjectId id, UniSetTypes::ObjectId shmID, 
                        SharedMemory* ic=0, const std::string& prefix="unet" );
        virtual ~UniExchange();

        void execute();

        static UniExchange* init_exchange( int argc, const char* const* argv,
                                    UniSetTypes::ObjectId shmID, SharedMemory* ic=0,
                                    const std::string& prefix="unet" );

        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char** argv );

        virtual IOController_i::ShortMapSeq* getSensors();

    protected:

        virtual void sysCommand( const UniSetTypes::SystemMessage* sm );
        virtual void askSensors( UniversalIO::UIOCommand cmd );
        virtual void sigterm( int signo );

        xmlNode* cnode;
        std::string s_field;
        std::string s_fvalue;
        SMInterface* shm;
        
        struct SInfo
        {
            SInfo():
                val(0),
                id(UniSetTypes::DefaultObjectId),
                type(UniversalIO::UnknownIOType)
            {}

            IOController::IOStateList::iterator ioit;
            long val;
            long id;
            UniversalIO::IOType type;
            UniSetTypes::uniset_rwmutex val_lock;
        };
        
        typedef std::vector<SInfo> SList;
        
        struct NetNodeInfo
        {
            NetNodeInfo();
        
            CORBA::Object_var oref;
            IOController_i_var shm;
            UniSetTypes::ObjectId id;
            UniSetTypes::ObjectId node;
            UniSetTypes::ObjectId sidConnection; /*!< датчик связи */
            IOController::IOStateList::iterator conn_it;
            SList smap;
            
            void update(IOController_i::ShortMapSeq_var& map, SMInterface* shm );
        };
        
        typedef std::list<NetNodeInfo> NetNodeList;
        NetNodeList nlst;

        void readConfiguration();
        bool readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec );
        bool initItem( UniXML_iterator& it );
        void updateLocalData();
        void initIterators();
        
        int polltime;
        PassiveTimer ptUpdate;
        bool init_ok;
        
        SList mymap;
        size_t maxIndex;
        int smReadyTimeout;
        
    private:
};
// -----------------------------------------------------------------------------
#endif // UniExchange_H_
