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
#ifndef JSEngine_H_
// --------------------------------------------------------------------------
#include <unordered_map>
extern "C" {
#include "quickjs/quickjs.h"
}
#include "UInterface.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    class JSEngine
    {
        public:
            explicit JSEngine( const std::string& jsfile,
                               std::vector<std::string>& searchPaths,
                               std::shared_ptr<UInterface>& ui,
                               int jsLoopCount = 5,
                               bool esmModuleMode = false);
            virtual ~JSEngine();

            inline std::shared_ptr<DebugStream> log() noexcept
            {
                return mylog;
            }

            inline std::shared_ptr<DebugStream> js_log() noexcept
            {
                return jslog;
            }

            void init();
            bool isActive();

            void start();
            void stop();
            void askSensors( UniversalIO::UIOCommand cmd );
            void sensorInfo( const uniset::SensorMessage* sm );
            void updateOutputs();
            void step();

        protected:
            void initJS();
            void freeJS();
            void initGlobal( JSContext* ctx );
            void exportAllFunctionsFromTimerModule();
            void createUInterfaceObject();
            void jsLoop();

        private:
            std::atomic_bool activated = { false };
            std::shared_ptr<DebugStream> mylog;
            std::string jsfile;
            std::vector<std::string> searchPaths;
            std::shared_ptr<UInterface> ui;
            int jsLoopCount = { 5 };
            bool esmModuleMode;
            JSRuntime* rt = { nullptr };
            JSContext* ctx = { nullptr };
            uint8_t* jsbuf = { nullptr };
            std::shared_ptr<DebugStream> jslog = { nullptr };

            struct jsSensor
            {
                uniset::ObjectId id;
                std::string name;
                bool set( JSContext* ctx, JSValue& global, int64_t v );
            };

            std::unordered_map<uniset::ObjectId, jsSensor> inputs;
            std::unordered_map<uniset::ObjectId, jsSensor> outputs;

            JSValue jsFnStep = { JS_UNDEFINED };
            JSValue jsFnStart = { JS_UNDEFINED };
            JSValue jsFnStop = { JS_UNDEFINED };
            JSValue jsFnTimers = { JS_UNDEFINED };
            JSValue jsFnOnSensor = { JS_UNDEFINED };
            JSValue jsGlobal = { JS_UNDEFINED };
            JSValue jsModule = { JS_UNDEFINED };

            JSValue js_ui_getValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_ui_askSensor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_ui_setValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_log(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_log_level(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

            // Статические обертки для вызова нестатических методов
            static JSValue jsUiGetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUiAskSensor_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUiSetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsLog_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsLogLevel_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

            // "синтаксический сахар" для логов
#ifndef myinfo
#define myinfo if( log()->debugging(Debug::INFO) ) log()->info()
#endif
#ifndef mywarn
#define mywarn if( log()->debugging(Debug::WARN) ) log()->warn()
#endif
#ifndef mycrit
#define mycrit if( log()->debugging(Debug::CRIT) ) log()->crit()
#endif
#ifndef mylog1
#define mylog1 if( log()->debugging(Debug::LEVEL1) ) log()->level1()
#endif
#ifndef mylog2
#define mylog2 if( log()->debugging(Debug::LEVEL2) ) log()->level2()
#endif
#ifndef mylog3
#define mylog3 if( log()->debugging(Debug::LEVEL3) ) log()->level3()
#endif
#ifndef mylog4
#define mylog4 if( log()->debugging(Debug::LEVEL4) ) log()->level4()
#endif
#ifndef mylog5
#define mylog5 if( log()->debugging(Debug::LEVEL5) ) log()->level5()
#endif
#ifndef mylog6
#define mylog6 if( log()->debugging(Debug::LEVEL6) ) log()->level6()
#endif
#ifndef mylog7
#define mylog7 if( log()->debugging(Debug::LEVEL7) ) log()->level7()
#endif
#ifndef mylog8
#define mylog8 if( log()->debugging(Debug::LEVEL8) ) log()->level8()
#endif
#ifndef mylog9
#define mylog9 if( log()->debugging(Debug::LEVEL9) ) log()->level9()
#endif
#ifndef mylogany
#define mylogany log()->any()
#endif
    };
    // ----------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif
