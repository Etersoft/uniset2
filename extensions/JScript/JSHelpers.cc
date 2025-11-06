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
// -------------------------------------------------------------------------
#include <stdio.h>
#include <quickjs/quickjs-libc.h>
#include "UniSetTypes.h"
#include "JSHelpers.h"
// -------------------------------------------------------------------------
using namespace std;
// -------------------------------------------------------------------------
namespace uniset
{
    namespace jshelper
    {
        // -------------------------------------------------------------------------
        IOProp convert_js_to_io_prop( JSContext* ctx, JSValueConst obj_val )
        {
            IOProp ret;

            if (JS_IsObject(obj_val))
            {
                JSValue name_val = JS_GetPropertyStr(ctx, obj_val, "name");

                if (JS_IsString(name_val))
                {
                    const char* name_str = JS_ToCString(ctx, name_val);

                    if (name_str)
                    {
                        ret.name = name_str;
                        JS_FreeCString(ctx, name_str);
                    }
                }

                JS_FreeValue(ctx, name_val);

                JSValue sensor_val = JS_GetPropertyStr(ctx, obj_val, "sensor");

                if (JS_IsString(sensor_val))
                {
                    const char* name_str = JS_ToCString(ctx, sensor_val);

                    if (name_str)
                    {
                        ret.sensor = name_str;
                        JS_FreeCString(ctx, name_str);
                    }
                }

                if( ret.sensor.empty() )
                    ret.sensor = ret.name;

                JS_FreeValue(ctx, sensor_val);
            }

            return ret;
        }

        // -------------------------------------------------------------------------
        void dump_exception_details( JSContext* ctx )
        {
            JSValue exception = JS_GetException(ctx);

            // 1. Получаем сообщение об ошибке
            JSValue message_val = JS_GetPropertyStr(ctx, exception, "message");

            if (JS_IsString(message_val))
            {
                const char* message = JS_ToCString(ctx, message_val);
                printf("Message: %s\n", message);
                JS_FreeCString(ctx, message);
            }

            JS_FreeValue(ctx, message_val);

            // 2. Получаем имя конструктора ошибки
            JSValue constructor_val = JS_GetPropertyStr(ctx, exception, "constructor");
            JSValue name_val = JS_GetPropertyStr(ctx, constructor_val, "name");

            if (JS_IsString(name_val))
            {
                const char* error_name = JS_ToCString(ctx, name_val);
                printf("Error type: %s\n", error_name);
                JS_FreeCString(ctx, error_name);
            }

            JS_FreeValue(ctx, name_val);
            JS_FreeValue(ctx, constructor_val);

            // 3. Получаем стектрейс
            JSValue stack_val = JS_GetPropertyStr(ctx, exception, "stack");

            if (JS_IsString(stack_val))
            {
                const char* stack = JS_ToCString(ctx, stack_val);
                printf("Stack trace:\n%s\n", stack);
                JS_FreeCString(ctx, stack);
            }

            JS_FreeValue(ctx, stack_val);

            // 4. Получаем строковое представление всей ошибки
            const char* exception_str = JS_ToCString(ctx, exception);

            if (exception_str)
            {
                printf("Full exception: %s\n", exception_str);
                JS_FreeCString(ctx, exception_str);
            }

            JS_FreeValue(ctx, exception);
        }

