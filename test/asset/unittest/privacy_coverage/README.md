# HPIC C++ TDD 与覆盖率

本测试套运行真实生产 C++ 代码，使用 LLVM source-based coverage，合并软实现、闭源入口、协议、HUKS 派生和 IPC 编排测试。
行覆盖率门槛为 **75%**；用例失败、统计文件遗漏或覆盖率不足时脚本返回非零退出码。

## 统计范围

统计 `privacy_protocol`、`frameworks/privacy_computation/ipc/src`、
`frameworks/privacy_computation/js/napi/src`、`services/asset/privacy/src` 下的全部 16 个生产 `.cpp` 文件。
文件清单保存在 `output/privacy_tdd/coverage_manifest.json`，报告文件集合必须与清单完全相同。
测试、测试替身、头文件、Rust 和第三方依赖不计入此 C++ 行覆盖率；不会按函数覆盖率替代行覆盖率。
分支覆盖率单独报告，75% 门槛目前只应用于合计行覆盖率，并非要求每个文件或分支都达到 75%。

使用真实 OpenSSL 完成摘要、随机数、HKDF 和 AES-GCM 操作，使用 HUKS 源码中的真实参数构造实现。
HiFHE、HUKS 服务、OSAccount、NAPI、动态加载与 IPC 边界使用测试替身。
HiFHE 替身只模拟模 65537 的槽位运算及上下文 seed 校验，序列化格式仅供测试，不能验证真实密文格式、
真实 HE 安全性或 TA/软实现互通。此套测试也不验证真实 Binder 事务和 HUKS 持久化，仍需在真机联调。

测试代码集中在专用测试目录，替身头文件仅由宿主机脚本引入，不进入生产 BUILD.gn 或生产库。

协议测试位于 `test/asset/unittest/privacy_protocol/src`，HE 测试位于
`test/asset/unittest/privacy_he/src`；对应的 GN 测试目标也定义在各自的测试目录下。
NAPI 参数校验及异步失败由 boundary 测试组覆盖。

## 覆盖的行为

- 四个协议接口的 intersection/PIR 流程、2048 槽位重复、批次顺序和匹配结果。
- 默认和另行注册的SoftHeEngine均直接创建句柄；intersection/PIR多批每批一次Deserialize和一次最终导出。
- 失败批次不导出中间密文，独立句柄互不干扰，引擎销毁后句柄仍可运算。
- 真实 AES-GCM 已知答案、空值/缺省值、篡改拒绝及失败时输出不变。
- HUKS CE 的 userId、根密钥创建策略、Init/Update/Finish 顺序、失败时 Abort 和 seed 长度。
- 第三接口逐个 Bytes 调用 SA 解密，对称密文留在业务进程，最后统一本地后处理。
- NAPI Uint8Array、数组、整数和枚举边界、默认 HASH_NONE、Promise 错误映射、异步工作清理。
- 配置闭源库时的入口分流、加载失败、符号缺失、闭源NAPI的Promise返回及错误转发。
- DRBG 首次成功、重复初始化可接受、其他初始化失败；使用独立进程隔离静态初始化缓存。

首次实际执行已有协议用例发现 7 个失败：修复了 HASH_NONE 被 OpenSSL provider 拒绝的问题，
补齐 intersection 结果携带对称密文的格式拒绝；同步修正过期的 PIR SIZE_512、缺省关联值和随机数重试预期。
PIR 两组掩码继续独立采样，测试通过给相邻组提供不同熵来验证槽位映射，不额外要求独立样本永远不相等。

## 运行

当前 Windows 工作区已经准备好依赖，在项目根目录执行：

```powershell
python test/asset/unittest/privacy_coverage/run_coverage.py
```

每次执行重新编译生产源码；单次执行内部共享相同编译对象。
各测试组重新生成 profile，全部通过后合并数据、验证完整文件清单并检查覆盖率门槛。
闭源库加载失败单独启动进程，避免成功加载后的进程缓存影响断言。
`--group protocol|root|client|boundary|hard` 可用于定位单套测试；仅完整运行执行合计覆盖率门禁。

输出均在忽略提交的 `output/privacy_tdd` 下：

- `coverage.md`：覆盖率、分支覆盖率及逐文件统计。
- `html/index.html`：可定位未覆盖行的 HTML 报告。
- `coverage.json`、`coverage.profdata`：机器可读结果与合并 profile。
- `protocol.xml`、`root.xml`、`client.xml`、`boundary.xml`、`hard.xml`、`hard-open-failure.xml`：GTest 结果。
- `drbg-success.xml`、`drbg-failure.xml`：独立进程中的 DRBG 初始化结果；报告区分唯一用例和重复执行次数。

## 更换开发机

需要 Python 3.10+、支持 Windows x86_64/UCRT 的 LLVM/MinGW（含匹配的 llvm-cov、llvm-profdata 和 profile runtime）、
GoogleTest 源码、OpenSSL 3 UCRT 开发包、OpenHarmony SDK NAPI 头文件，以及对应版本的 HiFHE/openHiTLS/HUKS 头文件。
已验证 LLVM/MinGW 15.0.0、GoogleTest v1.15.2、OpenSSL 3.6.5。通过 `PRIVACY_HIFHE_INCLUDE` 指向
包含 `HIFHE_CTRL_BGV_SET_WIRE_FORMAT` 的 HiFHE 头文件，本次验证使用提交 `48869a6`。
工具与第三方依赖放在 `output` 下，不纳入版本库。

脚本默认使用本工作区准备的依赖目录；其他开发机通过以下环境变量指定本地目录：

| 环境变量 | 内容 |
| --- | --- |
| `PRIVACY_LLVM_BIN` | 包含 clang/clang++/llvm-cov/llvm-profdata 的 bin 目录 |
| `PRIVACY_GTEST_ROOT` | 包含 src/gtest-all.cc 和 include 的 googletest 子目录 |
| `PRIVACY_OPENSSL_ROOT` | 包含 include、lib/libcrypto.dll.a、bin 的 UCRT OpenSSL 根目录 |
| `PRIVACY_HUKS_ROOT` | security_huks 仓库根目录 |
| `PRIVACY_OHOS_SDK_INCLUDE` | SDK native/sysroot/usr/include 目录 |
| `PRIVACY_HIFHE_INCLUDE` | 包含正式 hifhe.h 的目录 |
| `PRIVACY_OPENHITLS_INCLUDE` | 包含 crypto、bsl、config 的 include 根目录 |
| `PRIVACY_SECUREC_INCLUDE` | 包含 securec.h 的目录 |
| `PRIVACY_NAPI_COMMON` | OpenHarmony napi/native_common.h 的完整文件路径 |

宿主机编译只复制 NAPI 所需的正式头文件，避免将整套 OHOS sysroot 头文件混入 Windows CRT。
