# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Build instrumented production code; exclude boundary doubles from coverage."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import re
import shutil
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
OUT = ROOT / "output/privacy_tdd"
TOOL = Path(os.getenv("PRIVACY_LLVM_BIN", OUT / "llvm-mingw-20220906-ucrt-x86_64/bin"))
GTEST = Path(os.getenv("PRIVACY_GTEST_ROOT", OUT / "googletest/googletest"))
HUKS = Path(os.getenv("PRIVACY_HUKS_ROOT", ROOT.parent / "security_huks"))
OPENSSL = Path(os.getenv("PRIVACY_OPENSSL_ROOT", OUT / "ucrt64"))
SDK = Path(os.getenv("PRIVACY_OHOS_SDK_INCLUDE",
                     "D:/Huawei/DevEco Studio/sdk/default/openharmony/native/sysroot/usr/include"))
HIFHE = Path(os.getenv("PRIVACY_HIFHE_INCLUDE", ROOT / "output/hifhe_pr6_reference/include"))
HITLS = Path(os.getenv("PRIVACY_OPENHITLS_INCLUDE", ROOT / "output/openhitls_reference/include"))
SECUREC = Path(os.getenv("PRIVACY_SECUREC_INCLUDE", ROOT / "output/securec_reference/include"))
NAPI_COMMON = Path(os.getenv("PRIVACY_NAPI_COMMON",
                            ROOT / "output/napi_reference/interfaces/inner_api/napi/native_common.h"))
BUILT = set()
PROTOCOL = sorted((ROOT / "privacy_protocol").glob("*/src/*.cpp"))
PROTOCOL += [ROOT / "privacy_protocol/src/privacy_protocol.cpp"]
CLIENT = ROOT / "frameworks/privacy_computation/ipc/src/privacy_client.cpp"
SERVICE = sorted((ROOT / "services/asset/privacy/src").glob("*.cpp"))
NAPI = sorted((ROOT / "frameworks/privacy_computation/js/napi/src").glob("*.cpp"))
MANIFEST = PROTOCOL + [CLIENT] + SERVICE + NAPI
INCLUDES = [HERE / "include", GTEST / "include", GTEST, OPENSSL / "include", SECUREC, HIFHE]
INCLUDES += [HITLS / part for part in ("crypto", "bsl", "config")]
INCLUDES += [ROOT / "privacy_protocol" / part for part in
             ("include", "common/inc", "crypto/inc", "he/inc", "protocol/inc", "core/inc")]
INCLUDES += [ROOT / "frameworks/privacy_computation/ipc/inc", ROOT / "services/asset/privacy/inc",
             ROOT / "frameworks/privacy_computation/js/napi/inc",
             HUKS / "interfaces/inner_api/huks_standard/main/include",
             HUKS / "frameworks/huks_standard/main/common/include"]
INCLUDES += [OUT / "napi_include"]
ENV = dict(os.environ)
ENV.pop("PRIVACY_TEST_DRBG", None)
ENV["PATH"] = str(TOOL) + os.pathsep + str(OPENSSL / "bin") + os.pathsep + ENV["PATH"]


def run(args, **kwargs):
    subprocess.run([str(item) for item in args], cwd=ROOT, env=ENV, check=True, **kwargs)


def compile_source(source, production=False, hard=False):
    relative = source.relative_to(ROOT) if source.is_relative_to(ROOT) else source
    name = re.sub(r"[^a-zA-Z0-9_]", "_", str(relative)) + ".o"
    target = OUT / "obj" / name
    flags = ["-std=c++17", "-O0", "-g", "-Wno-inconsistent-dllimport", "-Wno-availability",
             "-include", HERE / "include/host_test.h"]
    compiler = TOOL / "clang++.exe"
    if source.suffix == ".c":
        flags = ["-std=gnu11", "-O0", "-g", "-Wno-inconsistent-dllimport"]
        compiler = TOOL / "clang.exe"
    if production:
        flags += ["-fprofile-instr-generate", "-fcoverage-mapping"]
    if hard and source.name == "privacy_computation_napi_loader.cpp":
        target = target.with_name("hard_" + target.name)
        flags += ['-DASSET_ENABLE_PRIVACY_COMPUTATION_CONFIG="host.so"']
    command = [compiler, *flags, *["-I" + str(path) for path in INCLUDES], "-c", source, "-o", target]
    if target not in BUILT:
        run(command)
        BUILT.add(target)
    return target


