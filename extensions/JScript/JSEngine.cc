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
                    int _jsLoopCount,
                    bool _esmModuleMode ):
    jsfile(_jsfile),
    searchPaths(_searchPaths),
    ui(_ui),
    jsLoopCount(_jsLoopCount),
    esmModuleMode(_esmModuleMode)
{
    if( jsfile.empty() )
        throw SystemError("JS file undefined");

    jslog = make_shared<DebugStream>();
    jslog->setLogName("JSLog");

    mylog = make_shared<DebugStream>();
    mylog->setLogName("JSEngine");
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

    if( esmModuleMode )
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

    if( esmModuleMode )
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

    if( esmModuleMode )
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

    // ПОТОМ загружаем таймеры
    exportAllFunctionsFromTimerModule();
    createUInterfaceObject();

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
void JSEngine::freeJS()
{
    myinfo << "(freeJS): freeing JS resources..." << endl;

    if( activated )
        stop();

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

        if (!JS_IsUndefined(jsGlobal))
            JS_FreeValue(ctx, jsGlobal);

        if ( !esmModuleMode && !JS_IsUndefined(jsModule))
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
            mycrit << "(jsSetValue): can't update value for " << it->second.name << endl;
    }

    if( !JS_IsUndefined(jsFnOnSensor) )
    {
        JSValue argv[2];
        argv[0] = JS_NewInt64(ctx, sm->id);
        argv[1] = JS_NewInt64(ctx, sm->value);
        jshelper::safe_function_call(ctx, jsGlobal, jsFnOnSensor, 2, argv);
        JS_FreeValue(ctx, argv[0]);
        JS_FreeValue(ctx, argv[1]);
    }

    jsLoop();
}
// ----------------------------------------------------------------------------
void JSEngine::jsLoop()
{
    // js loop processing
    for( int i = 0; i < jsLoopCount; i++ )
        js_std_loop(ctx);
}
// ----------------------------------------------------------------------------
void JSEngine::step()
{
    // timers processing
    if( !JS_IsUndefined(jsFnTimers) )
        jshelper::safe_function_call(ctx, jsGlobal, jsFnTimers, 0, nullptr);

    jsLoop();

    // step processing
    jshelper::safe_function_call(ctx, jsGlobal, jsFnStep, 0, nullptr);

    jsLoop();
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
    // TODO: Реализовать вызов UInterface::setValue
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
