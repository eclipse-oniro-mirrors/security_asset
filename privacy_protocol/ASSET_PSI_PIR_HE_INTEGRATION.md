> 2026-10-27 方案替换：当前已使用HUKS托管256位DERIVE业务根，CE级别、按OS账号选择固定别名，
> 再由HUKS HKDF-SHA256三阶段接口导出32字节临时HE seed。已移除自建RKC、设备UDID、根材料文件和mask。
> 第一接口允许创建缺失根，第三接口禁止创建；CE不可用不回退DE或本地派生。
> 当前说明以first-third-interface-call-chain.md第6节为准；下文自建RKC及其SELinux联动要求属于历史方案。

> 2026-10-27 RKC更新：SA使用HUKS RKC v2软实现的R1/R2、设备摘要、HKDF-RMK、AES-GCM封装MK、
> 双副本和内存mask链路。调用者seed由MK通过HKDF-SHA256派生，info为空。
> 不包含旧root.v1读取和迁移。以first-third-interface-call-chain.md第6节为准，
> 下文旧单文件根材料/自定义info描述已被替代。设备UDID读取及文件rename的SELinux联动仍待目标环境验证。

> 2026-10-26 三接口编排更新：公开 `genPrivacyTarget`、`privacySearch`、`getSearchResult`
> 均转发到 `PsiPirProtocolImpl` 的同名方法。目标向量构造、seed 派生、逐批解密及结果解析
> 由核心编排；`SoftHeEngine` 只提供 HE 原语，`ProtocolCodec` 负责槽字节布局。
> 已移除 BgvEncryptor/BgvDecryptor 类。NAPI 的 CCZ/Asset SA 分流保持原路径。
> 以 `docs/features/hpic-sa-ta-impl/first-third-interface-call-chain.md` 的最新调用链为准；
> 下文涉及旧独立加解密类的内容为历史设计。

> 2026-10-26 当前阶段：一、三接口有 CCZ 配置时在 NAPI 直接动态转发完整闭源接口，无配置时经 Asset SA 调用本库完整软实现。
> 本库不选择 CCZ 后端，getSearchResult(input, protocol, CallerIdentity, output) 完成软解密和结果解析。
> TargetInput 仅含 elemData、hashAlg，不含 isCt。下文按后端选择、闭源 raw HE 解密等描述属于后续方案，当前不接入。

# 协议库 HE 实现集成说明

所有实现统一编入 `psi_pir_protocol` 静态库，对外通过 `include/psi_pir_protocol.h` 提供三个完整接口：

- `psi_pir::genPrivacyTarget`：目标生成与 HE 加密。
- `psi_pir::privacySearch`：本地同态计算，安装内部默认引擎但保留显式 Provider 覆盖。
- `psi_pir::getSearchResult`：按后端解密，再执行统一开源后处理。

HE 实现位于 `he/`，内部类型位于 `psi_pir::internal`，共同后处理由公开 `GetSearchResult` 承担。不存在独立 soft_impl 库或 SDK 包装接口。
NAPI 仅负责参数转换、异步、平台身份与配置；直接链接协议库。既有闭源目标生成转发仍在平台入口，CCZ 原始解密桥接位于协议库内部，尚待正式 ABI 对齐。

HIFHE 和 CCZ 桥接目前仍明确失败，不能据此宣称真实通路已经可用。旧 Asset 协议适配与 HE 注册源文件已移除；本文件名保留以兼容历史链接。
