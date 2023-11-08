#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2010-2023. All rights reserved.

set -e

../../../../../OpenHarmony_prebuilts/linux/rust-1.68.0-dev-x86_64-unknown-linux-gnu/rust-1.68.0-dev-x86_64-unknown-linux-gnu/llvm-tools-preview/lib/rustlib/x86_64-unknown-linux-gnu/bin/llvm-profdata merge --sparse ../services/db_operator/*.profraw -o ../services/db_operator/default.profdata

grcov ../services/db_operator/ -s ../services/db_operator --binary-path ../../../../out/generic_generic_arm_64only/hisi_higeneric_newphone_standard/exe.unstripped/tests/unittest/asset/asset_UT_test/asset_db_operator_test -t html --branch --ignore-not-existing -o ./report