def build_and_run(name, production, tests, support):
    print("Building", name, flush=True)
    objects = [compile_source(path, True, name == "hard") for path in production]
    objects += [compile_source(path) for path in tests + support]
    objects += [compile_source(GTEST / "src" / file) for file in ("gtest-all.cc", "gtest_main.cc")]
    executable = OUT / (name + ".exe")
    run([TOOL / "clang++.exe", "-fprofile-instr-generate", *objects,
         OPENSSL / "lib/libcrypto.dll.a", "-o", executable])
    ENV["LLVM_PROFILE_FILE"] = str(OUT / (name + "-%p.profraw"))
    for profile in OUT.glob(name + "-*.profraw"):
        profile.unlink()
    filters = ["--gtest_filter=-ClosedLoaderTest.OpenFailure"] if name == "hard" else []
    run([executable, *filters, "--gtest_output=xml:" + str(OUT / (name + ".xml"))])
    if name == "hard":
        run([executable, "--gtest_filter=ClosedLoaderTest.OpenFailure",
             "--gtest_output=xml:" + str(OUT / "hard-open-failure.xml")])
    if name == "boundary":
        for mode in ("success", "failure"):
            ENV["PRIVACY_TEST_DRBG"] = mode
            run([executable, "--gtest_filter=PrivacyBoundaryTest.DrbgInitializationStatus",
                 "--gtest_output=xml:" + str(OUT / ("drbg-" + mode + ".xml"))])
        ENV.pop("PRIVACY_TEST_DRBG", None)
    return executable


