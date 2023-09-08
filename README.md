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

## 编译运行待补充
在vendor/huawei/build/component_config/system/generic_generic_arm_64only/hisi_higeneric/newphone_standard/part_config.json添加
"security:asset":{},

```bash
#首次编译指令：
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset_test

#非首次编译指令
./build_system.sh --abi-type generic_generic_arm_64only --device-type hisi_higeneric_newphone_standard --ccache --build-variant root --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset --build-target out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/build_configs/security/asset:asset_test --fast-rebuild
```


支持SA自启：各种脚本/push_asset_cfg.bat ，解压执行bat脚本即可。

推包命令： 各种脚本/push_asset.bat 修改小包目录位置，执行即可。
