/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "bundle_installer.h"

#include <cinttypes>
#include <fstream>
#include <sys/statfs.h>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "app_log_tag_wrapper.h"
#include "bundle_mgr_service.h"
#include "datetime_ex.h"
#include "ffrt.h"
#include "parameter.h"
#include "parameters.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
constexpr const char* USER_DATA_DIR = "/data";
constexpr double MIN_FREE_INODE_PERCENT = 0.005; // 0.5%

bool CheckSystemInodeSatisfied(const std::string &bundleName)
{
    std::string appGalleryName = OHOS::system::GetParameter(ServiceConstants::CLOUD_SHADER_OWNER, "");
    if (appGalleryName.empty() || appGalleryName != bundleName) {
        return true;
    }
    struct statfs stat;
    if (statfs(USER_DATA_DIR, &stat) != 0) {
        LOG_E(BMS_TAG_INSTALLER, "statfs failed for %{public}s, error %{public}d",
            USER_DATA_DIR, errno);
        return false;
    }
    uint32_t minFreeInodeNum = static_cast<uint32_t>(stat.f_files * MIN_FREE_INODE_PERCENT);
    if (stat.f_ffree < minFreeInodeNum) {
        LOG_E(BMS_TAG_INSTALLER, "free inodes not satisfied");
        return false;
    }
    LOG_D(BMS_TAG_INSTALLER, "total inodes: %{public}" PRIu64 ", free inodes: %{public}" PRIu64,
        stat.f_files, stat.f_ffree);
    return BundleUtil::CheckOrphanNodeUseRateIsSufficient();
}

std::vector<std::string> DeduplicateBundleNames(const std::vector<std::string> &bundleNames)
{
    std::vector<std::string> result;
    std::unordered_set<std::string> bundleNameSet;
    for (const auto &bundleName : bundleNames) {
        if (bundleNameSet.find(bundleName) != bundleNameSet.end()) {
            continue;
        }
        bundleNameSet.emplace(bundleName);
        result.emplace_back(bundleName);
    }
    return result;
}

int32_t GetUninstallUserIdForOobePreload(const std::shared_ptr<BundleDataMgr> &dataMgr, const std::string &bundleName,
    int32_t userId)
{
    if (dataMgr == nullptr) {
        return userId;
    }
    auto userIds = dataMgr->GetUserIds(bundleName);
    if (userIds.size() != 1) {
        return userId;
    }
    if (userIds[0] == Constants::DEFAULT_USERID || userIds[0] == Constants::U1) {
        return userIds[0];
    }
    return userId;
}
} // namespace
BundleInstaller::BundleInstaller(const int64_t installerId, const sptr<IStatusReceiver> &statusReceiver)
    : installerId_(installerId), statusReceiver_(statusReceiver)
{
    LOG_NOFUNC_I(BMS_TAG_INSTALLER, "create bundle installer instance id:%{public}" PRId64 "", installerId_);
}

BundleInstaller::~BundleInstaller()
{
    LOG_NOFUNC_I(BMS_TAG_INSTALLER, "destroy installer id:%{public}" PRId64 "", installerId_);
}

void BundleInstaller::Install(const std::string &bundleFilePath, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    if (installParam.userId == Constants::ALL_USERID) {
        auto userInstallParam = installParam;
        userInstallParam.allUser = true;
        for (auto userId : GetExistsCommonUserIds()) {
            userInstallParam.userId = userId;
            userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
            resultCode = InstallBundle(
                bundleFilePath, userInstallParam, Constants::AppType::THIRD_PARTY_APP);
            ResetInstallProperties();
        }

        NotifyAllBundleStatus();
    } else {
        resultCode = InstallBundle(
            bundleFilePath, installParam, Constants::AppType::THIRD_PARTY_APP);
        InstallForAllUsers(installParam);
    }
    std::string resultMsg = GetCheckResultMsg();
    SetCheckResultMsg("");
    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, resultMsg);
    }
}

void BundleInstaller::Recover(const std::string &bundleName, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    if (installParam.userId == Constants::ALL_USERID) {
        auto userInstallParam = installParam;
        for (auto userId : GetExistsCommonUserIds()) {
            userInstallParam.userId = userId;
            userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
            resultCode = BaseBundleInstaller::Recover(bundleName, userInstallParam);
            ResetInstallProperties();
        }
    } else {
        resultCode = BaseBundleInstaller::Recover(bundleName, installParam);
        RecoverDriverForAllUsers(bundleName, installParam);
    }

    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

void BundleInstaller::Install(const std::vector<std::string> &bundleFilePaths, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    if (installParam.userId == Constants::ALL_USERID) {
        auto userInstallParam = installParam;
        userInstallParam.allUser = true;
        for (auto userId : GetExistsCommonUserIds()) {
            userInstallParam.userId = userId;
            userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
            resultCode = InstallBundle(
                bundleFilePaths, userInstallParam, Constants::AppType::THIRD_PARTY_APP);
            ResetInstallProperties();
        }

        NotifyAllBundleStatus();
    } else {
        resultCode = InstallBundle(bundleFilePaths, installParam, Constants::AppType::THIRD_PARTY_APP);
        InstallForAllUsers(installParam);
    }
    std::string resultMsg = GetCheckResultMsg();
    SetCheckResultMsg("");
    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, resultMsg);
    }
}