        // -------------------------------------------------------------------------
        void safe_function_call_by_name( JSContext* ctx, const char* func_name,
                                         int argc, JSValueConst* argv)
        {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue func = JS_GetPropertyStr(ctx, global, func_name);

            printf("Calling '%s' with %d arguments\n", func_name, argc);

            if( JS_IsUndefined(func) )
            {
                printf("ERROR: Function '%s' not found in global scope\n", func_name);

                // Выводим все глобальные свойства для отладки
                printf("Available globals: ");
                JSPropertyEnum* props = nullptr;
                uint32_t count = 0;

                int ret = JS_GetOwnPropertyNames(ctx, &props, &count, global, JS_GPN_STRING_MASK);

                if( ret == 0 && props != nullptr )
                {
                    printf("Global properties (%d):\n", count);

                    for( uint32_t i = 0; i < count; i++ )
                    {
                        JSValue value = JS_GetProperty(ctx, global, props[i].atom);
                        const char* name = JS_ToCString(ctx, value);
                        printf("  %s: ", name);

                        if( JS_IsFunction(ctx, value) )
                        {
                            printf("[function]");
                            // Получаем свойство 'name' функции
                            JSValue name_val = JS_GetPropertyStr(ctx, value, "name");
                            const char* name = nullptr;

                            if (JS_IsString(name_val))
                            {
                                name = JS_ToCString(ctx, name_val);
                            }

                            // Освобождаем промежуточное значение
                            JS_FreeValue(ctx, name_val);
                            printf("function name: %s", name);
                            JS_FreeCString(ctx, name);
                        }
                        else if (JS_IsObject(value))
                        {
                            printf("[object]");
                        }
                        else if (JS_IsString(value))
                        {
                            const char* str = JS_ToCString(ctx, value);
                            printf("'%s'", str);
                            JS_FreeCString(ctx, str);
                        }
                        else if (JS_IsNumber(value))
                        {
                            double num;
                            JS_ToFloat64(ctx, &num, value);
                            printf("%g", num);
                        }
                        else if (JS_IsUndefined(value))
                        {
                            printf("undefined");
                        }
                        else if (JS_IsNull(value))
                        {
                            printf("null");
                        }
                        else
                        {
                            printf("[unknown]");
                        }

                        printf("\n");

                        JS_FreeCString(ctx, name);
                        JS_FreeValue(ctx, value);
                    }
                }

                JS_FreeValue(ctx, func);
                JS_FreeValue(ctx, global);
                return;
            }

            if (!JS_IsFunction(ctx, func))
            {
                printf("ERROR: '%s' is not a function\n", func_name);
                JS_FreeValue(ctx, func);
                JS_FreeValue(ctx, global);
                return;
            }

            safe_function_call(ctx, global, func, argc, argv);
            JS_FreeValue(ctx, func);
            JS_FreeValue(ctx, global);
        }
        // -------------------------------------------------------------------------
        void safe_function_call(JSContext* ctx, JSValueConst global, JSValueConst func, int argc, JSValueConst* argv)
        {
            JSValue result = JS_Call(ctx, func, global, argc, argv);

            if( JS_IsException(result) )
            {
                js_std_dump_error(ctx);
                dump_exception_details(ctx);
            }
            else
            {
                if( !JS_IsUndefined(result) )
                {
                    const char* str = JS_ToCString(ctx, result);

                    if( str )
                    {
                        printf("  Result: %s\n", str);
                        JS_FreeCString(ctx, str);
                    }
                }
            }

            if( !JS_IsUndefined(result) )
                JS_FreeValue(ctx, result);
        }

