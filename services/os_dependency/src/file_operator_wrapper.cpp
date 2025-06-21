/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


#include "file_operator_wrapper.h"
#include <sys/statfs.h>
#include <cstring>
#include <dirent.h>

#include "directory_ex.h"
#include "asset_log.h"
#include "asset_type.h"

using namespace OHOS;

int32_t GetRemainPartitionSize(const char *partitionName, double *partitionSize)
{
    if (partitionName == nullptr) {
        LOGE("Fail to get partition name");
        return ASSET_INVALID_ARGUMENT;
    }
    struct statfs stat;
    if (statfs(partitionName, &stat) != 0) {
        LOGE("Failed to get partition information for %{public}s", partitionName);
        return ASSET_FILE_OPERATION_ERROR;
    }
    /* Calculate free space in megabytes */
    constexpr double units = 1024.0;
    *partitionSize = (static_cast<double>(stat.f_bfree) / units) * (static_cast<double>(stat.f_bsize) / units);
    return ASSET_SUCCESS;
}

uint64_t GetDirSize(const char *dir)
{
    return GetFolderSize(dir);
}
