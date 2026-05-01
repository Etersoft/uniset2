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
// --------------------------------------------------------------------------
#ifndef JSProxy_H_
#define JSProxy_H_
// --------------------------------------------------------------------------
/*! \see \ref pageJScript "JScript - Поддержка скриптов на JS (README.md)"
*/
// --------------------------------------------------------------------------
// Documentation moved to extensions/JScript/README.md
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
#include "JSProxy_SK.h"
#include "JSEngine.h"
#include "USingleProcess.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    class JSProxy final:
        private USingleProcess,
        public JSProxy_SK
    {
        public:
            explicit JSProxy( uniset::ObjectId id
                              , xmlNode* confnode = uniset::uniset_conf()->getNode("JSProxy")
                              , const std::string& prefix = "js" );
            ~JSProxy() override;

            static std::shared_ptr<JSProxy> init( int argc, const char* const* argv, const std::string& prefix = "js" );
            static void help_print();

        protected:

            virtual void callback() noexcept override;
            virtual void step() override;
            virtual void sysCommand(const uniset::SystemMessage* sm) override;
            virtual bool deactivateObject() override;
            virtual void askSensors( UniversalIO::UIOCommand cmd) override;
            virtual void sensorInfo(const uniset::SensorMessage* sm) override;
            //            virtual std::string getMonitInfo() const override;

        private:
            std::shared_ptr<JSEngine> js;
    };
    // ----------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif
