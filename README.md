# asset

## 环境配置
1. 单框架代码下载：
```bash
mkdir hmos_trunk
cd hmos_trunk
repo init -u http://mgit-tm.rnd.huawei.com/platform/manifest -m system_general.xml -b hmos_trunk --repo-branch=stable --no-repo-verify --repo-branch=stable_py3
repo sync -c -j32
./build/prebuilts_download.sh --skip-ssl --tool-repo=http://hmf.inhuawei.com:9080 --npm-registry=http://mirrors.tools.huawei.com/npm/ --pypi-url=http://mirrors.tools.huawei.com/pypi/simple/
```

2. asset代码下载：
```bash
cd base/security
rm -rf asset
git clone ssh://git@szv-y.codehub.huawei.com:2222/y00522150/asset.git
```

## 编译运行
```bash
# 运行前配置
cd vendor/huawei/build
git fetch https://szv-cr-y.codehub.huawei.com/CBG_CR_HarmonyOS/huawei/build.git refs/change-requests/1032/1 && git cherry-pick FETCH_HEAD
cd -
cd third_party/sqlite
git fetch https://szv-open.codehub.huawei.com/OpenSourceCenter_CR/openharmony/third_party_sqlite.git refs/change-requests/12/2 && git cherry-pick FETCH_HEAD
cd -
cd base/security/huks
git fetch https://szv-open.codehub.huawei.com/OpenSourceCenter_CR/openharmony/security_huks.git refs/change-requests/67/2 && git cherry-pick FETCH_HEAD
git fetch https://szv-open.codehub.huawei.com/OpenSourceCenter_CR/openharmony/security_huks.git refs/change-requests/59/2 && git cherry-pick FETCH_HEAD
cd -

# 首次编译命令：（修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_all_phone_standard --ccache --build-variant root --build-target asset --build-target asset_test

# 非首次编译命令：（未修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_all_phone_standard --ccache --build-variant root --build-target asset --build-target asset_test --fast-rebuild

# 打点文件编译命令：（修改hisysevent.yaml时执行）
./build/ohos/hisysevent/gen_def_from_all_yaml.py --yaml-list base/security/asset/hisysevent.yaml --def-path out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/security/asset

# 支持SA自启：(仅在调试设备上执行一次)
./scripts/push_asset_cfg.bat

# 运行环境：(每次代码修改后执行)
./scripts/push_asset.bat


```

## 测试
测试DEMO：https://codehub-y.huawei.com/y00522150/AssetDemo/files?ref=master
测试Sample: https://codehub-y.huawei.com/z00639827/asset_sample/files?ref=master

```bash
# 自动化用例推包运行
hdc file send Z:\workspace\hmos_trunk\out\generic_generic_arm_64only\hisi_all_phone_standard\tests\unittest\asset\asset_UT_test\asset_rust_test ./data/
hdc file send Z:\workspace\hmos_trunk\out\generic_generic_arm_64only\hisi_all_phone_standard\tests\unittest\asset\asset_UT_test\asset_ndk_test ./data/
hdc file send Z:\workspace\hmos_trunk\out\generic_generic_arm_64only\hisi_all_phone_standard\tests\unittest\asset\asset_UT_test\asset_module_test ./data/

hdc shell chmod 777 ./data/asset_rust_test
hdc shell chmod 777 ./data/asset_ndk_test
hdc shell chmod 777 ./data/asset_module_test

hdc shell "./data/asset_rust_test --test-threads=1"
hdc shell "./data/asset_ndk_test --gtest_output=xml:/data/"
hdc shell "./data/asset_module_test --test-threads=1"
```

### 单线程运行测试用例
执行的时候 在后面加 --test-threads=1

## 工具汇总
```bash
# 格式化BUILD.gn文件（在asset目录下执行）
find -name "*.gn" -or -name "*.gni" | xargs ../../../prebuilts/build-tools/linux-x86/bin/gn format

# 批量整改gn文件格式（在asset目录下执行）
find -name "*.gn" | xargs ../../../prebuilts/build-tools/linux-x86/bin/gn format

# 代码格式化（在asset目录下执行）
cargo fmt

# disable样机打卡软件, 在设备上执行
hdc shell
find -name com.huawei.hmsapp.samplemanagement | xargs rm -rf
```

## WIKI汇总
**BUILD.gn规范：**https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/subsystems/subsys-build-component-building-rules.md
**日志打印规范：**https://gitee.com/openharmony/docs/blob/master/zh-cn/contribute/OpenHarmony-Log-guide.md
**可靠性设计和编码规范：**https://w3.huawei.com/ipd/tsl/#!tsl_new/standard/standard.html?standardId=152795
**RUST编码规范：**https://w3.huawei.com/tsl/#/standard/standardDetail?standardId=217651
**可信构建：**https://wiki.huawei.com/domains/6660/wiki/8/WIKI20230410978631?title=_15