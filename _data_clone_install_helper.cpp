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

#include "data_clone_install_helper.h"

#include "bundle_constants.h"
#include "bundle_util.h"
#include "bundle_service_constants.h"
#include <dirent.h>
#include "installd_client.h"
#include <sys/stat.h>
#include <unistd.h>

#include "app_log_tag_wrapper.h"
#include "directory_ex.h"

namespace OHOS {
namespace AppExecFwk {

bool DataCloneInstallHelper::AreAllCloneInstallPaths(const std::vector<std::string> &bundlePaths)
{
    if (bundlePaths.empty()) {
        LOG_W(BMS_TAG_INSTALLER, "bundlePaths is empty");
        return false;
    }

    if (bundlePaths.size() != 1) {
        LOG_W(BMS_TAG_INSTALLER, "bundlePaths size must be 1, but got %{public}zu", bundlePaths.size());
        return false;
    }

    const auto &path = bundlePaths[0];
    if (path.empty() || path.find(ServiceConstants::APP_CLONE_SANDBOX_PATH) != 0) {
        LOG_W(BMS_TAG_INSTALLER, "Path does not have required prefix %{public}s: %{public}s",
            ServiceConstants::APP_CLONE_SANDBOX_PATH, path.c_str());
        return false;
    }

    LOG_D(BMS_TAG_INSTALLER, "Path has valid clone install prefix with %{public}s",
        ServiceConstants::APP_CLONE_SANDBOX_PATH);
    return true;
}

std::string DataCloneInstallHelper::RenameDirToSecurityDir(const std::string &dirPath,
    std::vector<std::string> &bundlePaths, std::vector<std::string> &toDeletePaths, int32_t userId)
{
    std::string destination = ServiceConstants::HAP_COPY_PATH;
    std::string bundleName = dirPath.substr(dirPath.rfind(ServiceConstants::PATH_SEPARATOR) + 1);
    destination.append(ServiceConstants::PATH_SEPARATOR).append(ServiceConstants::SECURITY_STREAM_INSTALL_PATH);
    
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    CreateDirParam createDirParam;
    createDirParam.bundleDirScene = BundleDirScene::SERVICE_BMS_SECURITY_STREAM_INSTALL_DIR;
    if (InstalldClient::GetInstance()->Mkdir(
        destination, mode, Constants::FOUNDATION_UID, ServiceConstants::BMS_GID, createDirParam) != ERR_OK) {
        LOG_E(BMS_TAG_INSTALLER, "Failed to create security directory: %{public}s", destination.c_str());
    }
    destination.append(ServiceConstants::PATH_SEPARATOR);
    destination.append("+app_clone").append(bundleName).append(ServiceConstants::PLUS_SIGN)
        .append(std::to_string(userId)).append(ServiceConstants::PLUS_SIGN);
    static std::atomic<uint64_t> installCount = 0;
    destination.append(std::to_string(BundleUtil::GetCurrentTime()) + "_" + std::to_string(++installCount));
    toDeletePaths.emplace_back(destination);
    LOG_D(BMS_TAG_INSTALLER, "Renaming directory %{public}s to security directory %{public}s",
        dirPath.c_str(), destination.c_str());
    return destination;
}

ErrCode DataCloneInstallHelper::RenameToRealCodePath(
    const std::vector<std::string> bundlePaths,
    const std::string &bundleName)
{
    if (bundlePaths.empty()) {
        LOG_E(BMS_TAG_INSTALLER, "-n %{public}s rename failed due to bundlePaths empty", bundleName.c_str());
        return ERR_APPEXECFWK_INSTALL_FILE_PATH_INVALID;
    }
    std::string codePath = std::string(Constants::BUNDLE_CODE_DIR) + ServiceConstants::PATH_SEPARATOR + bundleName;
    ErrCode ret = InstalldClient::GetInstance()->RenameModuleDir(bundlePaths[0], codePath,
        bundleName, BundleDirScene::COPY_HAP_TO_INSTALL_PATH);
    if (ret != ERR_OK) {
        LOG_E(BMS_TAG_INSTALLER, "-n %{public}s rename to real code path failed", bundleName.c_str());
        return ret;
    }
    return ret;
}
} // namespace AppExecFwk
} // namespace OHOS

