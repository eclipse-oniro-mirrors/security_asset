/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CXX_BRIDGE_FUNCTIONS_H
#define CXX_BRIDGE_FUNCTIONS_H

#include <cstdint>

// This header is included by cxx bridge (wrapper.rs.h) AFTER rust/cxx.h.
// rust::Vec, rust::String etc. are fully defined at this point.
// CxxCommandInfo / CxxTicketQueryResult are forward-declared here and
// fully defined later in wrapper.rs.h.

namespace OHOS {
namespace Security {
namespace SAF {

struct CxxCommandInfo;
struct CxxTicketQueryResult;

int32_t CxxRequestToolTicket(
    const rust::Vec<CxxCommandInfo>& cliCmdInfos,
    uint64_t ticketExpireTimeMs,
    CxxTicketQueryResult& result);

} // namespace SAF
} // namespace Security
} // namespace OHOS

#endif
