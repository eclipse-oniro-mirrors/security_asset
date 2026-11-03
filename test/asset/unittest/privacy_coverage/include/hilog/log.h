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

#ifndef PRIVACY_COVERAGE_HILOG_H
#define PRIVACY_COVERAGE_HILOG_H
#define LOG_CORE 0
// Resolve the same module identifiers as real HiLog, even when discarding log output.
#define HILOG_DEBUG(...) ((void)(LOG_DOMAIN), (void)(LOG_TAG))
#define HILOG_INFO(...) ((void)(LOG_DOMAIN), (void)(LOG_TAG))
#define HILOG_WARN(...) ((void)(LOG_DOMAIN), (void)(LOG_TAG))
#define HILOG_ERROR(...) ((void)(LOG_DOMAIN), (void)(LOG_TAG))
#endif
