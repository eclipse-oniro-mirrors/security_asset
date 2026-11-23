# BMS 事件契约实证（可核查）

> 本文档为 Q-001/Q-002/Q-006 的证据补充：将"文件名+日期"升级为**仓库+文件指纹+行号+复验命令**级引用。

## 1. 证据身份

| 项 | 值 |
|----|-----|
| 仓库 | `openharmony/bundlemanager_bundle_framework`（master 快照，本地路径 `D:\bundlemanager_bundle_framework-master (1)\bundlemanager_bundle_framework-master`） |
| 快照说明 | 本地为源码包（无 .git，无 commit hash）；以**文件 SHA256 + 行号 + grep 复验命令**锚定内容；如需 commit 级引用，请向 BMS Owner 索取对应 master commit 或正式事件契约文档 |
| 证据文件 | `services/bundlemgr/src/bundle_common_event_mgr.cpp` |
| 文件指纹 | SHA256 = `BFB8AB10915A24357270C5E07B7AE53B5900C6FCF153158886C1C8428B8F6141` |
| 证据日期 | 2026-11-17 提取，2026-11-18 补充行号 |

## 2. PACKAGE_REMOVED 契约（发布函数 `SetNotifyWant`，L309-345）

| want 键 | 类型 | 必填性 | 证据行号 |
|---------|------|--------|----------|
| action | CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED | 必有 | L313 |
| bundleName | string（SetElement + **参数双写**） | 必有 | L315/L319 |
| uid | int | 必有 | L320 |
| userId | int（由 uid 换算） | 必有 | L321 |
| appId | string | 必有 | L325 |
| appIdentifier | string | 必有 | L327 |
| bundleType | string | 必有 | L329 |
| appIndex | int | 必有 | L331 |
| resultCode | int | 必有 | L333 |
| **keepData** | **bool** | **必有** | **L334（常量定义 L53：`constexpr const char* KEEP_DATA = "keepData"`）** |
| isAppUpdate | bool | 必有 | L335 |
| isBmsExtensionUninstalled | bool(false) | PACKAGE_REMOVED 分支 | L337 |
| **assetAccessGroups** | string（逗号分隔） | **条件（非空才写）** | L338-340（`if (!assetAccessGroups.empty())` 守卫） |
| **developerId** | string | **条件（随 assetAccessGroups）** | L340 |

## 3. PACKAGE_UNINSTALLED_DATA_CLEARED 契约（发布函数 `NotifyUninstalledBundleCleared`，L696-712）

| want 键 | 类型 | 必填性 | 证据行号 |
|---------|------|--------|----------|
| action | "usual.event.PACKAGE_UNINSTALLED_DATA_CLEARED"（**本地 constexpr，无 CommonEventSupport 常量**） | 必有 | L699（常量定义 L74） |
| bundleName | **仅 SetElement（无 bundleName 参数！）** | 必有 | L701 |
| uid | int（**无 userId**） | 必有 | L705 |
| developerId | string | 必有 | L706 |
| assetAccessGroups | string | 必有 | L707 |
| bundleType | string | 必有 | L708 |
| **appId** | string | 必有 | L709 |
| appIdentifier | string | 必有 | L710 |
| **keepData** | **bool** | 必有 | L711 |
| **appIndex** | int | 必有 | L712 |

## 4. 关键结论（与实现对照）

1. **字段名实证为 `keepData`（驼峰，bool 类型）**：常量 L53 + 两处 SetParam（L334/L711）。原规格术语 `keepdata` 为转写差异，实现两侧已统一按 `keepData` 解析（spec 术语已同步）。
2. **清理事件身份字段**：携带 `appId`/`appIndex`（L709/L712）——支撑清理按"触发应用自身"收敛（Q-002 结论）；携带 `uid` 而非 `userId`（L705，对照 L321）——支撑 `GetUserIdByUid` 换算兜底（Q-001 结论）。
3. **bundleName 获取方式差异**：PACKAGE_REMOVED 双写（参数+Element），清理事件仅 Element——C++ 侧 `GetBundle()` 兜底为必需路径。
4. **清理范围语义**：BMS 以 `appId+appIndex` 标识触发应用，事件按单应用身份发布（与 bundle 级整体清理语义相对）——支持 Q-002 的"hap 粒度收敛"设计决策。

## 5. 复验命令（任何人可在 BMS 仓复现）

```bash
# 字段名与类型（keepData 驼峰）
grep -n 'constexpr const char\* KEEP_DATA' services/bundlemgr/src/bundle_common_event_mgr.cpp
# 清理事件发布函数与 want 构造
grep -n 'NotifyUninstalledBundleCleared\|SetParam(KEEP_DATA\|SetParam(APP_ID\|SetParam(APP_INDEX' services/bundlemgr/src/bundle_common_event_mgr.cpp
# 文件指纹
sha256sum services/bundlemgr/src/bundle_common_event_mgr.cpp
# 期望：BFB8AB10915A24357270C5E07B7AE53B5900C6FCF153158886C1C8428B8F6141
```

## 6. PACKAGE_UNINSTALLED_DATA_CLEARED 触发时序（源码实证，2026-11-18 补充）

调用链（base_bundle_installer.cpp）：

```
BaseBundleInstaller::UninstallBundle(bundleName, installParam)        // L558
  ├─ ProcessBundleUninstall(...) → result                             // L578
  └─ if (!installParam.isKeepData                                     // L592 本轮未选保留
        && (result == ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE  // 已卸载（保留数据中）
            || result == ERR_APPEXECFWK_USER_NOT_INSTALL_HAP)
        && DeleteUninstallBundleInfoFromDb(bundleName))                // L593
        ├─ RemoveBundleDataDir(targetBundleName, ...)                  // L5118 删除保留的数据目录
        ├─ DeleteEncryptionKeyId / DeleteUninstallBundleResource ...  // L5129-5135
        └─ NotifyUninstalledBundleCleared(installRes)                 // L5148 发布清理事件
             └─ installRes: appIndex=0（硬编码 L5140）、keepData=false（硬编码 L5145）
```

**结论**：清理事件 = **对"已卸载且保留数据"的 bundle 再次执行不带 keepData 的卸载**。"重装后再卸载（不勾保留）"是用户态典型路径（满足同一代码条件）；代码级条件为二次无保留卸载，重装并非必要条件。C-4 的"仅重装再卸载触发"在用户流程层面成立。附加契约事实：**keepData 与 appIndex 均携带触发卸载操作的实际值**——本快照唯一调用点写 `keepData=false`（L5145，与触发条件 L592 `!isKeepData` 语义自洽）与 `appIndex=0`（L5140，主应用路径），**设备分支分身路径发送实际值（设备实测实证）**；消费侧一律按事件值处理、不假设取值（Asset 实现的 targets 过滤与清理链路均不依赖 keepData/appIndex 的特定取值）。

消费侧缺省语义：发布方按第 2/3 节表格 SetParam（PACKAGE_REMOVED 的 assetAccessGroups/developerId 非空才写，L338-340 守卫）；消费侧缺键行为：bool→false、int→-1/invalid、string→空串按缺省处理（Asset 两侧实现一致）。

## 7. 遗留说明

- 本地快照无 commit hash；若评审要求 commit 级证据，需 BMS Owner 提供对应 master commit 号或正式事件契约文档（已列入依赖风险）。字段/类型/必填性/触发时序均已行号级实证（本文件第 2/3/6 节）。
- 测试构造可直接引用第 2/3/6 节表格作为权威契约；宽松解析（字符串 "true" 兜底）仅为消费侧容错，不构成契约内容。
