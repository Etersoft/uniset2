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
#ifndef IOConfig_H_
#define IOConfig_H_
// ------------------------------------------------------------------------------------------
#include "IOController.h"
#include "IONotifyController.h"
// ------------------------------------------------------------------------------------------
namespace uniset
{
    /*! Интерфейс конфигурирования IOController-ов */
    class IOConfig
    {
        public:

            IOConfig() {}
            virtual ~IOConfig() {}

            /*! считать список io */
            virtual IOController::IOStateList read() = 0;

            //          /*! записать текущий список io */
            //          virtual bool write( const IOController::IOStateList& iolist ) = 0;
    };
    // --------------------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------------------
#endif
