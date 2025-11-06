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
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include "quickjs/quickjs.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    namespace jshelper
    {
        struct IOProp
        {
            std::string name;
            std::string sensor;
        };

        IOProp convert_js_to_io_prop(JSContext* ctx, JSValueConst obj_val );
        void dump_exception_details( JSContext* ctx );

        void safe_function_call(JSContext* ctx, JSValueConst obj, JSValueConst func,
                                int argc = 0, JSValueConst* argv = nullptr );
        void safe_function_call_by_name(JSContext* ctx, const char* func_name,
                                        int argc = 0, JSValue* argv = nullptr );

        void debug_function_call( JSContext* ctx, const char* func_name );

        struct JSPrivateData
        {
            std::vector<std::string> search_paths;
        };

        const std::string js_search_paths_object = "__load_search_paths";
        // ----------------------------------------------------------------------
        struct JsArgs
        {
            JSContext* ctx;
            std::vector<JSValue > vals;

            explicit JsArgs(JSContext* c) : ctx(c) {}
            ~JsArgs()
            {
                for (auto& v : vals)
                    JS_FreeValue(ctx, v);
            }

            // Добавить аргументы
            JsArgs& i64(int64_t x)
            {
                vals.push_back(JS_NewInt64(ctx, x));
                return *this;
            }

            JsArgs& str(const char* s)
            {
                vals.push_back(JS_NewString(ctx, s));
                return *this;
            }

            int size() const
            {
                return (int)vals.size();
            }

            // Важно: возвращаем JSValueConst* !
            JSValueConst* data()
            {
                return vals.data();
            }
        };

        // ----------------------------------------------------------------------
        JSModuleDef* module_loader_with_path( JSContext* ctx, const char* module_name, void* opaque );
        JSValue js_load_file_with_data( JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv );
        std::string find_file( const std::string filename, const std::vector<std::string>& search_paths );
        JSModuleDef* qjs_module_loader(JSContext* ctx, const char* module_name, void* opaque);
        char* qjs_module_normalize(JSContext* ctx, const char* base_name, const char* name, void* opaque);
        // ----------------------------------------------------------------------
        class JSValueGuard
        {
                JSContext* ctx;
                JSValue value;
            public:
                JSValueGuard(JSContext* ctx, JSValue val) : ctx(ctx), value(val) {}
                ~JSValueGuard()
                {
                    if( ctx && !JS_IsUndefined(value) && !JS_IsNull(value) )
                        JS_FreeValue(ctx, value);
                }
                JSValue get() const
                {
                    return value;
                }

                JSValueGuard(const JSValueGuard&) = delete;
                JSValueGuard& operator=(const JSValueGuard&) = delete;
        };

        // ----------------------------------------------------------------------
        template<typename T>
        std::vector<T> js_array_to_vector(JSContext* ctx, JSValueConst array_val,
                                          std::function<T(JSContext*, JSValue)> converter)
        {
            std::vector<T> result;

            if (!JS_IsArray(ctx, array_val))
            {
                return result;
            }

            JSValue length_val = JS_GetPropertyStr(ctx, array_val, "length");
            int length;
            JS_ToInt32(ctx, &length, length_val);
            JS_FreeValue(ctx, length_val);

            for( int i = 0; i < length; i++ )
            {
                JSValue item_val = JS_GetPropertyUint32(ctx, array_val, i);
                T item = converter(ctx, item_val);
                result.push_back(item);
                JS_FreeValue(ctx, item_val);
            }

            return result;
        }
        // ----------------------------------------------------------------------
        class ModuleRegistry
        {
            private:
                static std::unordered_map<JSRuntime*, std::set<std::string>> loaded_modules;

            public:
                static void mark_module_loaded(JSRuntime* rt, const std::string& module_name);
                static bool is_module_loaded(JSRuntime* rt, const std::string& module_name);
                static std::vector<std::string> get_loaded_modules(JSRuntime* rt);
        };

        bool is_module_loaded(JSContext* ctx, const std::string& module_name);

    }
    // ----------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif
