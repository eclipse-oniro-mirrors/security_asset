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

#ifndef PRIVACY_COVERAGE_DLFCN_H
#define PRIVACY_COVERAGE_DLFCN_H
#include <cstdint>
#define RTLD_NOW 2
#define RTLD_LOCAL 0
namespace PrivacyHost {
void *OpenLibrary(const char *path, int32_t mode);
void *FindLibrarySymbol(void *handle, const char *name);
char *GetLibraryError();
} // namespace PrivacyHost

// The host loader uses the same call sites as the OpenHarmony build.
#define dlopen ::PrivacyHost::OpenLibrary
#define dlsym ::PrivacyHost::FindLibrarySymbol
#define dlerror ::PrivacyHost::GetLibraryError
#endif
