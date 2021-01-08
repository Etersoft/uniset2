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
//--------------------------------------------------------------------------------
#ifndef _SMVIEWER_H
#define _SMVIEWER_H
//--------------------------------------------------------------------------------
#include <string>
#include <memory>
#include "SViewer.h"
#include "SMInterface.h"
//--------------------------------------------------------------------------------
class SMViewer:
    public uniset::SViewer
{
    public:
        SMViewer( uniset::ObjectId shmID );
        virtual ~SMViewer();

        void run();

    protected:

        std::shared_ptr<uniset::SMInterface> shm;
    private:
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
