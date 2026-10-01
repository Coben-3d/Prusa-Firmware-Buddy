#!/usr/bin/env python3
"""Run the MK4 color host tests using existing tools, with no bootstrap/install.

This compiles production color, JSON and EEPROM journal sources. It does not
compile the full GUI/Marlin firmware or interact with a printer.
"""
import argparse
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build-dir", type=Path, default=root / "build-loaded-color-tests")
parser.add_argument("--compiler", default="clang++")
args = parser.parse_args()
build = args.build_dir.resolve()
build.mkdir(parents=True, exist_ok=True)
option = build / "include" / "option"
option.mkdir(parents=True, exist_ok=True)
for name in ("has_chamber_api", "has_filament_heatbreak_param"):
    (option / (name + ".h")).write_text(f"#pragma once\n#define {name.upper()}() 0\n")

includes = [build / "include", root / "tests/stubs/global", root / "tests/unit/mock/freertos/include",
            root / "src/common", root / "src/common/utils", root / "src/persistent_stores",
            root / "src", root / "include", root / "src/module/utils/bsod", root / "lib/SG14",
            root / "lib/Catch2/single_include", root / "lib/WUI/nhttp"]
sources = ["tests/unit/test_main.cpp", "tests/unit/common/loaded_filament_color_tests.cpp",
           "tests/unit/mock/freertos/mutex.cpp", "tests/unit/mock/crc32_sw.cpp", "tests/unit/mock/bsod.cpp",
           "src/common/filament_to_load.cpp", "src/common/segmented_json.cpp", "src/common/json_encode.cpp",
           "src/persistent_stores/journal/backend.cpp", "lib/WUI/nhttp/filament_renderer.cpp"]
binary = build / "loaded_filament_color_tests"
environment = dict(os.environ, TMPDIR=str(build))
subprocess.run([args.compiler, "-std=c++23", "-DUNITTESTS", "-DCATCH_CONFIG_FAST_COMPILE", "-pthread",
                "-Wall", "-Wextra", "-Wno-deprecated-declarations", *[f"-I{p}" for p in includes],
                *[str(root / p) for p in sources], "-o", str(binary)], check=True, env=environment)
subprocess.run([str(binary), "[loaded-color]"], check=True, env=environment)
