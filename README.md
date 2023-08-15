# asset

## 环境配置
1. 单框架代码下载：
mkdir hmos_trunk
cd hmos_trunk
repo init -u http://mgit-tm.rnd.huawei.com/platform/manifest -m system_general.xml -b hmos_trunk --repo-branch=stable --no-repo-verify --repo-branch=stable_py3

2. asset代码下载：
cd base/security
git clone ssh://git@szv-y.codehub.huawei.com:2222/<工号>/asset.git

## 编译运行待补充