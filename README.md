# asset

## 环境配置
1. 单框架代码下载：
```bash
mkdir hmos_trunk
cd hmos_trunk
repo init -u http://mgit-tm.rnd.huawei.com/platform/manifest -m system_general.xml -b hmos_trunk --repo-branch=stable --no-repo-verify --repo-branch=stable_py3
repo sync -c -j32
```

2. asset代码下载：
```bash
cd base/security
rm -rf asset
git clone ssh://git@szv-y.codehub.huawei.com:2222/y00522150/asset.git
```

## 编译运行
在vendor/huawei/build/component_config/system/generic_generic_arm_64only/hisi_higeneric/newphone_standard/part_config.json添加
"security:asset":{},

在vendor/huawei/build/component_config/system/generic_generic_arm_64only/hisi_newbaltimore/pc_standard/part_config.json添加
"security:asset":{},

```bash
# 首次编译命令：（修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target asset_test

# 非首次编译命令：（未修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target asset_test --fast-rebuild

# PC编译命令：
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_newbaltimore_pc_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_newbaltimore_pc_standard/build_configs/security/asset:asset --build-target asset_test

# 支持SA自启：(仅在调试设备上执行一次)
./scripts/push_asset_cfg.bat

# 运行环境：(每次代码修改后执行)
./scripts/push_asset.bat

# 批量整改gn文件格式（在asset目录下执行）
find -name "*.gn" | xargs ../../../prebuilts/build-tools/linux-x86/bin/gn format
```

## 测试
测试DEMO：https://codehub-y.huawei.com/y00522150/AssetDemo/files?ref=master

```bash
# 自动化用例推包运行
hdc file send Z:\workspace\hmos_trunk\out\generic_generic_arm_64only\hisi_higeneric_newphone_standard\tests\unittest\asset\asset_UT_test\asset_rust_test ./data/

hdc shell chmod 777 ./data/asset_rust_test

hdc shell "./data/asset_rust_test --test-threads=1"

hdc file send Z:\workspace\hmos_trunk\out\generic_generic_arm_64only\hisi_higeneric_newphone_standard\tests\unittest\asset\asset_UT_test\asset_ndk_test ./data/

hdc shell chmod 777 ./data/asset_ndk_test

hdc shell "./data/asset_ndk_test --gtest_output=xml:/data/"
```

## 工具汇总
```bash
# 格式化BUILD.gn文件, 在asset目录下执行
find -name "*.gn" -or -name "*.gni" | xargs ../../../prebuilts/build-tools/linux-x86/bin/gn format

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