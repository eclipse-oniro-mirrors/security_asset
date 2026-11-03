/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "napi_host.h"
#include "host_faults.h"
#include <algorithm>
#include <cstring>

namespace {
napi_module *g_module = nullptr;
}
namespace PrivacyHost {
napi_value Value(napi_env env, napi_valuetype type)
{
    env->values.push_back(std::make_unique<napi_value__>());
    auto result = env->values.back().get();
    result->type = type;
    return result;
}
napi_value Number(napi_env env, double number)
{
    auto result = Value(env, napi_number);
    result->number = number;
    return result;
}
napi_value Bytes(napi_env env, const std::vector<uint8_t> &bytes)
{
    auto result = Value(env);
    result->typed = true;
    result->storage = bytes;
    result->count = bytes.size();
    return result;
}
napi_value Array(napi_env env, const std::vector<napi_value> &items)
{
    auto result = Value(env);
    result->array = true;
    result->elements = items;
    return result;
}
napi_value Call(napi_env env, const char *name, std::vector<napi_value> args)
{
    auto exports = Value(env);
    g_module->nm_register_func(env, exports);
    auto function = exports->properties.at(name);
    napi_callback_info__ info{std::move(args)};
    return function->callback(env, &info);
}
void Drain(napi_env env)
{
    while (!env->queued.empty()) {
        auto work = env->queued.front();
        env->queued.erase(env->queued.begin());
        auto complete = work->complete;
        auto data = work->data;
        work->execute(env, data);
        complete(env, env->completion, data);
    }
}
} // namespace PrivacyHost

#define HOST_STATUS(name)                                                                                              \
    if (PrivacyHost::Fail(name)) {                                                                                     \
        return napi_generic_failure;                                                                                   \
    }
