/*
 * Copyright (c) 2025 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify,
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
// -------------------------------------------------------------------------
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>
extern "C" {
#include "quickjs/quickjs-libc.h"
}
#include "Configuration.h"
#include "Exceptions.h"
#include "JSHelpers.h"
#include "JSEngine.h"
#include <cstdio>
#include <cstring>
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset;

namespace
{
    inline JSEngine* get_engine_from_context(JSContext* ctx)
    {
        JSValue global = JS_GetGlobalObject(ctx);
        JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
        JS_FreeValue(ctx, global);

        if( JS_IsNumber(engine_ptr_val) )
        {
            int64_t ptr_int;
            JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
            JS_FreeValue(ctx, engine_ptr_val);
            return reinterpret_cast<JSEngine*>(ptr_int);
        }

        JS_FreeValue(ctx, engine_ptr_val);
        return nullptr;
    }
}
// -------------------------------------------------------------------------
JSEngine::JSEngine( const std::string& _jsfile,
                    std::vector<std::string>& _searchPaths,
                    std::shared_ptr<UInterface>& _ui,
                    JSOptions& _opts ):
    jsfile(_jsfile),
    searchPaths(_searchPaths),
    ui(_ui),
    opts(_opts)
{
    if( jsfile.empty() )
        throw SystemError("JS file undefined");

    oind = ui->getObjectIndex();
    jslog = make_shared<DebugStream>();
    jslog->setLogName("JSLog");

    mylog = make_shared<DebugStream>();
    mylog->setLogName("JSEngine");

    modbusClient = std::make_shared<JSModbusClient>();
    modbusClient->setLog(mylog);
#ifdef JS_OPCUA_ENABLED
    opcuaClient = std::make_shared<JSOPCUAClient>();
    opcuaClient->setLog(mylog);
#endif

    httpHandleFn = [this](const JHttpServer::RequestSnapshot & request)->JHttpServer::ResponseSnapshot
    {
        if( JS_IsUndefined(jsFnHttpRequest) )
        {
            JHttpServer::ResponseSnapshot resp;
            resp.headers = { {"Content-Type", "application/json"} };
            resp.status = Poco::Net::HTTPResponse::HTTP_FOUND;
            return resp;
        }

        JHttpServer::ResponseAdapter adapter;
        JSValue req = jsMakeRequest(ctx, jsReqProto_, reqAtoms_, request);
        JSValue res = jsMakeResponse(ctx, jsResProto_, &adapter);

        JSValueConst argv[2] = { req, res };
        JSValue ret = JS_Call(ctx, jsFnHttpRequest, JS_UNDEFINED, 2, argv);

        JHttpServer::ResponseSnapshot finalResp;

        if (!adapter.finished && JS_IsObject(ret))
            jsApplyResponseObject(ctx, ret, finalResp);
        else
            jsApplyResponseAdapter(adapter, finalResp);

        JS_FreeValue(ctx, req);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, ret);

        return finalResp;
    };

    httpserv = std::make_shared<JHttpServer>(opts.httpMaxThreads, opts.httpMaxRequestQueue);
    httpserv->setMaxQueueSize(opts.httpMaxQueueSize);
    httpserv->setProcessTimeout(opts.httpResponseTimeout);
}
// -------------------------------------------------------------------------
void JSEngine::initGlobal( JSContext* ctx )
{
    JSValue __g = JS_GetGlobalObject(ctx);
    JSValue search_paths_array = JS_NewArray(ctx);

    for( size_t i = 0; i < searchPaths.size(); ++i )
    {
        JSValue path_str = JS_NewString(ctx, searchPaths[i].c_str());
        JS_SetPropertyUint32(ctx, search_paths_array, i, path_str);
    }

    JS_SetPropertyStr(ctx, __g, jshelper::js_search_paths_object.c_str(), search_paths_array);
    JS_SetPropertyStr(ctx, __g, "load", JS_NewCFunction(ctx, jshelper::js_load_file_with_data, "load", 1));

    JSValue log_func = JS_NewCFunction(ctx, jsLog_wrapper, "uniset_internal_log", 2);
    JS_SetPropertyStr(ctx, __g, "uniset_internal_log", log_func);

    JSValue log_level_func = JS_NewCFunction(ctx, jsLogLevel_wrapper, "uniset_internal_log_level", 1);
    JS_SetPropertyStr(ctx, __g, "uniset_internal_log_level", log_level_func);

    JSValue engine_ptr = JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(this)));
    JS_SetPropertyStr(ctx, __g, "__js_engine_instance", engine_ptr);

    JS_FreeValue(ctx, __g);
}
// -------------------------------------------------------------------------
void JSEngine::initJS()
{
    auto conf = uniset_conf();
    rt = JS_NewRuntime();

    if( !rt )
        throw SystemError("init JS Runtime error");

    ctx = JS_NewContext(rt);

    if( !ctx )
        throw SystemError("init JS context error");

    js_std_init_handlers(rt);
    js_std_add_helpers(ctx, 0, NULL);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    auto private_data = new jshelper::JSPrivateData{searchPaths};

    if( opts.esmModuleMode )
        JS_SetModuleLoaderFunc(rt, jshelper::qjs_module_normalize, jshelper::qjs_module_loader, private_data);
    else
        JS_SetModuleLoaderFunc(rt, NULL, jshelper::module_loader_with_path, private_data);

    jsGlobal = JS_GetGlobalObject(ctx);

    if( JS_IsUndefined(jsGlobal) )
    {
        freeJS();
        throw SystemError("Undefined JS global");
    }

    initGlobal(ctx);
    createUnisetObject();
    createUInterfaceObject();
    createResponsePrototype(ctx);
    createRequestAtoms(ctx);
    createRequestPrototype(ctx);

    // load main script
    size_t jsbuf_len;
    jsbuf = js_load_file(ctx, &jsbuf_len, jsfile.c_str());

    if( !jsbuf )
    {
        mycrit << "Failed to load file: " << jsfile << endl;
        freeJS();
        throw SystemError("Failed to load jsfile");
    }

    myinfo << "(init): load js " << jsfile << " OK" << endl;

    if( opts.esmModuleMode )
        jsModule = JS_Eval(ctx, (char*) jsbuf, jsbuf_len, jsfile.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    else
        jsModule = JS_Eval(ctx, (char*) jsbuf, jsbuf_len, jsfile.c_str(), JS_EVAL_TYPE_GLOBAL);

    if( JS_IsException(jsModule) )
    {
        mycrit << "Failed to eval file: " << jsfile << endl;
        jshelper::dump_exception_details(ctx);
        freeJS();
        throw SystemError("Failed to eval jsfile");
    }

    if( opts.esmModuleMode )
    {
        JSValue ret = JS_EvalFunction(ctx, jsModule);

        if( JS_IsException(ret) )
        {
            mycrit << "(init): failed to eval module function" << endl;
            jshelper::dump_exception_details(ctx);
            freeJS();
            throw SystemError("Failed to eval module function");
        }

        JS_FreeValue(ctx, ret);
    }

    myinfo << "(init): parse js " << jsfile << " OK" << endl;

    // экспортируем таймеры если загружены
    exportAllFunctionsFromTimerModule();

    // ДАЛЕЕ инициализация INPUTS/OUTPUTS
    JSValue js_inputs = JS_GetPropertyStr(ctx, jsGlobal, "uniset_inputs");

    if( JS_IsUndefined(js_inputs) || JS_IsNull(js_inputs) )
    {
        mycrit << "(init): Undefined JS uniset_inputs" << endl;
        JS_FreeValue(ctx, js_inputs);
        freeJS();
        throw SystemError("Undefined JS uniset_inputs");
    }

    std::vector<jshelper::IOProp> inp = jshelper::js_array_to_vector<jshelper::IOProp>(ctx, js_inputs,
                                        jshelper::convert_js_to_io_prop);

    JS_FreeValue(ctx, js_inputs);

    if( inp.empty() )
    {
        mycrit << "(init): Can`t parse uniset_inputs" << endl;
        freeJS();
        throw SystemError("Can`t parse uniset_inputs");
    }

    for( const auto& p : inp )
    {
        jsSensor jsIO;
        jsIO.id = conf->getSensorID(p.sensor);
        jsIO.name = "in_" + p.name;

        if( jsIO.id == uniset::DefaultObjectId )
        {
            mycrit << "(init): " << "Not found ID for input " << p.sensor << endl;
            freeJS();
            throw SystemError("Not found ID for input " + p.sensor);
        }

        // set in_VariableName
        int ret = JS_SetPropertyStr(ctx, jsGlobal, jsIO.name.c_str(), JS_NewInt64(ctx, 0));

        if( ret == -1 )
        {
            mycrit << "(init): " << "Init input '" << p.name << "' error" << endl;
            freeJS();
            throw SystemError("Init input '" + p.name + "' error");
        }

        inputs.emplace(jsIO.id, jsIO);

        // set ID (name)
        ret = JS_SetPropertyStr(ctx, jsGlobal, p.name.c_str(), JS_NewInt64(ctx, jsIO.id));

        if( ret == -1 )
        {
            mycrit << "(init): " << "Init input ID '" << p.name << "' error" << endl;
            freeJS();
            throw SystemError("Init input ID '" + p.name + "' error");
        }
    }

    {
        ostringstream s;

        for( const auto& p : inputs )
            s << " (" << p.first << ")" << p.second.name;

        myinfo << "INPUTS:[" << s.str() << " ]" << endl;
    }

    // OUTPUTS
    JSValue js_outputs = JS_GetPropertyStr(ctx, jsGlobal, "uniset_outputs");

    if( JS_IsUndefined(js_outputs) || JS_IsNull(js_outputs) )
    {
        mycrit << "(init): Undefined JS uniset_outputs" << endl;
        JS_FreeValue(ctx, js_outputs);
        freeJS();
        throw SystemError("Undefined JS uniset_outputs");
    }

    std::vector<jshelper::IOProp> outp = jshelper::js_array_to_vector<jshelper::IOProp>(ctx, js_outputs,
                                         jshelper::convert_js_to_io_prop);

    JS_FreeValue(ctx, js_outputs);

    if( outp.empty() )
    {
        mycrit << "(init): Can`t parse uniset_outputs" << endl;
        freeJS();
        throw SystemError("Can`t parse uniset_outputs");
    }

    for( const auto& p : outp )
    {
        jsSensor jsIO;
        jsIO.id = conf->getSensorID(p.sensor);
        jsIO.name = "out_" + p.name;

        if( jsIO.id == uniset::DefaultObjectId )
        {
            mycrit << "(init): Not found ID for output " << p.name << endl;
            freeJS();
            throw SystemError("Not found ID for output " + p.name);
        }

        int ret = JS_SetPropertyStr(ctx, jsGlobal, jsIO.name.c_str(), JS_NewInt64(ctx, 0));

        if( ret == -1 )
        {
            js_std_dump_error(ctx);
            freeJS();
            throw SystemError("Init output '" + jsIO.name + "' error");
        }

        outputs.emplace(jsIO.id, jsIO);
        ret = JS_SetPropertyStr(ctx, jsGlobal, p.name.c_str(), JS_NewInt64(ctx, jsIO.id));

        if( ret == -1 )
        {
            freeJS();
            throw SystemError("Init output ID '" + p.name + "' error");
        }
    }

    {
        ostringstream s;

        for( const auto& p : outputs )
            s << " (" << p.first << ")" << p.second.name;

        myinfo << "OUTPUTS:[" << s.str() << " ]" << endl;
    }

    jsFnStep = JS_GetPropertyStr(ctx, jsGlobal, "uniset_on_step");

    if( JS_IsUndefined(jsFnStep) || !JS_IsFunction(ctx, jsFnStep) )
    {
        mywarn << "(init): Not found uniset_on_step() function" << endl;
        freeJS();
        throw SystemError("Not found function uniset_on_step()");
    }

    jsFnStart = JS_GetPropertyStr(ctx, jsGlobal, "uniset_on_start");

    if( JS_IsUndefined(jsFnStart) || !JS_IsFunction(ctx, jsFnStart) )
        mywarn << "(init): Not found uniset_on_start() function" << endl;

    jsFnStop = JS_GetPropertyStr(ctx, jsGlobal, "uniset_on_stop");

    if( JS_IsUndefined(jsFnStop) || !JS_IsFunction(ctx, jsFnStop) )
        mywarn << "(init): Not found uniset_on_stop() function" << endl;

    jsFnTimers = JS_GetPropertyStr(ctx, jsGlobal, "uniset_process_timers");

    if( JS_IsUndefined(jsFnTimers) || !JS_IsFunction(ctx, jsFnTimers) )
        mywarn << "(init): Not found uniset_process_timers() function" << endl;

    jsFnOnSensor = JS_GetPropertyStr(ctx, jsGlobal, "uniset_on_sensor");

    if( JS_IsUndefined(jsFnOnSensor) || !JS_IsFunction(ctx, jsFnOnSensor) )
        mywarn << "(init): Not found uniset_on_sensor() function" << endl;

    js_std_loop(ctx);
}
// ----------------------------------------------------------------------------
void JSEngine::preStop()
{
    for( const auto& cb : stopFunctions )
        jshelper::safe_function_call(ctx, jsGlobal, cb, 0, nullptr);

    jsLoop();
}
// ----------------------------------------------------------------------------
void JSEngine::freeJS()
{
    myinfo << "(freeJS): freeing JS resources..." << endl;

    try
    {
        if( activated )
            preStop();
    }
    catch( Poco::Exception& ex )
    {
        mycrit << "(freeJS): preStop error: " << ex.displayText() << endl;
    }

    if( activated )
        stop();

    if( ctx )
    {
        try
        {
            for( auto&& cb : stepFunctions )
                JS_FreeValue(ctx, cb);

            stepFunctions.clear();
        }
        catch(...) {}

        try
        {
            for( auto&& cb : stopFunctions )
                JS_FreeValue(ctx, cb);

            stopFunctions.clear();
        }
        catch(...) {}
    }

    if( rt )
    {
        JSContext* job_ctx;

        while (JS_ExecutePendingJob(rt, &job_ctx)) {}

        JS_RunGC(rt);
    }

    if( ctx )
    {
        // Сначала освободим все JS значения
        if (!JS_IsUndefined(jsFnStep))
            JS_FreeValue(ctx, jsFnStep);

        if (!JS_IsUndefined(jsFnStart))
            JS_FreeValue(ctx, jsFnStart);

        if (!JS_IsUndefined(jsFnStop))
            JS_FreeValue(ctx, jsFnStop);

        if (!JS_IsUndefined(jsFnTimers))
            JS_FreeValue(ctx, jsFnTimers);

        if (!JS_IsUndefined(jsFnOnSensor))
            JS_FreeValue(ctx, jsFnOnSensor);

        if (!JS_IsUndefined(jsFnHttpRequest))
            JS_FreeValue(ctx, jsFnHttpRequest);

        if (!JS_IsUndefined(jsGlobal))
            JS_FreeValue(ctx, jsGlobal);

        if ( !opts.esmModuleMode && !JS_IsUndefined(jsModule))
            JS_FreeValue(ctx, jsModule);

        // Затем освободим буфер
        if (jsbuf)
        {
            js_free(ctx, jsbuf);
            jsbuf = nullptr;
        }

        // В конце контекст и runtime
        JS_FreeContext(ctx);
        ctx = nullptr;
    }

    if (rt)
    {
        js_std_free_handlers(rt);
        JS_FreeRuntime(rt);
        rt = nullptr;
    }

    myinfo << "(freeJS): done" << endl;
}
// ----------------------------------------------------------------------------
JSEngine::~JSEngine()
{
    if (!JS_IsUndefined(jsResProto_))
    {
        JS_FreeValue(ctx, jsResProto_);
        jsResProto_ = JS_UNDEFINED;
    }

    if (reqAtomsInited_)
    {
        JS_FreeAtom(ctx, reqAtoms_.method);
        JS_FreeAtom(ctx, reqAtoms_.uri);
        JS_FreeAtom(ctx, reqAtoms_.version);
        JS_FreeAtom(ctx, reqAtoms_.url);
        JS_FreeAtom(ctx, reqAtoms_.path);
        JS_FreeAtom(ctx, reqAtoms_.query);
        JS_FreeAtom(ctx, reqAtoms_.headers);
        JS_FreeAtom(ctx, reqAtoms_.body);
        reqAtomsInited_ = false;
    }

    if (!JS_IsUndefined(jsReqProto_))
    {
        JS_FreeValue(ctx, jsReqProto_);
        jsReqProto_ = JS_UNDEFINED;
    }

    freeJS();
}
// ----------------------------------------------------------------------------
bool JSEngine::isActive()
{
    return activated;
}
// ----------------------------------------------------------------------------
void JSEngine::init()
{
    if( activated )
        return;

    activated = true;
    initJS();
}
// ----------------------------------------------------------------------------
void JSEngine::start()
{
    if( !JS_IsUndefined(jsFnStart) )
    {
        jshelper::safe_function_call(ctx, jsGlobal, jsFnStart, 0, nullptr);
        jsLoop();
    }
}
// ----------------------------------------------------------------------------
void JSEngine::stop()
{
    if( httpserv->isRunning() )
    {
        try
        {
            httpserv->softStop(std::chrono::seconds(5) );
        }
        catch( Poco::Exception& ex )
        {
            mycrit << "(preStop): http server error: " << ex.displayText() << endl;
        }
    }

    if( !JS_IsUndefined(jsFnStop) )
    {
        jshelper::safe_function_call(ctx, jsGlobal, jsFnStop, 0, nullptr);
        jsLoop();
    }
}
// ----------------------------------------------------------------------------
void JSEngine::askSensors( UniversalIO::UIOCommand cmd )
{
    for( const auto& s : inputs )
    {
        try
        {
            ui->askSensor(s.first, cmd);
        }
        catch( std::exception& ex )
        {
            mycrit << "(askSensors): " << ex.what() << endl;
        }
    }
}
// ----------------------------------------------------------------------------
void JSEngine::sensorInfo( const uniset::SensorMessage* sm )
{
    auto it = inputs.find(sm->id);

    if( it != inputs.end() )
    {
        if( !it->second.set(ctx, jsGlobal, int64_t(sm->value)) )
            mycrit << "(sensorInfo): can't update value for " << it->second.name << endl;
    }

    if( !JS_IsUndefined(jsFnOnSensor) )
    {
        std::string sname;
        auto oinf = oind->getObjectInfo(sm->id);

        if( oinf )
            sname = oinf->name;

        jshelper::JsArgs args(ctx);
        args.i64(sm->id).i64(sm->value).str(sname.c_str());
        jshelper::safe_function_call(ctx, jsGlobal, jsFnOnSensor, args.size(), args.data());
    }

    jsLoop();
}
// ----------------------------------------------------------------------------
void JSEngine::jsLoop()
{
    // js loop processing
    for( size_t i = 0; i < opts.jsLoopCount; i++ )
        js_std_loop(ctx);
}
// ----------------------------------------------------------------------------
void JSEngine::step()
{
    // timers processing
    if( !JS_IsUndefined(jsFnTimers) )
        jshelper::safe_function_call(ctx, jsGlobal, jsFnTimers, 0, nullptr);

    jsLoop();

    for( const auto& cb : stepFunctions )
        jshelper::safe_function_call(ctx, jsGlobal, cb, 0, nullptr);

    jsLoop();

    // step processing
    jshelper::safe_function_call(ctx, jsGlobal, jsFnStep, 0, nullptr);

    jsLoop();

    if( !JS_IsUndefined(jsFnHttpRequest) )
        httpserv->httpLoop(httpHandleFn, opts.httpLoopCount,  opts.httpQueueWaitTimeout );
}
// ----------------------------------------------------------------------------
void JSEngine::updateOutputs()
{
    for( const auto& o : outputs )
    {
        try
        {
            jshelper::JSValueGuard v(ctx, JS_GetPropertyStr(ctx, jsGlobal, o.second.name.c_str()));
            int64_t x;
            JS_ToInt64(ctx, &x, v.get());
            ui->setValue(o.first, x);
        }
        catch( std::exception& ex )
        {
            mycrit << "(updateOutputs): update failed for " << o.second.name << " error: " << ex.what() << endl;
        }
    }
}
// ----------------------------------------------------------------------------
bool JSEngine::jsSensor::set( JSContext* ctx, JSValue& global, int64_t v )
{
    int ret = JS_SetPropertyStr(ctx, global, name.c_str(), JS_NewInt64(ctx, v));
    return ret != -1;
}
// ----------------------------------------------------------------------------
void JSEngine::exportAllFunctionsFromTimerModule()
{
    if( !ctx ) return;

    const std::string moduleFileName = "uniset2-timers.js";
    JSValue moduleVersion = JS_GetPropertyStr(ctx, jsGlobal, "unisetTimersModuleVersion");
    auto isModuleLoaded = !JS_IsUndefined(moduleVersion);

    // не экспортируем функции, если модуль не загружен пользователем
    if( !isModuleLoaded )
    {
        JS_FreeValue(ctx, moduleVersion);
        return;
    }

    if( JS_IsString(moduleVersion) )
    {
        const char* version_str = JS_ToCString(ctx, moduleVersion);

        if( version_str )
        {
            std::string version(version_str); // Конвертируем в std::string
            myinfo << "(exportAllFunctionsFromTimerModule): unisetTimersModule = '" << version << "'" << endl;
            JS_FreeCString(ctx, version_str);
        }
    }

    JS_FreeValue(ctx, moduleVersion);

    // Загружаем и выполняем файл напрямую
    auto utimers_filename = jshelper::find_file(moduleFileName, searchPaths);

    size_t jsbuf_len;
    uint8_t* timer_buf = js_load_file(ctx, &jsbuf_len, utimers_filename.c_str());

    if( !timer_buf )
    {
        mywarn << "(exportAllFunctionsFromTimerModule): " << moduleFileName << " not found" << endl;
        return;
    }

    // Выполняем в глобальной области
    JSValue result = JS_Eval(ctx, (char*)timer_buf, jsbuf_len, moduleFileName.c_str(), JS_EVAL_TYPE_GLOBAL);

    if (JS_IsException(result))
    {
        mywarn << "(exportAllFunctionsFromTimerModule): evaluation failed" << endl;
        jshelper::dump_exception_details(ctx);
        JS_FreeValue(ctx, result);
        js_free(ctx, timer_buf);
        return;
    }

    JS_FreeValue(ctx, result);
    js_free(ctx, timer_buf);

    myinfo << "(exportAllFunctionsFromTimerModule): " << moduleFileName << " executed in global scope" << endl;

    // Проверяем доступность функций
    JSValue global = JS_GetGlobalObject(ctx);

    const char* function_names[] =
    {
        "askTimer", "removeTimer", "pauseTimer", "resumeTimer",
        "getTimerInfo", "clearAllTimers", "getTimerStats", "uniset_process_timers"
    };

    int available_count = 0;

    for (const char* func_name : function_names)
    {
        JSValue func = JS_GetPropertyStr(ctx, global, func_name);
        bool is_function = (!JS_IsUndefined(func) && JS_IsFunction(ctx, func));

        if (is_function)
        {
            available_count++;
            myinfo << "(exportAllFunctionsFromTimerModule): " << func_name << " is available" << endl;
        }
        else
        {
            mywarn << "(exportAllFunctionsFromTimerModule): " << func_name << " is NOT available" << endl;
        }

        JS_FreeValue(ctx, func);
    }

    JS_FreeValue(ctx, global);
    myinfo << "(exportAllFunctionsFromTimerModule): " << available_count << " functions available" << endl;
}
// -------------------------------------------------------------------------
void JSEngine::createUInterfaceObject()
{
    if( !ctx ) return;

    // Создаем объект ui
    JSValue ui_obj = JS_NewObject(ctx);

    // Регистрируем функции с статическими обертками
    JSValue ask_sensor_func = JS_NewCFunction(ctx, jsUiAskSensor_wrapper, "askSensor", 2);
    JS_SetPropertyStr(ctx, ui_obj, "askSensor", ask_sensor_func);

    JSValue get_value_func = JS_NewCFunction(ctx, jsUiGetValue_wrapper, "getValue", 1);
    JS_SetPropertyStr(ctx, ui_obj, "getValue", get_value_func);

    JSValue set_value_func = JS_NewCFunction(ctx, jsUiSetValue_wrapper, "setValue", 2);
    JS_SetPropertyStr(ctx, ui_obj, "setValue", set_value_func);


    // Добавляем объект ui в глобальное пространство
    JS_SetPropertyStr(ctx, jsGlobal, "ui", ui_obj);

    JS_SetPropertyStr(ctx, jsGlobal, "UIONotify", JS_NewInt32(ctx, UniversalIO::UIONotify));
    JS_SetPropertyStr(ctx, jsGlobal, "UIODontNotify", JS_NewInt32(ctx, UniversalIO::UIODontNotify));
    JS_SetPropertyStr(ctx, jsGlobal, "UIONotifyChange", JS_NewInt32(ctx, UniversalIO::UIONotifyChange));
    JS_SetPropertyStr(ctx, jsGlobal, "UIONotifyFirstNotNull", JS_NewInt32(ctx, UniversalIO::UIONotifyFirstNotNull));

    myinfo << "(createUInterfaceObject): ui object created with askSensor, getValue, setValue functions" << endl;
}
// ----------------------------------------------------------------------------
void JSEngine::createUnisetObject()
{
    JSValue uobj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, uobj, "step_cb",   JS_NewCFunction(ctx, jsUniSetStepCb_wrapper,   "step_cb",   1));
    JS_SetPropertyStr(ctx, uobj, "stop_cb",   JS_NewCFunction(ctx, jsUniSetStopCb_wrapper,   "stop_cb",   1));
    JS_SetPropertyStr(ctx, uobj, "http_start",   JS_NewCFunction(ctx, jsUniSetHttpStart_wrapper,   "http_start",   3));

    JSValue modbus_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, modbus_obj, "connectTCP", JS_NewCFunction(ctx, jsModbusConnectTCP_wrapper, "connectTCP", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "disconnect", JS_NewCFunction(ctx, jsModbusDisconnect_wrapper, "disconnect", 0));
    JS_SetPropertyStr(ctx, modbus_obj, "read01", JS_NewCFunction(ctx, jsModbusRead01_wrapper, "read01", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "read02", JS_NewCFunction(ctx, jsModbusRead02_wrapper, "read02", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "read03", JS_NewCFunction(ctx, jsModbusRead03_wrapper, "read03", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "read04", JS_NewCFunction(ctx, jsModbusRead04_wrapper, "read04", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "write05", JS_NewCFunction(ctx, jsModbusWrite05_wrapper, "write05", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "write06", JS_NewCFunction(ctx, jsModbusWrite06_wrapper, "write06", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "write0F", JS_NewCFunction(ctx, jsModbusWrite0F_wrapper, "write0F", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "write10", JS_NewCFunction(ctx, jsModbusWrite10_wrapper, "write10", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "diag08", JS_NewCFunction(ctx, jsModbusDiag08_wrapper, "diag08", 3));
    JS_SetPropertyStr(ctx, modbus_obj, "read4314", JS_NewCFunction(ctx, jsModbusRead4314_wrapper, "read4314", 3));

    JS_SetPropertyStr(ctx, uobj, "modbus", modbus_obj);
#ifdef JS_OPCUA_ENABLED
    JSValue opcua_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, opcua_obj, "connect", JS_NewCFunction(ctx, jsOpcuaConnect_wrapper, "connect", 3));
    JS_SetPropertyStr(ctx, opcua_obj, "disconnect", JS_NewCFunction(ctx, jsOpcuaDisconnect_wrapper, "disconnect", 0));
    JS_SetPropertyStr(ctx, opcua_obj, "read", JS_NewCFunction(ctx, jsOpcuaRead_wrapper, "read", 1));
    JS_SetPropertyStr(ctx, opcua_obj, "write", JS_NewCFunction(ctx, jsOpcuaWrite_wrapper, "write", 2));
    JS_SetPropertyStr(ctx, uobj, "opcua", opcua_obj);
#endif
    JS_SetPropertyStr(ctx, jsGlobal, "uniset", uobj);
}
// ----------------------------------------------------------------------------
// Статические обертки для вызова нестатических методов
JSValue JSEngine::jsUiAskSensor_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // Получаем указатель на экземпляр JSEngine из глобального объекта
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_ui_askSensor(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsUiGetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue result = engine->js_ui_getValue(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return result;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_NewInt64(ctx, 0);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsUiSetValue_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_ui_setValue(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsUniSetStepCb_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_uniset_StepCb(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsUniSetStopCb_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_uniset_StopCb(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsUniSetHttpStart_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_uniset_httpStart(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsLog_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_log(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsLogLevel_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue engine_ptr_val = JS_GetPropertyStr(ctx, global, "__js_engine_instance");
    JS_FreeValue(ctx, global);

    if( JS_IsNumber(engine_ptr_val) )
    {
        int64_t ptr_int;
        JS_ToInt64(ctx, &ptr_int, engine_ptr_val);
        JSEngine* engine = reinterpret_cast<JSEngine*>(ptr_int);

        if( engine )
        {
            JSValue ret = engine->js_log_level(ctx, this_val, argc, argv);
            JS_FreeValue(ctx, engine_ptr_val);
            return ret;
        }
    }

    JS_FreeValue(ctx, engine_ptr_val);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusConnectTCP_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_connectTCP(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusDisconnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_disconnect(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusRead01_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_read01(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusRead02_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_read02(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusRead03_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_read03(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusRead04_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_read04(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusWrite05_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_write05(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusWrite06_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_write06(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusWrite0F_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_write0F(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusWrite10_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_write10(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusDiag08_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_diag08(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsModbusRead4314_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_modbus_read4314(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
#ifdef JS_OPCUA_ENABLED
JSValue JSEngine::jsOpcuaConnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_opcua_connect(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsOpcuaDisconnect_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_opcua_disconnect(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsOpcuaRead_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_opcua_read(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsOpcuaWrite_wrapper(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto engine = get_engine_from_context(ctx);

    if( !engine )
    {
        JS_ThrowInternalError(ctx, "JS engine undefined");
        return JS_EXCEPTION;
    }

    return engine->js_opcua_write(ctx, this_val, argc, argv);
}
// ----------------------------------------------------------------------------
#endif // JS_OPCUA_ENABLED
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_connectTCP(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.connectTCP(host, port [, timeout])");
        return JS_EXCEPTION;
    }

    const char* host = JS_ToCString(jsctx, argv[0]);

    if( !host )
        return JS_EXCEPTION;

    int32_t port = 0;

    if( JS_ToInt32(jsctx, &port, argv[1]) != 0 )
    {
        JS_FreeCString(jsctx, host);
        JS_ThrowTypeError(jsctx, "modbus.connectTCP: invalid port");
        return JS_EXCEPTION;
    }

    timeout_t timeout = 2000;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        int32_t tmp = 0;

        if( JS_ToInt32(jsctx, &tmp, argv[2]) != 0 )
        {
            JS_FreeCString(jsctx, host);
            JS_ThrowTypeError(jsctx, "modbus.connectTCP: invalid timeout");
            return JS_EXCEPTION;
        }

        if( tmp > 0 )
            timeout = tmp;
    }

    bool forceDisconnect = false;

    if( argc >= 4 && !JS_IsUndefined(argv[3]) )
        forceDisconnect = JS_ToBool(jsctx, argv[3]);

    try
    {
        modbusClient->connectTCP(host, port, timeout, forceDisconnect);
    }
    catch( const std::exception& ex )
    {
        JS_FreeCString(jsctx, host);
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }

    JS_FreeCString(jsctx, host);
    return JS_NewBool(jsctx, 1);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_disconnect(JSContext* jsctx, JSValueConst, int, JSValueConst*)
{
    if( modbusClient )
        modbusClient->disconnect();

    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
namespace
{
    inline bool js_to_uint32(JSContext* ctx, JSValueConst value, uint32_t& out, const char* errmsg)
    {
        if( JS_ToUint32(ctx, &out, value) != 0 )
        {
            JS_ThrowTypeError(ctx, "%s", errmsg);
            return false;
        }

        return true;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_read01(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.read01(slave, reg [, count])");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;
    uint32_t count = 1;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.read01: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.read01: invalid register") )
        return JS_EXCEPTION;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        if( !js_to_uint32(jsctx, argv[2], count, "modbus.read01: invalid count") )
            return JS_EXCEPTION;
    }

    try
    {
        auto ret = modbusClient->read01(static_cast<ModbusRTU::ModbusAddr>(slave),
                                        static_cast<ModbusRTU::ModbusData>(reg),
                                        static_cast<ModbusRTU::ModbusData>(count));
        return jsMakeBitsReply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_read02(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.read02(slave, reg [, count])");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;
    uint32_t count = 1;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.read02: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.read02: invalid register") )
        return JS_EXCEPTION;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        if( !js_to_uint32(jsctx, argv[2], count, "modbus.read02: invalid count") )
            return JS_EXCEPTION;
    }

    try
    {
        auto ret = modbusClient->read02(static_cast<ModbusRTU::ModbusAddr>(slave),
                                        static_cast<ModbusRTU::ModbusData>(reg),
                                        static_cast<ModbusRTU::ModbusData>(count));
        return jsMakeBitsReply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_read03(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.read03(slave, reg [, count])");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;
    uint32_t count = 1;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.read03: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.read03: invalid register") )
        return JS_EXCEPTION;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        if( !js_to_uint32(jsctx, argv[2], count, "modbus.read03: invalid count") )
            return JS_EXCEPTION;
    }

    try
    {
        auto ret = modbusClient->read03(static_cast<ModbusRTU::ModbusAddr>(slave),
                                        static_cast<ModbusRTU::ModbusData>(reg),
                                        static_cast<ModbusRTU::ModbusData>(count));
        return jsMakeRegisterReply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_read04(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.read04(slave, reg [, count])");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;
    uint32_t count = 1;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.read04: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.read04: invalid register") )
        return JS_EXCEPTION;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        if( !js_to_uint32(jsctx, argv[2], count, "modbus.read04: invalid count") )
            return JS_EXCEPTION;
    }

    try
    {
        auto ret = modbusClient->read04(static_cast<ModbusRTU::ModbusAddr>(slave),
                                        static_cast<ModbusRTU::ModbusData>(reg),
                                        static_cast<ModbusRTU::ModbusData>(count));
        return jsMakeRegisterReply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_write05(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 3 )
    {
        JS_ThrowTypeError(jsctx, "modbus.write05(slave, reg, value)");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.write05: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.write05: invalid register") )
        return JS_EXCEPTION;

    int32_t state = JS_ToBool(jsctx, argv[2]);

    if( state < 0 )
        return JS_EXCEPTION;

    try
    {
        auto ret = modbusClient->write05(static_cast<ModbusRTU::ModbusAddr>(slave),
                                         static_cast<ModbusRTU::ModbusData>(reg),
                                         state != 0);
        return jsMakeModbusBoolAck(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_write06(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 3 )
    {
        JS_ThrowTypeError(jsctx, "modbus.write06(slave, reg, value)");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t reg = 0;
    uint32_t value = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.write06: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], reg, "modbus.write06: invalid register") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[2], value, "modbus.write06: invalid value") )
        return JS_EXCEPTION;

    try
    {
        auto ret = modbusClient->write06(static_cast<ModbusRTU::ModbusAddr>(slave),
                                         static_cast<ModbusRTU::ModbusData>(reg),
                                         static_cast<ModbusRTU::ModbusData>(value));
        return jsMakeWriteSingleAck(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_write0F(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 3 )
    {
        JS_ThrowTypeError(jsctx, "modbus.write0F(slave, start, values)");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t start = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.write0F: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], start, "modbus.write0F: invalid start register") )
        return JS_EXCEPTION;

    if( !JS_IsArray(jsctx, argv[2]) )
    {
        JS_ThrowTypeError(jsctx, "modbus.write0F: values must be array");
        return JS_EXCEPTION;
    }

    JSValue lenVal = JS_GetPropertyStr(jsctx, argv[2], "length");
    uint32_t length = 0;

    if( JS_ToUint32(jsctx, &length, lenVal) != 0 )
    {
        JS_FreeValue(jsctx, lenVal);
        JS_ThrowTypeError(jsctx, "modbus.write0F: invalid values length");
        return JS_EXCEPTION;
    }

    JS_FreeValue(jsctx, lenVal);

    vector<uint8_t> bits;
    bits.reserve(length);

    for( uint32_t i = 0; i < length; ++i )
    {
        JSValue item = JS_GetPropertyUint32(jsctx, argv[2], i);

        if( JS_IsException(item) )
            return JS_EXCEPTION;

        int32_t val = JS_ToBool(jsctx, item);
        JS_FreeValue(jsctx, item);

        if( val < 0 )
            return JS_EXCEPTION;

        bits.emplace_back(val ? 1 : 0);
    }

    try
    {
        auto ret = modbusClient->write0F(static_cast<ModbusRTU::ModbusAddr>(slave),
                                         static_cast<ModbusRTU::ModbusData>(start),
                                         bits);
        return jsMakeWriteAck(ret.start, ret.quant);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_write10(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 3 )
    {
        JS_ThrowTypeError(jsctx, "modbus.write10(slave, start, values)");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t start = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.write10: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], start, "modbus.write10: invalid start register") )
        return JS_EXCEPTION;

    if( !JS_IsArray(jsctx, argv[2]) )
    {
        JS_ThrowTypeError(jsctx, "modbus.write10: values must be array");
        return JS_EXCEPTION;
    }

    JSValue lenVal = JS_GetPropertyStr(jsctx, argv[2], "length");
    uint32_t length = 0;

    if( JS_ToUint32(jsctx, &length, lenVal) != 0 )
    {
        JS_FreeValue(jsctx, lenVal);
        JS_ThrowTypeError(jsctx, "modbus.write10: invalid values length");
        return JS_EXCEPTION;
    }

    JS_FreeValue(jsctx, lenVal);

    vector<ModbusRTU::ModbusData> values;
    values.reserve(length);

    for( uint32_t i = 0; i < length; ++i )
    {
        JSValue item = JS_GetPropertyUint32(jsctx, argv[2], i);

        if( JS_IsException(item) )
            return JS_EXCEPTION;

        uint32_t val = 0;

        if( JS_ToUint32(jsctx, &val, item) != 0 )
        {
            JS_FreeValue(jsctx, item);
            JS_ThrowTypeError(jsctx, "modbus.write10: invalid data value");
            return JS_EXCEPTION;
        }

        JS_FreeValue(jsctx, item);
        values.emplace_back(static_cast<ModbusRTU::ModbusData>(val));
    }

    try
    {
        auto ret = modbusClient->write10(static_cast<ModbusRTU::ModbusAddr>(slave),
                                         static_cast<ModbusRTU::ModbusData>(start),
                                         values);
        return jsMakeWriteAck(ret.start, ret.quant);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_diag08(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 2 )
    {
        JS_ThrowTypeError(jsctx, "modbus.diag08(slave, subfunc [, data])");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t subf = 0;
    uint32_t data = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.diag08: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], subf, "modbus.diag08: invalid subfunction") )
        return JS_EXCEPTION;

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        if( !js_to_uint32(jsctx, argv[2], data, "modbus.diag08: invalid data") )
            return JS_EXCEPTION;
    }

    try
    {
        auto ret = modbusClient->diag08(static_cast<ModbusRTU::ModbusAddr>(slave),
                                        static_cast<ModbusRTU::DiagnosticsSubFunction>(subf),
                                        static_cast<ModbusRTU::ModbusData>(data));
        return jsMakeDiagReply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_modbus_read4314(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 3 )
    {
        JS_ThrowTypeError(jsctx, "modbus.read4314(slave, devID, objID)");
        return JS_EXCEPTION;
    }

    uint32_t slave = 0;
    uint32_t devID = 0;
    uint32_t objID = 0;

    if( !js_to_uint32(jsctx, argv[0], slave, "modbus.read4314: invalid slave id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[1], devID, "modbus.read4314: invalid device id") )
        return JS_EXCEPTION;

    if( !js_to_uint32(jsctx, argv[2], objID, "modbus.read4314: invalid object id") )
        return JS_EXCEPTION;

    try
    {
        auto ret = modbusClient->read4314(static_cast<ModbusRTU::ModbusAddr>(slave),
                                          static_cast<ModbusRTU::ModbusByte>(devID),
                                          static_cast<ModbusRTU::ModbusByte>(objID));
        return jsMake4314Reply(ret);
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
#ifdef JS_OPCUA_ENABLED
JSValue JSEngine::js_opcua_connect(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 1 )
    {
        JS_ThrowTypeError(jsctx, "opcua.connect(endpoint [, user, pass])");
        return JS_EXCEPTION;
    }

    if( !opcuaClient )
    {
        JS_ThrowInternalError(jsctx, "opcua client not initialized");
        return JS_EXCEPTION;
    }

    if( !JS_IsString(argv[0]) )
    {
        JS_ThrowTypeError(jsctx, "opcua.connect: endpoint must be string");
        return JS_EXCEPTION;
    }

    const char* endpoint = JS_ToCString(jsctx, argv[0]);

    if( !endpoint )
        return JS_EXCEPTION;

    std::string user;
    std::string pass;
    bool useAuth = false;

    if( argc >= 2 && !JS_IsUndefined(argv[1]) )
    {
        const char* ustr = JS_ToCString(jsctx, argv[1]);

        if( !ustr )
        {
            JS_FreeCString(jsctx, endpoint);
            return JS_EXCEPTION;
        }

        user = ustr;
        JS_FreeCString(jsctx, ustr);
        useAuth = true;
    }

    if( argc >= 3 && !JS_IsUndefined(argv[2]) )
    {
        const char* pstr = JS_ToCString(jsctx, argv[2]);

        if( !pstr )
        {
            JS_FreeCString(jsctx, endpoint);
            return JS_EXCEPTION;
        }

        pass = pstr;
        JS_FreeCString(jsctx, pstr);
        useAuth = true;
    }

    try
    {
        if( useAuth )
            opcuaClient->connect(endpoint, user, pass);
        else
            opcuaClient->connect(endpoint);
    }
    catch( const std::exception& ex )
    {
        JS_FreeCString(jsctx, endpoint);
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }

    JS_FreeCString(jsctx, endpoint);
    return JS_NewBool(jsctx, 1);
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_opcua_disconnect(JSContext* jsctx, JSValueConst, int, JSValueConst*)
{
    if( opcuaClient )
        opcuaClient->disconnect();

    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_opcua_read(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 1 )
    {
        JS_ThrowTypeError(jsctx, "opcua.read(nodeId or array)");
        return JS_EXCEPTION;
    }

    if( !opcuaClient )
    {
        JS_ThrowInternalError(jsctx, "opcua client not initialized");
        return JS_EXCEPTION;
    }

    std::vector<std::string> nodes;

    auto addNode = [&](JSValueConst val) -> bool
    {
        if( !JS_IsString(val) )
        {
            JS_ThrowTypeError(jsctx, "opcua.read: nodeId must be string");
            return false;
        }

        const char* nid = JS_ToCString(jsctx, val);

        if( !nid )
            return false;

        nodes.emplace_back(nid);
        JS_FreeCString(jsctx, nid);
        return true;
    };

    if( JS_IsArray(jsctx, argv[0]) )
    {
        JSValue lenVal = JS_GetPropertyStr(jsctx, argv[0], "length");
        uint32_t length = 0;

        if( JS_ToUint32(jsctx, &length, lenVal) != 0 )
        {
            JS_FreeValue(jsctx, lenVal);
            JS_ThrowTypeError(jsctx, "opcua.read: invalid array length");
            return JS_EXCEPTION;
        }

        JS_FreeValue(jsctx, lenVal);

        for( uint32_t i = 0; i < length; ++i )
        {
            JSValue item = JS_GetPropertyUint32(jsctx, argv[0], i);

            if( JS_IsException(item) )
                return JS_EXCEPTION;

            bool ok = addNode(item);
            JS_FreeValue(jsctx, item);

            if( !ok )
                return JS_EXCEPTION;
        }
    }
    else
    {
        if( !addNode(argv[0]) )
            return JS_EXCEPTION;
    }

    try
    {
        auto results = opcuaClient->read(nodes);
        JSValue arr = JS_NewArray(jsctx);

        for( size_t i = 0; i < nodes.size(); ++i )
        {
            JSValue obj = JS_NewObject(jsctx);
            JS_SetPropertyStr(jsctx, obj, "nodeId", JS_NewString(jsctx, nodes[i].c_str()));

            UA_StatusCode status = (i < results.size()) ? results[i].status : UA_STATUSCODE_BADUNEXPECTEDERROR;
            bool ok = (i < results.size()) ? results[i].statusOk() : false;
            JS_SetPropertyStr(jsctx, obj, "status", JS_NewInt64(jsctx, static_cast<int64_t>(status)));
            JS_SetPropertyStr(jsctx, obj, "ok", JS_NewBool(jsctx, ok));

            const char* typeStr = "int32";

            if( i < results.size() && results[i].type == JSOPCUAClient::VarType::Float )
                typeStr = "float";

            JS_SetPropertyStr(jsctx, obj, "type", JS_NewString(jsctx, typeStr));

            JSValue value = JS_NULL;

            if( ok )
            {
                if( results[i].type == JSOPCUAClient::VarType::Float )
                    value = JS_NewFloat64(jsctx, static_cast<double>(results[i].as<float>()));
                else
                    value = JS_NewInt32(jsctx, results[i].as<int32_t>());
            }

            JS_SetPropertyStr(jsctx, obj, "value", value);
            JS_SetPropertyUint32(jsctx, arr, i, obj);
        }

        return arr;
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_opcua_write(JSContext* jsctx, JSValueConst, int argc, JSValueConst* argv)
{
    if( argc < 1 )
    {
        JS_ThrowTypeError(jsctx, "opcua.write(nodeId, value [, type]) or opcua.write(array)");
        return JS_EXCEPTION;
    }

    if( !opcuaClient )
    {
        JS_ThrowInternalError(jsctx, "opcua client not initialized");
        return JS_EXCEPTION;
    }

    std::vector<JSOPCUAClient::WriteItem> items;

    auto parseValue =
        [&](JSValueConst valueVal, JSValueConst typeVal, JSOPCUAClient::WriteItem& item) -> bool
    {
        enum class ValueKind { Int32, Float, Bool };
        ValueKind kind = ValueKind::Int32;

        if( !JS_IsUndefined(typeVal) )
        {
            if( !JS_IsString(typeVal) )
            {
                JS_ThrowTypeError(jsctx, "opcua.write: type must be string");
                return false;
            }

            const char* typeStr = JS_ToCString(jsctx, typeVal);

            if( !typeStr )
                return false;

            std::string t(typeStr);
            JS_FreeCString(jsctx, typeStr);

            std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

            if( t == "float" || t == "double" )
                kind = ValueKind::Float;
            else if( t == "bool" || t == "boolean" )
                kind = ValueKind::Bool;
            else
                kind = ValueKind::Int32;
        }
        else
        {
            if( JS_IsBool(valueVal) )
                kind = ValueKind::Bool;
            else if( JS_IsNumber(valueVal) )
            {
                double d = 0;

                if( JS_ToFloat64(jsctx, &d, valueVal) != 0 )
                {
                    JS_ThrowTypeError(jsctx, "opcua.write: invalid numeric value");
                    return false;
                }

                if( std::floor(d) != d )
                    kind = ValueKind::Float;
                else
                    kind = ValueKind::Int32;
            }
            else
            {
                JS_ThrowTypeError(jsctx, "opcua.write: value must be number or boolean");
                return false;
            }
        }

        switch( kind )
        {
            case ValueKind::Bool:
            {
                int32_t bv = JS_ToBool(jsctx, valueVal);

                if( bv < 0 )
                    return false;

                item.value = (bv != 0);
                break;
            }
            case ValueKind::Float:
            {
                double dv = 0;

                if( JS_ToFloat64(jsctx, &dv, valueVal) != 0 )
                {
                    JS_ThrowTypeError(jsctx, "opcua.write: invalid float value");
                    return false;
                }

                item.value = static_cast<float>(dv);
                break;
            }
            case ValueKind::Int32:
            default:
            {
                int32_t iv = 0;

                if( JS_ToInt32(jsctx, &iv, valueVal) != 0 )
                {
                    JS_ThrowTypeError(jsctx, "opcua.write: invalid int32 value");
                    return false;
                }

                item.value = iv;
                break;
            }
        }

        return true;
    };

    auto addItem = [&](const std::string& nodeId, JSValueConst valueVal, JSValueConst typeVal) -> bool
    {
        if( nodeId.empty() )
        {
            JS_ThrowTypeError(jsctx, "opcua.write: nodeId must be non-empty");
            return false;
        }

        JSOPCUAClient::WriteItem item;
        item.nodeId = nodeId;

        if( !parseValue(valueVal, typeVal, item) )
            return false;

        items.emplace_back(std::move(item));
        return true;
    };

    if( JS_IsArray(jsctx, argv[0]) )
    {
        JSValue lenVal = JS_GetPropertyStr(jsctx, argv[0], "length");
        uint32_t length = 0;

        if( JS_ToUint32(jsctx, &length, lenVal) != 0 )
        {
            JS_FreeValue(jsctx, lenVal);
            JS_ThrowTypeError(jsctx, "opcua.write: invalid array length");
            return JS_EXCEPTION;
        }

        JS_FreeValue(jsctx, lenVal);

        for( uint32_t i = 0; i < length; ++i )
        {
            JSValue entry = JS_GetPropertyUint32(jsctx, argv[0], i);

            if( JS_IsException(entry) )
                return JS_EXCEPTION;

            if( !JS_IsObject(entry) )
            {
                JS_FreeValue(jsctx, entry);
                JS_ThrowTypeError(jsctx, "opcua.write: array items must be objects");
                return JS_EXCEPTION;
            }

            JSValue nodeVal = JS_GetPropertyStr(jsctx, entry, "nodeId");
            JSValue valueVal = JS_GetPropertyStr(jsctx, entry, "value");
            JSValue typeVal = JS_GetPropertyStr(jsctx, entry, "type");

            if( JS_IsException(nodeVal) || JS_IsException(valueVal) || JS_IsException(typeVal) )
            {
                JS_FreeValue(jsctx, entry);
                JS_FreeValue(jsctx, nodeVal);
                JS_FreeValue(jsctx, valueVal);
                JS_FreeValue(jsctx, typeVal);
                return JS_EXCEPTION;
            }

            if( JS_IsUndefined(nodeVal) || JS_IsNull(nodeVal) )
            {
                JS_FreeValue(jsctx, entry);
                JS_FreeValue(jsctx, nodeVal);
                JS_FreeValue(jsctx, valueVal);
                JS_FreeValue(jsctx, typeVal);
                JS_ThrowTypeError(jsctx, "opcua.write: nodeId is required");
                return JS_EXCEPTION;
            }

            const char* nodeStr = JS_ToCString(jsctx, nodeVal);

            if( !nodeStr )
            {
                JS_FreeValue(jsctx, entry);
                JS_FreeValue(jsctx, nodeVal);
                JS_FreeValue(jsctx, valueVal);
                JS_FreeValue(jsctx, typeVal);
                return JS_EXCEPTION;
            }

            std::string nodeId(nodeStr);
            JS_FreeCString(jsctx, nodeStr);

            bool ok = addItem(nodeId, valueVal, typeVal);

            JS_FreeValue(jsctx, nodeVal);
            JS_FreeValue(jsctx, valueVal);
            JS_FreeValue(jsctx, typeVal);
            JS_FreeValue(jsctx, entry);

            if( !ok )
                return JS_EXCEPTION;
        }
    }
    else
    {
        if( !JS_IsString(argv[0]) )
        {
            JS_ThrowTypeError(jsctx, "opcua.write: nodeId must be string");
            return JS_EXCEPTION;
        }

        if( argc < 2 )
        {
            JS_ThrowTypeError(jsctx, "opcua.write: value argument required");
            return JS_EXCEPTION;
        }

        const char* nodeStr = JS_ToCString(jsctx, argv[0]);

        if( !nodeStr )
            return JS_EXCEPTION;

        std::string nodeId(nodeStr);
        JS_FreeCString(jsctx, nodeStr);

        JSValueConst typeVal = (argc >= 3) ? argv[2] : JS_UNDEFINED;

        if( !addItem(nodeId, argv[1], typeVal) )
            return JS_EXCEPTION;
    }

    try
    {
        auto status = opcuaClient->write(items);
        return JS_NewInt64(jsctx, static_cast<int64_t>(status));
    }
    catch( const std::exception& ex )
    {
        JS_ThrowInternalError(jsctx, "%s", ex.what());
        return JS_EXCEPTION;
    }
}
// ----------------------------------------------------------------------------
#endif // JS_OPCUA_ENABLED
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeBitsReply( const ModbusRTU::BitsBuffer& buf )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "byteCount", JS_NewInt32(ctx, buf.bcnt));
    JSValue arr = JS_NewArray(ctx);

    for( size_t i = 0; i < buf.bcnt; ++i )
        JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, buf.data[i]));

    JS_SetPropertyStr(ctx, result, "data", arr);
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeRegisterReply( const ModbusRTU::ReadOutputRetMessage& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "count", JS_NewInt32(ctx, msg.count));
    JSValue arr = JS_NewArray(ctx);

    for( size_t i = 0; i < msg.count; ++i )
        JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, msg.data[i]));

    JS_SetPropertyStr(ctx, result, "values", arr);
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeRegisterReply( const ModbusRTU::ReadInputRetMessage& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "count", JS_NewInt32(ctx, msg.count));
    JSValue arr = JS_NewArray(ctx);

    for( size_t i = 0; i < msg.count; ++i )
        JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, msg.data[i]));

    JS_SetPropertyStr(ctx, result, "values", arr);
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeDiagReply( const ModbusRTU::DiagnosticRetMessage& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "subfunc", JS_NewInt32(ctx, msg.subf));
    JS_SetPropertyStr(ctx, result, "count", JS_NewInt32(ctx, msg.count));

    JSValue arr = JS_NewArray(ctx);

    for( size_t i = 0; i < msg.count; ++i )
        JS_SetPropertyUint32(ctx, arr, i, JS_NewInt32(ctx, msg.data[i]));

    JS_SetPropertyStr(ctx, result, "values", arr);
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeWriteAck( ModbusRTU::ModbusData start, ModbusRTU::ModbusData count )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "start", JS_NewInt32(ctx, start));
    JS_SetPropertyStr(ctx, result, "count", JS_NewInt32(ctx, count));
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeWriteSingleAck( const ModbusRTU::WriteSingleOutputRetMessage& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "start", JS_NewInt32(ctx, msg.start));
    JS_SetPropertyStr(ctx, result, "value", JS_NewInt32(ctx, msg.data));
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeModbusBoolAck( const ModbusRTU::ForceSingleCoilRetMessage& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "start", JS_NewInt32(ctx, msg.start));
    JS_SetPropertyStr(ctx, result, "value", JS_NewBool(ctx, msg.cmd()));
    return result;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMake4314Reply( const ModbusRTU::MEIMessageRetRDI& msg )
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "deviceId", JS_NewInt32(ctx, msg.devID));
    JS_SetPropertyStr(ctx, result, "conformity", JS_NewInt32(ctx, msg.conformity));
    JS_SetPropertyStr(ctx, result, "moreFollows", JS_NewBool(ctx, msg.mf == 0xFF));
    JS_SetPropertyStr(ctx, result, "objectId", JS_NewInt32(ctx, msg.objID));
    JS_SetPropertyStr(ctx, result, "objectCount", JS_NewInt32(ctx, msg.objNum));

    JSValue arr = JS_NewArray(ctx);
    uint32_t idx = 0;

    for( const auto& item : msg.dlist )
    {
        JSValue entry = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, entry, "id", JS_NewInt32(ctx, item.id));
        JS_SetPropertyStr(ctx, entry, "value", JS_NewString(ctx, item.val.c_str()));
        JS_SetPropertyUint32(ctx, arr, idx++, entry);
    }

    JS_SetPropertyStr(ctx, result, "objects", arr);
    return result;
}
// ----------------------------------------------------------------------------
// Нестатические методы-члены класса
JSValue JSEngine::js_ui_askSensor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // TODO: Реализовать вызов UInterface::askSensor
    // Параметры:
    // argv[0] - ID датчика (number or string)
    // argv[1] - команда (number)
    if( argc < 2 )
    {
        JS_ThrowTypeError(ctx, "askSensor expects 2 argument (sensor ID, value)");
        return JS_UNDEFINED;
    }

    int64_t sid = DefaultObjectId;

    if( JS_IsString(argv[0]) )
    {
        const char* sname = JS_ToCString(ctx, argv[0]);

        if( sname )
        {
            sid = ui->getConf()->getSensorID(sname);
            JS_FreeCString(ctx, sname);
        }
    }
    else
    {
        if (JS_ToInt64(ctx, &sid, argv[0]) != 0)
        {
            JS_ThrowTypeError(ctx, "Invalid sensor ID");
            return JS_UNDEFINED;
        }
    }

    if( sid == uniset::DefaultObjectId )
    {
        JS_ThrowTypeError(ctx, "Invalid sensor ID or name");
        return JS_UNDEFINED;
    }

    int64_t cmd;

    if (JS_ToInt64(ctx, &cmd, argv[1]) != 0)
    {
        JS_ThrowTypeError(ctx, "Invalid cmd");
        return JS_UNDEFINED;
    }

    try
    {
        mylog5 << "(js_ui_askSensor): sid=" << sid << " cmd=" << to_string((UniversalIO::UIOCommand)cmd) << endl;
        ui->askSensor(sid, (UniversalIO::UIOCommand)cmd);
        return JS_NewBool(ctx, 1);
    }
    catch(std::exception& ex)
    {
        mycrit << "(js_ui_askSensor): " << ex.what() << endl;
    }

    JS_ThrowTypeError(ctx, "askSensor catch exception");
    return JS_UNDEFINED;

}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_ui_getValue( JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv )
{
    // Параметры:
    // argv[0] - ID датчика (number or string)
    if( argc < 1 )
    {
        JS_ThrowTypeError(ctx, "getValue expects 1 argument (sensor ID)");
        return JS_UNDEFINED;
    }

    int64_t sid = DefaultObjectId;

    if( JS_IsString(argv[0]) )
    {
        const char* sname = JS_ToCString(ctx, argv[0]);

        if( sname )
        {
            sid = ui->getConf()->getSensorID(sname);
            JS_FreeCString(ctx, sname);
        }
    }
    else
    {

        if (JS_ToInt64(ctx, &sid, argv[0]) != 0)
        {
            JS_ThrowTypeError(ctx, "Invalid sensor ID");
            return JS_UNDEFINED;
        }
    }

    if( sid == uniset::DefaultObjectId )
    {
        JS_ThrowTypeError(ctx, "Invalid sensor ID or name");
        return JS_UNDEFINED;
    }

    try
    {
        mylog5 << "(js_ui_getValue): sid=" << sid  << endl;
        auto val = ui->getValue(sid);
        return JS_NewInt64(ctx, val);
    }
    catch( std::exception& ex )
    {
        mycrit << "(js_ui_getValue): " << ex.what() << endl;
    }

    JS_ThrowTypeError(ctx, "getValue catch exception");
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_ui_setValue(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // Параметры:
    // argv[0] - ID датчика (number or string)
    // argv[1] - значение (number)
    if( argc < 2 )
    {
        JS_ThrowTypeError(ctx, "setValue expects 2 argument (sensor ID, value)");
        return JS_UNDEFINED;
    }

    int64_t sid = DefaultObjectId;

    if( JS_IsString(argv[0]) )
    {
        const char* sname = JS_ToCString(ctx, argv[0]);

        if( sname )
        {
            sid = ui->getConf()->getSensorID(sname);
            JS_FreeCString(ctx, sname);
        }
    }
    else
    {
        if (JS_ToInt64(ctx, &sid, argv[0]) != 0)
        {
            JS_ThrowTypeError(ctx, "Invalid sensor ID");
            return JS_UNDEFINED;
        }
    }

    if( sid == uniset::DefaultObjectId )
    {
        JS_ThrowTypeError(ctx, "Invalid sensor ID or name");
        return JS_UNDEFINED;
    }

    int64_t value;

    if (JS_ToInt64(ctx, &value, argv[1]) != 0)
    {
        JS_ThrowTypeError(ctx, "Invalid value");
        return JS_UNDEFINED;
    }

    try
    {
        mylog5 << "(js_ui_setValue): sid=" << sid << " value=" << value << endl;
        ui->setValue(sid, value);

        auto out = outputs.find(sid);

        if( out != outputs.end() )
        {
            if( !out->second.set(ctx, jsGlobal, int64_t(value)) )
                mycrit << "(js_ui_setValue): can't update value for " << out->second.name << endl;
        }

        return JS_NewBool(ctx, 1);
    }
    catch(std::exception& ex)
    {
        mycrit << "(js_ui_setValue): " << ex.what() << endl;
    }

    JS_ThrowTypeError(ctx, "setValue catch exception");
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_uniset_StepCb(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // Параметры:
    // argv[0] - callback function
    if( argc < 1 )
    {
        JS_ThrowTypeError(ctx, "js_uniset_StepCb expects 1 argument (function)");
        return JS_UNDEFINED;
    }

    if( !JS_IsFunction(ctx, argv[0]) )
    {
        JS_ThrowTypeError(ctx, "js_uniset_StepCb: First argument must be 'function'");
        return JS_UNDEFINED;
    }

    stepFunctions.push_back(JS_DupValue(ctx, argv[0]));
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_uniset_StopCb(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // Параметры:
    // argv[0] - callback function
    if( argc < 1 )
    {
        JS_ThrowTypeError(ctx, "js_uniset_StopCb expects 1 argument (function)");
        return JS_UNDEFINED;
    }

    if( !JS_IsFunction(ctx, argv[0]) )
    {
        JS_ThrowTypeError(ctx, "js_uniset_StopCb: First argument must be 'function'");
        return JS_UNDEFINED;
    }

    stopFunctions.push_back(JS_DupValue(ctx, argv[0]));
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_uniset_httpStart(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    if( httpserv->isRunning() )
    {
        mylog5 << "(js_uniset_httpStart): http server is already running.." << endl;
        return JS_UNDEFINED;
    }

    // Параметры:
    // argv[0] - host
    // argv[1] - port
    // argv[2] - http request callback
    if( argc < 3 )
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart expects 3 argument (host, port, function)");
        return JS_UNDEFINED;
    }

    if( !JS_IsString(argv[0]) )
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart: First argument must be 'string' (host)");
        return JS_UNDEFINED;
    }

    const char* chost = JS_ToCString(ctx, argv[0]);

    if( !chost )
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart: Undefined host parameter");
        return JS_UNDEFINED;
    }

    std::string host = string(chost);
    JS_FreeCString(ctx, chost);

    if( !JS_IsNumber(argv[1]) )
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart: Second argument must be 'number' (port)");
        return JS_UNDEFINED;
    }

    int32_t port;

    if (JS_ToInt32(ctx, &port, argv[1]) != 0)
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart: Invalid port");
        return JS_UNDEFINED;
    }

    if( !JS_IsFunction(ctx, argv[2]) )
    {
        JS_ThrowTypeError(ctx, "js_uniset_httpStart: Third argument must be 'function' (on_http_request)");
        return JS_UNDEFINED;
    }

    jsFnHttpRequest = JS_DupValue(ctx, argv[2]);

    mylog5 << "(js_uniset_httpStart): starting http server host=" << host << " port=" << port << endl;
    httpserv->start(host, port);
    mylog5 << "(js_uniset_httpStart): http server started" << endl;
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_log( JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv )
{
    // Параметры:
    // argv[0] - message
    // argv[1] - log level
    if( argc < 2 )
    {
        JS_ThrowTypeError(ctx, "js_log expects 2 argument (message, level)");
        return JS_UNDEFINED;
    }

    std::string msg = { "" };

    if( !JS_IsString(argv[0]) )
    {
        JS_ThrowTypeError(ctx, "js_log: First argument must be string");
        return JS_UNDEFINED;
    }

    {
        const char* txt = JS_ToCString(ctx, argv[0]);

        if( txt )
        {
            msg = txt;
            JS_FreeCString(ctx, txt);
        }
    }

    if( !JS_IsNumber(argv[1]) )
    {
        JS_ThrowTypeError(ctx, "js_log: Second argument must be int (debug level)");
        return JS_UNDEFINED;
    }

    int64_t value;

    if (JS_ToInt64(ctx, &value, argv[1]) != 0)
    {
        JS_ThrowTypeError(ctx, "Invalid value");
        return JS_UNDEFINED;
    }

    Debug::type dlevel = (Debug::type)value;

    if( jslog->debugging(dlevel) || dlevel == Debug::ANY )
        jslog->debug(dlevel) << msg << endl;

    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::js_log_level( JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv )
{
    // Параметры:
    // argv[1] - log level
    if( argc < 1 )
    {
        JS_ThrowTypeError(ctx, "js_log_level: expects 1 argument (log level)");
        return JS_UNDEFINED;
    }

    if( !JS_IsNumber(argv[0]) )
    {
        JS_ThrowTypeError(ctx, "js_log_level: Second argument must be int (debug level)");
        return JS_UNDEFINED;
    }

    int64_t value;

    if (JS_ToInt64(ctx, &value, argv[0]) != 0)
    {
        JS_ThrowTypeError(ctx, "Invalid value");
        return JS_UNDEFINED;
    }

    mylog5 << "(js_log_level): set log level: " << Debug::str((Debug::type)value) << endl;
    jslog->level((Debug::type)value);
    return JS_UNDEFINED;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeRequest(JSContext* ctx, JSValueConst& jsReqProto_, JSReqAtom& reqAtoms_, const JHttpServer::RequestSnapshot& r)
{
    JSValue req = JS_NewObjectProto(ctx, jsReqProto_);

    JS_SetProperty(ctx, req, reqAtoms_.method,  JS_NewString(ctx, r.method.c_str()));
    JS_SetProperty(ctx, req, reqAtoms_.uri,     JS_NewString(ctx, r.uri.c_str()));
    JS_SetProperty(ctx, req, reqAtoms_.version, JS_NewString(ctx, r.version.c_str()));
    // на всякий случай совместимость с libs, которые читают url:
    JS_SetProperty(ctx, req, reqAtoms_.url,     JS_NewString(ctx, r.uri.c_str()));

    // router ожидает path; дадим url/path/query из uri
    {
        const std::string& u = r.uri;
        auto qm = u.find('?');

        if( qm == std::string::npos )
        {
            JS_SetProperty(ctx, req, reqAtoms_.path,  JS_NewStringLen(ctx, u.data(), u.size()));
            JS_SetProperty(ctx, req, reqAtoms_.query, JS_NewStringLen(ctx, "", 0));
        }
        else
        {
            JS_SetProperty(ctx, req, reqAtoms_.path,  JS_NewStringLen(ctx, u.data(), qm));
            JS_SetProperty(ctx, req, reqAtoms_.query, JS_NewStringLen(ctx, u.data() + qm + 1, u.size() - qm - 1));
        }
    }

    // headers
    JSValue h = JS_NewObject(ctx);

    for (const auto& kv : r.headers)
        JS_SetPropertyStr(ctx, h, kv.first.c_str(), JS_NewString(ctx, kv.second.c_str()));

    JS_SetProperty(ctx, req, reqAtoms_.headers, h);

    // body (как строка, без копирования бинарных данных)
    JS_SetProperty(ctx, req, reqAtoms_.body, JS_NewStringLen(ctx, r.body.data(), r.body.size()));

    return req;
}
// ----------------------------------------------------------------------------
JSValue JSEngine::jsMakeResponse(JSContext* ctx, JSValueConst& jsResProto_, JHttpServer::ResponseAdapter* ad)
{
    JSValue res = JS_NewObjectProto(ctx, jsResProto_);
    JS_SetPropertyStr(ctx, res, "__ptr", JS_NewInt64(ctx, (int64_t)ad));

    // методы уже заданы на прототипе; здесь только привязываем адаптер
    return res;
}
// ----------------------------------------------------------------------------
void JSEngine::jsApplyResponseObject(JSContext* ctx, JSValue ret, JHttpServer::ResponseSnapshot& out)
{
    if (!JS_IsObject(ret))
        return;

    // status
    JSValue v = JS_GetPropertyStr(ctx, ret, "status");

    if (JS_IsNumber(v))
    {
        int64_t x = 200;
        JS_ToInt64(ctx, &x, v);
        out.status = (int)x;
    }

    JS_FreeValue(ctx, v);

    // reason
    v = JS_GetPropertyStr(ctx, ret, "reason");

    if (JS_IsString(v))
    {
        size_t l = 0;
        const char* s = JS_ToCStringLen(ctx, &l, v);

        if(s)
        {
            out.reason.assign(s, l);
            JS_FreeCString(ctx, s);
        }
    }

    JS_FreeValue(ctx, v);

    // headers
    v = JS_GetPropertyStr(ctx, ret, "headers");

    if (JS_IsObject(v))
    {
        JSPropertyEnum* tab;
        uint32_t len;

        if (JS_GetOwnPropertyNames(ctx, &tab, &len, v, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0)
        {
            for(uint32_t i = 0; i < len; i++)
            {
                JSAtom a = tab[i].atom;
                JSValue k = JS_AtomToValue(ctx, a);
                JSValue val = JS_GetProperty(ctx, v, a);

                if (JS_IsString(k) && JS_IsString(val))
                {
                    size_t lk = 0, lv = 0;
                    const char* sk = JS_ToCStringLen(ctx, &lk, k);
                    const char* sv = JS_ToCStringLen(ctx, &lv, val);

                    if (sk && sv) out.headers.emplace_back(std::string(sk, lk), std::string(sv, lv));

                    if (sk) JS_FreeCString(ctx, sk);

                    if (sv) JS_FreeCString(ctx, sv);
                }

                JS_FreeValue(ctx, k);
                JS_FreeValue(ctx, val);
                JS_FreeAtom(ctx, a);
            }

            js_free(ctx, tab);
        }
    }

    JS_FreeValue(ctx, v);

    // body
    v = JS_GetPropertyStr(ctx, ret, "body");

    if (JS_IsString(v))
    {
        size_t l = 0;
        const char* s = JS_ToCStringLen(ctx, &l, v);

        if(s)
        {
            out.body.assign(s, l);
            JS_FreeCString(ctx, s);
        }
    }

    JS_FreeValue(ctx, v);
}
// ----------------------------------------------------------------------------
void JSEngine::jsApplyResponseAdapter( const JHttpServer::ResponseAdapter& ad, JHttpServer::ResponseSnapshot& out )
{
    out = ad.snap;

    // если причина не задана — карта статусов на стандартную фразу
    if (out.reason.empty())
    {
        switch(out.status)
        {
            case 200:
                out.reason = "OK";
                break;

            case 400:
                out.reason = "Bad Request";
                break;

            case 404:
                out.reason = "Not Found";
                break;

            case 500:
                out.reason = "Internal Server Error";
                break;

            default:
                break;
        }
    }
}
// ----------------------------------------------------------------------------
void JSEngine::createResponsePrototype( JSContext* ctx )
{
    if( !JS_IsUndefined(jsResProto_) )
        return;

    JSValue proto = JS_NewObject(ctx);

    // ---- Methods injected once on the prototype ----
    // status, setHeader, end, json, sendStatus
    auto fn_status = [](JSContext * c, JSValueConst self, int argc, JSValueConst * argv)->JSValue
    {
        JSValue pv = JS_GetPropertyStr(c, self, "__ptr");
        int64_t p = 0;
        JS_ToInt64(c, &p, pv);
        JS_FreeValue(c, pv);
        auto* ad = (JHttpServer::ResponseAdapter*)p;

        if (!ad || argc < 1) return JS_UNDEFINED;

        int32_t code = 200;
        JS_ToInt32(c, &code, argv[0]);
        ad->snap.status = code;

        if (argc > 1 && JS_IsString(argv[1]))
        {
            size_t l = 0;
            const char* s = JS_ToCStringLen(c, &l, argv[1]);

            if (s)
            {
                ad->snap.reason.assign(s, l);
                JS_FreeCString(c, s);
            }
        }

        return JS_UNDEFINED;
    };
    JS_SetPropertyStr(ctx, proto, "status", JS_NewCFunction(ctx, fn_status, "status", 2));

    auto fn_setHeader = [](JSContext * c, JSValueConst self, int argc, JSValueConst * argv)->JSValue
    {
        JSValue pv = JS_GetPropertyStr(c, self, "__ptr");
        int64_t p = 0;
        JS_ToInt64(c, &p, pv);
        JS_FreeValue(c, pv);
        auto* ad = (JHttpServer::ResponseAdapter*)p;

        if (!ad || argc < 2) return JS_UNDEFINED;

        size_t lk = 0, lv = 0;
        const char* sk = JS_ToCStringLen(c, &lk, argv[0]);
        const char* sv = JS_ToCStringLen(c, &lv, argv[1]);

        if (sk && sv) ad->snap.headers.emplace_back(std::string(sk, lk), std::string(sv, lv));

        if (sk) JS_FreeCString(c, sk);

        if (sv) JS_FreeCString(c, sv);

        return JS_UNDEFINED;
    };
    JS_SetPropertyStr(ctx, proto, "setHeader", JS_NewCFunction(ctx, fn_setHeader, "setHeader", 2));

    auto fn_end = [](JSContext * c, JSValueConst self, int argc, JSValueConst * argv)->JSValue
    {
        JSValue pv = JS_GetPropertyStr(c, self, "__ptr");
        int64_t p = 0;
        JS_ToInt64(c, &p, pv);
        JS_FreeValue(c, pv);
        auto* ad = (JHttpServer::ResponseAdapter*)p;

        if (!ad) return JS_UNDEFINED;

        if (argc > 0 && JS_IsString(argv[0]))
        {
            size_t l = 0;
            const char* s = JS_ToCStringLen(c, &l, argv[0]);

            if (s)
            {
                ad->snap.body.assign(s, l);
                JS_FreeCString(c, s);
            }
        }

        JS_SetPropertyStr(c, (JSValue)self, "__ended", JS_NewBool(c, 1));
        ad->finished = true;
        return JS_UNDEFINED;
    };
    JS_SetPropertyStr(ctx, proto, "end", JS_NewCFunction(ctx, fn_end, "end", 1));

    auto fn_json = [](JSContext * c, JSValueConst self, int argc, JSValueConst * argv)->JSValue
    {
        JSValue pv = JS_GetPropertyStr(c, self, "__ptr");
        int64_t p = 0;
        JS_ToInt64(c, &p, pv);
        JS_FreeValue(c, pv);
        auto* ad = (JHttpServer::ResponseAdapter*)p;

        if (!ad || argc < 1) return JS_UNDEFINED;

        JSValue j = JS_JSONStringify(c, argv[0], JS_UNDEFINED, JS_UNDEFINED);
        size_t l = 0;
        const char* s = JS_ToCStringLen(c, &l, j);

        if (s)
        {
            ad->snap.headers.emplace_back("Content-Type", "application/json");
            ad->snap.body.assign(s, l);
            JS_FreeCString(c, s);
        }

        JS_FreeValue(c, j);
        JS_SetPropertyStr(c, (JSValue)self, "__ended", JS_NewBool(c, 1));
        ad->finished = true;
        return JS_UNDEFINED;
    };
    JS_SetPropertyStr(ctx, proto, "json", JS_NewCFunction(ctx, fn_json, "json", 1));

    auto fn_sendStatus = [](JSContext * c, JSValueConst self, int argc, JSValueConst * argv)->JSValue
    {
        JSValue pv = JS_GetPropertyStr(c, self, "__ptr");
        int64_t p = 0;
        JS_ToInt64(c, &p, pv);
        JS_FreeValue(c, pv);
        auto* ad = (JHttpServer::ResponseAdapter*)p;

        if (!ad || argc < 1) return JS_UNDEFINED;

        int32_t code = 200;
        JS_ToInt32(c, &code, argv[0]);
        ad->snap.status = code;

        if (code != 204) ad->snap.body = std::to_string(code);

        JS_SetPropertyStr(c, (JSValue)self, "__ended", JS_NewBool(c, 1));
        ad->finished = true;
        return JS_UNDEFINED;
    };
    JS_SetPropertyStr(ctx, proto, "sendStatus", JS_NewCFunction(ctx, fn_sendStatus, "sendStatus", 1));

    jsResProto_ = proto;
}
// ----------------------------------------------------------------------------
void JSEngine::createRequestPrototype( JSContext* ctx )
{
    if (!JS_IsUndefined(jsReqProto_))
        return;

    // У req нет методов — прототип пустой, но создаётся один раз
    jsReqProto_ = JS_NewObject(ctx);
}
// ----------------------------------------------------------------------------
void JSEngine::createRequestAtoms( JSContext* ctx )
{
    if (reqAtomsInited_) return;

    reqAtoms_.method  = JS_NewAtom(ctx, "method");
    reqAtoms_.uri     = JS_NewAtom(ctx, "uri");
    reqAtoms_.version = JS_NewAtom(ctx, "version");
    reqAtoms_.url     = JS_NewAtom(ctx, "url");
    reqAtoms_.path    = JS_NewAtom(ctx, "path");
    reqAtoms_.query   = JS_NewAtom(ctx, "query");
    reqAtoms_.headers = JS_NewAtom(ctx, "headers");
    reqAtoms_.body    = JS_NewAtom(ctx, "body");

    reqAtomsInited_ = true;
}
// ----------------------------------------------------------------------------
