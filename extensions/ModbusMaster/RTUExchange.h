#ifndef _RTUEXCHANGE_H_
#define _RTUEXCHANGE_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <map>
#include <vector>
#include "MBExchange.h"
#include "modbus/ModbusRTUMaster.h"
#include "RTUStorage.h"
// -----------------------------------------------------------------------------
class RTUExchange:
    public MBExchange
{
    public:
        RTUExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmID,
                      const std::shared_ptr<SharedMemory> ic=nullptr, const std::string& prefix="rs" );
        virtual ~RTUExchange();

        /*! глобальная функция для инициализации объекта */
        static std::shared_ptr<RTUExchange> init_rtuexchange( int argc, const char* const* argv,
                                            UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory> ic=nullptr,
                                            const std::string& prefix="rs" );

        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char* const* argv );

    protected:
        std::shared_ptr<ModbusRTUMaster> mbrtu;
        UniSetTypes::uniset_mutex mbMutex;
        std::string devname;
        ComPort::Speed defSpeed;
        bool use485F;
        bool transmitCtl;

        virtual void step() override;
        virtual bool poll() override;

        virtual std::shared_ptr<ModbusClient> initMB( bool reopen=false ) override;
        virtual bool initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it ) override;

    private:
        RTUExchange();

        bool rs_pre_clean;
};
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
