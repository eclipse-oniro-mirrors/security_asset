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

#ifndef PRIVACY_COVERAGE_NAPI_HOST_H
#define PRIVACY_COVERAGE_NAPI_HOST_H
#include "napi/native_api.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

struct napi_value__ {
    napi_valuetype type = napi_undefined;
    double number = 0;
    bool boolean = false;
    bool array = false;
    bool typed = false;
    napi_typedarray_type typedKind = napi_uint8_array;
    std::string text;
    std::map<std::string, napi_value> properties;
    std::vector<napi_value> elements;
    std::vector<uint8_t> storage;
    napi_value buffer = nullptr;
    size_t count = 0;
    bool settled = false;
    bool rejected = false;
    napi_value result = nullptr;
    napi_callback callback = nullptr;
};
struct napi_callback_info__ {
    std::vector<napi_value> args;
};
struct napi_async_work__ {
    napi_async_execute_callback execute;
    napi_async_complete_callback complete;
    void *data;
};
struct napi_deferred__ {
    napi_value promise;
};
struct napi_env__ {
    std::vector<std::unique_ptr<napi_value__>> values;
    std::vector<std::unique_ptr<napi_deferred__>> deferreds;
    std::vector<napi_async_work> queued;
    napi_value exception = nullptr;
    napi_status completion = napi_ok;
    uint32_t deletedWorks = 0;
};
namespace PrivacyHost {
napi_value Value(napi_env env, napi_valuetype type = napi_object);
napi_value Number(napi_env env, double number);
napi_value Bytes(napi_env env, const std::vector<uint8_t> &bytes);
napi_value Array(napi_env env, const std::vector<napi_value> &items);
napi_value Call(napi_env env, const char *name, std::vector<napi_value> args);
void Drain(napi_env env);
} // namespace PrivacyHost
#endif