void BundleInstaller::InstallByBundleName(const std::string &bundleName, const InstallParam &installParam)
{
    ErrCode resultCode = InstallBundleByBundleName(bundleName, installParam);
    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

void BundleInstaller::Uninstall(const std::string &bundleName, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    bool isDriver = IsDriverForAllUser(bundleName);
    int32_t userId = GetDriverInstallUser(bundleName);
    if (isDriver && (userId == Constants::DEFAULT_USERID || userId == Constants::U1)) {
        InstallParam param = installParam;
        param.userId = userId;
        resultCode = UninstallBundle(bundleName, param);
        if (statusReceiver_ != nullptr) {
            statusReceiver_->OnFinished(resultCode, "");
        }
        return;
    }
    if (installParam.userId == Constants::ALL_USERID ||
        (!installParam.isRemoveUser && isDriver)) {
        std::vector<ErrCode> errCode;
        auto userInstallParam = installParam;
        for (auto userId : GetExistsCommonUserIds()) {
            userInstallParam.userId = userId;
            resultCode = UninstallBundle(bundleName, userInstallParam);
            errCode.push_back(resultCode);
            ResetInstallProperties();
        }
        if (std::find(errCode.begin(), errCode.end(), ERR_OK) != errCode.end()) {
            for (const auto &err : errCode) {
                if (!(err == ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE ||
                    err == ERR_APPEXECFWK_USER_NOT_INSTALL_HAP || err == ERR_OK)) {
                    resultCode = err;
                    break;
                }
                resultCode = ERR_OK;
            }
        } else {
            resultCode = (errCode.size() > 0) ? errCode[0] : ERR_OK;
        }
    } else {
        resultCode = UninstallBundle(bundleName, installParam);
    }

    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

void BundleInstaller::Uninstall(const UninstallParam &uninstallParam)
{
    ErrCode resultCode = ERR_OK;
    resultCode = UninstallBundleByUninstallParam(uninstallParam);
    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

void BundleInstaller::Uninstall(
    const std::string &bundleName, const std::string &modulePackage, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    bool isDriver = IsDriverForAllUser(bundleName);
    int32_t userId = GetDriverInstallUser(bundleName);
    if (isDriver && (userId == Constants::DEFAULT_USERID || userId == Constants::U1)) {
        InstallParam param = installParam;
        param.userId = userId;
        resultCode = UninstallBundle(bundleName, modulePackage, param);
        if (statusReceiver_ != nullptr) {
            statusReceiver_->OnFinished(resultCode, "");
        }
        return;
    }
    if (installParam.userId == Constants::ALL_USERID || isDriver) {
        std::vector<ErrCode> errCode;
        auto userInstallParam = installParam;
        for (auto userId : GetExistsCommonUserIds()) {
            userInstallParam.userId = userId;
            resultCode = UninstallBundle(bundleName, modulePackage, userInstallParam);
            errCode.push_back(resultCode);
            ResetInstallProperties();
        }
        if (std::find(errCode.begin(), errCode.end(), ERR_OK) != errCode.end()) {
            for (const auto &err : errCode) {
                if (!(err == ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE ||
                    err == ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_MODULE ||
                    err == ERR_APPEXECFWK_USER_NOT_INSTALL_HAP || err == ERR_OK)) {
                    resultCode = err;
                    break;
                }
                resultCode = ERR_OK;
            }
        } else {
            resultCode = (errCode.size() > 0) ? errCode[0] : ERR_OK;
        }
    } else {
        resultCode = UninstallBundle(bundleName, modulePackage, installParam);
    }

    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

void BundleInstaller::UpdateInstallerState(const InstallerState state)
{
    LOG_D(BMS_TAG_INSTALLER, "state: %{public}d", static_cast<int32_t>(state));
    SetInstallerState(state);
    if (statusReceiver_) {
        statusReceiver_->OnStatusNotify(static_cast<int>(state));
    }
}

std::set<int32_t> BundleInstaller::GetExistsCommonUserIds()
{
    std::set<int32_t> userIds;
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        LOG_E(BMS_TAG_INSTALLER, "Get dataMgr shared_ptr nullptr");
        return userIds;
    }

    for (auto userId : dataMgr->GetAllUser()) {
        if (userId >= Constants::START_USERID) {
            userIds.insert(userId);
        }
    }
    return userIds;
}

void BundleInstaller::InstallForAllUsers(const InstallParam &installParam)
{
    std::string bundleName = GetCurrentBundleName();
    if (!IsDriverForAllUser(bundleName) && !IsEnterpriseForAllUser(installParam, bundleName)) {
        APP_LOGD("bundle %{public}s has no driver extension ability", bundleName.c_str());
        return;
    }
    APP_LOGI("bundle %{public}s need to install for all users", bundleName.c_str());
    ResetInstallProperties();
    auto userInstallParam = installParam;
    for (auto userId : GetExistsCommonUserIds()) {
        if (userId == installParam.userId) {
            continue;
        }
        userInstallParam.userId = userId;
        userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
        ErrCode resultCode = InstallBundleByBundleName(bundleName, userInstallParam);
        ResetInstallProperties();
        if (resultCode != ERR_OK) {
            APP_LOGE("install for user %{public}d failed, resultCode: %{public}d", userId, resultCode);
        }
    }
    NotifyAllBundleStatus();
}

void BundleInstaller::RecoverDriverForAllUsers(const std::string &bundleName, const InstallParam &installParam)
{
    if (!IsDriverForAllUser(bundleName)) {
        APP_LOGD("bundle %{public}s has no driver extension ability", bundleName.c_str());
        return;
    }
    APP_LOGI("bundle %{public}s has driver extension ability, need to recover for all users", bundleName.c_str());
    ResetInstallProperties();
    auto userInstallParam = installParam;
    for (auto userId : GetExistsCommonUserIds()) {
        if (userId == installParam.userId) {
            continue;
        }
        userInstallParam.userId = userId;
        userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
        ErrCode resultCode = BaseBundleInstaller::Recover(bundleName, userInstallParam);
        ResetInstallProperties();
        if (resultCode != ERR_OK) {
            APP_LOGE("recover driver for user %{public}d failed, resultCode: %{public}d", userId, resultCode);
        }
    }
}

void BundleInstaller::UninstallAndRecover(const std::string &bundleName, const InstallParam &installParam)
{
    ErrCode resultCode = ERR_OK;
    std::vector<ErrCode> errCode;
    auto userInstallParam = installParam;
    std::vector<int32_t> userIds;
    if (!installParam.isOTA && !CheckSystemInodeSatisfied(bundleName)) {
        APP_LOGE("System inode not satisfied for uninstall and recover");
        if (statusReceiver_ != nullptr) {
            statusReceiver_->OnFinished(ERR_BUNDLE_MANAGER_INSUFFICIENT_NUMBER_OF_SYSTEM_INODES, "");
        }
        return;
    }
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr != nullptr) {
        userIds = dataMgr->GetUserIds(bundleName);
    }
    for (auto userId : userIds) {
        userInstallParam.userId = userId;
        userInstallParam.SetIsUninstallAndRecover(true);
        resultCode = UninstallBundle(bundleName, userInstallParam);
        errCode.push_back(resultCode);
        ResetInstallProperties();
    }
    for (auto userId : userIds) {
        userInstallParam.userId = userId;
        userInstallParam.installFlag = InstallFlag::REPLACE_EXISTING;
        resultCode = BaseBundleInstaller::Recover(bundleName, userInstallParam);
        errCode.push_back(resultCode);
        ResetInstallProperties();
    }

    if (std::find(errCode.begin(), errCode.end(), ERR_OK) != errCode.end()) {
        for (const auto &err : errCode) {
            if (err != ERR_OK) {
                resultCode = err;
                break;
            }
            resultCode = ERR_OK;
        }
    } else {
        resultCode = (errCode.size() > 0) ? errCode[0] : ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE;
    }
    if (statusReceiver_ != nullptr) {
        statusReceiver_->OnFinished(resultCode, "");
    }
}

ErrCode BundleInstaller::UninstallNewPreinstalledApps(const std::vector<std::string> &bundleNames, int32_t userId)
{
    auto service = DelayedSingleton<BundleMgrService>::GetInstance();
    if (service == nullptr) {
        APP_LOGE("service is nullptr");
        return ERR_APPEXECFWK_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR;
    }
    auto dataMgr = service->GetDataMgr();
    auto pendingMgr = service->GetOobePreloadUninstallMgr();
    if (dataMgr == nullptr || pendingMgr == nullptr) {
        APP_LOGE("dataMgr or pendingMgr is nullptr");
        return ERR_APPEXECFWK_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR;
    }
    if (!dataMgr->HasUserId(userId)) {
        APP_LOGE("userId:%{public}d does not exist", userId);
        return ERR_APPEXECFWK_USER_NOT_EXIST;
    }
    if (bundleNames.empty()) {
        APP_LOGI("bundleNames is empty");
        return ERR_OK;
    }
    std::vector<std::string> deduplicatedBundleNames = DeduplicateBundleNames(bundleNames);
    std::vector<std::string> validBundleNames;
    for (const auto &bundleName : deduplicatedBundleNames) {
        if (bundleName.empty()) {
            APP_LOGW("skip empty bundleName");
            continue;
        }
        PreInstallBundleInfo preInstallBundleInfo;
        preInstallBundleInfo.SetBundleName(bundleName);
        if (!dataMgr->GetPreInstallBundleInfo(bundleName, preInstallBundleInfo) ||
            !preInstallBundleInfo.HasOtaNewInstallUser(userId)) {
            APP_LOGW("skip invalid bundleName:%{public}s for user:%{public}d",
                bundleName.c_str(), userId);
            continue;
        }
        validBundleNames.emplace_back(bundleName);
    }
    if (validBundleNames.empty()) {
        APP_LOGI("no valid bundleNames to uninstall");
        return ERR_OK;
    }

    std::vector<std::pair<std::string, int32_t>> pendingBundles;
    for (const auto &bundleName : validBundleNames) {
        int32_t uninstallUserId = GetUninstallUserIdForOobePreload(dataMgr, bundleName, userId);
        InstallParam installParam;
        installParam.userId = uninstallUserId;
        installParam.isKeepData = true;
        ErrCode ret = UninstallBundle(bundleName, installParam);
        if (ret != ERR_OK) {
            APP_LOGE("first stage uninstall failed, bundleName:%{public}s ret:%{public}d", bundleName.c_str(), ret);
            continue;
        }
        pendingBundles.emplace_back(bundleName, uninstallUserId);
        if (!pendingMgr->AddPendingBundle(bundleName, uninstallUserId)) {
            APP_LOGE("AddPendingBundle failed, bundleName:%{public}s", bundleName.c_str());
            continue;
        }
        PreInstallBundleInfo preInstallBundleInfo;
        preInstallBundleInfo.SetBundleName(bundleName);
        if (!dataMgr->GetPreInstallBundleInfo(bundleName, preInstallBundleInfo)) {
            APP_LOGE("GetPreInstallBundleInfo failed, bundleName:%{public}s", bundleName.c_str());
            continue;
        }
        preInstallBundleInfo.DeleteOtaNewInstallUser(uninstallUserId);
        if (!dataMgr->SavePreInstallBundleInfo(bundleName, preInstallBundleInfo)) {
            APP_LOGE("SavePreInstallBundleInfo failed, bundleName:%{public}s", bundleName.c_str());
            continue;
        }
        APP_LOGI("first stage uninstall succeeded, bundleName:%{public}s, userId:%{public}d",
            bundleName.c_str(), uninstallUserId);
    }

    auto task = [pendingBundles]() {
        auto service = DelayedSingleton<BundleMgrService>::GetInstance();
        if (service == nullptr) {
            APP_LOGE("service is nullptr");
            return;
        }
        auto pendingMgr = service->GetOobePreloadUninstallMgr();
        if (pendingMgr == nullptr) {
            APP_LOGE("pendingMgr is nullptr");
            return;
        }
        auto installer = std::make_shared<BundleInstaller>(GetMicroTickCount(), nullptr);
        for (const auto &pendingBundle : pendingBundles) {
            const auto &bundleName = pendingBundle.first;
            int32_t uninstallUserId = pendingBundle.second;
            InstallParam installParam;
            installParam.userId = uninstallUserId;
            installParam.isKeepData = false;
            ErrCode ret = installer->UninstallForInternal(bundleName, installParam);
            if (ret == ERR_OK) {
                (void)pendingMgr->RemovePendingBundle(bundleName, uninstallUserId);
                APP_LOGI("second stage uninstall succeeded, bundleName:%{public}s, userId:%{public}d",
                    bundleName.c_str(), uninstallUserId);
            } else {
                APP_LOGE("second stage uninstall failed, bundleName:%{public}s ret:%{public}d",
                    bundleName.c_str(), ret);
            }
        }
    };
    ffrt::submit(task);
    return ERR_OK;
}

ErrCode BundleInstaller::UninstallForInternal(const std::string &bundleName, const InstallParam &installParam)
{
    return UninstallBundle(bundleName, installParam);
}
}  // namespace AppExecFwk
}  // namespace OHOS
