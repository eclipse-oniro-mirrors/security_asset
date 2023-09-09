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

```bash
# 首次编译命令：（修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset_test

# 非首次编译命令：（未修改BUILD.gn时执行）
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset_test --fast-rebuild

# 支持SA自启：(仅在调试设备上执行一次)
./scripts/push_asset_cfg.bat

# 运行环境：(每次代码修改后执行)
./scripts/push_asset.bat
```

## 工具汇总
```bash
# 格式化BUILD.gn文件
../../../prebuilts/build-tools/linux-x86/bin/gn format *.gn
../../../prebuilts/build-tools/linux-x86/bin/gn format **/*.gn
../../../prebuilts/build-tools/linux-x86/bin/gn format **/**/*.gn
../../../prebuilts/build-tools/linux-x86/bin/gn format **/**/**/*.gn
```

### WIKI汇总
**BUILD.gn规范：**https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/subsystems/subsys-build-component-building-rules.md
**日志打印规范：**https://gitee.com/openharmony/docs/blob/master/zh-cn/contribute/OpenHarmony-Log-guide.md
**可靠性设计和编码规范：**https://w3.huawei.com/ipd/tsl/#!tsl_new/standard/standard.html?standardId=152795
**RUST编码规范：**https://w3.huawei.com/ipd/tsl/#!tsl_new/standard/standard.html?standardId=217651