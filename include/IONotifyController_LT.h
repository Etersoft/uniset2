/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 * \author Pavel Vainerman
 */
//---------------------------------------------------------------------------
#ifndef IONotifyController_LT_H_
#define IONotifyController_LT_H_
//--------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "IONotifyController.h"
#include "LT_Object.h"

//---------------------------------------------------------------------------
/*!
    Реализация базового класса с использованием локальных таймеров
*/ 
class IONotifyController_LT:
    public IONotifyController
{
    public:
        IONotifyController_LT( UniSetTypes::ObjectId id );
        IONotifyController_LT();
        virtual ~IONotifyController_LT();

    protected:

        /*! заказ локального таймера
            \param timerid - идентификатор таймера
            \param timeMS - период. 0 - означает отказ от таймера
            \param ticks - количество уведомлений. "-1"- постоянно
            \return Возвращает время [мсек] оставшееся до срабатывания
            очередного таймера
        */
        void askTimer( UniSetTypes::TimerId timerid, timeout_t timeMS, short ticks=-1,
                        UniSetTypes::Message::Priority p=UniSetTypes::Message::High );

        /*! функция вызываемая из потока */
        virtual void callback();

        timeout_t sleepTime;
        LT_Object lt;
    private:

};
//--------------------------------------------------------------------------
#endif