extern "C" void napi_module_register(napi_module *module)
{
    g_module = module;
}
extern "C" napi_status napi_create_object(napi_env env, napi_value *out)
{
    HOST_STATUS("create_object");
    *out = PrivacyHost::Value(env);
    return napi_ok;
}
extern "C" napi_status napi_typeof(napi_env, napi_value value, napi_valuetype *type)
{
    HOST_STATUS("typeof");
    if (value == nullptr) {
        return napi_invalid_arg;
    }
    *type = value->type;
    return napi_ok;
}
extern "C" napi_status napi_create_uint32(napi_env env, uint32_t number, napi_value *out)
{
    HOST_STATUS("create_uint32");
    *out = PrivacyHost::Number(env, number);
    return napi_ok;
}
extern "C" napi_status napi_create_int32(napi_env env, int32_t number, napi_value *out)
{
    HOST_STATUS("create_int32");
    *out = PrivacyHost::Number(env, number);
    return napi_ok;
}
extern "C" napi_status napi_get_value_double(napi_env, napi_value value, double *out)
{
    HOST_STATUS("get_double");
    *out = value->number;
    return napi_ok;
}
extern "C" napi_status napi_get_boolean(napi_env env, bool value, napi_value *out)
{
    HOST_STATUS("get_boolean");
    *out = PrivacyHost::Value(env, napi_boolean);
    (*out)->boolean = value;
    return napi_ok;
}
extern "C" napi_status napi_get_named_property(napi_env env, napi_value object, const char *name, napi_value *out)
{
    HOST_STATUS("get_property");
    auto found = object->properties.find(name);
    *out = found == object->properties.end() ? PrivacyHost::Value(env, napi_undefined) : found->second;
    return napi_ok;
}
extern "C" napi_status napi_set_named_property(napi_env, napi_value object, const char *name, napi_value value)
{
    HOST_STATUS("set_property");
    object->properties[name] = value;
    return napi_ok;
}
extern "C" napi_status napi_is_array(napi_env, napi_value value, bool *out)
{
    HOST_STATUS("is_array");
    *out = value->array;
    return napi_ok;
}
extern "C" napi_status napi_get_array_length(napi_env, napi_value value, uint32_t *out)
{
    HOST_STATUS("array_length");
    *out = static_cast<uint32_t>(value->elements.size());
    return napi_ok;
}
extern "C" napi_status napi_get_element(napi_env, napi_value value, uint32_t index, napi_value *out)
{
    HOST_STATUS("get_element");
    *out = value->elements.at(index);
    return napi_ok;
}
extern "C" napi_status napi_set_element(napi_env, napi_value value, uint32_t index, napi_value item)
{
    HOST_STATUS("set_element");
    value->elements.at(index) = item;
    return napi_ok;
}
extern "C" napi_status napi_create_array_with_length(napi_env env, size_t size, napi_value *out)
{
    HOST_STATUS("create_array");
    *out = PrivacyHost::Array(env, std::vector<napi_value>(size));
    return napi_ok;
}
extern "C" napi_status napi_is_typedarray(napi_env, napi_value value, bool *out)
{
    HOST_STATUS("is_typedarray");
    *out = value->typed;
    return napi_ok;
}
extern "C" napi_status napi_get_typedarray_info(napi_env, napi_value value, napi_typedarray_type *type, size_t *count,
                                                void **data, napi_value *buffer, size_t *offset)
{
    HOST_STATUS("typedarray_info");
    *type = value->typedKind;
    *count = value->count;
    auto &storage = value->buffer == nullptr ? value->storage : value->buffer->storage;
    *data = storage.empty() ? nullptr : storage.data();
    if (buffer != nullptr) {
        *buffer = value->buffer;
    }
    if (offset != nullptr) {
        *offset = 0;
    }
    return napi_ok;
}
extern "C" napi_status napi_create_arraybuffer(napi_env env, size_t size, void **data, napi_value *out)
{
    HOST_STATUS("create_buffer");
    *out = PrivacyHost::Value(env);
    (*out)->storage.resize(size);
    *data = (*out)->storage.data();
    return napi_ok;
}
extern "C" napi_status napi_create_typedarray(napi_env env, napi_typedarray_type type, size_t count, napi_value buffer,
                                              size_t, napi_value *out)
{
    HOST_STATUS("create_typedarray");
    *out = PrivacyHost::Value(env);
    (*out)->typed = true;
    (*out)->typedKind = type;
    (*out)->count = count;
    (*out)->buffer = buffer;
    return napi_ok;
}
extern "C" napi_status napi_create_string_utf8(napi_env env, const char *text, size_t size, napi_value *out)
{
    HOST_STATUS("create_string");
    *out = PrivacyHost::Value(env, napi_string);
    (*out)->text.assign(text, size == NAPI_AUTO_LENGTH ? std::strlen(text) : size);
    return napi_ok;
}
extern "C" napi_status napi_create_error(napi_env env, napi_value, napi_value text, napi_value *out)
{
    HOST_STATUS("create_error");
    *out = PrivacyHost::Value(env);
    (*out)->properties["message"] = text;
    return napi_ok;
}
extern "C" napi_status napi_throw(napi_env env, napi_value error)
{
    HOST_STATUS("throw");
    env->exception = error;
    return napi_ok;
}
extern "C" napi_status napi_throw_error(napi_env env, const char *, const char *message)
{
    auto error = PrivacyHost::Value(env);
    error->text = message;
    env->exception = error;
    return napi_ok;
}
extern "C" napi_status napi_get_last_error_info(napi_env, const napi_extended_error_info **info)
{
    static const napi_extended_error_info value{"host boundary failure", nullptr, 0, napi_generic_failure};
    *info = &value;
    return napi_ok;
}
extern "C" napi_status napi_is_exception_pending(napi_env env, bool *out)
{
    HOST_STATUS("exception_pending");
    *out = env->exception != nullptr;
    return napi_ok;
}
extern "C" napi_status napi_get_and_clear_last_exception(napi_env env, napi_value *out)
{
    HOST_STATUS("clear_exception");
    *out = env->exception;
    env->exception = nullptr;
    return napi_ok;
}
extern "C" napi_status napi_create_promise(napi_env env, napi_deferred *deferred, napi_value *out)
{
    HOST_STATUS("create_promise");
    *out = PrivacyHost::Value(env);
    env->deferreds.push_back(std::make_unique<napi_deferred__>(napi_deferred__{*out}));
    *deferred = env->deferreds.back().get();
    return napi_ok;
}
extern "C" napi_status napi_resolve_deferred(napi_env, napi_deferred deferred, napi_value value)
{
    HOST_STATUS("resolve");
    deferred->promise->settled = true;
    deferred->promise->result = value;
    return napi_ok;
}
extern "C" napi_status napi_reject_deferred(napi_env, napi_deferred deferred, napi_value value)
{
    HOST_STATUS("reject");
    deferred->promise->settled = true;
    deferred->promise->rejected = true;
    deferred->promise->result = value;
    return napi_ok;
}
extern "C" napi_status napi_get_cb_info(napi_env, napi_callback_info info, size_t *count, napi_value *args,
                                        napi_value *, void **)
{
    HOST_STATUS("cb_info");
    size_t copied = std::min(*count, info->args.size());
    std::memcpy(args, info->args.data(), copied * sizeof(napi_value));
    *count = copied;
    return napi_ok;
}
extern "C" napi_status napi_create_async_work(napi_env, napi_value, napi_value, napi_async_execute_callback execute,
                                              napi_async_complete_callback complete, void *data, napi_async_work *out)
{
    HOST_STATUS("create_work");
    *out = new napi_async_work__{execute, complete, data};
    return napi_ok;
}
extern "C" napi_status napi_queue_async_work(napi_env env, napi_async_work work)
{
    HOST_STATUS("queue_work");
    env->queued.push_back(work);
    return napi_ok;
}
extern "C" napi_status napi_delete_async_work(napi_env env, napi_async_work work)
{
    ++env->deletedWorks;
    delete work;
    HOST_STATUS("delete_work");
    return napi_ok;
}
extern "C" napi_status napi_define_properties(napi_env env, napi_value object, size_t count,
                                              const napi_property_descriptor *properties)
{
    HOST_STATUS("define_properties");
    for (size_t i = 0; i < count; ++i) {
        auto value = properties[i].value;
        if (properties[i].method != nullptr) {
            value = PrivacyHost::Value(env, napi_function);
            value->callback = properties[i].method;
        }
        object->properties[properties[i].utf8name] = value;
    }
    return napi_ok;
}
