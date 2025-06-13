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

 
 #include "data_size_wrapper.h"
 #include <sys/statfs.h>
 #include <cstring>
 #include <dirent.h>

 #include "directory_ex.h"
 #include "asset_log.h"

 using namespace OHOS;

 constexpr double INVAILD_QUOTA = -2.00;

 double GetRemainPartitionSize(const char* path)
 {
    std::string partitionName(path);
    if(partitionName.empty()) {
        LOGE("Fail to get partition name");
        return INVAILD_QUOTA;
    }
    struct statfs stat;
    if (statfs(partitionName.c_str(), &stat) != 0) {
        LOGE("Partition '%s' does not exist", partitionName.c_str());
        return INVAILD_QUOTA;
    }
    /* charge Byte size to M */
    constexpr double units = 1024.0;
    return (static_cast<double>(stat.f_bfree) / units) * (static_cast<double>(stat.f_bsize) / units);
 }

 uint64_t GetDirSize(const char* path)
 {
    const std::string pathStr(path);
    uint64_t dirSize = GetFolderSize(pathStr);
    return dirSize;
 }