        // -------------------------------------------------------------------------
        void debug_function_call( JSContext* ctx, const char* func_name )
        {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue func = JS_GetPropertyStr(ctx, global, func_name);

            printf("=== Debugging function '%s' ===\n", func_name);

            // 1. Проверяем существует ли функция
            if (JS_IsUndefined(func))
            {
                printf("❌ Function '%s' is undefined\n", func_name);
                JS_FreeValue(ctx, global);
                return;
            }

            // 2. Проверяем тип
            if (!JS_IsFunction(ctx, func))
            {
                printf("❌ '%s' is not a function (type: ", func_name);

                if (JS_IsObject(func)) printf("object");
                else if (JS_IsString(func)) printf("string");
                else if (JS_IsNumber(func)) printf("number");
                else if (JS_IsNull(func)) printf("null");
                else printf("unknown");

                printf(")\n");
            }
            else
            {
                printf("✅ '%s' is a function\n", func_name);
            }

            // 3. Пробуем вызвать
            JSValue result = JS_Call(ctx, func, global, 0, nullptr);

            if (JS_IsException(result))
            {
                printf("❌ Function call threw exception\n");
                JSValue exception = JS_GetException(ctx);
                const char* error_str = JS_ToCString(ctx, exception);

                if (error_str)
                {
                    printf("   Error: %s\n", error_str);
                    JS_FreeCString(ctx, error_str);
                }

                JS_FreeValue(ctx, exception);
            }
            else
            {
                printf("✅ Function call succeeded\n");

                if (!JS_IsUndefined(result))
                {
                    const char* result_str = JS_ToCString(ctx, result);

                    if (result_str)
                    {
                        printf("   Returned: %s\n", result_str);
                        JS_FreeCString(ctx, result_str);
                    }
                }

                JS_FreeValue(ctx, result);
            }

            JS_FreeValue(ctx, func);
            JS_FreeValue(ctx, global);
            printf("==============================\n\n");
        }
        // -------------------------------------------------------------------------
        JSModuleDef* module_loader_with_path( JSContext* ctx, const char* module_name, void* opaque )
        {
            if( opaque == nullptr )
                return nullptr;

            JSRuntime* rt = JS_GetRuntime(ctx);

            if( ModuleRegistry::is_module_loaded(rt, module_name) )
            {
                //                printf("Module '%s' already loaded in registry\n", filename);
                return nullptr;
            }

            JSPrivateData* data = static_cast<JSPrivateData*>(opaque);

            for( const auto& path : data->search_paths )
            {
                std::string full_path = path + "/" + module_name;

                JSModuleDef* module = js_module_loader(ctx, full_path.c_str(), nullptr);

                if( module )
                {
                    ModuleRegistry::mark_module_loaded(rt, module_name);
                    return module;
                }

                // Пробуем с расширением .js
                full_path = path + "/" + module_name + ".js";
                module = js_module_loader(ctx, full_path.c_str(), nullptr);

                if( module )
                {
                    ModuleRegistry::mark_module_loaded(rt, module_name);
                    return module;
                }
            }

            return nullptr;
        }
        // -------------------------------------------------------------------------
        static std::vector<std::string> get_search_paths( JSContext* ctx, JSValueConst val )
        {
            std::vector<std::string> search_paths;

            if (JS_IsArray(ctx, val))
            {
                JSValue length_val = JS_GetPropertyStr(ctx, val, "length");
                int length;
                JS_ToInt32(ctx, &length, length_val);
                JS_FreeValue(ctx, length_val);

                for (int i = 0; i < length; i++)
                {
                    JSValue path_val = JS_GetPropertyUint32(ctx, val, i);

                    if (JS_IsString(path_val))
                    {
                        const char* path_str = JS_ToCString(ctx, path_val);

                        if (path_str)
                        {
                            search_paths.push_back(path_str);
                            JS_FreeCString(ctx, path_str);
                        }
                    }

                    JS_FreeValue(ctx, path_val);
                }
            }

            return search_paths;
        }
        // -------------------------------------------------------------------------
        std::string find_file( const std::string filename, const std::vector<std::string>& search_paths )
        {
            if( filename.empty() )
                return "";

            std::string found_path;

            if( uniset::file_exist(filename) || filename[0] == '.' || filename[0] == '/' )
            {
                found_path = filename;
            }
            else
            {
                const std::string fname(filename);

                for (const auto& p : search_paths)
                {
                    if( uniset::file_exist(p + "/" + fname) )
                    {
                        found_path = p + "/" + fname;
                        break;
                    }
                }
            }

            return found_path;
        }
        // -------------------------------------------------------------------------
        JSValue js_load_file_with_data( JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv )
        {
            if (argc != 1) return JS_UNDEFINED;

            const char* filename = JS_ToCString(ctx, argv[0]);

            if (!filename) return JS_UNDEFINED;

            JSRuntime* rt = JS_GetRuntime(ctx);
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue search_paths_val = JS_GetPropertyStr(ctx, global, js_search_paths_object.c_str());
            JS_FreeValue(ctx, global);

            auto paths = get_search_paths(ctx, search_paths_val);
            JS_FreeValue(ctx, search_paths_val);

            std::string found_path;

            if( uniset::file_exist(filename)  || filename[0] == '.' || filename[0] == '/' )
            {
                found_path = filename;
            }
            else
            {
                const std::string fname(filename);

                for (const auto& p : paths)
                {
                    if (uniset::file_exist(p + "/" + fname))
                    {
                        found_path = p + "/" + fname;
                        break;
                    }
                }
            }

            if( found_path.empty() )
            {
                JS_FreeCString(ctx, filename);
                return JS_UNDEFINED;
            }

            if( ModuleRegistry::is_module_loaded(rt, filename) )
            {
                //                printf("Module '%s' already loaded in registry\n", filename);
                JS_FreeCString(ctx, filename);
                return JS_UNDEFINED;
            }

            size_t buf_len;
            uint8_t* buf = js_load_file(ctx, &buf_len, found_path.c_str());
            JS_FreeCString(ctx, filename);

            if (!buf) return JS_UNDEFINED;

            JSValue result = JS_Eval(ctx, (char*)buf, buf_len, filename, JS_EVAL_TYPE_GLOBAL);
            js_free(ctx, buf);

            if( !JS_IsException(result) )
                ModuleRegistry::mark_module_loaded(rt, filename);

            return result;
        }
        // -------------------------------------------------------------------------
        std::unordered_map<JSRuntime*, std::set<std::string>> ModuleRegistry::loaded_modules;
        // -------------------------------------------------------------------------
        void ModuleRegistry::mark_module_loaded(JSRuntime* rt, const std::string& module_name)
        {
            loaded_modules[rt].insert(module_name);
        }
        // -------------------------------------------------------------------------
        bool ModuleRegistry::is_module_loaded(JSRuntime* rt, const std::string& module_name)
        {
            auto it = loaded_modules.find(rt);

            if (it == loaded_modules.end()) return false;

            return it->second.find(module_name) != it->second.end();
        }

