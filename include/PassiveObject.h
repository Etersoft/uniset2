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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------
#ifndef PassiveObject_H_
#define PassiveObject_H_
// -------------------------------------------------------------------------

#include <string>
#include "UniSetTypes.h"
#include "MessageType.h"
#include "ProxyManager.h"
// -------------------------------------------------------------------------
namespace uniset
{

    /*!
     * Пассивный объект, не имеющий самостоятельного потока обработки сообщений, но имеющий
     * уникальный идентификатор. Предназначен для работы под управлением ProxyManager.
     *
     * \todo Перейти на shared_ptr, weak_ptr для взаимодействия с ProxyManager
     *
     * DEPRECATED
    */
    class PassiveObject
    {
        public:
            PassiveObject();
            PassiveObject( uniset::ObjectId id );
            PassiveObject( uniset::ObjectId id, ProxyManager* mngr );
            virtual ~PassiveObject();

            virtual void processingMessage( const uniset::VoidMessage* msg );

            void setID( uniset::ObjectId id );
            void init(ProxyManager* mngr);

            inline uniset::ObjectId getId() const
            {
                return id;
            }
            inline std::string getName() const
            {
                return myname;
            }

        protected:
            virtual void sysCommand( const uniset::SystemMessage* sm );
            virtual void askSensors( UniversalIO::UIOCommand cmd ) {}
            virtual void timerInfo( const uniset::TimerMessage* tm ) {}
            virtual void sensorInfo( const uniset::SensorMessage* sm ) {}

            std::string myname = { "" };
            ProxyManager* mngr = { nullptr };

        private:
            uniset::ObjectId id = { uniset::DefaultObjectId };
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // PassiveObject_H_
// -------------------------------------------------------------------------
