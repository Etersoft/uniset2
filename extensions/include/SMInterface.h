#ifndef SMInterface_H_
#define SMInterface_H_
//--------------------------------------------------------------------------
#include <string>
#include <memory>
#include "UniSetTypes.h"
#include "Mutex.h"
#include "IONotifyController.h"
#include "UInterface.h"
class SMInterface
{
    public:

        SMInterface( UniSetTypes::ObjectId _shmID, const std::shared_ptr<UInterface>& ui,
                        UniSetTypes::ObjectId myid, const std::shared_ptr<IONotifyController> ic=nullptr );
        ~SMInterface();

        void setValue ( UniSetTypes::ObjectId, long value );
        void setUndefinedState( IOController_i::SensorInfo& si, bool undefined, UniSetTypes::ObjectId supplier );

        long getValue ( UniSetTypes::ObjectId id );

        void askSensor( UniSetTypes::ObjectId id, UniversalIO::UIOCommand cmd,
                        UniSetTypes::ObjectId backid = UniSetTypes::DefaultObjectId );

        IOController_i::SensorInfoSeq* getSensorsMap();
        IONotifyController_i::ThresholdsListSeq* getThresholdsList();

        void localSetValue( IOController::IOStateList::iterator& it,
                                UniSetTypes::ObjectId sid,
                                CORBA::Long newvalue, UniSetTypes::ObjectId sup_id );

          long localGetValue( IOController::IOStateList::iterator& it,
                            UniSetTypes::ObjectId sid );

        /*! функция выставления признака неопределённого состояния для аналоговых датчиков
            // для дискретных датчиков необходимости для подобной функции нет.
            // см. логику выставления в функции localSaveState
        */
        void localSetUndefinedState( IOController::IOStateList::iterator& it,
                                    bool undefined, UniSetTypes::ObjectId sid );

        // специальные функции
        IOController::IOStateList::iterator ioEnd();
        void initIterator( IOController::IOStateList::iterator& it );

        bool exist();
        bool waitSMready( int msec, int pause=5000 );
        bool waitSMworking( UniSetTypes::ObjectId, int msec, int pause=3000 );

        inline bool isLocalwork(){ return (ic==NULL); }
        inline UniSetTypes::ObjectId ID(){ return myid; }
        inline const std::shared_ptr<IONotifyController> SM(){ return ic; }
        inline UniSetTypes::ObjectId getSMID(){ return shmID; }

    protected:
        const std::shared_ptr<IONotifyController> ic;
        const std::shared_ptr<UInterface> ui;
        CORBA::Object_var oref;
        UniSetTypes::ObjectId shmID;
        UniSetTypes::ObjectId myid;
        UniSetTypes::uniset_rwmutex shmMutex;
};

//--------------------------------------------------------------------------
#endif