        // -------------------------------------------------------------------------
        std::vector<std::string> ModuleRegistry::get_loaded_modules(JSRuntime* rt)
        {
            std::vector<std::string> result;
            auto it = loaded_modules.find(rt);

            if (it != loaded_modules.end())
            {
                result.assign(it->second.begin(), it->second.end());
            }

            return result;
        }
        // -------------------------------------------------------------------------
        static char* qjs_strdup(JSContext* ctx, const char* s)
        {
            size_t n = strlen(s) + 1;
            char* r = (char*)js_malloc(ctx, n);

            if (!r) return NULL;

            memcpy(r, s, n);
            return r;
        }
        // -------------------------------------------------------------------------
        char* qjs_module_normalize(JSContext* ctx, const char* base_name, const char* name, void* opaque)
        {
            if (!name) return NULL;

            if (name[0] == '/' || strncmp(name, "file:", 5) == 0 || strncmp(name, "http:", 5) == 0 || strncmp(name, "https:", 6) == 0)
            {
                return qjs_strdup(ctx, name);
            }

            if (name[0] == '.')
            {
                std::string base = base_name ? base_name : "";
                auto pos = base.find_last_of('/');
                std::string base_dir = (pos == std::string::npos) ? std::string(".") : base.substr(0, pos);
                std::string norm = base_dir + "/" + std::string(name);
                return qjs_strdup(ctx, norm.c_str());
            }

            return qjs_strdup(ctx, name);
        }
        // -------------------------------------------------------------------------
        JSModuleDef* qjs_module_loader(JSContext* ctx, const char* module_name, void* opaque)
        {
            if (!opaque || !module_name) return nullptr;

            auto* data = static_cast<jshelper::JSPrivateData*>(opaque);

            std::string found_path;

            if( uniset::file_exist(module_name)  || module_name[0] == '.' || module_name[0] == '/' )
            {
                found_path = module_name;
            }
            else
            {
                const std::string fname(module_name);

                for (const auto& p : data->search_paths)
                {
                    if( uniset::file_exist(p + "/" + fname) )
                    {
                        found_path = p + "/" + fname;
                        break;
                    }
                }
            }

            if( found_path.empty() )
                return nullptr;

            size_t buf_len = 0;
            uint8_t* buf = js_load_file(ctx, &buf_len, found_path.c_str());

            if (!buf) return nullptr;

            JSValue func_val = JS_Eval(ctx, (const char*)buf, buf_len, found_path.c_str(),
                                       JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            js_free(ctx, buf);

            if (JS_IsException(func_val))
            {
                jshelper::dump_exception_details(ctx);
                return nullptr;
            }

            return (JSModuleDef*)JS_VALUE_GET_PTR(func_val);
        }
        // -------------------------------------------------------------------------
    } // end of namespace JSHelper
} // end of namespace uniset
// -------------------------------------------------------------------------
