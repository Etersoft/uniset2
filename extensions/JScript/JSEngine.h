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
#include "JHttpServer.h"
#include "JSModbusClient.h"
#ifdef JS_OPCUA_ENABLED
#include "JSOPCUAClient.h"
#endif
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    struct JSOptions
    {
        size_t jsLoopCount = { 5 };
        size_t httpLoopCount = { 5 };
        size_t httpMaxQueueSize = { 100 };
        size_t httpMaxThreads = { 3 };
        size_t httpMaxRequestQueue = { 50 };
        std::chrono::milliseconds httpResponseTimeout = { std::chrono::milliseconds(5000) };
        std::chrono::milliseconds httpQueueWaitTimeout = {std::chrono::milliseconds(5000) };
        bool esmModuleMode = { false };
    };
    // ----------------------------------------------------------------------
    class JSEngine
    {
        public:
            explicit JSEngine( const std::string& jsfile,
                               std::vector<std::string>& searchPaths,
                               std::shared_ptr<UInterface>& ui,
                               JSOptions& opts );
            virtual ~JSEngine();

            inline std::shared_ptr<DebugStream> log() noexcept
            {
                return mylog;
            }

            inline std::shared_ptr<DebugStream> js_log() noexcept
            {
                return jslog;
            }

            inline std::shared_ptr<DebugStream> http_log() noexcept
            {
                return httpserv->log();
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
            void createUnisetObject();
            void createResponsePrototype( JSContext* ctx );
            void createRequestAtoms(JSContext* ctx);
            void createRequestPrototype(JSContext* ctx);
            void jsLoop();
            void preStop();

        private:
            JSValue jsReqProto_ = { JS_UNDEFINED };
            JSValue jsResProto_ = { JS_UNDEFINED };

            bool reqAtomsInited_ = false;
            struct JSReqAtom
            {
                JSAtom method, uri, version, url, path, query, headers, body;
            } reqAtoms_{};

            std::atomic_bool activated = { false };
            std::shared_ptr<DebugStream> mylog;
            std::string jsfile;
            std::vector<std::string> searchPaths;
            std::shared_ptr<UInterface> ui;
            JSRuntime* rt = { nullptr };
            JSContext* ctx = { nullptr };
            uint8_t* jsbuf = { nullptr };
            std::shared_ptr<DebugStream> jslog = { nullptr };
            std::shared_ptr<uniset::JHttpServer> httpserv = { nullptr };
            JSOptions opts;
            std::shared_ptr<uniset::ObjectIndex> oind;

            struct jsSensor
            {
                uniset::ObjectId id;
                std::string name;
                bool set( JSContext* ctx, JSValue& global, int64_t v );
            };

            std::unordered_map<uniset::ObjectId, jsSensor> inputs;
            std::unordered_map<uniset::ObjectId, jsSensor> outputs;
            std::list<JSValue> stepFunctions;
            std::list<JSValue> stopFunctions;

            JSValue jsFnStep = { JS_UNDEFINED };
            JSValue jsFnStart = { JS_UNDEFINED };
            JSValue jsFnStop = { JS_UNDEFINED };
            JSValue jsFnTimers = { JS_UNDEFINED };
            JSValue jsFnOnSensor = { JS_UNDEFINED };
            JSValue jsGlobal = { JS_UNDEFINED };
            JSValue jsModule = { JS_UNDEFINED };
            JSValue jsFnHttpRequest = { JS_UNDEFINED };
            JHttpServer::HandlerFn httpHandleFn;
            std::shared_ptr<JSModbusClient> modbusClient;

            JSValue js_ui_getValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_ui_askSensor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_ui_setValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_uniset_StepCb(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_uniset_StopCb(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_uniset_httpStart(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_log(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_log_level(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_connectTCP(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_disconnect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_read01(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_read02(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_read03(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_read04(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_write05(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_write06(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_write0F(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_write10(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_diag08(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_modbus_read4314(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
#ifdef JS_OPCUA_ENABLED
            std::shared_ptr<JSOPCUAClient> opcuaClient;
            JSValue js_opcua_connect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_opcua_disconnect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_opcua_read(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            JSValue js_opcua_write(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
#endif
            // Статические обертки для вызова нестатических методов
            static JSValue jsUiGetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUiAskSensor_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUiSetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsLog_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsLogLevel_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUniSetStepCb_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUniSetStopCb_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsUniSetHttpStart_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusConnectTCP_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusDisconnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusRead01_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusRead02_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusRead03_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusRead04_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusWrite05_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusWrite06_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusWrite0F_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusWrite10_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusDiag08_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsModbusRead4314_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
#ifdef JS_OPCUA_ENABLED
            static JSValue jsOpcuaConnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsOpcuaDisconnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsOpcuaRead_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
            static JSValue jsOpcuaWrite_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
#endif
            // http convert
            static JSValue jsMakeRequest(JSContext* ctx, JSValueConst& jsReqProto_, JSReqAtom& atom, const JHttpServer::RequestSnapshot& r);
            static JSValue jsMakeResponse(JSContext* ctx, JSValueConst& jsResProto_, JHttpServer::ResponseAdapter* ad);
            static void jsApplyResponseObject(JSContext* ctx, JSValue ret, JHttpServer::ResponseSnapshot& out);
            static void jsApplyResponseAdapter( const JHttpServer::ResponseAdapter& ad, JHttpServer::ResponseSnapshot& out );
            JSValue jsMakeBitsReply( const ModbusRTU::BitsBuffer& buf );
            JSValue jsMakeRegisterReply( const ModbusRTU::ReadOutputRetMessage& msg );
            JSValue jsMakeRegisterReply( const ModbusRTU::ReadInputRetMessage& msg );
            JSValue jsMakeDiagReply( const ModbusRTU::DiagnosticRetMessage& msg );
            JSValue jsMakeWriteAck( ModbusRTU::ModbusData start, ModbusRTU::ModbusData count );
            JSValue jsMakeWriteSingleAck( const ModbusRTU::WriteSingleOutputRetMessage& msg );
            JSValue jsMakeModbusBoolAck( const ModbusRTU::ForceSingleCoilRetMessage& msg );
            JSValue jsMake4314Reply( const ModbusRTU::MEIMessageRetRDI& msg );

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
