/*
 * Copyright (c) 2025 Pavel Vainerman.
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
//---------------------------------------------------------------------------
#ifndef USingleProcess_H_
#define USingleProcess_H_
//--------------------------------------------------------------------------
#include <memory>
#include "Configuration.h"
#include "RunLock.h"
#include "UniXML.h"
//---------------------------------------------------------------------------
namespace uniset
{
    class USingleProcess
    {
        public:
            USingleProcess( xmlNode* cnode, int argc, const char* const argv[], const std::string& prefix = "" );
            virtual ~USingleProcess();

        protected:
            std::shared_ptr<RunLock> rlock = nullptr;

    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