def report():
    profiles = [path for name in ("protocol", "root", "client", "boundary", "hard")
                for path in OUT.glob(name + "-*.profraw")]
    merged = OUT / "coverage.profdata"
    run([TOOL / "llvm-profdata.exe", "merge", "-sparse", *profiles, "-o", merged])
    objects = [OUT / (name + ".exe") for name in ("protocol", "root", "client", "boundary", "hard")]
    command = [TOOL / "llvm-cov.exe", "export", objects[0],
               *["-object=" + str(path) for path in objects[1:]], "-instr-profile=" + str(merged),
               *MANIFEST]
    result = subprocess.check_output([str(value) for value in command], cwd=ROOT, env=ENV)
    (OUT / "coverage.json").write_bytes(result)
    data = json.loads(result)["data"][0]
    files = {Path(file["filename"]).resolve(): file for file in data["files"]}
    assert set(files) == set(MANIFEST), "Coverage file set differs from the complete production manifest"
    rows = []
    covered = 0
    count = 0
    for source in MANIFEST:
        lines = files[source]["summary"]["lines"]
        covered += lines["covered"]
        count += lines["count"]
        rows.append(f"| {source.relative_to(ROOT).as_posix()} | {lines['covered']}/{lines['count']} | "
                    f"{lines['percent']:.2f}% |")
    percent = 100 * covered / count
    branches = data["totals"]["branches"]
    results = [ET.parse(OUT / (name + ".xml")).getroot() for name in
               ("protocol", "root", "client", "boundary", "hard", "hard-open-failure",
                "drbg-success", "drbg-failure")]
    assert all(int(result.get("failures", 0)) == 0 and int(result.get("errors", 0)) == 0 for result in results)
    tests = sum(int(result.get("tests", 0)) for result in results)
    unique = {(case.get("classname"), case.get("name")) for result in results
              for case in result.findall(".//testcase")}
    body = "# HPIC C++ host coverage\n\n"
    body += f"Production line coverage: **{covered}/{count} = {percent:.2f}%**. Required: 75%.\n\n"
    body += f"Tests: **{len(unique)} unique cases / {tests} passing executions** "
    body += "(DRBG case rerun in fresh processes).\n\n"
    body += f"Branch coverage: {branches['covered']}/{branches['count']} "
    body += f"= {branches['percent']:.2f}% (reported separately; not the line-coverage gate).\n\n"
    body += f"Scope: all {len(MANIFEST)} production C++ implementation files; "
    body += "headers, test doubles and dependencies excluded.\n\n"
    body += "OpenSSL and HUKS parameter construction are real; "
    body += "HE, HUKS services, NAPI and IPC boundaries are doubles.\n\n"
    body += "| File | Covered/lines | Coverage |\n| --- | --- | --- |\n" + "\n".join(rows) + "\n"
    (OUT / "coverage.md").write_text(body, encoding="utf-8")
    print(body, flush=True)
    report_command = command.copy()
    report_command[1] = "show"
    report_command += ["-format=html", "-output-dir=" + str(OUT / "html")]
    run(report_command, stdout=subprocess.DEVNULL)
    if percent < 75:
        raise SystemExit(f"Line coverage gate failed: {percent:.2f}% < 75%")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--group", default="all", choices=("protocol", "root", "client", "boundary", "hard", "all"))
    options = parser.parse_args()
    (OUT / "obj").mkdir(parents=True, exist_ok=True)
    sdk = SDK
    pending = [sdk / "napi/native_api.h"]
    visited = set()
    while pending:
        source = pending.pop()
        if source in visited:
            continue
        visited.add(source)
        target = OUT / "napi_include" / source.relative_to(sdk)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        for name in re.findall(r'#include "([^"]+)"', source.read_text(encoding="utf-8")):
            child = source.parent / name
            pending.append(child if child.exists() else sdk / name)
    shutil.copyfile(NAPI_COMMON, OUT / "napi_include/napi/native_common.h")
    (OUT / "coverage_manifest.json").write_text(json.dumps([str(path) for path in MANIFEST], indent=2))
    securec = HERE / "src/securec_mock.cpp"
    if options.group in ("protocol", "all"):
        tests = [ROOT / "test/asset/unittest/privacy_protocol/src/privacy_protocol_test.cpp"]
        tests += sorted((ROOT / "test/asset/unittest/privacy_he/src").glob("*.cpp"))
        tests += [ROOT / "test/asset/unittest/privacy_protocol/src/protocol_codec_test.cpp"]
        build_and_run("protocol", PROTOCOL, tests, [securec, HERE / "src/hifhe_mock.cpp"])
    if options.group in ("root", "all"):
        production = [ROOT / "services/asset/privacy/src/huks_seed_deriver.cpp",
                      ROOT / "privacy_protocol/he/src/he_key_deriver.cpp"]
        support = [securec, HUKS / "frameworks/huks_standard/main/common/src/hks_param.c"]
        build_and_run("root", production,
                      [ROOT / "test/asset/unittest/privacy_root/src/huks_seed_deriver_test.cpp"], support)
    if options.group in ("client", "all"):
        build_and_run("client", [CLIENT],
                      [ROOT / "test/asset/unittest/privacy_protocol/src/privacy_client_test.cpp"], [])
    if options.group in ("boundary", "hard", "all"):
        support = [securec, HERE / "src/hifhe_mock.cpp", HERE / "src/napi_mock.cpp",
                   HERE / "src/platform_mock.cpp", HUKS / "frameworks/huks_standard/main/common/src/hks_param.c"]
        if options.group in ("boundary", "all"):
            build_and_run("boundary", MANIFEST, [HERE / "src/privacy_boundary_test.cpp"], support)
        if options.group in ("hard", "all"):
            build_and_run("hard", MANIFEST, [HERE / "src/privacy_closed_loader_test.cpp"], support)
    if options.group == "all":
        report()


if __name__ == "__main__":
    main()
