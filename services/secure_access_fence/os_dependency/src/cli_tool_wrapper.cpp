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

#include "cli_tool_wrapper.h"

#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "cli_tool_mgr_client.h"
#include "saf_log.h"
#include "saf_result_code.h"
#include "saf_result_defs.h"

using namespace OHOS::CliTool;

static bool ValidateCmdInfo(const CxxCmdInfo& cmd)
{
    if (cmd.cmd_name.ptr == nullptr || cmd.cmd_name.len <= 0) {
        LOGE("Invalid cmd_name: ptr is nullptr or len <= 0");
        return false;
    }
    if (cmd.sub_cmd.ptr == nullptr || cmd.sub_cmd.len <= 0) {
        LOGE("Invalid sub_cmd: ptr is nullptr or len <= 0");
        return false;
    }
    return true;
}

static int32_t ConvertCxxCmdsToCommands(
    const CxxCmdInfo* cmds,
    int32_t cmd_count,
    std::vector<Command>& cliCmds)
{
    for (int32_t i = 0; i < cmd_count; i++) {
        if (!ValidateCmdInfo(cmds[i])) {
            LOGE("Invalid cmd info at index %{public}d", i);
            return SAF_ERR_ARG_INVALID;
        }
        cliCmds.push_back(Command{
            std::string(cmds[i].cmd_name.ptr, cmds[i].cmd_name.len),
            std::string(cmds[i].sub_cmd.ptr, cmds[i].sub_cmd.len)
        });
    }
    return 0;
}

static int32_t QueryCliPermissions(
    const std::vector<Command>& cliCmds,
    std::vector<CommandPermission>& cliCmdPermissions)
{
    int32_t ret = CliToolMGRClient::GetInstance().BatchQueryPermissionBySubCommand(cliCmds, cliCmdPermissions);
    if (ret != 0) {
        LOGE("BatchQueryPermissionBySubCommand failed, ret=%{public}d", ret);
        return SAF_ERR_TOOL_ERROR;
    }
    return 0;
}

static void CollectAllPermissions(
    const std::vector<CommandPermission>& cliCmdPermissions,
    std::set<std::string>& allPermissions)
{
    for (const auto& perm : cliCmdPermissions) {
        for (const auto& p : perm.permissions) {
            allPermissions.insert(p);
        }
    }
}

static int32_t WritePermissionsToBuffer(
    const std::set<std::string>& allPermissions,
    char* out_buf,
    int32_t buf_size,
    int32_t& permCount)
{
    int32_t offset = 0;
    permCount = 0;
    
    for (const auto& perm : allPermissions) {
        int32_t needed = static_cast<int32_t>(perm.size()) + 1;
        if (offset + needed > buf_size) {
            break;
        }
        if (memcpy_s(out_buf + offset, static_cast<size_t>(buf_size - offset),
                     perm.c_str(), perm.size()) != 0) {
            LOGE("memcpy_s failed");
            return SAF_ERR_TOOL_ERROR;
        }
        out_buf[offset + perm.size()] = '\0';
        offset += needed;
        permCount++;
    }
    return 0;
}

extern "C" int32_t CxxBatchQueryCliPermissions(
    const CxxCmdInfo* cmds,
    int32_t cmd_count,
    char* out_buf,
    int32_t buf_size,
    CxxQueryResult* out_result)
{
    if (cmds == nullptr || out_buf == nullptr || out_result == nullptr) {
        return SAF_ERR_ARG_INVALID;
    }

    out_result->ret_code = 0;
    out_result->result_code = 0;
    out_result->perm_count = 0;

    std::vector<Command> cliCmds;
    int32_t ret = ConvertCxxCmdsToCommands(cmds, cmd_count, cliCmds);
    if (ret != 0) {
        return ret;
    }

    std::vector<CommandPermission> cliCmdPermissions;
    ret = QueryCliPermissions(cliCmds, cliCmdPermissions);
    if (ret != 0) {
        out_result->ret_code = ret;
        out_result->result_code = ret;
        return ret;
    }

    std::set<std::string> allPermissions;
    CollectAllPermissions(cliCmdPermissions, allPermissions);

    int32_t permCount = 0;
    ret = WritePermissionsToBuffer(allPermissions, out_buf, buf_size, permCount);
    if (ret != 0) {
        out_result->ret_code = ret;
        return ret;
    }

    out_result->perm_count = permCount;
    return 0;
}