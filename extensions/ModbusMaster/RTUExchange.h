/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -----------------------------------------------------------------------------
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
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    class RTUExchange:
        public MBExchange
    {
        public:
            RTUExchange( uniset::ObjectId objId, uniset::ObjectId shmID,
                         const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "rs" );
            virtual ~RTUExchange();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<RTUExchange> init_rtuexchange( int argc, const char* const* argv,
                    uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "rs" );

            static void help_print( int argc, const char* const* argv );

        protected:
            std::shared_ptr<ModbusRTUMaster> mbrtu;
            std::mutex mbMutex;
            std::string devname;
            ComPort::Speed defSpeed;
            bool use485F;
            bool transmitCtl;

            virtual void step() override;
            virtual bool poll() override;
			virtual std::shared_ptr<ModbusClient> initMB( bool reopen = false ) override;

        private:
            RTUExchange();

            bool rs_pre_clean;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _RS_EXCHANGE_H_
// -----------------------------------------------------------------------------
