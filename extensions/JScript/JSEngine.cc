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

        JS_FreeValue(ctx, argv[0]);
        JS_FreeValue(ctx, argv[1]);
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
    for( int i = 0; i < opts.jsLoopCount; i++ )
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
